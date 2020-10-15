#include <iostream>
#include "fs.h"

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
    //format();
    catalogs = new dir_helper[MAX_DIRECTORIES];
    nrOfDirs = 0;

    //format();
    boot();

    // //Creating meta-data for a new sub-directory
    // catalogs[nrOfDirs].dirs[ITSELF] = new dir_entry;
    // strcpy(catalogs[nrOfDirs].dirs[ITSELF]->file_name, "sub");
    // catalogs[nrOfDirs].dirs[ITSELF]->first_blk = 2;
    // fat[2] = EOF;
    // catalogs[nrOfDirs].dirs[PARENT] = catalogs[ROOT_DIR].dirs[ITSELF];
    // catalogs[nrOfDirs].dirs[ITSELF]->type = TYPE_DIR;
    // catalogs[nrOfDirs].dirs[ITSELF]->size = BLOCK_SIZE;
    // catalogs[nrOfDirs].dirs[ITSELF]->access_rights = READ | WRITE | EXECUTE;

    // // Create a reference of the new sub-dir to ROOT (nrOfSubDir starts at 2. 0 and 1 reserved for itself/parent)
    // catalogs[ROOT_DIR].dirs[catalogs[ROOT_DIR].nrOfSubDir++] = catalogs[nrOfDirs].dirs[ITSELF];
    // nrOfDirs++;

    // fat[3] = 4;
    // fat[4] = EOF;

    // catalogs[nrOfDirs].dirs[ITSELF] = new dir_entry;
    // strcpy(catalogs[nrOfDirs].dirs[ITSELF]->file_name, "hello");
    // catalogs[nrOfDirs].dirs[ITSELF]->first_blk = 3;
    // catalogs[nrOfDirs].dirs[ITSELF]->type = TYPE_FILE;
    // catalogs[nrOfDirs].dirs[ITSELF]->size = 5000;
    // catalogs[nrOfDirs].dirs[ITSELF]->access_rights = READ | WRITE;
    // catalogs[nrOfDirs].dirs[PARENT] = catalogs[1].dirs[ITSELF];

    // //Place the file "hello" as a subdir entry of the sub-file sub
    // catalogs[1].dirs[catalogs[1].nrOfSubDir++] = catalogs[nrOfDirs].dirs[ITSELF];

    // char *buffer;
    // buffer = new char[BLOCK_SIZE];

    // strcpy(buffer, "hellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohello\n");

    // disk.write(3, (uint8_t *)buffer);
    // strcpy(buffer, "rofl\n");
    // disk.write(4, (uint8_t *)buffer);
    // delete[] buffer;
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

void FS::bootHelper(int readBlock)
{
    disk.read(readBlock, (uint8_t *)&catalogs[nrOfDirs++]);

    for (int i = 2; i < catalogs[nrOfDirs - 1].nrOfSubDir; ++i)
    {
        if (catalogs[nrOfDirs - 1].dirs[i]->type == TYPE_DIR)
        {
            bootHelper(catalogs[nrOfDirs - 1].dirs[i]->first_blk);
        }
    }
}

void FS::boot()
{
    disk.read(FAT_BLOCK, (uint8_t *)&fat);
    for (int i = 0; i < 10; ++i)
    {
        std::cout << fat[i] << std::endl;
    }
    disk.read(ROOT_BLOCK, (uint8_t *)&catalogs[nrOfDirs++]);
    //std::cout << catalogs[ROOT_DIR].dirs[ITSELF]->file_name << std::endl;
    for (int i = 2; i < catalogs[ROOT_DIR].nrOfSubDir; ++i)
    {
        if (catalogs[ROOT_DIR].dirs[i]->type == TYPE_DIR)
        {
            bootHelper(catalogs[ROOT_DIR].dirs[i]->first_blk);
        }
    }
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
    disk.write(FAT_BLOCK, (uint8_t *)&fat);
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

void FS::appendFile(int blocksToWrite, std::vector<std::string> &data, int appendIndex)
{
    int i = ROOT_BLOCK + 2;
    bool inserted = false;
    int previous = -1;

    for (int j = 0; j < data.size(); ++j)
    {
        if (j > 0)
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
            disk.write(i, (uint8_t *)data[j].c_str());
            inserted = false;
        }
        else
        {
            disk.write(appendIndex, (uint8_t *)data[j].c_str());
        }
    }
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

std::string FS::getNameOfPath(std::string pathName)
{
    std::string paths;

    for (int i = 0; i < pathName.size(); ++i)
    {
        if (pathName[i] != '/')
        {
            paths += pathName[i];
        }
        else
        {
            paths.clear();
        }
    }

    return paths;
}

bool FS::goToPath(std::string newPath)
{
    std::string paths;
    bool found = false;
    bool validPath = true;

    for (int i = 0; i < newPath.size() && validPath; ++i)
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
                            else if (workingDirAsString.size() > 1)
                            {
                                workingDirAsString.erase(workingDirAsString.size() - strlen(workingDir->dirs[ITSELF]->file_name) - 1,
                                                         strlen(workingDir->dirs[ITSELF]->file_name) + 1);
                            }
                            else
                            {
                                if (k > 0)
                                {
                                    workingDirAsString.erase(workingDirAsString.size() - 1, 2);
                                }
                            }

                            workingDir = &catalogs[k];
                            found = true;
                            paths.clear();
                        }
                    }
                }
            }
            if (found == false)
            {
                std::cout << newPath << ": Invalid path!" << std::endl;
                validPath = false;
            }
        }
        found = false;
    }
    return validPath;
}

// formats the disk, i.e., creates an empty file system starting on an offset of 2
// disregarding ROOT_BLOCK and FAT_BLOCK
int FS::format()
{
    std::cout << "FS::format()\n";
    for (int i = ROOT_BLOCK + 2; i < (BLOCK_SIZE / 2); ++i)
    {
        fat[i] = FAT_FREE;
    }

    delete[] catalogs;
    catalogs = new dir_helper[MAX_DIRECTORIES];
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

    disk.write(ROOT_BLOCK, (uint8_t *)&catalogs[ROOT_BLOCK]);
    dir_helper catalog;
    disk.read(ROOT_BLOCK, (uint8_t *)&catalog);

    workingDir = &catalogs[nrOfDirs];
    workingDirAsString = "~";
    nrOfDirs++;
    updateFat();
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
    if ((goToPath(filepath)) == true)
    {
        std::string fileName = getNameOfPath(filepath);
        int index = fileExists(fileName, TYPE_FILE);

        // User shouldn't be able to create a file with same name as root
        if (fileName == "~" || fileName.empty())
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
                newEntry->access_rights = workingDir->dirs[ITSELF]->access_rights;

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
                updateFat();
            }
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
    dir_helper *tempDir = workingDir;
    std::string stringDir = workingDirAsString;
    std::string fileName = getNameOfPath(filepath);
    if (goToPath(filepath) == true)
    {
        int index = fileExists(fileName, TYPE_FILE);
        if (index == -2)
        {
            std::cout << fileName << ": Is a directory!" << std::endl;
        }
        else if (index != EOF)
        {
            if ((decodeAccessRights(workingDir->dirs[index]->access_rights).at(0)) == 'r')
            {
                std::vector<std::string> vec = readFile(workingDir->dirs[index]->first_blk);
                for (int i = 0; i < vec.size(); i++)
                {
                    std::cout << vec[i];
                }
            }
            else
            {
                std::cout << "Permission denied: You do not have permission to read this file." << std::endl;
            }
        }
        else
        {
            std::cout << fileName << ": No such file or directory!" << std::endl;
        }
    }
    workingDir = tempDir;
    workingDirAsString = stringDir;
    return 0;
}

std::string FS::decodeAccessRights(uint8_t accessRights)
{
    std::string decoded;

    switch (accessRights)
    {
    case 1:
    {
        decoded = "--x";
        break;
    }
    case 2:
    {
        decoded = "-w-";
        break;
    }
    case 3:
    {
        decoded = "-wx";
        break;
    }
    case 4:
    {
        decoded = "r--";
        break;
    }
    case 5:
    {
        decoded = "r-x";
        break;
    }
    case 6:
    {
        decoded = "rw-";
        break;
    }
    case 7:
    {
        decoded = "rwx";
        break;
    }
    default:
    {
        decoded = "---";
        break;
    }
    }

    return decoded;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls()
{
    std::cout << "FS::ls()\n";

    std::cout << "Filename:\t\tType:\t\tAccess-rights:\tSize:\tStart:\t" << std::endl;

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
        std::cout << decodeAccessRights(workingDir->dirs[i]->access_rights) << "\t\t" << workingDir->dirs[i]->size << "\t" << workingDir->dirs[i]->first_blk << std::endl;
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
    std::string fileName = getNameOfPath(sourcefilepath);
    if (goToPath(sourcefilepath) == true)
    {
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
            if (decodeAccessRights(workingDir->dirs[index]->access_rights).at(0) == 'r')
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

                workingDirAsString = dirString;
                workingDir = tempDir;
                if (destfilepath[destfilepath.size() - 1] != '/')
                {
                    destfilepath.append("/");
                }
                if (goToPath(destfilepath) == true)
                {
                    workingDir->dirs[workingDir->nrOfSubDir] = copyEntry;

                    writeFile(noBlocks, data);
                    workingDir->nrOfSubDir++;
                    updateFat();
                }
            }
            else
            {
                std::cout << "Permission denied: You do not have access to read!" << std::endl;
            }
        }
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
    dir_helper *tempDir = workingDir;
    std::string dirString = workingDirAsString;
    std::string fileName = getNameOfPath(sourcepath);
    if (goToPath(sourcepath) == true)
    {
        std::string tempDirString2 = workingDirAsString;
        dir_helper *tempDir2 = workingDir;
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
            dir_entry *temp = workingDir->dirs[index];
            workingDirAsString = dirString;
            workingDir = tempDir;
            if (goToPath(destpath) == true)
            {
                fileName = getNameOfPath(destpath);
                if (workingDirAsString == tempDirString2)
                {
                    std::cout << "Same directory, renaming\n";
                    strcpy(workingDir->dirs[index]->file_name, fileName.c_str());
                }
                else
                {
                    if (goToPath(fileName.append("/")) == true)
                    {
                        workingDir->dirs[workingDir->nrOfSubDir++] = tempDir2->dirs[index];
                        for (int i = index; i < tempDir2->nrOfSubDir; i++)
                        {
                            tempDir2->dirs[i] = tempDir2->dirs[i + 1];
                        }
                        tempDir2->nrOfSubDir--;
                    }
                }
                updateFat();
            }
        }
    }
    workingDirAsString = dirString;
    workingDir = tempDir;
    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int FS::rm(std::string filepath)
{
    std::cout << "FS::rm(" << filepath << ")\n";

    dir_helper *tempDir = workingDir;
    std::string dirString = workingDirAsString;
    std::string fileName = getNameOfPath(filepath);
    if (goToPath(filepath) == true)
    {
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
            updateFat();
        }
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
    dir_helper *tempDir = workingDir;
    std::string dirString = workingDirAsString;
    std::string fileName = getNameOfPath(filepath2);
    if (goToPath(filepath2) == true)
    {
        int index2 = fileExists(fileName, TYPE_FILE);
        if (decodeAccessRights(workingDir->dirs[index2]->access_rights).at(1) == 'w')
        {
            workingDir = tempDir;
            workingDirAsString = dirString;
            fileName = getNameOfPath(filepath1);
            if (goToPath(filepath1) == true)
            {
                int index1 = fileExists(fileName, TYPE_FILE);
                int appendIndex = -1;
                if (decodeAccessRights(workingDir->dirs[index1]->access_rights).at(0) == 'r')
                {
                    if (index1 != NOT_FOUND && index2 != NOT_FOUND)
                    {
                        std::string newData;
                        std::string tempData;
                        std::vector<std::string> file = readFile(workingDir->dirs[index1]->first_blk);
                        int appendSize = workingDir->dirs[index1]->size;
                        for (int i = 0; i < file.size(); ++i)
                        {
                            tempData.append(file[i]);
                        }
                        workingDir = tempDir;
                        workingDirAsString = dirString;
                        if (goToPath(filepath2) == true)
                        {
                            fileName = getNameOfPath(filepath2);
                            file.clear();
                            file = readFile(workingDir->dirs[index2]->first_blk);

                            newData.append(file[file.size() - 1]);
                            newData.append(tempData);

                            int iterator2 = workingDir->dirs[index2]->first_blk;

                            while (iterator2 != EOF)
                            {
                                //Only increment appendIndex if we're not at EOF, so that we can start append other files FAT-indexes.
                                if (iterator2 != EOF)
                                {
                                    appendIndex = iterator2;
                                }
                                iterator2 = fat[iterator2];
                            }

                            uint32_t noBlocks = (newData.length() / BLOCK_SIZE) + 1;
                            uint32_t lastBlock = (newData.length() % BLOCK_SIZE);

                            file.clear();

                            for (int i = 0; i < noBlocks; ++i)
                            {
                                if (i == noBlocks - 1)
                                {
                                    file.push_back(newData.substr((i * BLOCK_SIZE), lastBlock));
                                }
                                else
                                {
                                    file.push_back(newData.substr((i * BLOCK_SIZE), BLOCK_SIZE));
                                }
                            }
                            workingDir->dirs[index2]->size += appendSize;
                            appendFile(noBlocks, file, appendIndex);
                            updateFat();
                        }
                    }
                    else
                    {
                        if (index1 == NOT_FOUND && index2 == NOT_FOUND)
                        {
                            std::cout << "'" << filepath1 << "' and '" << filepath2 << "' does not exist!" << std::endl;
                        }
                        else if (index1 == NOT_FOUND && index2 != NOT_FOUND)
                        {
                            std::cout << "'" << filepath1 << "' does not exist!" << std::endl;
                        }
                        else if (index1 != NOT_FOUND && index2 == NOT_FOUND)
                        {
                            std::cout << "'" << filepath2 << "' does not exist!" << std::endl;
                        }
                    }
                }
                else
                {
                    std::cout << "Permission denied: " << filepath1 << " do not have read access!" << std::endl;
                }
            }
        }
        else
        {
            std::cout << "Permission denied: "
                      << "'" << filepath2 << "' do not have write access!" << std::endl;
        }
    }
    workingDirAsString = dirString;
    workingDir = tempDir;
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
    dir_helper *dirTemp = workingDir;
    std::string dirString = workingDirAsString;
    std::string dirName = getNameOfPath(dirpath);

    if (goToPath(dirpath) == true)
    {
        bool found = false;
        int index = fileExists(dirName, TYPE_DIR);

        if (dirName == "~" || dirName.empty())
        {
            std::cout << dirName << ": Reserved name, aborting!" << std::endl;
        }
        else if (index > -1)
        {
            std::cout << dirName << ": Directory already exists!" << std::endl;
        }
        else
        {
            // Creating new meta-data for directory.
            dir_helper newDir;
            newDir.dirs[ITSELF] = new dir_entry;
            strcpy(newDir.dirs[ITSELF]->file_name, dirName.c_str());
            newDir.dirs[ITSELF]->size = BLOCK_SIZE;
            newDir.dirs[ITSELF]->access_rights = workingDir->dirs[ITSELF]->access_rights;
            newDir.dirs[ITSELF]->type = TYPE_DIR;
            newDir.dirs[PARENT] = workingDir->dirs[ITSELF];
            writeDir(&newDir);

            // Working directory receives a reference to the new sub-directory
            workingDir->dirs[workingDir->nrOfSubDir] = newDir.dirs[ITSELF];
            catalogs[nrOfDirs++] = newDir;
            workingDir->nrOfSubDir++;
            updateFat();
        }
    }
    workingDir = dirTemp;
    workingDirAsString = dirString;
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int FS::cd(std::string dirpath)
{
    std::cout << "FS::cd(" << dirpath << ")\n";
    dir_helper *temp = workingDir;
    std::string dirString = workingDirAsString;

    if (goToPath(dirpath + "/") == false)
    {
        workingDir = temp;
        workingDirAsString = dirString;
    }

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

    dir_helper *tempDir = workingDir;
    std::string dirString = workingDirAsString;

    if (goToPath(filepath) == true)
    {
        std::string fileName = getNameOfPath(filepath);

        int index = fileExists(fileName, TYPE_FILE);

        if (index != NOT_FOUND)
        {
            if (accessrights.size() == 1)
            {
                char ar = accessrights[0];
                switch (ar)
                {
                case '1':
                {
                    workingDir->dirs[index]->access_rights = EXECUTE;
                    break;
                }
                case '2':
                {
                    workingDir->dirs[index]->access_rights = WRITE;
                    break;
                }
                case '3':
                {
                    workingDir->dirs[index]->access_rights = EXECUTE | WRITE;
                    break;
                }
                case '4':
                {
                    workingDir->dirs[index]->access_rights = READ;
                    break;
                }
                case '5':
                {
                    workingDir->dirs[index]->access_rights = EXECUTE | READ;
                    break;
                }
                case '6':
                {
                    workingDir->dirs[index]->access_rights = READ | WRITE;
                    break;
                }
                case '7':
                {
                    workingDir->dirs[index]->access_rights = READ | WRITE | EXECUTE;
                    break;
                }
                default:
                {
                    std::cout << accessrights << ": Wrong input!" << std::endl;
                    break;
                }
                }
            }
            else
            {
                std::cout << "Wrong input!" << std::endl;
            }
        }
        else
        {
            std::cout << filepath << ": No such file or directory!" << std::endl;
        }
    }

    workingDirAsString = dirString;
    workingDir = tempDir;

    return 0;
}