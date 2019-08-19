/*
 * Copyright (C) 2019 Anders Larsen gislagard@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/system/error_code.hpp>
#include <vector>
#include <string>

namespace ba = boost::algorithm;
namespace fs = boost::filesystem;
using namespace std;

char usage[] = R"~~~(
sift 0.1 - a file sifter

Usage: sift [OPTIONS] source destination

 -m --move    : Move items. (Files and/or folders.)
 -l --link    : Make folders in destination, hardlink files from source.
 -t --test    : Test and report any items failing to matching anything.
 -d --deep    : Sift deeper using source folder sub-subfolders.
 -h --help    : Display more help.
 -q --quiet   : Donẗ show warnings about files already existing.
 -v --verbose : Output info about moved/linked items.)~~~";

char help[] = R"~~~(
sift move or link items (files or folders) in a source folder to matching sub-
folders in a destination "sieve" folder. A destination subfolder match if the 
name of the folder contains a word-group where all words are found in the name
of the item being tested. if --move then the item will be moved to the first 
matching folder.

Word-groups are separated using ',', ')' or '('. Prefix a word in the word-
group with '!' to specify that it must NOT match the item. Word order, spaces
and case (for a-z/A-Z) in word groups are ignored when attempting to match. 

Example destination subfolder names:

/destination sieve path/Science Fiction (sci-fi, space opera)/
/destination sieve path/Science/
/destination sieve path/E-books, (epub, pdf, cbr, cbz, djvu, mobi, azw3)/

Matches between items and destination subfolder word-groups are tried in order
of decreasing complexity/difficulty as measured by length of wordgroup minus
number of words. Fewer longer words are considered more complex and harder to 
match than the same words split into several shorter words.

Destination and source folders must be on the same filesystem. 

Using the --deep option it is possible to use a previous destination sieve
folder as the new source folder to further refine the sifting process in 
several chained steps.
)~~~";

class Sifter
{
    /*
     * Data structure notes
     * 
     * Groups of words are associated with paths in the destination sieve 
     * folder. The groups of words are "encoded" as text in the filenames of 
     * the paths itself. Word-groups are delimited by ',', '(' and ')'. 
     * Words are delimited by spaces.
     * 
     * Paths are stored in a vector, vPath, as they are discovered. Indices 
     * into vPath are used instead of the path itself. That saves some memory
     * if the same path is used by several word-groups. And it speeds up 
     * sorting.
     * 
     * A word-group is stored as a vectors of words. And a word is a string.
     * 
     * Word-groups and paths are associated in pairs. That is a mapping.
     * 
     * Mappings are "scored" by how complex they are. Complexity is simply
     * measured as the combined length of the word-group - the number of words.
     * The score allows mappings to be sorted so that the most complex or 
     * difficult mappings can be tried first. This might be important for moves.
     * 
     * The scoring and sorting ensures that "science fiction" match "science
     * fiction" before it matches "science". 
     * 
     * The scores and the mappings are combined in pairs. That is a TMapping.
     * 
     * All TMapping pairs of scores and pairs of path indices and word-groups 
     * are stored in a vector vMapping, this is the main datastructure of sift.
     */

private:

    boost::system::error_code ec;

    vector<fs::path> vPath;
    typedef pair<int, pair<int, vector<string> > > TMapping;
    vector<TMapping> vMapping;

    bool bDeep, bVerbose, bMove, bCopy, bLink, bTest, bQuiet;

public:

    Sifter()
    {
        bVerbose = bDeep = bMove = bLink = bCopy = bTest = bQuiet = false;
    };

    void setVerbose(void)
    {
        bVerbose = true;
    };

    void setDeep(void)
    {
        bDeep = true;
    };

    void setQuiet(void)
    {
        bQuiet = true;
    };

    void setMove(void)
    {
        bMove = true;
        bLink = bCopy = bTest = false;
    };

    void setLink(void)
    {
        bLink = true;
        bMove = bCopy = bTest = false;
    };

    void setCopy(void)
    {
        bCopy = true;
        bMove = bLink = bTest = false;
    };

    void setTest(void)
    {
        bTest = true;
        bMove = bLink = bCopy = false;
    };

    void sift(const fs::path& pDestination, const fs::path& pSource)
    {
        setSieve(pDestination);
        vector<fs::path> vItems;

        if (bDeep == false)
            for (auto& dEntry : fs::directory_iterator(pSource))
                vItems.push_back(dEntry.path());
        else
            for (auto& dEntry : fs::directory_iterator(pSource))
                if (fs::is_directory(dEntry))
                    for (auto& dSubEntry : fs::directory_iterator(dEntry))
                        vItems.push_back(dSubEntry.path());

        sort(vItems.begin(), vItems.end());

        for (auto& pItem : vItems)
            siftItem(pItem);
    }

private:

    void setSieve(const fs::path& pSieve)
    {
        for (auto& dSieveEntry : fs::directory_iterator(pSieve))
            if (fs::is_directory(dSieveEntry))
                vPath.push_back(dSieveEntry.path());

        // Improve predictability? 
        sort(vPath.begin(), vPath.end());

        for (int i = 0; i != vPath.size(); i++)
        {
            string sSieveEntryName = vPath[i].filename().string();
            vector<string> vsWordGroup;

            ba::trim(sSieveEntryName);
            ba::to_lower(sSieveEntryName);

            ba::split(vsWordGroup, sSieveEntryName, boost::is_any_of("(),"), boost::token_compress_on);

            for (auto& sWordGroup : vsWordGroup)
            {
                ba::trim(sWordGroup);

                int iScore = sWordGroup.length();

                if (iScore)
                {
                    vector<string> vsWord;
                    ba::split(vsWord, sWordGroup, boost::is_any_of(" "), boost::token_compress_on);
                    iScore = iScore - vsWord.size();
                    vMapping.push_back(make_pair(iScore, make_pair(i, vsWord)));
                }
            }
        }
        sort(vMapping.begin(), vMapping.end());
    }

    void hardlink(const fs::path& pItem, const fs::path& pDest, bool bShowError = true)
    {
        if (fs::is_regular_file(pItem))
        {
            fs::create_hard_link(pItem, pDest, ec);

            if (!bQuiet && (ec && (bVerbose || (bShowError || bVerbose))))
                cerr << "Warning! " << ec.message() << ": " << pDest << endl;
        }
        else if (fs::is_directory(pItem))
        {
            fs::copy_directory(pItem, pDest, ec);

            if (!bQuiet && (ec && (bShowError || bVerbose)))
                cerr << "Warning! " << ec.message() << ": " << pDest << endl;

            for (auto& dSubItem : fs::directory_iterator(pItem))
            {
                fs::path pNewDest = pDest / dSubItem.path().filename();
                hardlink(dSubItem.path(), pNewDest, false); // Recursion!
            }
        }
    }

    void move(const fs::path& pItem, const fs::path& pDest)
    {
        fs::rename(pItem, pDest, ec);

        if (!bQuiet && (ec && bVerbose))
            cerr << "Warning: " << ec.message() << pDest << endl;
    }

    void siftItem(const fs::path& pItem)
    {
        bool bNoMatch = true;

        string sItemName = pItem.filename().string();
        ba::to_lower(sItemName);

        /*
         * Horribly inefficient linear search. But it is in 
         * memory, so it is despite this reasonably fast.
         */

        for (auto& mapping : vMapping)
        {
            bool bFound = false;

            for (auto& vsWordGroup : mapping.second.second)
            {
                bFound = true;

                for (auto& sWord : mapping.second.second)
                {
                    // Normal match without ! first in word
                    if (sWord[0] != '!')
                    {
                        if (sItemName.find(sWord) == string::npos)
                        {
                            bFound = false;
                            break;
                        }
                    }
                        // Negated match
                    else if (sItemName.find(sWord.substr(1)) != string::npos)
                    {
                        bFound = false;
                        break;
                    }
                }
            }

            if (bFound == true)
            {
                const fs::path pDest = vPath[mapping.second.first] / pItem.filename();

                bNoMatch = false;

                if (bMove)
                {
                    if (bVerbose)
                        cout << "move\t" << pItem << "\\\n\t" << pDest << endl;

                    move(pItem, pDest);
                    return;
                }
                else if (bLink)
                {
                    if (bVerbose)
                        cout << "link\t" << pItem << "\\\n\t" << pDest << endl;

                    hardlink(pItem, pDest, true);
                }
            }
        }

        if (bTest && bNoMatch)
            cout << "No match: " << pItem << endl;
    }
};

int main(int argc, char* argv[])
{
    string sSource, sDestination;
    bool bError = false, bHelp = false;

    Sifter sifter;
    sifter.setTest(); // Default setting

    for (int i = 1; i < argc; i++)
    {
        string sArg = string(argv[i]);

        if ((sArg == "-m") || (sArg == "--move"))
            sifter.setMove();
        else if ((sArg == "-c") || (sArg == "--copy"))
            sifter.setCopy();
        else if ((sArg == "-l") || (sArg == "--link"))
            sifter.setLink();
        else if ((sArg == "-t") || (sArg == "--test"))
            sifter.setTest();
        else if ((sArg == "-d") || (sArg == "--deep"))
            sifter.setDeep();
        else if ((sArg == "-q") || (sArg == "--quiet"))
            sifter.setQuiet();
        else if ((sArg == "-h") || (sArg == "--help"))
            bHelp = true;
        else if ((sArg == "-v") || (sArg == "--verbose"))
            sifter.setVerbose();
        else if ((i == (argc - 2)) && fs::is_directory(sArg))
            sSource = sArg;
        else if ((i == (argc - 1)) && fs::is_directory(sArg))
            sDestination = sArg;
        else
        {
            cerr << "ERROR: " << sArg << endl;
            bError = true;
        }
    }

    if (!bHelp && ((sSource == "") || (sDestination == "")))
    {
        cerr << "Error: source and destination must be specified and exist!" << endl;
        bError = true;
    }

    if (bError || bHelp)
    {
        cerr << usage << endl;

        if (bHelp)
            cout << help << endl;

        return 1;
    }

    // Everything seems fine! Time to sift!
    sifter.sift(sDestination, sSource);

    return 0;
}
