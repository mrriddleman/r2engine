#include "r2pch.h"
#include "r2/Render/Model/Textures/TexturePack.h"
#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Utils/Utils.h"

namespace r2::draw::tex
{
	const u32 NUM_ARRAYS = 12; //number of arrays we have in the texture pack
	u64 TexturePackMemorySize(u64 capacity, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(TexturePack), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::asset::Asset>::MemorySize(capacity), alignment, headerSize, boundsChecking) * NUM_ARRAYS;
	}

	void UnloadAllTexturePackTexturesFromGPU(const TexturePack* texturePack, s64 slotID)
	{
		if (texturePack->metaData.assetType == asset::TEXTURE)
		{
			// go through each type

			if (texturePack->albedos)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->albedos);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->albedos, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}

			if (texturePack->normals)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->normals);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->normals, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}

			if (texturePack->roughnesses)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->roughnesses);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->roughnesses, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}

			if (texturePack->metallics)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->metallics);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->metallics, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}

			if (texturePack->occlusions)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->occlusions);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->occlusions, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}

			if (texturePack->heights)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->heights);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->heights, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}

			if (texturePack->details)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->details);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->details, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}

			if (texturePack->emissives)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->emissives);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->emissives, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}

			if (texturePack->anisotropys)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->anisotropys);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->anisotropys, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}

			if (texturePack->clearCoats)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->clearCoats);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->clearCoats, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}

			if (texturePack->clearCoatNormals)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->clearCoatNormals);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->clearCoatNormals, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}

			if (texturePack->clearCoatRoughnesses)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->clearCoatRoughnesses);

				for (u64 i = 0; i < numTextures; ++i)
				{
					asset::AssetHandle textureAssetHandle{ r2::sarr::At(*texturePack->clearCoatRoughnesses, i).HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);
				}
			}


		}
		else
		{
			//cubemap
			const auto numMipLevels = texturePack->metaData.numLevels;

			for (u32 i = 0; i < numMipLevels; ++i)
			{
				for (size_t s = 0; s < tex::NUM_SIDES; ++s)
				{
					asset::AssetHandle textureAssetHandle{ texturePack->metaData.mipLevelMetaData[i].sides[s].asset.HashID(), slotID };
					texsys::UnloadFromGPU(textureAssetHandle);

				}
			}
		}
	}

	bool IsTextureInTexturePack(const TexturePack* texturePack, u64 textureName, r2::asset::Asset& asset)
	{
		if (texturePack->metaData.assetType == asset::TEXTURE)
		{
			// go through each type

			if (texturePack->albedos)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->albedos);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->albedos, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->albedos, i);
						return true;
					}
				}
			}

			if (texturePack->normals)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->normals);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->normals, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->normals, i);
						return true;
					}
				}
			}

			if (texturePack->roughnesses)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->roughnesses);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->roughnesses, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->roughnesses, i);
						return true;
					}
				}
			}

			if (texturePack->metallics)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->metallics);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->metallics, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->metallics, i);
						return true;
					}
				}
			}

			if (texturePack->occlusions)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->occlusions);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->occlusions, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->occlusions, i);
						return true;
					}
				}
			}

			if (texturePack->heights)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->heights);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->heights, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->heights, i);
						return true;
					}
				}
			}

			if (texturePack->details)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->details);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->details, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->details, i);
						return true;
					}
				}
			}

			if (texturePack->emissives)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->emissives);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->emissives, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->emissives, i);
						return true;
					}
				}
			}

			if (texturePack->anisotropys)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->anisotropys);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->anisotropys, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->anisotropys, i);
						return true;
					}
				}
			}

			if (texturePack->clearCoats)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->clearCoats);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->clearCoats, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->clearCoats, i);
						return true;
					}
				}
			}

			if (texturePack->clearCoatNormals)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->clearCoatNormals);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->clearCoatNormals, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->clearCoatNormals , i);
						return true;
					}
				}
			}

			if (texturePack->clearCoatRoughnesses)
			{
				const auto numTextures = r2::sarr::Size(*texturePack->clearCoatRoughnesses);

				for (u64 i = 0; i < numTextures; ++i)
				{
					const auto nextTextureInPack = r2::sarr::At(*texturePack->clearCoatRoughnesses, i).HashID();
					if (nextTextureInPack == textureName)
					{
						asset = r2::sarr::At(*texturePack->clearCoatRoughnesses, i);
						return true;
					}
				}
			}


		}
		else
		{
			//cubemap
			const auto numMipLevels = texturePack->metaData.numLevels;

			for (u32 i = 0; i < numMipLevels; ++i)
			{
				for (size_t s = 0; s < tex::NUM_SIDES; ++s)
				{
					const auto nextTextureInPack = texturePack->metaData.mipLevelMetaData[i].sides[s].asset.HashID();
					if (nextTextureInPack == textureName)
					{
						asset = texturePack->metaData.mipLevelMetaData[i].sides[s].asset;
						return true;
					}
				}
			}
		}

		return false;
	}

	MipLevelMetaData& MipLevelMetaData::operator=(const MipLevelMetaData& mipLevelMetaData)
	{
		mipLevel = mipLevelMetaData.mipLevel;

		for (int i = 0; i < NUM_SIDES; ++i)
		{
			sides[i] = mipLevelMetaData.sides[i];
		}

		return *this;
	}


	TexturePackMetaData& TexturePackMetaData::operator=(const TexturePackMetaData& metaData)
	{
		for (int i = 0; i < MAX_MIP_LEVELS; ++i)
		{
			mipLevelMetaData[i] = metaData.mipLevelMetaData[i];
		}
		numLevels = metaData.numLevels;
		assetType = metaData.assetType;

		return *this;
	}
}