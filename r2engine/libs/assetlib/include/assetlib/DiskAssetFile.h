#pragma once

#include "assetlib/AssetFile.h"
#include <string>
#include <fstream>

namespace r2::assets::assetlib
{
	class DiskAssetFile : public r2::assets::assetlib::AssetFile
	{
	public:
		DiskAssetFile();

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
		virtual ~DiskAssetFile();
		virtual const char* FilePath() const override;

	private:
		mutable std::string mFilePath;
		mutable std::fstream mFile;
	};
}

