//
//  FileTypes.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-07.
//

#ifndef FileTypes_h
#define FileTypes_h

namespace r2
{
    namespace fs
    {
        enum class DeviceStorage: u8
        {
            Disk = 0,
            Memory,
            Net,
            Count
        };
        
        enum class DeviceModifier: u8
        {
            Zip = 0,
            Crypto,
            Count
        };
        
        enum Mode
        {
            Read = 1 << 0,
            Write = 1 << 1,
            Append = 1 << 2,
            Recreate = 1 << 3,
            Binary = 1 << 4,
        };
        
        using FileMode = r2::Flags<Mode, u32>;
        
        class DeviceConfig
        {
        public:

            static const u8 DEVICE_MOD_COUNT = u8(DeviceModifier::Count);
            
            DeviceConfig():mStorage(DeviceStorage::Disk), mModCount(0){}
            explicit DeviceConfig(DeviceStorage storage):mStorage(storage), mModCount(0){}
            inline bool HasModifiers() const {return mModCount > 0;}
            inline u8 NumModifiers() const {return mModCount;}
            inline const std::array<DeviceModifier, DEVICE_MOD_COUNT>& GetDeviceModifiers() const {return mModifiers;}
            
            inline void SetDeviceStorage(DeviceStorage storage){mStorage = storage;}
            
            inline void AddModifier(DeviceModifier mod)
            {
                R2_CHECK(mModCount + 1 < mModifiers.max_size(), "We can't add anymore modifiers!");
                mModifiers[mModCount++] = mod;
            }
            
        private:

            DeviceStorage mStorage;
            std::array<DeviceModifier, DEVICE_MOD_COUNT> mModifiers;
            u8 mModCount;
        };
        
      
        
    }
}

#endif /* FileTypes_h */
