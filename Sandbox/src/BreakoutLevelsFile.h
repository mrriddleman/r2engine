//
//  BreakoutLevelsFile.h
//  Sandbox
//
//  Created by Serge Lansiquot on 2019-07-03.
//

#ifndef BreakoutLevelsFile_h
#define BreakoutLevelsFile_h

#include "r2.h"
#include "r2/Core/Assets/AssetFile.h"
#include "r2/Core/File/File.h"


class BreakoutLevelsFile: public r2::asset::AssetFile {
public:
    BreakoutLevelsFile();
    ~BreakoutLevelsFile();
    bool Init(const char* path);
    virtual bool Open(bool writable = false) override;
    virtual bool Open(r2::fs::FileMode mode) override;
    virtual bool Close() override;
    virtual bool IsOpen() const override;
    virtual u64 RawAssetSize(const r2::asset::Asset& asset) override;
    
    virtual u64 LoadRawAsset(const r2::asset::Asset& asset, byte* data, u32 dataBufSize) override;
    
    virtual u64 NumAssets() override;
    virtual void GetAssetName(u64 index, char* name,u32 nameBufSize) override;
    virtual u64 GetAssetHandle(u64 index) override;
    
    virtual const char* FilePath() const override {return mPath;}
private:
    char mPath[r2::fs::FILE_PATH_LENGTH];
    r2::fs::File* mFile;
};

#endif /* BreakoutLevelsFile_h */
