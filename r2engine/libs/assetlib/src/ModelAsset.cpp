#include "assetlib/ModelAsset.h"
#include "assetlib/AssetFile.h"

#include "assetlib/RModel_generated.h"
#include "assetlib/RModelMetaData_generated.h"

#include <cassert>
#include <functional>
#include <algorithm>
#include <lz4.h>

namespace r2::assets::assetlib
{
	const flat::RModelMetaData* read_model_meta_data(const AssetFile& file)
	{
		return flat::GetRModelMetaData(file.metaData.data);
	}

	const flat::RModel* read_model_data(const flat::RModelMetaData* info, const AssetFile& file)
	{
		return flat::GetRModel(file.binaryBlob.data);
	}

	void pack_model(AssetFile& file, uint8_t* metaBuffer, size_t metaBufferSize, uint8_t* dataBuffer, size_t dataBufferSize)
	{
		assert(metaBuffer != nullptr && "Passed in null meta data");
		assert(dataBuffer != nullptr && "Passed in null model data");
		assert(metaBufferSize > 0 && "meta buffer size should be greater than 0");
		assert(dataBufferSize > 0 && "data buffer size should be greater than 0");
		//assert(file.binaryBlob.data != nullptr && "binary blob data shouldn't be null");

		file.type[0] = 'r';
		file.type[1] = 'm';
		file.type[2] = 'd';
		file.type[3] = 'l';
		
		file.version = 1;

		file.metaData.size = metaBufferSize;
		file.metaData.data = (char*)metaBuffer;

		file.binaryBlob.size = dataBufferSize;
		file.binaryBlob.data = (char*)dataBuffer;
	}

	const flatbuffers::Offset<flat::RMesh> pack_mesh(flatbuffers::FlatBufferBuilder& builder, flat::RModelMetaData* info, uint32_t meshIndex, char* data, int materialIndex, char* vertexData, char* indexData)
	{
		flat::MeshInfo* meshInfo = info->meshInfos()->GetMutableObject(meshIndex);

		size_t fullSize = meshInfo->vertexBufferSize() + meshInfo->indexBufferSize();

		std::vector<char> mergedBuffer;
		mergedBuffer.resize(fullSize);

		memcpy(mergedBuffer.data(), vertexData, meshInfo->vertexBufferSize());

		memcpy(mergedBuffer.data() + meshInfo->vertexBufferSize(), indexData, meshInfo->indexBufferSize());

		if (meshInfo->compressionMode() == flat::MeshCompressionMode_LZ4)
		{
			auto vertexCompressStaging = LZ4_compressBound(static_cast<int>(fullSize));

			//*data = new char[vertexCompressStaging];

			auto compressedSize = LZ4_compress_default(mergedBuffer.data(), data, static_cast<int>(mergedBuffer.size()), vertexCompressStaging);

			meshInfo->mutate_compressedSize(compressedSize);
		}
		else
		{
			//*data = new char[mergedBuffer.size()];

			memcpy(data, mergedBuffer.data(), mergedBuffer.size());

			meshInfo->mutate_compressedSize(mergedBuffer.size());
		}

		return flat::CreateRMesh(builder, materialIndex, builder.CreateVectorScalarCast<int8_t>(data, meshInfo->compressedSize()) );
	}

	void unpack_mesh(const flat::RModelMetaData* info, uint32_t meshIndex, char* sourceBuffer, size_t sourceSize, char** vertexBuffer, char** indexBuffer)
	{
		const flat::MeshInfo* meshInfo = info->meshInfos()->Get(meshIndex);
		assert(meshInfo->compressedSize() == sourceSize && "These should be the same in this case");
		
		if (meshInfo->compressionMode() == flat::MeshCompressionMode_LZ4)
		{
			std::vector<char> decompressedBuffer;
			decompressedBuffer.resize(meshInfo->vertexBufferSize() + meshInfo->indexBufferSize());

			LZ4_decompress_safe(sourceBuffer, decompressedBuffer.data(), static_cast<int>(sourceSize), static_cast<int>(decompressedBuffer.size()));

			memcpy(*vertexBuffer, decompressedBuffer.data(), meshInfo->vertexBufferSize());
			memcpy(*indexBuffer, decompressedBuffer.data() + meshInfo->vertexBufferSize(), meshInfo->indexBufferSize());
		}
		else
		{
			*vertexBuffer = sourceBuffer;
			*indexBuffer = (sourceBuffer + meshInfo->vertexBufferSize());
		}
	}

	Bounds CalculateBounds(Position* positions, size_t numPositions)
	{
		Bounds bounds;

		float min[3] = { std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max() };
		float max[3] = { std::numeric_limits<float>::min(),std::numeric_limits<float>::min(),std::numeric_limits<float>::min() };

		for (int i = 0; i < numPositions; i++) 
		{
			min[0] = std::min(min[0], positions[i].v[0]);
			min[1] = std::min(min[1], positions[i].v[1]);
			min[2] = std::min(min[2], positions[i].v[2]);

			max[0] = std::max(max[0], positions[i].v[0]);
			max[1] = std::max(max[1], positions[i].v[1]);
			max[2] = std::max(max[2], positions[i].v[2]);
		}

		bounds.extents[0] = (max[0] - min[0]) / 2.0f;
		bounds.extents[1] = (max[1] - min[1]) / 2.0f;
		bounds.extents[2] = (max[2] - min[2]) / 2.0f;

		bounds.origin[0] = bounds.extents[0] + min[0];
		bounds.origin[1] = bounds.extents[1] + min[1];
		bounds.origin[2] = bounds.extents[2] + min[2];

		//go through the vertices again to calculate the exact bounding sphere radius
		float r2 = 0;
		for (int i = 0; i < numPositions; i++) {

			float offset[3];
			offset[0] = positions[i].v[0] - bounds.origin[0];
			offset[1] = positions[i].v[1] - bounds.origin[1];
			offset[2] = positions[i].v[2] - bounds.origin[2];

			//pythagoras
			float distance = offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2];
			r2 = std::max(r2, distance);
		}

		bounds.radius = std::sqrt(r2);

		return bounds;
	}
}