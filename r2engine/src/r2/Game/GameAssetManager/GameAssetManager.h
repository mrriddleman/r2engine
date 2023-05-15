#ifndef __GAME_ASSET_MANAGER_H__
#define __GAME_ASSET_MANAGER_H__


#include "r2/Utils/Utils.h"
#include "r2/Game/Level/Level.h"
#include "r2/Core/Containers/SArray.h"
#ifdef R2_ASSET_PIPELINE
#include <string>
#include <vector>
#endif

namespace flat
{
	struct LevelData;
}

namespace r2::draw
{
	struct GPURenderMaterial;
	struct Model;
	struct AnimModel;
}

namespace r2
{
	struct GameAssetManager
	{

	};

	namespace asstmgr
	{

		GameAssetManager* Create();
		void Shutdown(GameAssetManager* gameAssetManager);
		u64 MemorySize();

		bool LoadLevelAssetsFromDisk(GameAssetManager& gameAssetManager, const flat::LevelData* levelData);
		bool UnloadLevelAssetsFromDisk(GameAssetManager& gameAssetManager, const flat::LevelData* levelData);

		bool UploadLevelAssetsToGPU(GameAssetManager& gameAssetManager);
		bool UnloadLevelAssetsFromGPU(GameAssetManager& gameAssetManager);

		void Update(GameAssetManager& gameAssetManager);

		const r2::draw::GPURenderMaterial* GetRenderMaterialForMaterialName(const GameAssetManager& gameAssetManager, u64 materialName);
		void GetRenderMaterialsForModel(const GameAssetManager& gameAssetManager, const r2::draw::Model& model, r2::SArray<r2::draw::GPURenderMaterial>* renderMaterials);

		const r2::draw::Model* GetModel(const GameAssetManager& gameAssetManager, u64 modelName);
		const r2::draw::AnimModel* GetAnimModel(const GameAssetManager& gameAssetManager, u64 modelName);

#ifdef R2_ASSET_PIPELINE
		void TextureChanged(GameAssetManager& gameAssetManager, const std::string& texturePath);
		void TexturePackAdded(GameAssetManager& gameAssetManager, const std::string& texturePacksManifestFilePath, const std::string& texturePackPath, const std::vector<std::vector<std::string>>& texturePathsAdded);
		void TextureAdded(GameAssetManager& gameAssetManager, const std::string& texturePacksManifestFilePath, const std::string& texturePath);
		void TextureRemoved(GameAssetManager& gameAssetManager, const std::string& texturePacksManifestFilePath, const std::string& textureRemoved);
		void TexturePackRemoved(GameAssetManager& gameAssetManager, const std::string& texturePacksManifestFilePath, const std::string& texturePackPath, const std::vector<std::vector<std::string>>& texturePathsLeft);

		void MaterialChanged(GameAssetManager& gameAssetManager, const std::string& materialPathChanged);
		void MaterialAdded(GameAssetManager& gameAssetManager, const std::string& materialPacksManifestFile, const std::string& materialPathAdded);
		void MaterialRemoved(GameAssetManager& gameAssetManager, const std::string& materialPacksManifestFile, const std::string& materialPathRemoved);
#endif
	}
}


#endif
