#include "r2pch.h"

#include "r2/Game/GameAssetManager/GameAssetManager.h"

namespace r2::asstmgr
{
	GameAssetManager* Create()
	{
		return nullptr;
	}

	void Shutdown(GameAssetManager* gameAssetManager)
	{

	}

	u64 MemorySize()
	{
		return 0;
	}

	bool LoadLevelAssetsFromDisk(GameAssetManager& gameAssetManager, const flat::LevelData* levelData)
	{
		return false;
	}

	bool UnloadLevelAssetsFromDisk(GameAssetManager& gameAssetManager, const flat::LevelData* levelData)
	{
		return false;
	}

	bool UploadLevelAssetsToGPU(GameAssetManager& gameAssetManager)
	{
		return false;
	}

	bool UnloadLevelAssetsFromGPU(GameAssetManager& gameAssetManager)
	{
		return false;
	}

	void Update(GameAssetManager& gameAssetManager)
	{

	}

	const r2::draw::GPURenderMaterial* GetRenderMaterialForMaterialName(const GameAssetManager& gameAssetManager, u64 materialName)
	{
		return nullptr;
	}

	void GetRenderMaterialsForModel(const GameAssetManager& gameAssetManager, const r2::draw::Model& model, r2::SArray<r2::draw::GPURenderMaterial>* renderMaterials)
	{

	}

	const r2::draw::Model* GetModel(const GameAssetManager& gameAssetManager, u64 modelName)
	{
		return nullptr;
	}

	const r2::draw::AnimModel* GetAnimModel(const GameAssetManager& gameAssetManager, u64 modelName)
	{
		return nullptr;
	}

#ifdef R2_ASSET_PIPELINE
	void TextureChanged(GameAssetManager& gameAssetManager, const std::string& texturePath)
	{

	}

	void TexturePackAdded(GameAssetManager& gameAssetManager, const std::string& texturePacksManifestFilePath, const std::string& texturePackPath, const std::vector<std::vector<std::string>>& texturePathsAdded)
	{

	}

	void TextureAdded(GameAssetManager& gameAssetManager, const std::string& texturePacksManifestFilePath, const std::string& texturePath)
	{

	}

	void TextureRemoved(GameAssetManager& gameAssetManager, const std::string& texturePacksManifestFilePath, const std::string& textureRemoved)
	{
	}

	void TexturePackRemoved(GameAssetManager& gameAssetManager, const std::string& texturePacksManifestFilePath, const std::string& texturePackPath, const std::vector<std::vector<std::string>>& texturePathsLeft)
	{

	}

	void MaterialChanged(GameAssetManager& gameAssetManager, const std::string& materialPathChanged)
	{

	}

	void MaterialAdded(GameAssetManager& gameAssetManager, const std::string& materialPacksManifestFile, const std::string& materialPathAdded)
	{

	}

	void MaterialRemoved(GameAssetManager& gameAssetManager, const std::string& materialPacksManifestFile, const std::string& materialPathRemoved) 
	{

	}
#endif
}