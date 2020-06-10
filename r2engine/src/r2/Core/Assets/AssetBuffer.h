//
//  AssetBuffer.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-23.
//

#ifndef AssetBuffer_h
#define AssetBuffer_h

#include "r2/Core//Assets/AssetTypes.h"
#include "r2/Core/Assets/Asset.h"

namespace r2::asset
{
    class AssetBuffer
    {
    public:
        //@TODO(Serge): POSSIBLE CHANGE
        //add in reference to the AssetCache for this buffer, we can check to see if this buffer is stale
        //
        
        AssetBuffer();
        void Load(const Asset& asset, s64 assetCache, byte* data, u64 dataSize);
        
        inline const byte* Data() const {return moptrData;}
        inline byte* MutableData() {return moptrData;}
        inline bool IsLoaded() const {return moptrData != nullptr;}
        inline u64 Size() const {return mDataSize;}
        const AssetHandle GetHandle() const { return mHandle; }
        //const Asset& GetAsset() const {return mAsset;}
    private:
        
        AssetHandle mHandle;
       // Asset mAsset; //@TODO(Serge): remove?
        byte* moptrData;
        u64 mDataSize;
    };
}

#endif /* AssetBuffer_h */
