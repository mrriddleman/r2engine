//
//  FileStorageArea.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-06.
//

#ifndef FileStorageArea_h
#define FileStorageArea_h

#include "r2/Core/File/FileTypes.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"

namespace r2
{
    namespace fs
    {
        class FileStorageDevice;
        class FileDeviceModifier;
        
        class R2_API FileStorageArea
        {
        public:
            using FileDeviceModifierListPtr = r2::SArray<FileDeviceModifier*>*;
            
            FileStorageArea(const char* rootPath, FileStorageDevice& storageDevice, FileDeviceModifierListPtr list = nullptr);
            virtual ~FileStorageArea();
            
            virtual bool Mount() {return true;}
            virtual bool Unmount() {return true;}
            virtual bool Commit() {return true;}
            
            inline const char* RootPath() const {return &mRootPath[0];}
            inline FileStorageDevice* GetFileDevice() {return mnoptrStorageDevice;}
            inline const FileDeviceModifierListPtr GetFileDeviceModifiers() {return mnoptrModifiers;}
            
        private:

            char mRootPath[Kilobytes(1)];
            FileStorageDevice* mnoptrStorageDevice;
            FileDeviceModifierListPtr mnoptrModifiers;
        };
    }
}

#endif /* FileStorageArea_h */
