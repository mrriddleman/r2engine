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
        Asset(u32 category, const char* name);
        Asset(const Asset& asset);
        Asset& operator=(const Asset& asset);
        
        inline bool Empty() const {return mHashedPathID == 0 || strcmp(mPath, "") == 0;}
        inline const char* Path() const {return mPath;}
        inline u64 HashID() const {return mHashedPathID;}
    private:
        char mPath[r2::fs::FILE_PATH_LENGTH];
        u64 mHashedPathID;
    };
}

#endif /* Asset_h */