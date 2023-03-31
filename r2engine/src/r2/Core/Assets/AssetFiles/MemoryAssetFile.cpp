#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/MemoryAssetFile.h"

namespace r2::asset
{

	MemoryAssetFile::MemoryAssetFile(const AssetCacheRecord& record)
		: mOffset(0)
		, mData(record.buffer->MutableData())
		, mSize(record.buffer->Size())
	{
		strcpy(mFilePath, "");
	}

	MemoryAssetFile::MemoryAssetFile(void* data, u64 size)
		: mOffset(0)
		, mData(data)
		, mSize(size)
	{

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
		return mData != nullptr;
	}

	uint32_t MemoryAssetFile::Size()
	{
		return mSize;
	}

	uint32_t MemoryAssetFile::Read(char* data, uint32_t dataBufSize)
	{
		if (!data || !IsOpen())
		{
			R2_CHECK(false, "We can't read from the file!");
			return 0;
		}
		
		const byte* memoryData = (byte*)mData;

		memcpy(data, &memoryData[mOffset], dataBufSize);

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

		const byte* memoryData = (byte*)mData;

		memcpy(data, &memoryData[mOffset], dataBufSize);

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
		metaData.data = (char*)r2::mem::utils::PointerAdd(mData, headerSize);
		return true;
	}

	bool MemoryAssetFile::LoadBinaryData() 
	{
		binaryBlob.data = (char*)r2::mem::utils::PointerAdd(mData, headerSize + metaData.size);
		return true;
	}

}