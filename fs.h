#include <iostream>
#include <cstdint>
#include <vector>
#include <stdio.h>
#include <string.h>
#include "disk.h"

#ifndef __FS_H__
#define __FS_H__

#define ROOT_BLOCK 0
#define FAT_BLOCK 1
#define FAT_FREE 0
#define FAT_EOF -1

// Defines for directories used in the array 'catalogs' and struct array 'dirs'.
// catalogs[ROOT] is a reference to root root can contain sub-directories 
// and files which is specified in 'dirs'.
// Example: catalogs[ROOT].dirs[ITSELF] is a reference to itself and 
// catalogs[ROOT].dirs[PARENT] is it parent which is also itself (edge case).
// Rest of the indexes is it's sub-directories and it's containing files.
#define MAX_DIRECTORIES 58
#define ROOT_DIR 0
#define ITSELF 0
#define PARENT 1

#define TYPE_FILE 0
#define TYPE_DIR 1
#define READ 0x04
#define WRITE 0x02
#define EXECUTE 0x01

#define PARENT_DIR ".."

#define NOT_FOUND -1
#define DIRECTORY -2

struct dir_entry
{
    char file_name[56];    // name of the file / sub-directory
    uint16_t first_blk;    // index in the FAT for the first block of the file
    uint32_t size;         // size of the file in bytes
    uint8_t type;          // directory (1) or file (0)
    uint8_t access_rights; // read (0x04), write (0x02), execute (0x01)
};

// Struct that contains all entries of files and directories, initializing nrOfSubDir to 2
// simply because every directory has a reference to itself and its parent.
struct dir_helper
{
    dir_entry dirs[MAX_DIRECTORIES];
    uint16_t nrOfSubDir = 2;
};

class FS
{
private:
    Disk disk;
    // size of a FAT entry is 2 bytes
    int16_t fat[BLOCK_SIZE / 2];
    dir_helper *workingDir;
    dir_helper* catalogs;
    std::string workingDirAsString;
    int nrOfDirs;

    void updateFat();
    // Checks if the file exists or not, returns the index where it belongs to in the catalogs
    int fileExists(std::string filename, uint8_t type) const;
    // Writes the directory's meta data to disk
    void writeDir(dir_helper &catalog);
    // Writes the file's data blocks to disk and updates FAT table
    void writeFile(int blocksToWrite, std::vector<std::string> &data);
    void appendFile(int blocksToWrite, std::vector<std::string> &data, int appendIndex);
    std::vector<std::string> readFile(int startBlock);
    // Changes working directory to the path given and returns the last string after the last '/'
    bool goToPath(std::string newPath);
    std::string getNameOfPath(std::string pathName);
    std::string decodeAccessRights(uint8_t accessRights);
    void boot();
    void bootHelper(int readBlock);
    void updateFolder(dir_helper &catalog);

public:
    FS();
    ~FS();
    // formats the disk, i.e., creates an empty file system
    int format();
    // create <filepath> creates a new file on the disk, the data content is
    // written on the following rows (ended with an empty row)
    int create(std::string filepath);
    // cat <filepath> reads the content of a file and prints it on the screen
    int cat(std::string filepath);
    // ls lists the content in the currect directory (files and sub-directories)
    int ls();

    std::string getWorkingDirAsString()const;
    // cp <sourcefilepath> <destfilepath> makes an exact copy of the file
    // <sourcefilepath> to a new file <destfilepath>
    int cp(std::string sourcefilepath, std::string destfilepath);
    // mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
    // or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
    int mv(std::string sourcepath, std::string destpath);
    // rm <filepath> removes / deletes the file <filepath>
    int rm(std::string filepath);
    // append <filepath1> <filepath2> appends the contents of file <filepath1> to
    // the end of file <filepath2>. The file <filepath1> is unchanged.
    int append(std::string filepath1, std::string filepath2);

    // mkdir <dirpath> creates a new sub-directory with the name <dirpath>
    // in the current directory
    int mkdir(std::string dirpath);
    // cd <dirpath> changes the current (working) directory to the directory named <dirpath>
    int cd(std::string dirpath);
    // pwd prints the full path, i.e., from the root directory, to the current
    // directory, including the currect directory name
    int pwd();

    // chmod <accessrights> <filepath> changes the access rights for the
    // file <filepath> to <accessrights>.
    int chmod(std::string accessrights, std::string filepath);
};

#endif // __FS_H__