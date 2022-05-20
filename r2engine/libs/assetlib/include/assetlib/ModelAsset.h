#ifndef __ASSET_LIB_MODEL_ASSET_H__
#define __ASSET_LIB_MODEL_ASSET_H__

#include "flatbuffers/flatbuffers.h"

namespace flat
{
	struct RModelMetaData;
	struct RModel;
	struct RMesh;
}

namespace r2::assets::assetlib
{
	struct AssetFile;

	struct Bounds
	{
		float origin[3];
		float radius;
		float extents[3];
	};

	struct Position
	{
		float v[3];
	};

	const flat::RModelMetaData* read_model_meta_data(const AssetFile& file);

	const flat::RModel* read_model_data(const flat::RModelMetaData* info, const AssetFile& file);

	void pack_model(AssetFile& file, uint8_t* metaBuffer, size_t metaBufferSize, uint8_t* dataBuffer, size_t dataBufferSize);

	const flatbuffers::Offset<flat::RMesh> pack_mesh(flatbuffers::FlatBufferBuilder& builder, flat::RModelMetaData* info, uint32_t meshIndex, char* data, int materialIndex, char* vertexData, char* indexData);

	void unpack_mesh(const flat::RModelMetaData* info, uint32_t meshIndex, char* sourceBuffer, size_t sourceSize, char** vertexBuffer, char** indexBuffer);

	Bounds CalculateBounds(Position* positions, size_t numPositions);
}


#endif