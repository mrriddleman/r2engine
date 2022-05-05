#include "AssetConverterAssetFile.h"



AssetConverterAssetFile::AssetConverterAssetFile()
	:mFilePath("")
{
}

bool AssetConverterAssetFile::OpenForRead(const char* filePath)
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

bool AssetConverterAssetFile::OpenForWrite(const char* filePath) const
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

bool AssetConverterAssetFile::OpenForReadWrite(const char* filePath)
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

bool AssetConverterAssetFile::Close() const
{
	if (IsOpen())
	{
		mFile.close();
		mFilePath = "";
	}

	return true;
}

bool AssetConverterAssetFile::IsOpen() const
{
	return mFile.is_open();
}

uint32_t AssetConverterAssetFile::Size()
{
	if (!IsOpen())
		return 0;

	auto currentOffset =  mFile.tellg();

	mFile.seekg(0, std::ios::end);

	std::streampos fSize = mFile.tellg();
	
	mFile.seekg(currentOffset);

	return fSize;
}

uint32_t AssetConverterAssetFile::Read(char* data, uint32_t dataBufSize)
{
	mFile.read(data, dataBufSize);
	return dataBufSize;
}

uint32_t AssetConverterAssetFile::Read(char* data, uint32_t offset, uint32_t dataBufSize)
{
	mFile.seekg(offset, std::ios::beg);
	mFile.read(data, dataBufSize);
	return dataBufSize;
}

uint32_t AssetConverterAssetFile::Write(const char* data, uint32_t size) const
{
	mFile.write(data, size);
	return size;
}

bool AssetConverterAssetFile::AllocateForBlob(r2::assets::assetlib::BinaryBlob& blob)
{
	if (blob.size > 0)
		blob.data = new char[blob.size];

	return blob.size > 0;
}

void AssetConverterAssetFile::FreeForBlob(const r2::assets::assetlib::BinaryBlob& blob)
{
	if (blob.data)
	{
		delete[] blob.data;
	}
}

AssetConverterAssetFile::~AssetConverterAssetFile()
{
	Close();

	if (binaryBlob.data)
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

const char* AssetConverterAssetFile::FilePath() const
{
	return mFilePath.c_str();
}
