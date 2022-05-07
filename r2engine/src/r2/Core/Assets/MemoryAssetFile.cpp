#include "r2pch.h"
#include "r2/Core/Assets/MemoryAssetFile.h"

namespace r2::asset
{

	MemoryAssetFile::MemoryAssetFile(const AssetCacheRecord& record)
		: mOffset(0)
		, mRecord(record)
	{
		strcpy(mFilePath, "");
	}

	MemoryAssetFile::~MemoryAssetFile()
	{
		Close();
	}

	bool MemoryAssetFile::OpenForRead(const char* filePath)
	{
		mOffset = 0;
		strcpy(mFilePath, filePath);
		return true;
	}

	bool MemoryAssetFile::OpenForWrite(const char* filePath) const
	{
		return false;
	}

	bool MemoryAssetFile::OpenForReadWrite(const char* filePath)
	{
		return false;
	}

	bool MemoryAssetFile::Close() const
	{
		strcpy(mFilePath, "");
		mOffset = 0;
		return true;
	}

	bool MemoryAssetFile::IsOpen() const
	{
		return mRecord.buffer && mRecord.buffer->IsLoaded() && mRecord.buffer->Data() != nullptr;
	}

	uint32_t MemoryAssetFile::Size()
	{
		return mRecord.buffer->Size();
	}

	uint32_t MemoryAssetFile::Read(char* data, uint32_t dataBufSize)
	{
		if (!data || !IsOpen())
		{
			R2_CHECK(false, "We can't read from the file!");
			return 0;
		}
		
		memcpy(data, &mRecord.buffer->Data()[mOffset], dataBufSize);

		mOffset += dataBufSize;

		return dataBufSize;
	}

	uint32_t MemoryAssetFile::Read(char* data, uint32_t offset, uint32_t dataBufSize)
	{
		if (!data || !IsOpen())
		{
			R2_CHECK(false, "We can't read from the file!");
			return 0;
		}

		mOffset = offset;

		memcpy(data, &mRecord.buffer->Data()[mOffset], dataBufSize);

		mOffset += dataBufSize;

		return dataBufSize;
	}

	uint32_t MemoryAssetFile::Write(const char* data, uint32_t size) const
	{
		R2_CHECK(false, "We can't write to a memory asset file!");
		return 0; //can't write
	}

	bool MemoryAssetFile::AllocateForBlob(r2::assets::assetlib::BinaryBlob& blob)
	{
		//empty
		return true;
	}

	void MemoryAssetFile::FreeForBlob(const r2::assets::assetlib::BinaryBlob& blob)
	{
		//empty
	}

	const char* MemoryAssetFile::FilePath() const
	{
		return mFilePath;
	}

	bool MemoryAssetFile::LoadMetaData()
	{
		metaData.data = (char*)r2::mem::utils::PointerAdd(mRecord.buffer->MutableData(), headerSize);
		return true;
	}

	bool MemoryAssetFile::LoadBinaryData() 
	{
		binaryBlob.data = (char*)r2::mem::utils::PointerAdd(mRecord.buffer->MutableData(), headerSize + metaData.size);
		return true;
	}

}