#include "r2pch.h"

#include "r2/Render/Model/Materials/MaterialHelpers.h"
#include "r2/Render/Model/Materials/Material_generated.h"
#include "r2/Render/Model/Materials/MaterialPack_generated.h"
#include "r2/Render/Model/Model_generated.h"
#include "r2/Render/Model/Shader/ShaderSystem.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Utils/Hash.h"

#include "r2/Core/Memory/InternalEngineMemory.h"

namespace r2::mat
{
	const flat::Material* GetMaterialForMaterialName(const flat::MaterialPack* materialPack, u64 materialName)
	{
		if (!materialPack)
		{
			return nullptr;
		}

		const auto numMaterialParams = materialPack->pack()->size();

		for (flatbuffers::uoffset_t i = 0; i < numMaterialParams; ++i)
		{
			if (materialPack->pack()->Get(i)->assetName() == materialName)
			{
				return materialPack->pack()->Get(i);
			}
		}

		return nullptr;
	}

	const flat::Material* GetMaterialForMaterialName(MaterialName materialName)
	{
		if (materialName.name == 0 || materialName.packName == 0)
		{
			return nullptr;
		}

		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

		const byte* manifestData = r2::asset::lib::GetManifestData(assetLib, materialName.packName);

		R2_CHECK(manifestData != nullptr, "");

		const flat::MaterialPack* materialParamsPack = flat::GetMaterialPack(manifestData);

		R2_CHECK(materialParamsPack != nullptr, "This should never be nullptr");

		const flat::Material* material = GetMaterialForMaterialName(materialParamsPack, materialName.name);

		return material;
	}

	u64 GetShaderNameForMaterialName(const flat::MaterialPack* materialPack, u64 materialName, flat::eMeshPass meshPass, r2::draw::eShaderEffectType shaderEffectType)
	{
		R2_CHECK(materialPack != nullptr, "This should never happen");
		R2_CHECK(meshPass != flat::eMeshPass_NUM_SHADER_EFFECT_PASSES, "Should never happen!");

		const flat::Material* material = GetMaterialForMaterialName(materialPack, materialName);

		if (!material)
		{
			return 0;
		}

		if (!material->shaderEffectPasses())
		{
			return 0;
		}

		const auto* shaderEffectPasses = material->shaderEffectPasses();

		const auto* shaderEffect = shaderEffectPasses->shaderEffectPasses()->Get(meshPass);

		if (shaderEffectType == draw::SET_STATIC)
		{
			return shaderEffect->staticShader();
		}

		return shaderEffect->dynamicShader();
	}

	r2::draw::ShaderHandle GetShaderHandleForMaterialName(MaterialName materialName, flat::eMeshPass meshPass, r2::draw::eShaderEffectType shaderEffectType)
	{
		const auto* material = GetMaterialForMaterialName(materialName);

		if (!material)
		{
			R2_CHECK(false, "Probably shouldn't ever happen?");
			return r2::draw::InvalidShader;
		}

		if (!material->shaderEffectPasses())
		{
			R2_CHECK(false, "Probably shouldn't ever happen?");
			return r2::draw::InvalidShader;
		}

		const auto* shaderEffectPasses = material->shaderEffectPasses();

		const auto* shaderEffect = shaderEffectPasses->shaderEffectPasses()->Get(meshPass);

		r2::draw::ShaderName shaderName = 0;

		shaderName = shaderEffect->staticShader();
		if (shaderEffectType == draw::SET_DYNAMIC)
		{
			shaderName = shaderEffect->dynamicShader();
		}

		r2::draw::ShaderHandle shaderHandle = r2::draw::shadersystem::FindShaderHandle(shaderName);

		R2_CHECK(shaderHandle != r2::draw::InvalidShader, "This can never be the case - you forgot to load the shader?");

		return shaderHandle;
	}

	r2::draw::ShaderEffectPasses GetShaderEffectPassesForMaterialName(MaterialName materialName)
	{	

		
		const auto* material = GetMaterialForMaterialName(materialName);

		if (!material)
		{
			R2_CHECK(false, "Probably shouldn't ever happen?");
			r2::draw::ShaderEffectPasses empty;
			return empty;
		}

		if (!material->shaderEffectPasses())
		{
			R2_CHECK(false, "Probably shouldn't ever happen?");
			r2::draw::ShaderEffectPasses empty;
			return empty;
		}

		const auto* flatShaderEffectPasses = material->shaderEffectPasses()->shaderEffectPasses();
		r2::draw::ShaderEffectPasses shaderEffectPasses;

		for (u32 i = flat::eMeshPass_FORWARD; i < flat::eMeshPass_NUM_SHADER_EFFECT_PASSES; ++i)
		{
			flat::eMeshPass flatMeshPass = static_cast<flat::eMeshPass>(i);

			const flat::ShaderEffect* flatShaderEffect = flatShaderEffectPasses->Get(flatMeshPass);

			r2::draw::ShaderEffect shaderEffect;
			shaderEffect.staticShaderHandle = r2::draw::shadersystem::FindShaderHandle(flatShaderEffect->staticShader());
			shaderEffect.dynamicShaderHandle = r2::draw::shadersystem::FindShaderHandle(flatShaderEffect->dynamicShader());

			shaderEffectPasses.meshPasses[flatMeshPass] = shaderEffect;
		}

		return shaderEffectPasses;
	}

	u64 GetAlbedoTextureNameForMaterialName(const flat::MaterialPack* materialPack, u64 materialName)
	{
		const flat::Material* material = GetMaterialForMaterialName(materialPack, materialName);

		if (!material || material->shaderParams())
		{
			return 0;
		}

		if (!material->shaderParams()->textureParams())
		{
			return 0;
		}

		const auto numTextureParams = material->shaderParams()->textureParams()->size();

		const auto* textureParams = material->shaderParams()->textureParams();

		for (flatbuffers::uoffset_t i = 0; i < numTextureParams; ++i)
		{
			const auto* texParam = textureParams->Get(i);

			if (texParam->propertyType() == flat::ShaderPropertyType_ALBEDO)
			{
				return texParam->value();
			}
		}

		return 0;
	}

	MaterialName MakeMaterialNameFromFlatMaterial(const flat::MaterialName* flatMaterialName)
	{
		MaterialName materialName;
		materialName.name = flatMaterialName->name();
		materialName.packName = flatMaterialName->materialPackName();

		return materialName;
	}

	void GetAllTexturePacksForMaterial(const flat::Material* material, r2::SArray<u64>* texturePacks)
	{
		R2_CHECK(material != nullptr, "Material is nullptr");
		R2_CHECK(texturePacks != nullptr, "texture packs is nullptr");

		if (!material->shaderParams())
		{
			return;
		}

		if (!material->shaderParams()->textureParams())
		{
			return;
		}

		const auto* textureParams = material->shaderParams()->textureParams();

		const flatbuffers::uoffset_t numTextureParams = textureParams->size();

		for (flatbuffers::uoffset_t i = 0; i < numTextureParams; ++i)
		{
			const auto* textureParam = textureParams->Get(i);

			u64 texturePackName = textureParam->texturePackName();

			if (r2::sarr::IndexOf(*texturePacks, texturePackName) != -1)
			{
				r2::sarr::Push(*texturePacks, texturePackName);
			}
		}
	}

#ifdef R2_ASSET_PIPELINE
	std::vector<const flat::Material*> GetAllMaterialsInMaterialPackThatContainTexture(const flat::MaterialPack* materialPack, u64 texturePackName, u64 textureName)
	{
		R2_CHECK(materialPack != nullptr, "Should never happen");
		R2_CHECK(texturePackName != 0 && texturePackName != STRING_ID(""), "Should never happen");
		R2_CHECK(textureName != 0 && textureName != STRING_ID(""), "Should never happen");

		auto numMaterials = materialPack->pack()->size();

		std::vector<const flat::Material*> materialToReturn = {};

		for (flatbuffers::uoffset_t i = 0; i < numMaterials; ++i)
		{
			const auto* material = materialPack->pack()->Get(i);

			R2_CHECK(material != nullptr, "Should never happen");
			if (!material->shaderParams())
			{
				continue;
			}

			const auto* textureParams = material->shaderParams()->textureParams();
			auto numTextureParams = textureParams->size();

			for (flatbuffers::uoffset_t t = 0; t < numTextureParams; ++t)
			{
				const auto* textureParam = textureParams->Get(t);

				if (textureParam->texturePackName() == texturePackName && textureParam->value() == textureName)
				{
					materialToReturn.push_back(material);
					break;
				}
			}
		}

		return materialToReturn;
	}

	std::vector<const flat::Material*> GetAllMaterialsInMaterialPackThatContainTexturePack(const flat::MaterialPack* materialPack, u64 texturePackName)
	{
		R2_CHECK(materialPack != nullptr, "Should never happen");
		R2_CHECK(texturePackName != 0 && texturePackName != STRING_ID(""), "Should never happen");
		auto numMaterialParams = materialPack->pack()->size();

		std::vector<const flat::Material*> materialsToReturn = {};

		for (flatbuffers::uoffset_t i = 0; i < numMaterialParams; ++i)
		{
			const auto* material = materialPack->pack()->Get(i);
			R2_CHECK(material != nullptr, "Should never happen");
			if (!material->shaderParams())
			{
				continue;
			}
			const auto* textureParams = material->shaderParams()->textureParams();

			auto numTextureParams = textureParams->size();

			for (flatbuffers::uoffset_t t = 0; t < numTextureParams; ++t)
			{
				const auto* textureParam = textureParams->Get(t);

				if (textureParam->texturePackName() == texturePackName)
				{
					materialsToReturn.push_back(material);
					break;
				}
			}
		}
		return materialsToReturn;
	}

	std::vector<MaterialParam> GetAllMaterialsThatMatchVertexLayout(flat::eMeshPass pass, flat::eVertexLayoutType staticVertexLayout, flat::eVertexLayoutType dynamicVertexLayout)
	{
		std::vector<MaterialParam> materialsToReturn = {};
		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

		u32 numManifests = r2::asset::lib::GetManifestDataCountForType(assetLib, r2::asset::EngineAssetType::MATERIAL_PACK_MANIFEST);

		R2_CHECK(numManifests > 0, "Should always have at least 1 manifest for materials");

		r2::SArray<const byte*>* manifests = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const byte*, numManifests);

		r2::asset::lib::GetManifestDataForType(assetLib, r2::asset::EngineAssetType::MATERIAL_PACK_MANIFEST, manifests);

		for (u32 i = 0; i < numManifests; ++i)
		{
			const byte* manifestData = r2::sarr::At(*manifests, i);

			R2_CHECK(manifestData != nullptr, "");

			const flat::MaterialPack* materialParamsPack = flat::GetMaterialPack(manifestData);

			const auto* pack = materialParamsPack->pack();

			for (flatbuffers::uoffset_t j = 0; j < pack->size(); ++j)
			{
				const flat::Material* material = pack->Get(j);

				const flat::ShaderEffect* shaderEffect = material->shaderEffectPasses()->shaderEffectPasses()->Get(pass);

				R2_CHECK(shaderEffect != nullptr, "Should always be valid");

				if (shaderEffect->staticVertexLayout() == staticVertexLayout && shaderEffect->dynamicVertexLayout() == dynamicVertexLayout)
				{
					MaterialParam materialParam;
					materialParam.flatMaterial = material;
					materialParam.materialName = { material->assetName(), materialParamsPack->assetName() };
					materialsToReturn.push_back(materialParam);
				}
			}
		}
		
		FREE(manifests, *MEM_ENG_SCRATCH_PTR);

		return materialsToReturn;
	}

#endif
}