#include "assetlib/DiskAssetFile.h"

namespace r2::assets::assetlib
{
	DiskAssetFile::DiskAssetFile()
		: mFilePath("")
		, mFreeDataBlob(true)
	{
	}

	bool DiskAssetFile::OpenForRead(const char* filePath)
	{
		Close();

		mFile.open(filePath, std::ios::binary | std::ios::in);

		bool isOpen = mFile.is_open();

		if (isOpen)
		{
			mFilePath = filePath;
		}

		return isOpen;
	}

	bool DiskAssetFile::OpenForWrite(const char* filePath) const
	{
		Close();

		mFile.open(filePath, std::ios::binary | std::ios::out);

		bool isOpen = mFile.is_open();

		if (isOpen)
		{
			mFilePath = filePath;
		}

		return isOpen;
	}

	bool DiskAssetFile::OpenForReadWrite(const char* filePath)
	{
		Close();

		mFile.open(filePath, std::ios::binary | std::ios::out | std::ios::in);

		bool isOpen = mFile.is_open();

		if (isOpen)
		{
			mFilePath = filePath;
		}

		return isOpen;
	}

	bool DiskAssetFile::Close() const
	{
		if (IsOpen())
		{
			mFile.close();
			mFilePath = "";
		}

		return true;
	}

	bool DiskAssetFile::IsOpen() const
	{
		return mFile.is_open();
	}

	uint32_t DiskAssetFile::Size()
	{
		if (!IsOpen())
			return 0;

		auto currentOffset = mFile.tellg();

		mFile.seekg(0, std::ios::end);

		std::streampos fSize = mFile.tellg();

		mFile.seekg(currentOffset);

		return fSize;
	}

	uint32_t DiskAssetFile::Read(char* data, uint32_t dataBufSize)
	{
		mFile.read(data, dataBufSize);
		return dataBufSize;
	}

	uint32_t DiskAssetFile::Read(char* data, uint32_t offset, uint32_t dataBufSize)
	{
		mFile.seekg(offset, std::ios::beg);
		mFile.read(data, dataBufSize);
		return dataBufSize;
	}

	uint32_t DiskAssetFile::Write(const char* data, uint32_t size) const
	{
		mFile.write(data, size);
		return size;
	}

	bool DiskAssetFile::AllocateForBlob(r2::assets::assetlib::BinaryBlob& blob)
	{
		if (blob.size > 0)
			blob.data = new char[blob.size];

		return blob.size > 0;
	}

	void DiskAssetFile::FreeForBlob(const r2::assets::assetlib::BinaryBlob& blob)
	{
		if (blob.data)
		{
			delete[] blob.data;
		}
	}

	DiskAssetFile::~DiskAssetFile()
	{
		Close();

		if (mFreeDataBlob && binaryBlob.data)
		{
			FreeForBlob(binaryBlob);
			binaryBlob.data = nullptr;
			binaryBlob.size = 0;
		}

		/*if (metaData.data)
		{
			FreeForBlob(metaData);
			metaData.data = nullptr;
			metaData.size = 0;
		}*/
	}

	const char* DiskAssetFile::FilePath() const
	{
		return mFilePath.c_str();
	}

	void DiskAssetFile::SetFreeDataBlob(bool freeDataBlob)
	{
		mFreeDataBlob = freeDataBlob;
	}

}


