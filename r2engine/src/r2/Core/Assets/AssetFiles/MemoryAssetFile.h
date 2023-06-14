#ifndef __MEMORY_ASSET_FILE_H__
#define __MEMORY_ASSET_FILE_H__

#include "assetlib/AssetFile.h"

#include "r2/Core/File/File.h"
#include "r2/Utils/Utils.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetBuffer.h"

namespace r2::asset
{
	class MemoryAssetFile : public r2::assets::assetlib::AssetFile
	{
	public:

		MemoryAssetFile(const AssetCacheRecord& record);
		MemoryAssetFile(const void* data, u64 size);

		virtual ~MemoryAssetFile();

		virtual bool OpenForRead(const char* filePath) override;
		virtual bool OpenForWrite(const char* filePath) const override;
		virtual bool OpenForReadWrite(const char* filePath) override;
		virtual bool Close() const override;
		virtual bool IsOpen() const override;
		virtual uint32_t Size() override;

		virtual uint32_t Read(char* data, uint32_t dataBufSize) override;
		virtual uint32_t Read(char* data, uint32_t offset, uint32_t dataBufSize) override;
		virtual uint32_t Write(const char* data, uint32_t size) const override;
		virtual bool AllocateForBlob(r2::assets::assetlib::BinaryBlob& blob) override;
		virtual void FreeForBlob(const r2::assets::assetlib::BinaryBlob& blob) override;
		
		virtual const char* FilePath() const override;

		virtual bool LoadMetaData() override;
		virtual bool LoadBinaryData() override;

	private:
		mutable u64 mOffset;
		mutable char mFilePath[r2::fs::FILE_PATH_LENGTH];
		const void* mData;
		u64 mSize;
	};
}


#endif // __MEMORY_ASSET_FILE_H__
