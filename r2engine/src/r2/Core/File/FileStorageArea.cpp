//
//  FileStorageArea.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-06.
//

#include "r2/Core/File/FileStorageArea.h"
#include <cstring>

namespace r2
{
    namespace fs
    {
        FileStorageArea::FileStorageArea(const char* rootPath, FileStorageDevice& device, FileDeviceModifierListPtr modList):mnoptrStorageDevice(&device), mnoptrModifiers(modList)
        {
            strcpy(mRootPath, rootPath);
        }
        
        FileStorageArea::~FileStorageArea()
        {
            Unmount();
            //@NOTE: we don't free these since they are a part of permanent storage
            mnoptrStorageDevice = nullptr;
            mnoptrModifiers = nullptr;
        }
    }
}
