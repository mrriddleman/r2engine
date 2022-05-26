#include "assetlib/AnimationAsset.h"
#include "assetlib/AssetFile.h"
#include "assetlib/RAnimation_generated.h"
#include "assetlib/RAnimationMetaData_generated.h"

#include <cassert>

namespace r2::assets::assetlib
{
	const flat::RAnimationMetaData* read_animation_meta_data(const AssetFile& file)
	{
		assert(file.metaData.data != nullptr && "Did you forget to load the file first?");
		return flat::GetRAnimationMetaData(file.metaData.data);
	}

	const flat::RAnimation* read_animation_data(const flat::RAnimationMetaData* metaData, const AssetFile& file)
	{
		assert(file.binaryBlob.data != nullptr && "Did you forget to load the file first?");
		return flat::GetRAnimation(file.binaryBlob.data);
	}

	void pack_animation(AssetFile& file, uint8_t* metaBuffer, size_t metaBufferSize, uint8_t* dataBuffer, size_t dataBufferSize)
	{
		assert(metaBuffer != nullptr && "Passed in null meta data");
		assert(dataBuffer != nullptr && "Passed in null model data");
		assert(metaBufferSize > 0 && "meta buffer size should be greater than 0");
		assert(dataBufferSize > 0 && "data buffer size should be greater than 0");

		file.type[0] = 'r';
		file.type[1] = 'a';
		file.type[2] = 'n';
		file.type[3] = 'm';

		file.version = 1;

		file.metaData.size = metaBufferSize;
		file.metaData.data = (char*)metaBuffer;

		file.binaryBlob.size = dataBufferSize;
		file.binaryBlob.data = (char*)dataBuffer;
	}
}