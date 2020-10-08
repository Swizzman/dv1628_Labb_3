#include <iostream>
#include "fs.h"

FS::FS()
    : nrOfEntries(0), capacity(10)
{
    std::cout << "FS::FS()... Creating file system\n";
    entries = new dir_entry[capacity];

    fat[2] = 3;
    fat[3] = EOF;

    strcpy(entries[nrOfEntries].file_name, "hello");
    entries[nrOfEntries].first_blk = 2;
    entries[nrOfEntries].size = 5000;
    entries[nrOfEntries].type = 0;
    entries[nrOfEntries].access_rights = WRITE | READ;
    nrOfEntries++;

    char *buffer;
    buffer = new char[4096];

    strcpy(buffer, "hellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohello\n");

    disk.write(2, (uint8_t *)buffer);
    strcpy(buffer, "rofl\n");
    disk.write(3, (uint8_t *)buffer);
    // char *newBuffer = new char[4096 * 2];

    // disk.read(2, (uint8_t *)newBuffer);
    // disk.read(3, (uint8_t *)newBuffer);

    // std::cout << newBuffer << std::endl;

    //disk.read(1, (uint8_t *)&fat);
}

FS::~FS()
{
}

void FS::expand()
{
    this->capacity += 10;
    dir_entry *temp = new dir_entry[this->capacity];

    for (int i = 0; i < this->nrOfEntries; ++i)
    {
        temp[i] = this->entries[i];
    }
    delete entries;
    entries = temp;
}

void FS::updateFat()
{
    disk.write(1, (uint8_t *)&fat);
}

int FS::fileExists(std::string filename) const
{
    bool found = false;
    int index = -1;
    for (int i = 0; i < nrOfEntries && !found; ++i)
    {
        if (filename == entries[i].file_name)
        {
            index = i;
            found = true;
        }
    }
    return index;
}

void FS::write(int blocksToWrite, std::vector<std::string> &data)
{
    int i = ROOT_BLOCK + 2;
    bool inserted = false;
    int previous = -1;

    for (int j = 0; j < blocksToWrite; ++j)
    {
        for (i; i < (BLOCK_SIZE / 2) && !inserted; ++i)
        {
            if (fat[i] == FAT_FREE)
            {
                if (entries[nrOfEntries].first_blk == FAT_FREE)
                {
                    entries[nrOfEntries].first_blk = i;
                }
                if (previous != -1)
                {
                    fat[previous] = i;
                }
                inserted = true;
                fat[i] = EOF;
                previous = i;
                i--;
            }
        }
        disk.write(i, (uint8_t *)data.at(j).c_str());
        inserted = false;
    }
}

// formats the disk, i.e., creates an empty file system starting on an offset of 2
// disregarding ROOT_BLOCK and FAT_BLOCK
int FS::format()
{
    for (int i = ROOT_BLOCK + 2; i < (BLOCK_SIZE / 2); ++i)
    {
        fat[i] = FAT_FREE;
    }

    delete[] entries;
    nrOfEntries = 0;
    //updateFat();
    std::cout << "FS::format()\n";
    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int FS::create(std::string filepath)
{
    std::cout << "FS::create(" << filepath << ")\n";

    std::string data;
    bool done = false;

    while (!done)
    {
        std::getline(std::cin, data);
        if (data.empty())
        {
            done = true;
        }
        squeue.emplace(data);
    }

    if (squeue.size() == 1)
    {
        std::cout << "No data entered." << std::endl;
        squeue.pop();
    }
    else
    {
        while (!squeue.empty())
        {
            data += squeue.front();
            data += '\n';
            squeue.pop();
        }
        data.pop_back();

        std::cout << "data size: " << data.size() << std::endl;
        uint16_t noBlocks = (data.length() / BLOCK_SIZE) + 1;

        if (nrOfEntries >= capacity)
        {
            expand();
        }
        entries[nrOfEntries].size = data.length();
        entries[nrOfEntries].type = 0;
        entries[nrOfEntries].first_blk = FAT_FREE;
        strcpy(entries[nrOfEntries].file_name, filepath.c_str());
        entries[nrOfEntries].access_rights = READ | WRITE;

        int i = ROOT_BLOCK + 2;
        bool inserted = false;
        int previous = -1;
        std::vector<std::string> writeData;
        uint16_t bytesLeft = data.size() % BLOCK_SIZE;

        for (int k = 0; k < noBlocks; ++k)
        {
            if (k == noBlocks - 1)
            {
                writeData.push_back(data.substr((k * BLOCK_SIZE), bytesLeft));
            }
            else
            {
                writeData.push_back(data.substr((k * BLOCK_SIZE), BLOCK_SIZE));
            }
        }

        write(noBlocks, writeData);

        //     for (i; i < (BLOCK_SIZE / 2) && !inserted; ++i)
        //     {
        //         if (fat[i] == FAT_FREE)
        //         {
        //             if (entries[nrOfEntries].first_blk == FAT_FREE)
        //             {
        //                 entries[nrOfEntries].first_blk = i;
        //             }
        //             if (previous != -1)
        //             {
        //                 fat[previous] = i;
        //             }
        //             inserted = true;
        //             fat[i] = EOF;
        //             previous = i;
        //             i--;
        //         }
        //     }
        //     disk.write(i, (uint8_t *)writeData.c_str());

        //     inserted = false;
        // }
        nrOfEntries++;
        for (int j = 2; j < 12; ++j)
        {
            std::cout << "Fat: " << fat[j] << std::endl;
        }
        updateFat();
    }
    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";

    bool found = false;
    int iterator = 0;
    char *toRead = new char[BLOCK_SIZE];

    for (int i = 0; i < nrOfEntries && !found; ++i)
    {
        if (filepath == entries[i].file_name)
        {
            iterator = entries[i].first_blk;
            while (iterator != EOF)
            {
                disk.read(iterator, (uint8_t *)toRead);
                iterator = fat[iterator];
                std::cout << toRead;
            }
            found = true;
        }
    }
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls()
{
    std::cout << "FS::ls()\n";

    std::cout << "Filename:\t\tSize:\tStart:\t" << std::endl;

    for (int i = 0; i < nrOfEntries; ++i)
    {
        std::cout << entries[i].file_name << "\t\t\t" << entries[i].size << "\t" << entries[i].first_blk << std::endl;
    }
    return 0;
}

// cp <sourcefilepath> <destfilepath> makes an exact copy of the file
// <sourcefilepath> to a new file <destfilepath>
int FS::cp(std::string sourcefilepath, std::string destfilepath)
{
    std::cout << "FS::cp(" << sourcefilepath << "," << destfilepath << ")\n";
    int index = fileExists(sourcefilepath);

    if (index != -1)
    {
        if (nrOfEntries >= capacity)
        {
            expand();
        }
        strcpy(entries[nrOfEntries].file_name, destfilepath.c_str());
        entries[nrOfEntries].access_rights = entries[index].access_rights;
        entries[nrOfEntries].size = entries[index].size;
        entries[nrOfEntries].type = entries[index].type;

        char *buffer;
        int iterator = entries[index].first_blk;
        bool inserted = false;
        int i = ROOT_BLOCK + 2;
        int previous = -1;

        int noBlocks = (entries[index].size / BLOCK_SIZE) + 1;
        std::vector<std::string> data;

        while (iterator != EOF)
        {
            buffer = new char[BLOCK_SIZE];
            disk.read(iterator, (uint8_t *)buffer);
            iterator = fat[iterator];

            // for (i; i < (BLOCK_SIZE / 2) && !inserted; ++i)
            // {
            //     if (fat[i] == FAT_FREE)
            //     {
            //         if (entries[nrOfEntries].first_blk == FAT_FREE)
            //         {
            //             entries[nrOfEntries].first_blk = i;
            //         }
            //         if (previous != -1)
            //         {
            //             fat[previous] = i;
            //         }
            //         inserted = true;
            //         fat[i] = EOF;
            //         previous = i;
            //         i--;
            //     }
            // }
            // disk.write(i, (uint8_t *)buffer);
            data.push_back(buffer);
            delete[] buffer;
            inserted = false;
        }
        write(noBlocks, data);
        //updateFat()
        nrOfEntries++;
    }

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    int index = fileExists(sourcepath);

    if (index != -1)
    {
        strcpy(entries[index].file_name, destpath.c_str());
    }
    else
    {
        std::cout << "That file does not exist!" << std::endl;
    }

    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int FS::rm(std::string filepath)
{
    std::cout << "FS::rm(" << filepath << ")\n";

    int index = fileExists(filepath);

    if (index != -1)
    {
        int iterator = entries[index].first_blk;
        int previous = entries[index].first_blk;
        while (iterator != EOF)
        {
            iterator = fat[iterator];
            fat[previous] = 0;
            previous = iterator;
        }
        //We are at EOF
        fat[iterator] = 0;

        for(int i = index; i < nrOfEntries; ++i)
        {
            entries[i] = entries[i + 1];
        }
        nrOfEntries--;
    }
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
