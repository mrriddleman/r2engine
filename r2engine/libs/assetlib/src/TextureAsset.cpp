#include "assetlib/TextureAsset.h"
#include "assetlib/AssetFile.h"
#include "flatbuffers/flatbuffers.h"
#include <lz4.h>
#include <cassert>

namespace r2::assets::assetlib
{
	const flat::TextureMetaData* read_texture_meta_data(const AssetFile& file)
	{
		return flat::GetTextureMetaData(file.metaData.data);
	}

	void unpack_texture(const flat::TextureMetaData* info, const char* sourcebuffer, size_t sourceSize, char* destination)
	{
		assert(info != nullptr && "Texture Meta Data is null");

		if (info->compressionMode() == flat::CompressionMode_LZ4)
		{
			for (flatbuffers::uoffset_t i = 0; i < info->mips()->size(); ++i)
			{
				const auto mip = info->mips()->Get(i);

				LZ4_decompress_safe(sourcebuffer, destination, mip->compressedSize(), mip->originalSize());
				sourcebuffer += mip->compressedSize();
				destination += mip->originalSize();
			}
		}
		else
		{
			memcpy(destination, sourcebuffer, sourceSize);
		}
	}

	void unpack_texture_page(const flat::TextureMetaData* info, int pageIndex, char* sourcebuffer, char* destination)
	{
		char* source = sourcebuffer;

		for (int i = 0; i < pageIndex; ++i)
		{
			source += info->mips()->Get(i)->compressedSize();
		}

		if (info->compressionMode() == flat::CompressionMode_LZ4)
		{
			if (info->mips()->Get(pageIndex)->compressedSize() != info->mips()->Get(pageIndex)->originalSize())
			{
				auto compressSize = info->mips()->Get(pageIndex)->compressedSize();
				auto originalSize = info->mips()->Get(pageIndex)->originalSize();
				int r = LZ4_decompress_safe(source, destination, compressSize, originalSize);

				if (r < 0)
				{
					int k = 0;
				}
			}
			else
			{
				memcpy(destination, source, info->mips()->Get(pageIndex)->originalSize());
			}
		}
		else
		{
			memcpy(destination, source, info->mips()->Get(pageIndex)->originalSize());
		}
	}

	void pack_texture(AssetFile& file, flat::TextureMetaData* info, void* pixelData)
	{
		assert(info != nullptr && "Passed in a null texture meta data object");
		assert(pixelData != nullptr && "Passed in empty pixel data");
		assert(file.binaryBlob.data != nullptr && "binary blob data shouldn't be null!");
		file.type[0] = 'r';
		file.type[1] = 't';
		file.type[2] = 'e';
		file.type[3] = 'x';
		file.version = 1; //@TODO(Serge): update as needed. 1 for now
		
		char* pixels = (char*)pixelData;

		std::vector<char> page_buffer; // I THINK because this function is only called in the pipeline that this is okay to do here

		file.binaryBlob.size = 0;

		for (flatbuffers::uoffset_t p = 0; p < info->mips()->size(); ++p)
		{
			flat::MipInfo* mip = info->mutable_mips()->GetMutableObject(p);

			page_buffer.resize(mip->originalSize());

			int compressStaging = LZ4_compressBound(mip->originalSize());

			page_buffer.resize(compressStaging);

			int compressedSize = LZ4_compress_default(pixels, page_buffer.data(), mip->originalSize(), compressStaging);

			float compressionRate = (float)compressedSize / (float)info->textureSize();

			if (compressionRate > 0.8f)
			{
				compressedSize = mip->originalSize();

				page_buffer.resize(compressedSize);

				memcpy(page_buffer.data(), pixels, compressedSize);
			}
			else
			{
				page_buffer.resize(compressedSize);
			}

			mip->mutate_compressedSize(compressedSize);

			memcpy(&file.binaryBlob.data[file.binaryBlob.size], page_buffer.data(), page_buffer.size() * sizeof(char));

			file.binaryBlob.size += page_buffer.size();

			pixels += mip->originalSize();			
		}

		//these will have to be set already since we can't get the data pointer or the size at this point
		assert(file.metaData.data != nullptr);
		assert(file.metaData.size != 0);

	}
}