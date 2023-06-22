#include "r2pch.h"
#include "r2/Render/Model/Materials/MaterialParams_generated.h"
#include "r2/Render/Model/Materials/MaterialParamsPack_generated.h"
#include "r2/Render/Model/Materials/MaterialParamsPackHelpers.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Render/Renderer/ShaderSystem.h"
#include "r2/Utils/Hash.h"

namespace r2::mat
{

	const flat::MaterialParams* GetMaterialParamsForMaterialName(const flat::MaterialParamsPack* materialPack, u64 materialName)
	{
		if (!materialPack)
		{
			return nullptr;
		}

		const auto numMaterialParams = materialPack->pack()->size();

		for (flatbuffers::uoffset_t i = 0; i < numMaterialParams; ++i)
		{
			if (materialPack->pack()->Get(i)->name() == materialName)
			{
				return materialPack->pack()->Get(i);
			}
		}

		return nullptr;
	}

	u64 GetShaderNameForMaterialName(const flat::MaterialParamsPack* materialPack, u64 materialName)
	{
		const flat::MaterialParams* material = GetMaterialParamsForMaterialName(materialPack, materialName);

		if (!material)
		{
			return 0;
		}

		const auto numShaderParams = material->ulongParams()->size();

		for (flatbuffers::uoffset_t i = 0; i < numShaderParams; ++i)
		{
			const auto* param = material->ulongParams()->Get(i);

			if (param->propertyType() == flat::MaterialPropertyType_SHADER)
			{
				return param->value();
			}
		}

		return 0;
	}

	u64 GetAlbedoTextureNameForMaterialName(const flat::MaterialParamsPack* materialPack, u64 materialName)
	{
		const flat::MaterialParams* material = GetMaterialParamsForMaterialName(materialPack, materialName);

		if (!material)
		{
			return 0;
		}

		const auto numTextureParams = material->textureParams()->size();

		for (flatbuffers::uoffset_t i = 0; i < numTextureParams; ++i)
		{
			const auto* texParam = material->textureParams()->Get(i);

			if (texParam->propertyType() == flat::MaterialPropertyType_ALBEDO)
			{
				return texParam->value();
			}
		}

		return 0;
	}

	r2::draw::ShaderHandle GetShaderHandleForMaterialName(MaterialName materialName)
	{
		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

		const byte* manifestData = r2::asset::lib::GetManifestData(assetLib, materialName.packName);

		R2_CHECK(manifestData != nullptr, "");

		const flat::MaterialParamsPack* materialParamsPack = flat::GetMaterialParamsPack(manifestData);

		R2_CHECK(materialParamsPack != nullptr, "This should never be nullptr");

		const flat::MaterialParams* material = GetMaterialParamsForMaterialName(materialParamsPack, materialName.name);

		const auto numShaderParams = material->ulongParams()->size();

		u64 shaderName = 0;

		for (flatbuffers::uoffset_t i = 0; i < numShaderParams; ++i)
		{
			const auto* param = material->ulongParams()->Get(i);

			if (param->propertyType() == flat::MaterialPropertyType_SHADER)
			{
				shaderName = param->value();
				break;
			}
		}

		r2::draw::ShaderHandle shaderHandle = r2::draw::shadersystem::FindShaderHandle(shaderName);

		R2_CHECK(shaderHandle != r2::draw::InvalidShader, "This can never be the case - you forgot to load the shader?");

		return shaderHandle;
	}

#ifdef R2_ASSET_PIPELINE
	std::vector<const flat::MaterialParams*> GetAllMaterialParamsInMaterialPackThatContainTexture(const flat::MaterialParamsPack* materialPack, u64 texturePackName, u64 textureName)
	{
		R2_CHECK(materialPack != nullptr, "Should never happen");
		R2_CHECK(texturePackName != 0 && texturePackName != STRING_ID(""), "Should never happen");
		R2_CHECK(textureName != 0 && textureName != STRING_ID(""), "Should never happen");

		auto numMaterialParams = materialPack->pack()->size();

		std::vector<const flat::MaterialParams*> materialParamsToReturn = {};

		for (flatbuffers::uoffset_t i = 0; i < numMaterialParams; ++i)
		{
			const auto* materialParams = materialPack->pack()->Get(i);

			auto numTextureParams = materialParams->textureParams()->size();

			for (flatbuffers::uoffset_t t = 0; t < numTextureParams; ++t)
			{
				const auto* textureParam = materialParams->textureParams()->Get(t);

				if (textureParam->texturePackName() == texturePackName && textureParam->value() == textureName)
				{
					materialParamsToReturn.push_back(materialParams);
					break;
				}
			}
		}

		return materialParamsToReturn;
	}

	std::vector<const flat::MaterialParams*> GetAllMaterialParamsInMaterialPackThatContainTexturePack(const flat::MaterialParamsPack* materialPack, u64 texturePackName)
	{
		R2_CHECK(materialPack != nullptr, "Should never happen");
		R2_CHECK(texturePackName != 0 && texturePackName != STRING_ID(""), "Should never happen");
		auto numMaterialParams = materialPack->pack()->size();

		std::vector<const flat::MaterialParams*> materialParamsToReturn = {};

		for (flatbuffers::uoffset_t i = 0; i < numMaterialParams; ++i)
		{
			const auto* materialParams = materialPack->pack()->Get(i);

			auto numTextureParams = materialParams->textureParams()->size();

			for (flatbuffers::uoffset_t t = 0; t < numTextureParams; ++t)
			{
				const auto* textureParam = materialParams->textureParams()->Get(t);

				if (textureParam->texturePackName() == texturePackName)
				{
					materialParamsToReturn.push_back(materialParams);
					break;
				}
			}
		}

		return materialParamsToReturn;
	}
#endif

#ifdef R2_EDITOR
	std::vector<std::string> GetAllTexturePacksForMaterialNames(const r2::SArray<MaterialName>& materialNames)
	{
		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

		const u32 numMaterialNames = r2::sarr::Size(materialNames);

		for (u32 i = 0; i < numMaterialNames; ++i)
		{
			const MaterialName& materialName = r2::sarr::At(materialNames, i);

			const byte* manifestData = r2::asset::lib::GetManifestData(assetLib, materialName.packName);

			R2_CHECK(manifestData != nullptr, "");

			const flat::MaterialParamsPack* materialParamsPack = flat::GetMaterialParamsPack(manifestData);

			R2_CHECK(materialParamsPack != nullptr, "This should never be nullptr");



		}

		


		return {};
	}
#endif
}