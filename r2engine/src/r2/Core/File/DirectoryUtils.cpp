//
//  DirectoryUtils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-20.
//
#include "r2pch.h"
#include "DirectoryUtils.h"
#include "r2/Core/File/FileTypes.h"
#include "r2/Core/File/FileSystem.h"
#include <filesystem>
#include <string>


namespace r2::fs::dir
{
    void CreateFileListFromDirectory(const char* directory, const char* ext, r2::SArray<char[r2::fs::FILE_PATH_LENGTH]>* fileList)
    {
        for (const auto& file : std::filesystem::recursive_directory_iterator(directory))
        {
            std::string fileExt = file.path().extension().string();
            
            if (fileExt == std::string(ext) || std::string(ext) == std::string(r2::fs::FileSystem::ALL_EXT))
            {
                
                R2_CHECK((fileList->mSize+1) < fileList->mCapacity, "We don't have enough space for this entry!");
                if ((fileList->mSize+1) < fileList->mCapacity)
                {
                    char* listFileName = r2::sarr::At(*fileList, fileList->mSize);
                    R2_CHECK(file.path().string().length() < r2::fs::FILE_PATH_LENGTH, "We don't have enough space for this file path! We have: %u but need: %zu", r2::fs::FILE_PATH_LENGTH, file.path().string().length());
                    if (file.path().string().length() < r2::fs::FILE_PATH_LENGTH)
                    {
                        strcpy(listFileName, file.path().string().c_str());
                        
                        ++fileList->mSize;
                    }
                }
            }
        }
    }
}
