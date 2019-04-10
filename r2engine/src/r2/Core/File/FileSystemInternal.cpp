//
//  FileSystemInternal.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-07.
//

#include "FileSystemInternal.h"
#include "r2/Core/File/FileStorageArea.h"

namespace r2
{
    namespace fs
    {
        const u8 FileSystemInternal::NUM_FILE_DEVICES;
        const u8 FileSystemInternal::NUM_FILE_DEVICE_MODIFIERS;
        
        FileSystemInternal::FileSystemInternal(FileStorageArea& fsArea):mFSArea(fsArea)
        {
            UnmountAllFileDevices();
            
            //Mount file devices and file device modifiers from fsArea
            Mount(mFSArea.GetFileDevice());
            
            const auto fileDeviceMods = mFSArea.GetFileDeviceModifiers();
            u64 size = r2::sarr::Size(*fileDeviceMods);
            
            for (u64 i = 0; i < size; ++i)
            {
                Mount(r2::sarr::At(*fileDeviceMods, i));
            }
        }
        
        FileSystemInternal::~FileSystemInternal()
        {
            UnmountAllFileDevices();
            
            //@NOTE: Maybe it should be here?
            mFSArea.Commit();
        }
        
        bool FileSystemInternal::Mount(FileStorageDevice* noptrDevice)
        {
            R2_CHECK(mFileDevice == nullptr, "We've already mounted a file storage device!");
            
            if (mFileDevice == nullptr)
            {
                mFileDevice = noptrDevice;
                return true;
            }
            
            return false;
        }
        
        bool FileSystemInternal::Unmount(FileStorageDevice* noptrDevice)
        {
            R2_CHECK(mFileDevice == noptrDevice, "We don't have the same file device!");
            
            mFileDevice = nullptr;
            
            return true;
        }
        
        bool FileSystemInternal::Mount(FileDeviceModifier* noptrDeviceMod)
        {
            for (u8 i = 0; i < NUM_FILE_DEVICE_MODIFIERS; ++i)
            {
                if (mDeviceModifiersList[i] == nullptr)
                {
                    mDeviceModifiersList[i] = noptrDeviceMod;
                    return true;
                }
            }
            
            R2_CHECK(false, "Did not find file device modifier to mount");
            
            return false;
        }
        
        bool FileSystemInternal::Unmount(FileDeviceModifier* noptrDeviceMod)
        {
            for (u8 i = 0; i < NUM_FILE_DEVICE_MODIFIERS; ++i)
            {
                if (mDeviceModifiersList[i] == noptrDeviceMod)
                {
                    mDeviceModifiersList[i] = nullptr;
                    return true;
                }
            }
            
            R2_CHECK(false, "Did not find file device modifier to unmount");
            
            return false;
        }
        
        void FileSystemInternal::UnmountAllFileDevices()
        {
            mFileDevice = nullptr;
            
            for (u32 i = 0; i < NUM_FILE_DEVICE_MODIFIERS; ++i)
            {
                mDeviceModifiersList[i] = nullptr;
            }
        }
        
        File* FileSystemInternal::Open(const DeviceConfig& deviceConfig, const char* path, FileMode mode)
        {
            
            
            
//            for (u32 i = 0; i < NUM_FILE_DEVICES; ++i)
//            {
//                if (mDeviceList[i] != nullptr && mDeviceList[i]->Flags() == flags)
//                {
//                    return mDeviceList[i]->Open(file, path, mode);
//                }
//            }
//
//            R2_CHECK(false, "Could not find suitable device to open the file: %s\n", path);
//
            return nullptr;
        }
        
        void FileSystemInternal::Close(File* file)
        {
//            bool found = false;
//            for (u32 i = 0; i < NUM_FILE_DEVICES && !found; ++i)
//            {
//                if (mDeviceList[i] != nullptr && file.Flags() == mDeviceList[i]->Flags())
//                {
//                    mDeviceList[i]->Close(file);
//                    found = true;
//                    break;
//                }
//            }
//
            R2_CHECK(false, "We should have found the device for this file");
        }
        
        bool FileSystemInternal::FileExists(const char* path) const
        {
            return false;
        }
        
        bool FileSystemInternal::DirExists(const char* path) const
        {
            return false;
        }
        
        bool FileSystemInternal::CreateDirectory(const char* path, bool recursive)
        {
            return false;
        }
    }
}
