//
//  FileStorageArea.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-06.
//

#ifndef FileStorageArea_h
#define FileStorageArea_h

#include "r2/Core/Memory/Allocators/LinearAllocator.h"

namespace r2::fs
{
    class FileStorageDevice;
    class FileDeviceModifier;
    
    class R2_API FileStorageArea
    {
    public:
        using FileDeviceModifierList = r2::SArray<FileDeviceModifier*>;
        
        FileStorageArea(const char* rootPath, u32 numFilesActive);
        virtual ~FileStorageArea();
        
        virtual bool Mount(r2::mem::LinearArena& storage);
        virtual bool Unmount(r2::mem::LinearArena& storage);
        virtual bool Commit() {return true;}
        
        inline const char* RootPath() const {return &mRootPath[0];}
        inline FileStorageDevice* GetFileStorageDevice() {return moptrStorageDevice;}
        inline const FileDeviceModifierList* GetFileDeviceModifiers() {return moptrModifiers;}

    protected:
        FileStorageDevice* moptrStorageDevice;
        FileDeviceModifierList* moptrModifiers;
        
    private:
        const u32 mNumFilesActive; // how many files you can have on this storage area active at once
        char mRootPath[Kilobytes(1)];
    };
}

#endif /* FileStorageArea_h */
