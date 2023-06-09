#include "r2pch.h"
#include "r2/Render/Model/Materials/MaterialParams_generated.h"
#include "r2/Render/Model/Materials/MaterialParamsPack_generated.h"
#include "r2/Render/Model/Materials/MaterialParamsPackHelpers.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Render/Renderer/ShaderSystem.h"

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

}