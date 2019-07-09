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

class BreakoutLevelsFile: public r2::asset::AssetFile {
public:
    virtual bool Open() override;
    virtual bool Close() override;
    virtual bool IsOpen() override;
    virtual u64 RawAssetSize(const r2::asset::Asset& asset) override;
    virtual u64 RawAssetSizeFromHandle(u64 handle) override;
    virtual u64 GetRawAsset(const r2::asset::Asset& asset, byte* data) override;
    virtual u64 GetRawAssetFromHandle(u64 handle, byte* data) override;
    virtual u64 NumAssets() const override;
    virtual char* GetAssetName(u64 index) const override;
    virtual u64 GetAssetHandle(u64 index) const override;
private:
    //@TODO(Serge): add file here
};

#endif /* BreakoutLevelsFile_h */
