#include "assetlib/AssetFile.h"
#include <lz4.h>
#include "flatbuffers/flatbuffers.h"


namespace r2::assets::assetlib
{
	bool save_binaryfile(const char* path, const AssetFile& file)
	{
		file.OpenForWrite(path);

		file.Write(file.type, 4);
		uint32_t version = file.version;
		//version
		file.Write((const char*)&version, sizeof(uint32_t));

		//json length
		uint32_t length = file.metaData.size;
		file.Write((const char*)&length, sizeof(uint32_t));

		//blob length
		uint32_t bloblength = file.binaryBlob.size;
		file.Write((const char*)&bloblength, sizeof(uint32_t));

		//json stream
		file.Write(file.metaData.data, length);
		//blob data
		file.Write(file.binaryBlob.data, file.binaryBlob.size);

		file.Close();

		return true;
	}

	bool load_binaryfile(const char* path, AssetFile& outputFile)
	{
		outputFile.OpenForRead(path);

		if (!outputFile.IsOpen()) return false;

		//move file cursor to beginning
		//infile.seekg(0);

		outputFile.Read(outputFile.type, 4);
		outputFile.Read((char*)&outputFile.version, sizeof(uint32_t));

		outputFile.Read((char*)&outputFile.metaData.size, sizeof(uint32_t));
		outputFile.Read((char*)&outputFile.binaryBlob.size, sizeof(uint32_t));

		outputFile.AllocateForBlob(outputFile.metaData);

		outputFile.Read(outputFile.metaData.data, outputFile.metaData.size);

		outputFile.AllocateForBlob(outputFile.binaryBlob);

		outputFile.Read(outputFile.binaryBlob.data, outputFile.binaryBlob.size);

		return true;
	}
}