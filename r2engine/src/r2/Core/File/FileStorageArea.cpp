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
        FileStorageArea::FileStorageArea(const char* rootPath)
        {
            strcpy(mRootPath, rootPath);
            Mount();
        }
        
        FileStorageArea::~FileStorageArea()
        {
            Unmount();
        }
    }
}
