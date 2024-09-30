/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include <string>
#include <iostream>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

namespace fs = std::filesystem;

static size_t listFile(std::string directory, std::vector<std::string>* fileNames)
{
    std::string path = directory;
    for (const auto & entry : fs::directory_iterator(path))
    {
        std::cout << entry.path() << std::endl;
        fileNames->push_back(entry.path().string());
    }

    // sort(fileNames->begin(), fileNames->end());
    return fileNames->size();
}

static bool fileExist(std::string directory, std::string fileName)
{
    return std::filesystem::exists(directory + fileName);
}

static bool fileExist(std::string filePath)
{
    return std::filesystem::exists(filePath);
}

// class FileManager
// {
//     public:
//         size_t listFile(std::string directory, std::vector<std::string>* fileNames)
//         {
//             std::string path = directory;
//             for (const auto & entry : fs::directory_iterator(path))
//             {
//                 std::cout << entry.path() << std::endl;
//                 fileNames->push_back(entry.path().string());
//             }

//             sort(fileNames->begin(), fileNames->end());
//             return fileNames->size();
//         }

//     private:
//         std::vector<std::string>* fileNames = new std::vector<std::string>;
        
// };

#endif // __FILE_MANAGER_H__