#include <iostream>
#include "fs.h"

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
    capacity = 25;
    format();
    catalogs = new dir_helper[MAX_DIRECTORIES];
    // catalogs[0].dirs = new dir_entry *[MAX_DIRECTORIES];
    // catalogs[1].dirs = new dir_entry *[MAX_DIRECTORIES];
    nrOfDirs = 0;

    //Creating metadata for Root-directory
    catalogs[nrOfDirs].dirs[ITSELF] = new dir_entry;
    strcpy(catalogs[nrOfDirs].dirs[ITSELF]->file_name, "~");
    catalogs[nrOfDirs].dirs[ITSELF]->first_blk = ROOT_BLOCK;
    catalogs[nrOfDirs].dirs[ITSELF]->size = BLOCK_SIZE;
    catalogs[nrOfDirs].dirs[ITSELF]->type = TYPE_DIR;
    catalogs[nrOfDirs].dirs[ITSELF]->access_rights = READ | WRITE | EXECUTE;
    catalogs[nrOfDirs].dirs[PARENT] = catalogs[nrOfDirs].dirs[ITSELF];
    fat[ROOT_BLOCK] = EOF;

    workingDir = &catalogs[nrOfDirs];
    nrOfDirs++;
    workingDirAsString = "~";

    //Creating meta-data for a new sub-directory
    catalogs[nrOfDirs].dirs[ITSELF] = new dir_entry;
    strcpy(catalogs[nrOfDirs].dirs[ITSELF]->file_name, "sub");
    catalogs[nrOfDirs].dirs[ITSELF]->first_blk = 2;
    fat[2] = EOF;
    catalogs[nrOfDirs].dirs[PARENT] = catalogs[ROOT_DIR].dirs[ITSELF];
    catalogs[nrOfDirs].dirs[ITSELF]->type = TYPE_DIR;
    catalogs[nrOfDirs].dirs[ITSELF]->size = BLOCK_SIZE;
    catalogs[nrOfDirs].dirs[ITSELF]->access_rights = READ | WRITE | EXECUTE;

    // Create a reference of the new sub-dir to ROOT (nrOfSubDir starts at 2. 0 and 1 reserved for itself/parent)
    catalogs[ROOT_DIR].dirs[catalogs[ROOT_DIR].nrOfSubDir++] = catalogs[nrOfDirs].dirs[ITSELF];
    nrOfDirs++;

    fat[3] = 4;
    fat[4] = EOF;

    catalogs[nrOfDirs].dirs[ITSELF] = new dir_entry;
    strcpy(catalogs[nrOfDirs].dirs[ITSELF]->file_name, "hello");
    catalogs[nrOfDirs].dirs[ITSELF]->first_blk = 3;
    catalogs[nrOfDirs].dirs[ITSELF]->type = TYPE_FILE;
    catalogs[nrOfDirs].dirs[ITSELF]->size = 5000;
    catalogs[nrOfDirs].dirs[ITSELF]->access_rights = READ | WRITE;
    catalogs[nrOfDirs].dirs[PARENT] = catalogs[1].dirs[ITSELF];

    //Place the file "hello" as a subdir entry of the sub-file sub
    catalogs[1].dirs[catalogs[1].nrOfSubDir++] = catalogs[nrOfDirs].dirs[ITSELF];

    char *buffer;
    buffer = new char[BLOCK_SIZE];

    strcpy(buffer, "hellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohello\n");

    disk.write(3, (uint8_t *)buffer);
    strcpy(buffer, "rofl\n");
    disk.write(4, (uint8_t *)buffer);
    delete[] buffer;
}

FS::~FS()
{
    for (int i = 1; i < nrOfDirs; ++i)
    {
        for (int j = 2; j < catalogs[i].nrOfSubDir; j++)
        {
            delete catalogs[i].dirs[j];
        }
    }
    delete catalogs[ROOT_BLOCK].dirs[0];
    delete[] catalogs;
}

void FS::expand()
{
}

std::string FS::getWorkingDirAsString() const
{
    return this->workingDirAsString;
}

void FS::updateFat()
{
    disk.write(1, (uint8_t *)&fat);
}

int FS::fileExists(std::string filename, uint8_t type) const
{
    bool found = false;
    int index = -1;

    for (int i = 2; i < workingDir->nrOfSubDir && !found; ++i)
    {
        if (filename == workingDir->dirs[i]->file_name && type == workingDir->dirs[i]->type)
        {
            index = i;
            found = true;
        }
        else if (filename == workingDir->dirs[i]->file_name && type != workingDir->dirs[i]->type)
        {
            index = -2;
            found = true;
        }
    }
    return index;
}

void FS::writeFile(int blocksToWrite, std::vector<std::string> &data)
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
                if (workingDir->dirs[workingDir->nrOfSubDir]->first_blk == FAT_FREE)
                {
                    workingDir->dirs[workingDir->nrOfSubDir]->first_blk = i;
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

std::string FS::goToPath(std::string newPath)
{
    std::string paths;
    bool found = false;

    for (int i = 0; i < newPath.size(); ++i)
    {
        if (newPath[i] != '/')
        {
            paths += newPath[i];
        }
        else
        {
            if ((paths == "~") || (paths.empty() && i == 0))
            {
                workingDir = &catalogs[ROOT_DIR];
                workingDirAsString = "~";
                paths.clear();
                continue;
            }
            else if (paths == "..")
            {
                paths = workingDir->dirs[PARENT]->file_name;
            }
            for (int j = 1; j < workingDir->nrOfSubDir && !found; ++j)
            {
                if (paths == workingDir->dirs[j]->file_name)
                {
                    for (int k = 0; k < nrOfDirs && !found; ++k)
                    {
                        if (paths == catalogs[k].dirs[ITSELF]->file_name)
                        {
                            if (paths != workingDir->dirs[PARENT]->file_name)
                            {
                                workingDirAsString.append("/" + paths);
                            }
                            else
                            {
                                workingDirAsString.erase(workingDirAsString.size() - strlen(workingDir->dirs[ITSELF]->file_name) - 1,
                                                         strlen(workingDir->dirs[ITSELF]->file_name) + 1);
                            }
                            workingDir = &catalogs[k];
                            found = true;
                            paths.clear();
                        }
                    }
                }
            }
        }
        found = false;
    }
    return paths;
}

// formats the disk, i.e., creates an empty file system starting on an offset of 2
// disregarding ROOT_BLOCK and FAT_BLOCK
int FS::format()
{
    for (int i = ROOT_BLOCK + 2; i < (BLOCK_SIZE / 2); ++i)
    {
        fat[i] = FAT_FREE;
    }

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
    dir_helper *tempDir = workingDir;
    std::string dirString = workingDirAsString;
    std::string fileName = goToPath(filepath);
    int index = fileExists(fileName, TYPE_FILE);

    if(fileName == "~" || fileName.empty())
    {
        std::cout << fileName << ": Reserved name, aborting!" << std::endl;
    }
    else if (index > -1)
    {
        std::cout << fileName << ": Already exists!" << std::endl;
    }
    else
    {
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

            dir_entry *newEntry = new dir_entry;
            strcpy(newEntry->file_name, fileName.c_str());
            newEntry->type = TYPE_FILE;
            newEntry->first_blk = FAT_FREE;
            newEntry->size = diskData.length();
            newEntry->access_rights = READ | WRITE;

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

            workingDir->dirs[workingDir->nrOfSubDir] = newEntry;
            writeFile(noBlocks, dataVec);
            workingDir->nrOfSubDir++;

            for (int j = (ROOT_BLOCK + 2); j < 12; ++j)
            {
                std::cout << "Fat: " << fat[j] << std::endl;
            }
            //updateFat();
        }
    }
    workingDir = tempDir;
    workingDirAsString = dirString;
    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";

    bool found = false;
    int iterator = 0;
    char *buffer = new char[BLOCK_SIZE];
    dir_helper *tempDir = workingDir;
    std::string stringDir = workingDirAsString;
    std::string fileName = goToPath(filepath);
    int index = fileExists(fileName, TYPE_FILE);
    if (index == -2)
    {
        std::cout << fileName << ": Is a directory!" << std::endl;
    }
    else if (index != EOF)
    {
        iterator = workingDir->dirs[index]->first_blk;
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
        std::cout << fileName << ": No such file or directory!" << std::endl;
    }
    workingDir = tempDir;
    workingDirAsString = stringDir;
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls()
{
    std::cout << "FS::ls()\n";

    std::cout << "Filename:\t\tType:\t\tSize:\tStart:\t" << std::endl;

    for (int i = 0; i < workingDir->nrOfSubDir; ++i)
    {
        if (i == ITSELF)
        {
            std::cout << "."
                      << "\t\t\t";
        }
        else if (i == PARENT)
        {
            std::cout << ".."
                      << "\t\t\t";
        }
        else
        {
            std::cout << workingDir->dirs[i]->file_name << "\t\t\t";
        }

        if (workingDir->dirs[i]->type == TYPE_FILE)
        {
            std::cout << "File\t\t";
        }
        else
        {
            std::cout << "Directory\t";
        }
        std::cout << workingDir->dirs[i]->size << "\t" << workingDir->dirs[i]->first_blk << std::endl;
    }
    return 0;
}

std::vector<std::string> FS::readFile(int startBlock)
{
    int iterator = startBlock;
    char *buffer;
    std::vector<std::string> data;

    while (iterator != EOF)
    {
        buffer = new char[BLOCK_SIZE];
        disk.read(iterator, (uint8_t *)buffer);
        iterator = fat[iterator];
        data.push_back(buffer);
        delete[] buffer;
    }
    return data;
}

// cp <sourcefilepath> <destfilepath> makes an exact copy of the file
// <sourcefilepath> to a new file <destfilepath>
int FS::cp(std::string sourcefilepath, std::string destfilepath)
{
    std::cout << "FS::cp(" << sourcefilepath << "," << destfilepath << ")\n";
    dir_helper *tempDir = workingDir;
    std::string dirString = workingDirAsString;

    std::string fileName = goToPath(sourcefilepath);
    int index = fileExists(fileName, TYPE_FILE);

    if (index == -2)
    {
        std::cout << fileName << ": Is a directory!" << std::endl;
    }
    else if (index == -1)
    {
        std::cout << fileName << ": No such file or directory!" << std::endl;
    }
    if (index > -1)
    {
        dir_entry *copyEntry = new dir_entry;
        strcpy(copyEntry->file_name, fileName.c_str());
        copyEntry->access_rights = workingDir->dirs[index]->access_rights;
        copyEntry->size = workingDir->dirs[index]->size;
        copyEntry->type = workingDir->dirs[index]->type;
        copyEntry->first_blk = FAT_FREE;

        // int noBlocks = (entries[index].size / BLOCK_SIZE) + 1;
        int noBlocks = (workingDir->dirs[index]->size / BLOCK_SIZE) + 1;
        std::vector<std::string> data = readFile(workingDir->dirs[index]->first_blk);

        goToPath(destfilepath.append("/"));

        workingDir->dirs[workingDir->nrOfSubDir] = copyEntry;

        writeFile(noBlocks, data);
        workingDir->nrOfSubDir++;
        //updateFat();
    }

    workingDirAsString = dirString;
    workingDir = tempDir;
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
        // strcpy(entries[index].file_name, destpath.c_str());
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

    dir_helper *tempDir = workingDir;
    std::string dirString = workingDirAsString;
    std::string fileName = goToPath(filepath);
    int index = fileExists(fileName, TYPE_FILE);

    if (index == -2)
    {
        std::cout << fileName << ": is a directory!" << std::endl;
    }
    else if (index == -1)
    {
        std::cout << fileName << ": No such file or directory!" << std::endl;
    }
    else
    {
        int iterator = workingDir->dirs[index]->first_blk;
        int previous = workingDir->dirs[index]->first_blk;
        while (iterator != EOF)
        {
            iterator = fat[iterator];
            fat[previous] = FAT_FREE;
            previous = iterator;
        }

        fat[iterator] = FAT_FREE;
        for (int i = index; i < workingDir->nrOfSubDir; ++i)
        {
            workingDir->dirs[i] = workingDir->dirs[i + 1];
        }

        workingDir->nrOfSubDir--;
    }

    workingDirAsString = dirString;
    workingDir = tempDir;
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
        // int iterator1 = entries[index1].first_blk;
        // int iterator2 = entries[index2].first_blk;
        int iterator1;
        int iterator2;

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

        // entries[index2].size += entries[index1].size;
        int count = 0;
        int previous = -1;
        int i = ROOT_BLOCK + 2;
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

void FS::writeDir(dir_helper *catalog)
{
    bool inserted = false;
    for (int i = ROOT_BLOCK + 2; i < (BLOCK_SIZE / 2) && !inserted; ++i)
    {
        if (fat[i] == FAT_FREE)
        {
            catalog->dirs[ITSELF]->first_blk = i;
            disk.write(i, (uint8_t *)catalog);
            inserted = true;
            fat[i] = EOF;
        }
    }
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    std::string dirName;
    dir_helper *dirTemp = workingDir;
    std::string dirString = workingDirAsString;

    dirName = goToPath(dirpath);
    bool found = false;
    int index = fileExists(dirName, TYPE_DIR);

    if(dirName == "~" || dirName.empty())
    {
        std::cout << dirName << ": Reserved name, aborting!" << std::endl;
    }
    else if (index > -1)
    {
        std::cout << dirName << ": Directory already exists!" << std::endl;
    }
    else
    {
        dir_helper newDir;
        newDir.dirs[ITSELF] = new dir_entry;
        strcpy(newDir.dirs[ITSELF]->file_name, dirName.c_str());
        newDir.dirs[ITSELF]->size = BLOCK_SIZE;
        newDir.dirs[ITSELF]->access_rights = READ | WRITE | EXECUTE;
        newDir.dirs[ITSELF]->type = TYPE_DIR;
        newDir.dirs[PARENT] = workingDir->dirs[ITSELF];
        writeDir(&newDir);

        workingDir->dirs[workingDir->nrOfSubDir] = newDir.dirs[ITSELF];
        catalogs[nrOfDirs++] = newDir;
        workingDir->nrOfSubDir++;
    }
    
    workingDir = dirTemp;
    workingDirAsString = dirString;
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
