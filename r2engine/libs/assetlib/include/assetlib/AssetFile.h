
#include <vector>
#include <string>

namespace r2::assets::assetlib {

	struct BinaryBlob
	{
		uint32_t size;
		char* data;
	};
	
	struct AssetFile 
	{
		virtual bool OpenForRead(const char* filePath) = 0;
		virtual bool OpenForWrite(const char* filePath) const = 0;
		virtual bool OpenForReadWrite(const char* filePath) = 0;
		virtual bool Close() const = 0;
		virtual bool IsOpen() const = 0;
		virtual uint32_t Size() = 0;

		virtual uint32_t Read(char* data, uint32_t dataBufSize) = 0;
		virtual uint32_t Write(const char* data, uint32_t size) const = 0;
		virtual bool AllocateForBlob(BinaryBlob& blob) = 0;

		virtual ~AssetFile() {}
		virtual const char* FilePath() const = 0;

		char type[4];
		uint32_t version;
		BinaryBlob metaData;
		BinaryBlob binaryBlob;
	};

	bool save_binaryfile(const char* path, const AssetFile& file);

	bool load_binaryfile(const char* path, AssetFile& outputFile);
}