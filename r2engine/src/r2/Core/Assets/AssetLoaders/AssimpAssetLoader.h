#ifndef __ASSIMP_ASSET_LOADER_H__
#define __ASSIMP_ASSET_LOADER_H__

#include "r2/Core/Assets/AssetLoaders/AssetLoader.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>


struct aiNode;
struct aiScene;
struct aiMesh;

namespace r2::draw
{
	struct Skeleton;
	struct AnimModel;
}

namespace r2::asset
{
	class AssimpAssetLoader : public AssetLoader
	{
	public:

		struct Joint
		{
			std::string name;
			s32 parentIndex;
			s32 jointIndex;
			glm::mat4 localTransform;
		};

		struct Bone
		{
			std::string name;
			glm::mat4 invBindPoseMat;
		};

		virtual const char* GetPattern() override;
		virtual AssetType GetType() const override;
		virtual bool ShouldProcess() override;
		virtual u64 GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking) override;
		virtual bool LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer) override;

		static aiNode* FindNodeInHeirarchy(const aiScene* scene, aiNode* node, const std::string& strName);
	private:
		//If we multithread this later we'll obviously need a different solution
		//for now store the data from GetLoadedAssetSize() to use in LoadAsset
		u64 mNumMeshes = 0;
		u64 mNumBones = 0;
		u64 mNumVertices = 0;



		
		void MarkBonesInMesh(aiNode* node, const aiScene* scene, aiMesh* mesh);
		void MarkBones(aiNode* node, const aiScene* scene);
		void CreateBonesVector(const aiScene* scene, aiNode* node, s32 parentIndex);

		void LoadBindPose(r2::draw::AnimModel& model, r2::draw::Skeleton& skeleton);
		//void LoadRestPose(r2::anim::Pose& pose, const std::vector<Joint>& joints);
		//void LoadBindPose(r2::anim::Pose& bindPose, const const r2::anim::Pose& restPose);

	};
}



#endif