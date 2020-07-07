//
//  Asset.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-20.
//

#ifndef Asset_h
#define Asset_h

namespace r2::asset
{
    class R2_API Asset
    {
    public:
        
        Asset();
        Asset(const char* name);
        Asset(const Asset& asset);
        Asset& operator=(const Asset& asset);
        

        inline bool Empty() const { return mHashedPathID == 0; }
        
#ifdef R2_ASSET_CACHE_DEBUG
        inline const char* Name() const {return mName;}
#endif
        inline u64 HashID() const {return mHashedPathID;}
        
    private:
        #ifdef R2_ASSET_CACHE_DEBUG
        char mName[r2::fs::FILE_PATH_LENGTH]; //@TODO(Serge): figure out how to get rid of this here
                                                //Seems only needed for Zip and debug output
        #endif

        u64 mHashedPathID;
    };
}

#endif /* Asset_h */
