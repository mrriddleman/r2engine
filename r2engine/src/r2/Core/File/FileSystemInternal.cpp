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
        FileSystemInternal::FileSystemInternal(FileStorageArea& fsArea):mFSArea(fsArea)
        {

        }
        
        FileSystemInternal::~FileSystemInternal()
        {

        }

        File* FileSystemInternal::Open(const DeviceConfig& config, const char* path, FileMode mode)
        {
            R2_CHECK(path != nullptr, "Path is nullptr!");
            R2_CHECK(path != "", "Path is empty");
            
            FileStorageDevice* storageDevice = mFSArea.GetFileStorageDevice();
            R2_CHECK(storageDevice != nullptr, "File Storage device is nullptr!");
            R2_CHECK(config.GetStorageDevice() == storageDevice->GetStorageType(), "Storage types do not match!");
            
            if (storageDevice == nullptr && config.GetStorageDevice() == storageDevice->GetStorageType())
            {
                R2_LOGE("Storage Device is null or we have a storage device mismatch");
                return nullptr;
            }
            
            File* file = storageDevice->Open(path, mode);
           // R2_CHECK(file != nullptr, "File is nullptr!");
            
            if (file == nullptr)
            {
                R2_LOGE("We could not open file: %s", path);
                return nullptr;
            }

            if (config.HasModifiers())
            {
                const auto configModifiers = config.GetDeviceModifiers();
                const auto numModifiers = config.NumModifiers();
                
                for (auto i = 0; i < numModifiers; ++i)
                {
                    FileDeviceModifier* modifier = GetModifier(configModifiers[i]);
                    R2_CHECK(modifier != nullptr, "We couldn't find the modifier: %hhu", configModifiers[i]);
                    
                    if (modifier)
                    {
                        file = modifier->Open(file);
                    }
                }
            }

            return file;
        }

        FileDeviceModifier* FileSystemInternal::GetModifier(DeviceModifier modType)
        {
            const auto* modifiers = mFSArea.GetFileDeviceModifiers();
            const u64 size = r2::sarr::Size(*modifiers);
            
            for (u64 i = 0; i < size; ++i)
            {
                FileDeviceModifier* nextMod = r2::sarr::At(*modifiers, i);
                if (nextMod->GetModifier() == modType)
                {
                    return nextMod;
                }
            }
            
            return nullptr;
        }
    }
}
