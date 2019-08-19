# sift
sift - a file sifter

sift is a utility that allows you to automatically sift source folders for items (files or folders) and selectively move or hard link these items to subfolders in a destination folder.

## Usage
```
sift [OPTIONS] source/ destination/

Both source and destination must exist and be in the same filesystem.

Options:
 -m --move    : Move items. (Files and/or folders.)
 -l --link    : Make folders in destination, hardlink files from source.
 -t --test    : Test and report any items failing to matching anything.
 -d --deep    : Sift deeper using source folder sub-subfolders.
 -h --help    : Display more help.
 -q --quiet   : Donẗ show warnings about files already existing.
 -v --verbose : Output info about moved/linked items.
```
## Compile and install sift
sift comes in just one single cpp-file, sift.cpp, so it's easy to compile. 

First make sure that build_essential and bost are installed:
```
sudo apt install build_essential libboost-all-dev
```
or (to avoid installing all of boost.)
```
sudo apt install build_essential libboost-filesystem-dev libboost-system-dev
```
Then compile sift with this command:
```
g++ sift.cpp -o sift -lboost-system -lboost-filesystem -O2
```
Then just place the newly compiled executable file "swift" somewhere in your path.

## The source folder
sift can be used with two different types of source folders. Flat without any extra subfolder structure or with a single level extra subfolder structure.

### Flat source folder without any subfolder structure
It is assumed that each file or folder at the level directly below the source folder is a separate item that you wish to sift through. A typical example might be a download folder.
### Source folder with a single level subfolder structure.
If you run sift with the --deep option it is assumed that the source folder contains a set of subfolders, each directly containing a bunch of files or folders that you wish to sift throug. An examples might be a folder with a set of several download folders. The destination folder for sift is this type of folder. That means that you can sift in several steps and use a previous destination folder as the new source folder.
## The destination folder
In the destination folder you need to create subfolders that work as the holes in a sieve. Each item is tested against each "hole" in the destination folder subfolders. Destination folder subfolders are given names containing groups of words. If one word-group match, then the item from the source folder is moved or hard linked to that destination subfolder.
#### Word-groups in destination subfolder names
Destination subfolders are named using one or more word-group. Word-groups in the destination subfolder name are separated using ',', ')' or '('. Words in the word-group are separated with spaces.
Example: 
```
/path to destination/Science Fiction (space-opera, sci-fi)
```
Normaly its is enough that all words in a word-group match, in any order. But that can give too many false matches. To avoid that is it possible to also specify words in a word-group that must NOT match by prefixing the words with a '!' (exclamation point). It is also possible to specify that two words must match and be in order and in addition also near each other. At the most 1 character between them. You do this by having a '_' (underscore) between those two words. (TODO: '_' not implemented yet.)
Example:
```
/path to destination/Science_Fiction !fantasy !steam-punk (space_opera, sci_fi)
```
### Move --move or hard link --link 
You may decide if matching source items are to be moved or hard linked using the options --move or --link.

If a source item is moved it is moved only to the first match. Matches are made in approximate difficulty order so that the most difficult matches are tried first. 

If a source item is hard linked, and it is a folder structure, then identical subfolders are created, recursively, in the matching destination subfolder and hard links to any files are also created in these subfolders.

### Test
Instead of using --move or --link you can use --test. Then sift will report all source items that doesn't match any destination subfolder. This is useful to find source items that otherwise would not be sifted successfully.
