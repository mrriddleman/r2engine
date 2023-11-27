#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/RModelsManifestUtils.h"
#include "r2/Render/Model/RModelManifest_generated.h"
#include "r2/Core/Assets/Asset.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::asset::pln
{
	const std::string RMODEL_MANIFEST_FBS = "RModelManifest.fbs";

	bool SaveRModelManifestAssetReferencesToManifestFile(u32 version, const std::vector<r2::asset::AssetReference>& assetReferences, const std::string& binFilePath, const std::string& rawFilePath)
	{
		flatbuffers::FlatBufferBuilder builder;
		std::vector < flatbuffers::Offset< flat::AssetRef >> flatAssetRefs;

		for (const r2::asset::AssetReference& assetReference : assetReferences)
		{
			auto flatAssetName = flat::CreateAssetName(builder, 0, assetReference.assetName.hashID, builder.CreateString(assetReference.assetName.assetNameString));

			flatAssetRefs.push_back(flat::CreateAssetRef(builder, flatAssetName, builder.CreateString(assetReference.binPath.string()), builder.CreateString(assetReference.rawPath.string())));
		}

		std::filesystem::path binPath = binFilePath;

		//add the texture packs to the manifest

		const auto rmodelManifestAssetName = r2::asset::Asset::GetAssetNameForFilePath(binPath.string().c_str(), r2::asset::EngineAssetType::RMODEL_MANIFEST);

		char rmodelManifestNameStr[r2::fs::FILE_PATH_LENGTH];

		r2::asset::MakeAssetNameStringForFilePath(binPath.string().c_str(), rmodelManifestNameStr, r2::asset::EngineAssetType::RMODEL_MANIFEST);

		auto flatAssetName = flat::CreateAssetName(builder, 0, rmodelManifestAssetName, builder.CreateString(rmodelManifestNameStr));

		auto manifest = flat::CreateRModelsManifest(builder, version, flatAssetName, builder.CreateVector(flatAssetRefs));

		//generate the manifest
		builder.Finish(manifest);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		utils::WriteFile(binPath.string(), (char*)buf, size);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char rmodelManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), rmodelManifestSchemaPath, RMODEL_MANIFEST_FBS.c_str());

		std::filesystem::path jsonPath = rawFilePath;

		bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(jsonPath.parent_path().string(), rmodelManifestSchemaPath, binPath.string());

		bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(binPath.parent_path().string(), rmodelManifestSchemaPath, jsonPath.string());

		return generatedJSON && generatedBinary;
	}
}

#endif