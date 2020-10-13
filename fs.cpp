#include <iostream>
#include "fs.h"

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
    dirs.capacity = 10;
    entries = new dir_entry[dirs.capacity];
    dirs.nrOfEntries = 0;
    format();

    // Initializing working directory
    entries[dirs.nrOfEntries].size = 0;
    strcpy(entries[dirs.nrOfEntries].file_name, "~");
    entries[dirs.nrOfEntries].first_blk = ROOT_BLOCK;
    entries[dirs.nrOfEntries].type = TYPE_DIR;
    entries[dirs.nrOfEntries].nrOfSubDir = 1;
    dirs.nrOfEntries++;
    workingDir = &entries[0];
    workingDirAsString = "~";

    fat[3] = 4;
    fat[4] = 5;
    fat[5] = EOF;

    strcpy(entries[dirs.nrOfEntries].file_name, "hello");
    entries[dirs.nrOfEntries].first_blk = 3;
    entries[dirs.nrOfEntries].size = 5000;
    entries[dirs.nrOfEntries].type = TYPE_FILE;
    entries[dirs.nrOfEntries].access_rights = WRITE | READ;
    entries[dirs.nrOfEntries].nrOfSubDir = 1;
    disk.write(3, (uint8_t *)&entries[dirs.nrOfEntries]);
    dirs.nrOfEntries++;

    char *buffer;
    buffer = new char[BLOCK_SIZE];

    strcpy(buffer, "hellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohello\n");

    disk.write(4, (uint8_t *)buffer);
    strcpy(buffer, "rofl\n");
    disk.write(5, (uint8_t *)buffer);
    delete[] buffer;
    entries[0].nrOfSubDir = 2;
    entries[0].subDir[1] = std::make_pair(3, "hello");
    // char *newBuffer = new char[4096 * 2];

    // disk.read(2, (uint8_t *)newBuffer);
    // disk.read(3, (uint8_t *)newBuffer);

    // std::cout << newBuffer << std::endl;

    //disk.read(1, (uint8_t *)&fat);
}

FS::~FS()
{
    delete[] entries;
}

void FS::expand()
{
    this->dirs.capacity += 10;
    dir_entry *temp = new dir_entry[this->dirs.capacity];

    for (int i = 0; i < this->dirs.nrOfEntries; ++i)
    {
        temp[i] = this->entries[i];
    }
    delete[] entries;
    entries = temp;
}

void FS::updateFat()
{
    disk.write(1, (uint8_t *)&fat);
}

int FS::fileExists(std::string filename, uint8_t type) const
{
    bool found = false;
    int index = -1;
    for (int i = 0; i < dirs.nrOfEntries && !found; ++i)
    {
        if (filename == entries[i].file_name && type == entries[i].type)
        {
            index = i;
            found = true;
        }
    }
    return index;
}

void FS::write(int blocksToWrite, std::vector<std::string> &data)
{
    int i = DIR_BLOCK + 1;
    bool inserted = false;
    int previous = -1;

    for (int j = 0; j < blocksToWrite + 1; ++j)
    {
        for (i; i < (BLOCK_SIZE / 2) && !inserted; ++i)
        {
            if (fat[i] == FAT_FREE)
            {
                if (entries[dirs.nrOfEntries].first_blk == FAT_FREE)
                {
                    entries[dirs.nrOfEntries].first_blk = i;
                    // IF SIZE PROBLEM OR ANYTHING, LOOK HERE, MOVE WRITE?
                    disk.write(i, (uint8_t *)&entries[dirs.nrOfEntries]);
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
        // Do not write data when j == 0, because thats reserved for the struct, rest is data.
        if (j != 0)
        {
            disk.write(i, (uint8_t *)data.at(j - 1).c_str());
        }
        inserted = false;
    }
}

std::string FS::goToPath(std::string newPath)
{
    std::string paths;
    workingDir = &entries[0];
    workingDirAsString = "~";
    bool found = false;
    for (int i = 0; i < newPath.size(); ++i)
    {
        if (newPath[i] != '/')
        {
            paths += newPath[i];
        }
        else
        {
            for (int j = 1; j < workingDir->nrOfSubDir && !found; ++j)
            {
                if (paths == workingDir->subDir[j].second)
                {
                    for (int k = 0; k < dirs.nrOfEntries && !found; ++k)
                    {
                        if (paths == entries[k].file_name)
                        {
                            workingDir = &entries[k];
                            workingDirAsString.append("/" + paths);
                            found = true;
                            paths.clear();
                        }
                    }
                }
            }
            found = false;
        }
    }
    return paths;
}

// formats the disk, i.e., creates an empty file system starting on an offset of 2
// disregarding ROOT_BLOCK and FAT_BLOCK
int FS::format()
{
    for (int i = DIR_BLOCK + 1; i < (BLOCK_SIZE / 2); ++i)
    {
        fat[i] = FAT_FREE;
    }

    delete[] entries;
    entries = new dir_entry[dirs.capacity];
    dirs.nrOfEntries = 0;

    //updateFat();
    std::cout << "FS::format()\n";
    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int FS::create(std::string filepath)
{
    std::cout << "FS::create(" << filepath << ")\n";

    std::string data, diskData;
    bool done = false;
    std::string fileName = goToPath(filepath);
    std::cout << "Creating file inside subdir: " << filepath << std::endl;
    std::cout << "File name: " << fileName << std::endl;

    while (!done)
    {
        std::getline(std::cin, data);
        if (data.empty())
        {
            done = true;
        }
        else
        {
            data += '\n';
            diskData.append(data);
        }
    }

    if (diskData.size() == 0)
    {
        std::cout << "No data entered." << std::endl;
    }
    else
    {
        std::cout << "data size: " << diskData.size() << std::endl;
        uint16_t noBlocks = (diskData.length() / BLOCK_SIZE) + 1;

        if (dirs.nrOfEntries >= dirs.capacity)
        {
            expand();
        }

        entries[dirs.nrOfEntries].size = diskData.length();
        entries[dirs.nrOfEntries].type = TYPE_FILE;
        entries[dirs.nrOfEntries].first_blk = FAT_FREE;
        strcpy(entries[dirs.nrOfEntries].file_name, fileName.c_str());
        entries[dirs.nrOfEntries].access_rights = READ | WRITE;

        int i = DIR_BLOCK + 1;
        bool inserted = false;
        int previous = -1;
        std::vector<std::string> dataVec;
        uint16_t bytesLeft = diskData.size() % BLOCK_SIZE;

        for (int k = 0; k < noBlocks; ++k)
        {
            if (k == noBlocks - 1)
            {
                dataVec.push_back(diskData.substr((k * BLOCK_SIZE), bytesLeft));
            }
            else
            {
                dataVec.push_back(diskData.substr((k * BLOCK_SIZE), BLOCK_SIZE));
            }
        }

        write(noBlocks, dataVec);
        workingDir->subDir[workingDir->nrOfSubDir++] = std::make_pair(entries[dirs.nrOfEntries].first_blk, fileName);
        dirs.nrOfEntries++;
        //Update the directoryinformation on disk
        disk.write(DIR_BLOCK, (uint8_t *)entries);

        for (int j = 3; j < 12; ++j)
        {
            std::cout << "Fat: " << fat[j] << std::endl;
        }
        //updateFat();
    }
    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";

    bool found = false;
    int iterator = 0;
    char *buffer = new char[BLOCK_SIZE];
    std::string fileName = goToPath(filepath);
    int index = fileExists(fileName, TYPE_FILE);

    if (index != EOF)
    {
        iterator = fat[entries[index].first_blk];
        while (iterator != EOF)
        {
            disk.read(iterator, (uint8_t *)buffer);
            iterator = fat[iterator];
            std::cout << buffer;
        }
        found = true;
    }
    else
    {
        std::cout << "'" << fileName << "' does not exist!" << std::endl;
    }

    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls()
{
    std::cout << "FS::ls()\n";

    std::cout << "Filename:\t\tType:\t\tSize:\tStart:\t" << std::endl;

    for (int i = 0; i < dirs.nrOfEntries; ++i)
    {
        std::cout << entries[i].file_name << "\t\t\t";
        if (entries[i].type == TYPE_FILE)
        {
            std::cout << "File\t\t";
        }
        else
        {
            std::cout << "Directory\t";
        }

        std::cout << entries[i].size << "\t" << entries[i].first_blk << std::endl;
    }
    return 0;
}

// cp <sourcefilepath> <destfilepath> makes an exact copy of the file
// <sourcefilepath> to a new file <destfilepath>
int FS::cp(std::string sourcefilepath, std::string destfilepath)
{
    std::cout << "FS::cp(" << sourcefilepath << "," << destfilepath << ")\n";
    int index = fileExists(sourcefilepath, TYPE_FILE);

    if (index != -1)
    {
        if (dirs.nrOfEntries >= dirs.capacity)
        {
            expand();
        }
        strcpy(entries[dirs.nrOfEntries].file_name, destfilepath.c_str());
        entries[dirs.nrOfEntries].access_rights = entries[index].access_rights;
        entries[dirs.nrOfEntries].size = entries[index].size;
        entries[dirs.nrOfEntries].type = entries[index].type;

        char *buffer;
        int iterator = entries[index].first_blk;
        bool inserted = false;
        int i = DIR_BLOCK + 1;
        int previous = -1;

        int noBlocks = (entries[index].size / BLOCK_SIZE) + 1;
        std::vector<std::string> data;

        while (iterator != EOF)
        {
            buffer = new char[BLOCK_SIZE];
            disk.read(iterator, (uint8_t *)buffer);
            iterator = fat[iterator];
            data.push_back(buffer);
            delete[] buffer;
            inserted = false;
        }
        write(noBlocks, data);
        dirs.nrOfEntries++;
        disk.write(DIR_BLOCK, (uint8_t *)entries);
        //updateFat()
    }

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    int index = fileExists(sourcepath, TYPE_FILE);

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

    int index = fileExists(filepath, TYPE_FILE);

    if (index != -1)
    {
        int iterator = entries[index].first_blk;
        int previous = entries[index].first_blk;
        while (iterator != EOF)
        {
            iterator = fat[iterator];
            fat[previous] = FAT_FREE;
            previous = iterator;
        }

        //fat[iterator] = FAT_FREE;
        for (int i = index; i < dirs.nrOfEntries; ++i)
        {
            entries[i] = entries[i + 1];
        }
        dirs.nrOfEntries--;
        disk.write(DIR_BLOCK, (uint8_t *)entries);
    }
    else
    {
        std::cout << "'" << filepath << "' does not exist!" << std::endl;
    }

    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";

    int index1 = fileExists(filepath1, TYPE_FILE);
    int index2 = fileExists(filepath2, TYPE_FILE);
    int appendIndex = -1;

    if (index1 != EOF && index2 != EOF)
    {
        std::string line;
        char *buffer = new char[BLOCK_SIZE];
        int iterator1 = entries[index1].first_blk;
        int iterator2 = entries[index2].first_blk;

        while (iterator2 != EOF)
        {
            //Only increment appendIndex if we're not at EOF, so that we can start append other files FAT-indexes.
            if (iterator2 != EOF)
            {
                appendIndex = iterator2;
            }
            iterator2 = fat[iterator2];
        }
        disk.read(appendIndex, (uint8_t *)buffer);
        line.append(buffer);
        while (iterator1 != EOF)
        {
            disk.read(iterator1, (uint8_t *)buffer);
            line.append(buffer);
            iterator1 = fat[iterator1];
        }

        uint32_t noBlocks = (line.length() / BLOCK_SIZE) + 1;
        uint32_t lastBlock = (line.length() % BLOCK_SIZE);
        std::vector<std::string> data;
        for (int i = 0; i < noBlocks; ++i)
        {
            if (i == noBlocks - 1)
            {
                data.push_back(line.substr((i * BLOCK_SIZE), lastBlock));
            }
            else
            {
                data.push_back(line.substr((i * BLOCK_SIZE), BLOCK_SIZE));
            }
        }
        disk.write(appendIndex, (uint8_t *)data[0].c_str());
        data.erase(data.begin());

        entries[index2].size += entries[index1].size;
        int count = 0;
        int previous = -1;
        int i = DIR_BLOCK + 1;
        bool inserted = false;
        if (data.size() > 0)
        {
            for (int j = 0; j < data.size(); ++j)
            {
                for (i; i < (BLOCK_SIZE / 2) && !inserted; ++i)
                {
                    if (fat[i] == FAT_FREE)
                    {
                        if (fat[appendIndex] == EOF)
                        {
                            fat[appendIndex] = i;
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
                inserted = false;
                disk.write(i, (uint8_t *)data[j].c_str());
            }
        }
    }
    else
    {
        if (index1 == EOF && index2 == EOF)
        {
            std::cout << "'" << filepath1 << "' and '" << filepath2 << "' does not exist!" << std::endl;
        }
        else if (index1 == EOF && index2 != EOF)
        {
            std::cout << "'" << filepath1 << "' does not exist!" << std::endl;
        }
        else if (index1 != EOF && index2 == EOF)
        {
            std::cout << "'" << filepath2 << "' does not exist!" << std::endl;
        }
    }

    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    std::string name;

    name = goToPath(dirpath);
    bool found = false;
    int index = fileExists(name, TYPE_DIR);

    if (index != -1)
    {
        std::cout << "Directory with that name already exists!" << std::endl;
        return -1;
    }

    strcpy(entries[dirs.nrOfEntries].file_name, name.c_str());
    entries[dirs.nrOfEntries].type = TYPE_DIR;
    entries[dirs.nrOfEntries].access_rights = READ | WRITE;
    entries[dirs.nrOfEntries].nrOfSubDir = 1;
    entries[dirs.nrOfEntries].size = 0;

    std::vector<std::string> temp;

    write(0, temp);
    workingDir->subDir[workingDir->nrOfSubDir] = make_pair(entries[dirs.nrOfEntries].first_blk, name);
    workingDir->nrOfSubDir++;
    dirs.nrOfEntries++;
    disk.write(DIR_BLOCK, (uint8_t *)entries);

    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int FS::cd(std::string dirpath)
{
    std::cout << "FS::cd(" << dirpath << ")\n";

    goToPath(dirpath + "/");

    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int FS::pwd()
{
    std::cout << "FS::pwd()\n";

    std::cout << workingDirAsString << std::endl;
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}
