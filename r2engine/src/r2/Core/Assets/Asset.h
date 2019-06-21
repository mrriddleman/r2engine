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
    
    using PathResolver = std::function<bool (u32 category, char* path)>;
    
    class R2_API Asset
    {
    public:
        
        Asset(u32 category, const char* name, PathResolver pathResolver);
        inline const char* ResolvePath() const {return mPath;}
        inline u64 HashID() const {return mHashedPathID;}
    private:
        char mPath[r2::fs::FILE_PATH_LENGTH];
        u64 mHashedPathID;
    };
}

#endif /* Asset_h */
