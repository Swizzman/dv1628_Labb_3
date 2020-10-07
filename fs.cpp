#include <iostream>
#include "fs.h"

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
}

FS::~FS()
{
}

// formats the disk, i.e., creates an empty file system starting on an offset of 2
// disregarding ROOT_BLOCK and FAT_BLOCK
int FS::format()
{
    for (int i = ROOT_BLOCK + 2; i < (BLOCK_SIZE / 2); ++i)
    {
        fat[i] = FAT_FREE;
    }
    std::cout << "FS::format()\n";
    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int FS::create(std::string filepath)
{
    std::cout << "FS::create(" << filepath << ")\n";

    std::string data;

    std::ifstream file("bajs.txt");

    if (file.is_open())
    {
        std::getline(file, data, '\n');
    }
    std::cout << std::endl
              << data.length() << std::endl;

    if (data.length() > 0)
    {
        dir_entry fileData;
        fileData.size = data.size();
        fileData.type = TYPE_FILE;
        int i = 0;
        //length() + 1 for null termination
        for (i; i < filepath.length() + 1; ++i)
        {
            fileData.file_name[i] = filepath[i];
        }
        fileData.access_rights = READ | WRITE;

        bool inserted = false;

        for (i = ROOT_BLOCK + 2; i < (BLOCK_SIZE / 2) && !inserted; ++i)
        {
            if (fat[i] == FAT_FREE)
            {
                fileData.first_blk = i;
                inserted = true;
            }
        }

        uint32_t noBlocks = 0;

        noBlocks = (data.length() / BLOCK_SIZE) + 1;

        std::cout << "#No blocks: " << noBlocks << " First block: " << fileData.first_blk << std::endl;

        //disk.write(noBlocks, (uint8_t*)fileData.first_blk);
    }

    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls()
{
    std::cout << "FS::ls()\n";
    return 0;
}

// cp <sourcefilepath> <destfilepath> makes an exact copy of the file
// <sourcefilepath> to a new file <destfilepath>
int FS::cp(std::string sourcefilepath, std::string destfilepath)
{
    std::cout << "FS::cp(" << sourcefilepath << "," << destfilepath << ")\n";
    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int FS::rm(std::string filepath)
{
    std::cout << "FS::rm(" << filepath << ")\n";
    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int FS::cd(std::string dirpath)
{
    std::cout << "FS::cd(" << dirpath << ")\n";
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int FS::pwd()
{
    std::cout << "FS::pwd()\n";
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}
