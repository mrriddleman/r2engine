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
        AssetBuffer();
        void Load(byte* data, u64 dataSize);
        
        inline const byte* Data() const {return moptrData;}
        inline byte* MutableData() {return moptrData;}
        inline bool IsLoaded() const {return moptrData != nullptr;}
        inline u64 Size() const {return mDataSize;}

    private:
        byte* moptrData;
        u64 mDataSize;
    };
}

#endif /* AssetBuffer_h */
