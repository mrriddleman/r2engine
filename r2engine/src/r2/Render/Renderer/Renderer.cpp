#include "r2pch.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Math/MathUtils.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Model/Light.h"

#include "r2/Render/Model/Materials/MaterialHelpers.h"

#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Render/Model/AreaTex.h"
#include "r2/Render/Model/SearchTex.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/Commands.h"
#include "r2/Render/Renderer/CommandBucket.h"
#include "r2/Render/Renderer/RendererImpl.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Model/Shader/ShaderSystem.h"


#include "r2/Game/GameAssetManager/GameAssetManager.h"

#include "r2/Utils/Hash.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/vector_angle.hpp"

#include <filesystem>

#include "r2/Utils/Timer.h"

namespace
{
	const u32 MAX_NUM_DRAWS = 2 << 15;
	
	const u32 AVG_NUM_OF_MESHES_PER_MODEL = 64;
	const u32 MAX_NUMBER_OF_MODELS_LOADED_AT_ONE_TIME = 1024;
	
	const u64 COMMAND_CAPACITY = 2048;
	const u64 COMMAND_AUX_MEMORY = Megabytes(16); //I dunno lol
	const u64 ALIGNMENT = 16;
	const u32 MAX_BUFFER_LAYOUTS = 32;
	
	const u32 MAX_DEFAULT_MODELS = 16;
	const u32 MAX_NUM_TEXTURES = 2048;
	const u32 SCATTER_TILE_DIM = 64;
	const u32 REDUCE_TILE_DIM = 128;
	const float DILATION_FACTOR = 10.0f / float(r2::draw::light::SHADOW_MAP_SIZE);
	const float EDGE_SOFTENING_AMOUNT = 0.02f;
	const u32 STATIC_MODELS_VERTEX_LAYOUT_SIZE = Megabytes(16);
	const u32 ANIM_MODELS_VERTEX_LAYOUT_SIZE = Megabytes(16);

	const u32 MAX_NUM_CONSTANT_BUFFERS = 16; //?
	const u32 MAX_NUM_CONSTANT_BUFFER_LOCKS = MAX_NUM_DRAWS;

	const u32 MAX_NUM_BONES = MAX_NUM_DRAWS;

	const bool USE_SDSM_SHADOWS = true;

	const u32 PRE_RENDER_STACK_ARENA_SIZE = Megabytes(4);
	const u32 NUM_RENDER_MATERIALS_TO_RENDER = 2048;

#ifdef R2_DEBUG
	const u32 MAX_NUM_DEBUG_DRAW_COMMANDS = MAX_NUM_DRAWS;//Megabytes(4) / sizeof(InternalDebugRenderCommand);
	const u32 MAX_NUM_DEBUG_LINES = MAX_NUM_DRAWS;// Megabytes(8) / (2 * sizeof(DebugVertex));
	const u32 MAX_NUM_DEBUG_MODELS = 5;
	const u32 DEBUG_COMMAND_AUX_MEMORY = Megabytes(4);
#endif

	const std::string MODL_EXT = ".modl";
	const std::string MESH_EXT = ".mesh";
}

namespace r2::draw
{
	void ConstantBufferData::AddDataSize(u64 size)
	{
		R2_CHECK(size <= bufferSize, "We're adding too much to this buffer. We're trying to add: %llu bytes to a %llu sized buffer", size, bufferSize);

		if (currentOffset + size > bufferSize)
		{
			currentOffset = (currentOffset + size) % bufferSize;
		}

		currentOffset += size;
	}
}


namespace r2::draw::cmd
{

#ifdef R2_DEBUG
	u64 FillVertexBufferCommand(FillVertexBuffer* cmd, const r2::SArray<r2::draw::DebugBone>& debugBones, VertexBufferHandle handle, u64 offset)
	{
		if (cmd == nullptr)
		{
			R2_CHECK(false, "cmd is null!");
			return 0;
		}

		const u64 numVertices = r2::sarr::Size(debugBones);

		cmd->vertexBufferHandle = handle;
		cmd->offset = offset;
		cmd->dataSize = sizeof(r2::sarr::At(debugBones, 0)) * numVertices;
		cmd->data = debugBones.mData;

		return cmd->dataSize + offset;
	}

	u64 FillVertexBufferCommand(FillVertexBuffer* cmd, const r2::SArray<r2::draw::DebugVertex>& vertices, VertexBufferHandle handle, u64 offset)
	{
		if (cmd == nullptr)
		{
			R2_CHECK(false, "cmd is null!");
			return 0;
		}

		const u64 numVertices = r2::sarr::Size(vertices);

		cmd->vertexBufferHandle = handle;
		cmd->offset = offset;
		cmd->dataSize = sizeof(r2::sarr::At(vertices, 0)) * numVertices;
		cmd->data = vertices.mData;

		return cmd->dataSize + offset;
	}
#endif

	u64 FillVertexBufferCommand(FillVertexBuffer* cmd, const Mesh& mesh, VertexBufferHandle handle, u64 offset)
	{
		if (cmd == nullptr)
		{
			R2_CHECK(false, "cmd or model is null");
			return 0;
		}

		const u64 numVertices = r2::sarr::Size(*mesh.optrVertices);

		cmd->vertexBufferHandle = handle;
		cmd->offset = offset;
		cmd->dataSize = sizeof(r2::draw::Vertex) * numVertices;
		cmd->data = r2::sarr::Begin(*mesh.optrVertices);

		return cmd->dataSize + offset;
	}

	u64 FillBonesBufferCommand(FillVertexBuffer* cmd, r2::SArray<r2::draw::BoneData>& boneData, VertexBufferHandle handle, u64 offset)
	{
		if (cmd == nullptr)
		{
			R2_CHECK(false, "cmd or model is null");
			return 0;
		}

		const u64 numBoneData = r2::sarr::Size(boneData);
		//for (u64 i = 0; i < numBoneData; ++i)
		//{
		//	const r2::draw::BoneData& d = r2::sarr::At(boneData, i);

		//	printf("vertex: %llu - weights: %f, %f, %f, %f, boneIds: %d, %d, %d, %d\n", i, d.boneWeights.x, d.boneWeights.y, d.boneWeights.z, d.boneWeights.w, d.boneIDs.x, d.boneIDs.y, d.boneIDs.z, d.boneIDs.w);
		//}

		cmd->vertexBufferHandle = handle;
		cmd->offset = offset;
		cmd->dataSize = sizeof(r2::draw::BoneData) * numBoneData;
		cmd->data = r2::sarr::Begin(boneData);

		return cmd->dataSize + offset;
	}

	u64 FillIndexBufferCommand(FillIndexBuffer* cmd, const Mesh& mesh, IndexBufferHandle handle, u64 offset)
	{
		if (cmd == nullptr)
		{
			R2_CHECK(false, "cmd or model is null");
			return  0;
		}

		const u64 numIndices = r2::sarr::Size(*mesh.optrIndices);

		cmd->indexBufferHandle = handle;
		cmd->offset = offset;
		cmd->dataSize = sizeof(r2::sarr::At(*mesh.optrIndices, 0)) * numIndices;
		cmd->data = r2::sarr::Begin(*mesh.optrIndices);

		return cmd->dataSize + offset;
	}

	u64 FillConstantBufferCommand(FillConstantBuffer* cmd, ConstantBufferHandle handle, r2::draw::ConstantBufferLayout::Type type, b32 isPersistent, const void* data, u64 size, u64 offset)
	{
		if (cmd == nullptr)
		{
			R2_CHECK(false, "cmd or model is null");
			return  0;
		}

		char* auxMemory = r2::draw::cmdpkt::GetAuxiliaryMemory<FillConstantBuffer>(cmd);
		memcpy(auxMemory, data, size);

		cmd->constantBufferHandle = handle;
		cmd->data = auxMemory;
		cmd->dataSize = size;
		cmd->offset = offset;
		cmd->type = type;
		cmd->isPersistent = isPersistent;

		return cmd->dataSize + offset;
	}

	u64 ZeroConstantBufferCommand(FillConstantBuffer* cmd, ConstantBufferHandle handle, r2::draw::ConstantBufferLayout::Type type, b32 isPersistent, u64 size, u64 offset)
	{
		if (cmd == nullptr)
		{
			R2_CHECK(false, "cmd or model is null");
			return  0;
		}

		char* auxMemory = r2::draw::cmdpkt::GetAuxiliaryMemory<FillConstantBuffer>(cmd);
		memset(auxMemory, 0, size);

		cmd->constantBufferHandle = handle;
		cmd->data = auxMemory;
		cmd->dataSize = size;
		cmd->offset = offset;
		cmd->type = type;
		cmd->isPersistent = isPersistent;

		return cmd->dataSize + offset;
	}
}

namespace r2::draw
{
	struct ClearSurfaceOptions
	{
		b32 shouldClear = false;
		b32 useNormalClear = true;
		u32 flags = 0;
		u32 numClearParams = 0;
		cmd::ClearBufferParams clearParams[4]={};
	};

	u64 RenderBatch::MemorySize(u32 numModels, u32 numModelRefs, u32 numMeshes, u32 numBoneTransforms, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		u64 totalBytes = 0;

		totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<cmd::DrawState>::MemorySize(numModelRefs), alignment, headerSize, boundsChecking);
		totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<u32>::MemorySize(numModelRefs), alignment, headerSize, boundsChecking);
		totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<vb::GPUModelRef>::MemorySize(numModelRefs), alignment, headerSize, boundsChecking);
		totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<MaterialBatch::Info>::MemorySize(numModelRefs), alignment, headerSize, boundsChecking);
		totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::RenderMaterialParams>::MemorySize(numModelRefs), alignment, headerSize, boundsChecking);
		totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ShaderEffectPasses>::MemorySize(numModelRefs), alignment, headerSize, boundsChecking); 
		totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<MeshRenderData>::MemorySize(numMeshes), alignment, headerSize, boundsChecking);

		if (numModels > 0)
		{
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<glm::mat4>::MemorySize(numModels), alignment, headerSize, boundsChecking);

#ifdef R2_EDITOR
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<u32>::MemorySize(numModels), alignment, headerSize, boundsChecking); //way overestimate
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<EntityInstanceBatchOffset>::MemorySize(numModels), alignment, headerSize, boundsChecking); //way overestimate
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<s32>::MemorySize(numModels), alignment, headerSize, boundsChecking); //kinda wrong since it should be MAX_ENTITIES * MAX_NUM_INSTANCES
#endif

		}

		if (numBoneTransforms > 0)
		{
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<b32>::MemorySize(numModelRefs), alignment, headerSize, boundsChecking);
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ShaderBoneTransform>::MemorySize(numBoneTransforms), alignment, headerSize, boundsChecking);
		}

		return totalBytes;
	}

#ifdef R2_DEBUG
	u64 DebugRenderBatch::MemorySize(u32 maxDraws, bool isDebugLines, u32 numShaders, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		u64 totalBytes =
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<glm::vec4>::MemorySize(maxDraws), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<DrawFlags>::MemorySize(maxDraws), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<u32>::MemorySize(maxDraws), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<DebugRenderConstants>::MemorySize(maxDraws), alignment, headerSize, boundsChecking);

		if (isDebugLines)
		{

			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<DebugVertex>::MemorySize(maxDraws) * 2, alignment, headerSize, boundsChecking); //*2 because we need double the memory for debug vertices
		}
		else
		{
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<DebugModelType>::MemorySize(maxDraws), alignment, headerSize, boundsChecking);
			
		}

		if (numShaders > 0)
		{
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ShaderHandle>::MemorySize(numShaders), alignment, headerSize, boundsChecking);
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<flat::eMeshPass>::MemorySize(numShaders), alignment, headerSize, boundsChecking);
		}
		
		return totalBytes;
	}
#endif
}

namespace
{
	u64 DefaultModelsMemorySize()
	{
		return Kilobytes(650);
//		u32 boundsChecking = 0;
//#ifdef R2_DEBUG
//		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
//#endif
//		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();
//
//		u64 quadMeshSize = r2::draw::Mesh::MemorySize(4, 6, ALIGNMENT, headerSize, boundsChecking);
//		u64 cubeMeshSize = r2::draw::Mesh::MemorySize(24, 36, ALIGNMENT, headerSize, boundsChecking);
//		u64 sphereMeshSize = r2::draw::Mesh::MemorySize(1089, 5952, ALIGNMENT, headerSize, boundsChecking);
//		u64 coneMeshSize = r2::draw::Mesh::MemorySize(148, 144 * 3, ALIGNMENT, headerSize, boundsChecking);
//		u64 cylinderMeshSize = r2::draw::Mesh::MemorySize(148, 144 * 3, ALIGNMENT, headerSize, boundsChecking);
//		u64 fullScreenMeshSize = r2::draw::Mesh::MemorySize(3, 3, ALIGNMENT, headerSize, boundsChecking);
//
//		u64 modelSize = r2::draw::Model::ModelMemorySize(1, 1, ALIGNMENT, headerSize, boundsChecking) * r2::draw::NUM_DEFAULT_MODELS;
//
//		return quadMeshSize + cubeMeshSize + sphereMeshSize + coneMeshSize + cylinderMeshSize + modelSize;
	}

	bool LoadEngineModels(r2::draw::Renderer& renderer)
	{
		if (renderer.mModelCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return false;
		}
		/*
		QUAD = 0,
		CUBE,
		SPHERE,
		CONE,
		CYLINDER,
		FULLSCREEN_TRIANGLE,
		SKYBOX,
		
		*/

		r2::SArray<r2::asset::Asset>* defaultModels = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::asset::Asset, MAX_DEFAULT_MODELS); 

		r2::sarr::Push(*defaultModels, r2::asset::Asset("models/quad.modl", r2::asset::MODEL));
		r2::sarr::Push(*defaultModels, r2::asset::Asset("models/cube.modl", r2::asset::MODEL));
		r2::sarr::Push(*defaultModels, r2::asset::Asset("models/sphere.modl", r2::asset::MODEL));
		r2::sarr::Push(*defaultModels, r2::asset::Asset("models/cone.modl", r2::asset::MODEL));
		r2::sarr::Push(*defaultModels, r2::asset::Asset("models/cylinder.modl", r2::asset::MODEL));
		
		r2::sarr::Push(*defaultModels, r2::asset::Asset("models/fullscreentriangle.modl", r2::asset::MODEL));
	//	r2::sarr::Push(*defaultModels, r2::asset::Asset("models/skybox.modl", r2::asset::MODEL));
		

		r2::draw::modlche::LoadModels(renderer.mModelCache, *defaultModels, *renderer.mDefaultModelHandles);

		FREE(defaultModels, *MEM_ENG_SCRATCH_PTR);

		return true;
	}
}

namespace r2::draw::renderer
{

	//Setup code
	void SetClearColor(const glm::vec4& color);

	const Model* GetDefaultModel(Renderer& renderer, r2::draw::DefaultModel defaultModel);
	const r2::SArray<vb::GPUModelRefHandle>* GetDefaultModelRefs(Renderer& renderer);
	vb::GPUModelRefHandle GetDefaultModelRef(Renderer& renderer, r2::draw::DefaultModel defaultModel);
	vb::GPUModelRefHandle GetModelRefHandleForModelAssetName(Renderer& renderer, const r2::asset::AssetName& modelAssetName);
	const RenderMaterialParams& GetMissingTextureRenderMaterialParam(Renderer& renderer);
	const tex::Texture* GetMissingTexture(Renderer* renderer);

	const RenderMaterialParams& GetBlueNoise64TextureMaterialParam(Renderer& renderer);
	const tex::Texture* GetBlueNoise64Texture(Renderer* renderer);

	vb::GPUModelRefHandle UploadModel(Renderer& renderer, const Model* model);
	void UploadModels(Renderer& renderer, const r2::SArray<const Model*>& models, r2::SArray<vb::GPUModelRefHandle>& modelRefs);

	//vb::GPUModelRefHandle UploadAnimModel(Renderer& renderer, const AnimModel* model);
	//void UploadAnimModels(Renderer& renderer, const r2::SArray<const AnimModel*>& models, r2::SArray<vb::GPUModelRefHandle>& modelRefs);

	void UnloadModel(Renderer& renderer, const vb::GPUModelRefHandle& modelRefHandle);
	void UnloadStaticModelRefHandles(Renderer& renderer, const r2::SArray<vb::GPUModelRefHandle>* handles);
	void UnloadAnimModelRefHandles(Renderer& renderer, const r2::SArray<vb::GPUModelRefHandle>* handles);
	void UnloadAllStaticModels(Renderer& renderer);
	void UnloadAllAnimModels(Renderer& renderer);

	bool IsModelLoaded(const Renderer& renderer, const vb::GPUModelRefHandle& modelRefHandle);
	bool IsModelRefHandleValid(const Renderer& renderer, const vb::GPUModelRefHandle& modelRefHandle);

	//void GetDefaultModelMaterials(Renderer& renderer, r2::SArray<r2::draw::MaterialHandle>& defaultModelMaterials);
	//r2::draw::MaterialHandle GetMaterialHandleForDefaultModel(Renderer& renderer, r2::draw::DefaultModel defaultModel);

	//r2::draw::ShaderHandle GetDefaultOutlineShaderHandle(Renderer& renderer, bool isStatic);
	//const r2::draw::RenderMaterialParams& GetDefaultOutlineRenderMaterialParams(Renderer& renderer, bool isStatic);

	r2::mat::MaterialName GetDefaultOutlineMaterialName(Renderer& renderer, bool alphaDiscard);

	void SetDefaultTransparencyBlendState(BlendState& blendState);
	void SetDefaultTransparentCompositeBlendState(BlendState& blendState);

	void UpdateShaderMatrices(Renderer& renderer);
	void UpdateShaderVectors(Renderer& renderer);

	void UpdateSceneLighting(Renderer& renderer, const r2::draw::LightSystem& lightSystem);
	void UpdateCamera(Renderer& renderer, Camera& camera);

	void ClearShadowData(Renderer& renderer);
	void UpdateShadowMapPages(Renderer& renderer);

	void ClearAllShadowMapPages(Renderer& renderer);
	void AssignShadowMapPagesForAllLights(Renderer& renderer);
	void AssignShadowMapPagesForSpotLight(Renderer& renderer, const SpotLight& spotLight);
	void RemoveShadowMapPagesForSpotLight(Renderer& renderer, const SpotLight& spotLight);
	void AssignShadowMapPagesForPointLight(Renderer& renderer, const PointLight& pointLight);
	void RemoveShadowMapPagesForPointLight(Renderer& renderer, const PointLight& pointLight);
	void AssignShadowMapPagesForDirectionLight(Renderer& renderer, const DirectionLight& directionLight);
	void RemoveShadowMapPagesForDirectionLight(Renderer& renderer, const DirectionLight& directionLight);

	void UpdateSDSMShaderParams(Renderer& renderer);

	//Clusters
	void ClearActiveClusters(Renderer& renderer);
	void UpdateClusters(Renderer& renderer);

	//SSR
	void UpdateSSRDataIfNeeded(Renderer& renderer);

	//Bloom
	void BloomRenderPass(Renderer& renderer);

	void UpdateBloomDataIfNeeded(Renderer& renderer);


	//FXAA
	void FXAARenderPass(Renderer& renderer, ConstantBufferHandle subCommandsConstantBufferHandle, BufferLayoutHandle bufferLayoutHandle, u32 numSubCommands, u32 startCommandIndex);
	void UpdateFXAADataIfNeeded(Renderer& renderer);

	//SMAA
	void SMAAx1RenderPass(Renderer& renderer, ConstantBufferHandle subCommandsConstantBufferHandle, BufferLayoutHandle bufferLayoutHandle, u32 numSubCommands, u32 startCommandIndex, ShaderHandle neighborhoodBlendingShader, u32 pass);
	void SMAAx1OutputRenderPass(Renderer& renderer, ConstantBufferHandle subCommandsConstantBufferHandle, BufferLayoutHandle bufferLayoutHandle, u32 numSubCommands, u32 startCommandIndex, ShaderHandle outputShader, u32 pass);
	void SMAAxT2SOutputRenderPass(Renderer& renderer, ConstantBufferHandle subCommandsConstantBufferHandle, BufferLayoutHandle bufferLayoutHandle, u32 numSubCommands, u32 startCommandIndex, ShaderHandle outputShader, u32 pass);
	void UpdateSMAADataIfNeeded(Renderer& renderer);
	glm::vec2 SMAAGetJitter(const Renderer& renderer, u64 frameIndex);
	glm::ivec4 SMAASubSampleIndices(const Renderer& renderer, u64 frameIndex);

	//NON AA
	void NonAARenderPass(Renderer& renderer, ConstantBufferHandle subCommandsConstantBufferHandle, BufferLayoutHandle bufferLayoutHandle, u32 numSubCommands, u32 startCommandIndex);

	void CheckIfValidShader(Renderer& renderer, ShaderHandle shader, const char* name);

	void SetOutputMergerType(Renderer& renderer, OutputMerger outputMerger);

	//Color Correction
	void SetColorCorrection(Renderer& renderer, const ColorCorrection& cc);
	void UpdateColorCorrectionIfNeeded(Renderer& renderer);

	void SetColorGradingLUT(Renderer& renderer, const tex::TextureAddress& lut, f32 halfColX, f32 halfColY, f32 numSwatches);
	void EnableColorGrading(Renderer& renderer, bool isEnabled);
	bool IsColorGradingEnabled(Renderer& renderer);
	void SetColorGradingContribution(Renderer& renderer, float contribution);
	float GetColorGradingContribution(Renderer& renderer);

	//Camera and Lighting
	void SetRenderCamera(Renderer& renderer, Camera* cameraPtr);
	Camera* GetRenderCamera(Renderer& renderer);
#ifdef R2_EDITOR
	r2::asset::FileList GetModelFiles(Renderer& renderer);
	const r2::draw::Model* GetDefaultModel(Renderer& renderer, const r2::asset::AssetName& assetName);
	EntityInstance ReadEntityInstanceAtMousePosition(Renderer& renderer, s32 x, s32 y);
#endif

	DirectionLightHandle AddDirectionLight(Renderer& renderer, const DirectionLight& light);
	PointLightHandle AddPointLight(Renderer& renderer, const PointLight& pointLight);
	SpotLightHandle AddSpotLight(Renderer& renderer, const SpotLight& spotLight);
	SkyLightHandle AddSkyLight(Renderer& renderer, const SkyLight& skylight, s32 numPrefilteredMips);
	SkyLightHandle AddSkyLight(Renderer& renderer, const RenderMaterialParams& diffuseMaterial, const RenderMaterialParams& prefilteredMaterial, const RenderMaterialParams& lutDFG, s32 numMips);

	const DirectionLight* GetDirectionLightConstPtr(Renderer& renderer, DirectionLightHandle dirLightHandle);
	DirectionLight* GetDirectionLightPtr(Renderer& renderer, DirectionLightHandle dirLightHandle);

	const PointLight* GetPointLightConstPtr(Renderer& renderer, PointLightHandle pointLightHandle);
	PointLight* GetPointLightPtr(Renderer& renderer, PointLightHandle pointLightHandle);

	const SpotLight* GetSpotLightConstPtr(Renderer& renderer, SpotLightHandle spotLightHandle);
	SpotLight* GetSpotLightPtr(Renderer& renderer, SpotLightHandle spotLightHandle);

	const SkyLight* GetSkyLightConstPtr(Renderer& renderer, SkyLightHandle skyLightHandle);
	SkyLight* GetSkyLightPtr(Renderer& renderer, SkyLightHandle skyLightHandle);

	//@TODO(Serge): add the get light properties functions here

	void RemoveDirectionLight(Renderer& renderer, DirectionLightHandle dirLightHandle);
	void RemovePointLight(Renderer& renderer, PointLightHandle pointLightHandle);
	void RemoveSpotLight(Renderer& renderer, SpotLightHandle spotLightHandle);
	void RemoveSkyLight(Renderer& renderer, SkyLightHandle skylightHandle);
	void ClearAllLighting(Renderer& renderer);

	void UpdateLighting(Renderer& renderer);
	void UploadAllSurfaces(Renderer& renderer);

	void DrawModel(
		Renderer& renderer,
		u32 entityID,
		const r2::SArray<s32>& entityInstances,
		const DrawParameters& drawParameters,
		const vb::GPUModelRefHandle& modelRefHandles,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<r2::mat::MaterialName>& materialNames,
		const r2::SArray<ShaderBoneTransform>* boneTransforms);


	void DrawModels(
		Renderer& renderer,

		const r2::SArray<u32>& entities,
		const r2::SArray<s32>& entityInstances,
		const r2::SArray<u32>& numEntityInstancesPerEntity,

		const DrawParameters& drawParameters,
		const r2::SArray<vb::GPUModelRefHandle>& modelRefHandles,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<u32>& numInstancesPerModel,

		const r2::SArray<r2::mat::MaterialName>& materialNames,

		const r2::SArray<ShaderBoneTransform>* boneTransforms);

	///More draw functions...


	ShaderHandle GetShadowDepthShaderHandle(const Renderer& renderer, bool isDynamic, light::LightType lightType);
	ShaderHandle GetDepthShaderHandle(const Renderer& renderer, bool isDynamic);


	vb::VertexBufferLayoutSize GetStaticVertexBufferRemainingSize(const Renderer& renderer);
	vb::VertexBufferLayoutSize GetAnimVertexBufferRemainingSize(const Renderer& renderer);

	vb::VertexBufferLayoutSize GetStaticVertexBufferSize(const Renderer& renderer);
	vb::VertexBufferLayoutSize GetAnimVertexBufferSize(const Renderer& renderer);

	vb::VertexBufferLayoutSize GetStaticVertexBufferCapacity(const Renderer& renderer);
	vb::VertexBufferLayoutSize GetAnimVertexBufferCapacity(const Renderer& renderer);
	const vb::GPUModelRef* GetGPUModelRef(const Renderer& renderer, const vb::GPUModelRefHandle& handle);
	//------------------------------------------------------------------------------

#ifdef R2_DEBUG

	void DrawDebugBones(Renderer& renderer, const r2::SArray<DebugBone>& bones, const glm::mat4& modelMatrix, const glm::vec4& color);

	void DrawDebugBones(
		Renderer& renderer,
		const r2::SArray<DebugBone>& bones,
		const r2::SArray<u64>& numBonesPerModel,
		const r2::SArray<glm::mat4>& numModelMats,
		const glm::vec4& color);

	void DrawQuad(Renderer& renderer, const glm::vec3& center, float scale, const glm::vec3& normal,const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawSphere(Renderer& renderer, const glm::vec3& center, float radius, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawCube(Renderer& renderer, const glm::vec3& center, float scale, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawCylinder(Renderer& renderer, const glm::vec3& basePosition, const glm::vec3& dir, float radius, float height, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawCone(Renderer& renderer, const glm::vec3& basePosition, const glm::vec3& dir, float radius, float height, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawArrow(Renderer& renderer, const glm::vec3& basePosition, const glm::vec3& dir, float length, float headBaseRadius, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawLine(Renderer& renderer, const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, bool disableDepth);
	void DrawLine(Renderer& renderer, const glm::vec3& p0, const glm::vec3& p1, const glm::mat4& modelMat, const glm::vec4& color, bool depthTest);

	void DrawTangentVectors(Renderer& renderer, DefaultModel model, const glm::mat4& transform);

	void DrawQuadInstanced(Renderer& renderer, const r2::SArray<glm::vec3>& centers, const r2::SArray<glm::vec2>& scales, const r2::SArray<glm::vec3>& normals, const r2::SArray<glm::vec4>& colors, bool filled, bool depthTest);

	void DrawQuad(Renderer& renderer, const glm::mat4& mat, flat::eMeshPass meshPass, ShaderHandle shader, bool depthTest);

	void DrawSphereInstanced(
		Renderer& renderer,
		const r2::SArray<glm::vec3>& centers,
		const r2::SArray<float>& radii,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest);

	void DrawCubeInstanced(
		Renderer& renderer,
		const r2::SArray<glm::vec3>& centers,
		const r2::SArray<float>& scales,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest);

	void DrawCylinderInstanced(
		Renderer& renderer,
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& radii,
		const r2::SArray<float>& heights,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest);

	void DrawConeInstanced(
		Renderer& renderer,
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& radii,
		const r2::SArray<float>& heights,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest);

	void DrawArrowInstanced(
		Renderer& renderer,
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& lengths,
		const r2::SArray<float>& headBaseRadii,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest);

#endif


	ConstantBufferData* GetConstData(Renderer& renderer, ConstantBufferHandle handle);
	ConstantBufferData* GetConstDataByConfigHandle(Renderer& renderer, ConstantConfigHandle handle);

	const r2::SArray<r2::draw::ConstantBufferHandle>* GetConstantBufferHandles(Renderer& renderer);

	void UploadEngineModels(Renderer& renderer);

	bool GenerateLayouts(Renderer& renderer);

	vb::VertexBufferLayoutHandle AddStaticModelLayout(Renderer& renderer, const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize);
	vb::VertexBufferLayoutHandle AddAnimatedModelLayout(Renderer& renderer, const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize);

	ConstantConfigHandle AddConstantBufferLayout(Renderer& renderer, ConstantBufferLayout::Type type, const std::initializer_list<ConstantBufferElement>& elements);
	ConstantConfigHandle AddModelsLayout(Renderer& renderer, ConstantBufferLayout::Type type);
	ConstantConfigHandle AddMaterialLayout(Renderer& renderer);
	ConstantConfigHandle AddSubCommandsLayout(Renderer& renderer); //we can use this for the debug 
	ConstantConfigHandle AddBoneTransformsLayout(Renderer& renderer);
	ConstantConfigHandle AddBoneTransformOffsetsLayout(Renderer& renderer);
	ConstantConfigHandle AddLightingLayout(Renderer& renderer);
	ConstantConfigHandle AddSurfacesLayout(Renderer& renderer);
	ConstantConfigHandle AddShadowDataLayout(Renderer& renderer);
	ConstantConfigHandle AddMaterialOffsetsLayout(Renderer& renderer);
	ConstantConfigHandle AddClusterVolumesLayout(Renderer& renderer);
	ConstantConfigHandle AddDispatchComputeIndirectLayout(Renderer& renderer);
	ConstantConfigHandle AddMeshDataLayout(Renderer& renderer);

	void InitializeVertexLayouts(Renderer& renderer, u32 staticVertexLayoutSizeInBytes, u32 animVertexLayoutSizeInBytes);
	bool GenerateConstantBuffers(Renderer& renderer, const r2::SArray<ConstantBufferLayoutConfiguration>* constantBufferConfigs);

	template <class T>
	void BeginRenderPass(Renderer& renderer, RenderPassType renderPassType, const ClearSurfaceOptions& clearOptions, r2::draw::CommandBucket<T>& commandBucket, T key, mem::StackArena& arena);

	template <typename T>
	void EndRenderPass(Renderer& renderer, RenderPassType renderPass, r2::draw::CommandBucket<T>& commandBucket);

	void SetupRenderTargetParams(rt::RenderTargetParams renderTargetParams[NUM_RENDER_TARGET_SURFACES]);
	void SwapRenderTargetsHistoryIfNecessary(Renderer& renderer);
	void UploadSwappingSurfaces(Renderer& renderer);

	//void UpdateRenderTargetsIfNecessary(Renderer& renderer);
	u32 GetRenderPassTargetOffset(Renderer& renderer, RenderTargetSurface surface);


	template<class ARENA, typename T>
	r2::draw::cmd::FillConstantBuffer* AddFillConstantBufferCommand(ARENA& arena, CommandBucket<T>& bucket, T key, u64 auxMemory);


	u64 AddFillConstantBufferCommandForData(Renderer& renderer, ConstantBufferHandle handle, u64 elementIndex, const void* data);
	u64 AddZeroConstantBufferCommand(Renderer& renderer, ConstantBufferHandle handle, u64 elementIndex);
	void AddFillConstantBufferCommandFull(Renderer& renderer, ConstantBufferHandle handle, const void* data, u64 size, u64 offset);

	RenderTarget* GetRenderTarget(Renderer& renderer, RenderTargetSurface surface);
	RenderPass* GetRenderPass(Renderer& renderer, RenderPassType renderPassType);

	void CreateRenderPasses(Renderer& renderer);
	void DestroyRenderPasses(Renderer& renderer);

	void ResizeRenderSurface(Renderer& renderer, u32 windowWidth, u32 windowHeight, u32 resolutionX, u32 resolutionY, float scaleX, float scaleY, float xOffset, float yOffset);
	
	void CreateShadowRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateZPrePassRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateZPrePassShadowsRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateAmbientOcclusionSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateAmbientOcclusionDenoiseSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateAmbientOcclusionTemporalDenoiseSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreatePointLightShadowRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateSSRRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateConeTracedSSRRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateBloomSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY, u32 numMips);
	void CreateBloomBlurSurface(Renderer& renderer, u32 resolutionX , u32 resolutionY , u32 numMips);
	void CreateBloomSurfaceUpSampled(Renderer& renderer, u32 resolutionX, u32 resolutionY, u32 numMips);
	void CreateCompositeSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateSMAAEdgeDetectionSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateSMAABlendingWeightSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateSMAANeighborhoodBlendingSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	void CreateTransparentAccumSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
	
#ifdef R2_EDITOR
	void CreateEditorPickingSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY);
#endif

	void DestroyRenderSurfaces(Renderer& renderer);

	void PreRender(Renderer& renderer);

	void ClearRenderBatches(Renderer& renderer);

	glm::vec2 GetJitter(Renderer& renderer, const u64 frameCount, bool isTAA);

	u32 GetCameraDepth(Renderer& renderer, const r2::draw::Bounds& meshBounds, const glm::mat4& modelMat);
	f32 GetSortKeyCameraDepth(Renderer& renderer, glm::vec3 worldAABBPosition);

#ifdef R2_DEBUG
	void DebugPreRender(Renderer& renderer);
	void ClearDebugRenderData(Renderer& renderer);

	vb::VertexBufferLayoutHandle AddDebugDrawLayout(Renderer& renderer);
#endif

	//@NOTE(Serge): these always need to happen right now to line with the shader layouts
	ConstantConfigHandle AddDebugLineSubCommandsLayout(Renderer& renderer);
	ConstantConfigHandle AddDebugModelSubCommandsLayout(Renderer& renderer);
	ConstantConfigHandle AddDebugColorsLayout(Renderer& renderer);

	//basic stuff
	Renderer* CreateRenderer(RendererBackend backendType, r2::mem::MemoryArea::Handle memoryAreaHandle)
	{
		R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "The memoryAreaHandle passed in is invalid!");

		if (memoryAreaHandle == r2::mem::MemoryArea::Invalid)
		{
			return false;
		}

		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u32 boundsChecking = 0;
#if defined(R2_DEBUG) || defined(R2_RELEASE)
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();

		rt::RenderTargetParams renderTargetParams[NUM_RENDER_TARGET_SURFACES];
		SetupRenderTargetParams(renderTargetParams);

		u64 renderTargetArenaSize = 0;

		for (int i = 0; i < NUM_RENDER_TARGET_SURFACES; ++i)
		{
			renderTargetArenaSize += RenderTarget::MemorySize(renderTargetParams[i], ALIGNMENT, stackHeaderSize, boundsChecking);
		}

		u64 subAreaSize = MemorySize(renderTargetArenaSize);

		if (memoryArea->UnAllocatedSpace() < subAreaSize)
		{
			R2_CHECK(false, "We don't have enough space to allocate the renderer! Needed amount of memory is: %llu, amount we have is: %llu, difference needed: %llu", subAreaSize, memoryArea->UnAllocatedSpace(), subAreaSize - memoryArea->UnAllocatedSpace());
			return false;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		if ((subAreaHandle = memoryArea->AddSubArea(subAreaSize, "Renderer")) == r2::mem::MemoryArea::SubArea::Invalid)
		{
			R2_CHECK(false, "We couldn't create a sub area for the renderer");
			return false;
		}

		//emplace the linear arena
		r2::mem::LinearArena* rendererArena = EMPLACE_LINEAR_ARENA(*memoryArea->GetSubArea(subAreaHandle));

		R2_CHECK(rendererArena != nullptr, "We couldn't emplace the linear arena - no way to recover!");

		Renderer* newRenderer = ALLOC(r2::draw::Renderer, *rendererArena);

		R2_CHECK(newRenderer != nullptr, "We couldn't allocate s_optrRenderer!");

		newRenderer->mBackendType = backendType;


		newRenderer->mMemoryAreaHandle = memoryAreaHandle;
		newRenderer->mSubAreaHandle = subAreaHandle;
		newRenderer->mSubAreaArena = rendererArena;

		newRenderer->mVertexBufferLayoutHandles = MAKE_SARRAY(*rendererArena, vb::VertexBufferLayoutHandle, NUM_VERTEX_BUFFER_LAYOUT_TYPES);

		R2_CHECK(newRenderer->mVertexBufferLayoutHandles != nullptr, "We couldn't create the newRenderer->mVertexBufferLayoutHandles array");

		newRenderer->mConstantBufferHandles = MAKE_SARRAY(*rendererArena, r2::draw::ConstantBufferHandle, MAX_BUFFER_LAYOUTS);
		
		R2_CHECK(newRenderer->mConstantBufferHandles != nullptr, "We couldn't create the constant buffer handles");

		newRenderer->mConstantBufferData = MAKE_SHASHMAP(*rendererArena, ConstantBufferData, MAX_BUFFER_LAYOUTS* r2::SHashMap<ConstantBufferData>::LoadFactorMultiplier());

		R2_CHECK(newRenderer->mConstantBufferData != nullptr, "We couldn't create the constant buffer data!");

		newRenderer->mDefaultModelHandles = MAKE_SARRAY(*rendererArena, ModelHandle, MAX_DEFAULT_MODELS);

		R2_CHECK(newRenderer->mDefaultModelHandles != nullptr, "We couldn't create the default model handles");

		newRenderer->mConstantLayouts = MAKE_SARRAY(*rendererArena, r2::draw::ConstantBufferLayoutConfiguration, MAX_BUFFER_LAYOUTS);

		R2_CHECK(newRenderer->mConstantLayouts != nullptr, "We couldn't create the constant layouts!");

		newRenderer->mEngineModelRefs = MAKE_SARRAY(*rendererArena, vb::GPUModelRefHandle, NUM_DEFAULT_MODELS);

		R2_CHECK(newRenderer->mEngineModelRefs != nullptr, "We couldn't create the engine model refs");

#ifdef R2_DEBUG

		newRenderer->mPreDebugCommandBucket = MAKE_CMD_BUCKET(*rendererArena, key::DebugKey, key::DecodeDebugKey, COMMAND_CAPACITY);

		newRenderer->mPostDebugCommandBucket = MAKE_CMD_BUCKET(*rendererArena, key::DebugKey, key::DecodeDebugKey, COMMAND_CAPACITY);

		newRenderer->mDebugCommandBucket = MAKE_CMD_BUCKET(*rendererArena, key::DebugKey, key::DecodeDebugKey, COMMAND_CAPACITY);

		newRenderer->mDebugCommandArena = MAKE_STACK_ARENA(*rendererArena, COMMAND_CAPACITY * cmd::LargestCommand() + DEBUG_COMMAND_AUX_MEMORY);
		newRenderer->mDebugRenderBatches = MAKE_SARRAY(*rendererArena, DebugRenderBatch, NUM_DEBUG_DRAW_TYPES);

		for (s32 i = 0; i < NUM_DEBUG_DRAW_TYPES; ++i)
		{
			DebugRenderBatch debugRenderBatch;

			debugRenderBatch.debugDrawType = (DebugDrawType)i;
			debugRenderBatch.vertexBufferLayoutHandle = vb::InvalidVertexBufferLayoutHandle;
		
			
			
		//	debugRenderBatch.colors = MAKE_SARRAY(*rendererArena, glm::vec4, MAX_NUM_DRAWS);
			debugRenderBatch.debugRenderConstants = MAKE_SARRAY(*rendererArena, DebugRenderConstants, MAX_NUM_DRAWS);

			debugRenderBatch.drawFlags = MAKE_SARRAY(*rendererArena, DrawFlags, MAX_NUM_DRAWS);
			debugRenderBatch.numInstances = MAKE_SARRAY(*rendererArena, u32, MAX_NUM_DRAWS);

			if (debugRenderBatch.debugDrawType == DDT_LINES)
			{
				debugRenderBatch.vertices = MAKE_SARRAY(*rendererArena, DebugVertex, MAX_NUM_DRAWS * 2);	
			}
			else if (debugRenderBatch.debugDrawType == DDT_MODELS_GENERIC)
			{
				debugRenderBatch.debugModelTypesToDraw = MAKE_SARRAY(*rendererArena, DebugModelType, MAX_NUM_DRAWS);
			}
			else
			{
				R2_CHECK(false, "Currently unsupported!");
			}

			debugRenderBatch.shaderHandles = MAKE_SARRAY(*rendererArena, r2::draw::ShaderHandle, MAX_NUM_DRAWS);
			debugRenderBatch.meshPasses = MAKE_SARRAY(*rendererArena, flat::eMeshPass, MAX_NUM_DRAWS);

			r2::sarr::Push(*newRenderer->mDebugRenderBatches, debugRenderBatch);
		}	

		newRenderer->linesBatchRenderOffsets = MAKE_SARRAY(*rendererArena, BatchRenderOffsets, MAX_NUM_DRAWS);

		newRenderer->linesDrawBatchSubCommands = MAKE_SARRAY(*rendererArena, cmd::DrawDebugBatchSubCommand, MAX_NUM_DRAWS);

		newRenderer->genericModelBatchRenderOffsets = MAKE_SARRAY(*rendererArena, BatchRenderOffsets, MAX_NUM_DRAWS);

		newRenderer->modelDrawBatchSubCommands = MAKE_SARRAY(*rendererArena, cmd::DrawBatchSubCommand, MAX_NUM_DRAWS);
		newRenderer->mDebugVertices = MAKE_SARRAY(*rendererArena, DebugVertex, MAX_NUM_DRAWS * 2);
#endif

		//@TODO(Serge): use backendType?
		bool rendererImpl = r2::draw::rendererimpl::RendererImplInit(memoryAreaHandle, MAX_NUM_CONSTANT_BUFFERS, MAX_NUM_DRAWS, "RendererImpl");
		if (!rendererImpl)
		{
			R2_CHECK(false, "We couldn't initialize the renderer implementation");
			return false;
		}

		SetClearDepth(1.0f);

		bool textureSystemInitialized = r2::draw::texsys::Init(memoryAreaHandle, MAX_NUM_TEXTURES, nullptr, "Texture System");
		if (!textureSystemInitialized)
		{
			R2_CHECK(false, "We couldn't initialize the texture system");
		
			return false;
		}

		newRenderer->mVertexBufferLayoutSystem = r2::draw::vb::CreateVertexBufferLayoutSystem(memoryAreaHandle, NUM_VERTEX_BUFFER_LAYOUT_TYPES, MAX_NUMBER_OF_MODELS_LOADED_AT_ONE_TIME, AVG_NUM_OF_MESHES_PER_MODEL);
		if (!newRenderer->mVertexBufferLayoutSystem)
		{
			R2_CHECK(false, "We couldn't initialize the vertex buffer");
			return false;
		}

		newRenderer->mRenderMaterialCache = r2::draw::rmat::Create(memoryAreaHandle, NUM_RENDER_MATERIALS_TO_RENDER, "RenderMaterials");

		if (!newRenderer->mRenderMaterialCache)
		{
			R2_CHECK(false, "We couldn't initialize the render material cache");
			return false;
		}

		newRenderer->mLightSystem = lightsys::CreateLightSystem(*newRenderer->mSubAreaArena);

#ifdef R2_DEBUG
		newRenderer->mDebugLinesShaderHandle = shadersystem::FindShaderHandle(STRING_ID("Debug"));
		newRenderer->mDebugModelShaderHandle = shadersystem::FindShaderHandle(STRING_ID("DebugModel"));
		newRenderer->mDebugTransparentLineShaderHandle = shadersystem::FindShaderHandle(STRING_ID("TransparentDebug"));
		newRenderer->mDebugTransparentModelShaderHandle = shadersystem::FindShaderHandle(STRING_ID("TransparentDebugModel"));
#endif
		newRenderer->mFinalCompositeShaderHandle = shadersystem::FindShaderHandle(STRING_ID("Screen"));
		//newRenderer->mDefaultStaticOutlineShaderHandle = shadersystem::FindShaderHandle(STRING_ID("StaticOutline"));
		//newRenderer->mDefaultDynamicOutlineShaderHandle = shadersystem::FindShaderHandle(STRING_ID("DynamicOutline"));



		//Get the depth shader handles
		newRenderer->mShadowDepthShaders[0] = shadersystem::FindShaderHandle(STRING_ID("StaticShadowDepth"));

		newRenderer->mShadowDepthShaders[1] = shadersystem::FindShaderHandle(STRING_ID("DynamicShadowDepth"));
		CheckIfValidShader(*newRenderer, newRenderer->mShadowDepthShaders[1], "DynamicShadowDepth");

		newRenderer->mSpotLightShadowShaders[0] = shadersystem::FindShaderHandle(STRING_ID("SpotLightStaticShadowDepth"));
		CheckIfValidShader(*newRenderer, newRenderer->mSpotLightShadowShaders[0], "SpotLightStaticShadowDepth");

		newRenderer->mSpotLightShadowShaders[1] = shadersystem::FindShaderHandle(STRING_ID("SpotLightDynamicShadowDepth"));
		CheckIfValidShader(*newRenderer, newRenderer->mSpotLightShadowShaders[1], "SpotLightDynamicShadowDepth");

		newRenderer->mSpotLightLightMatrixShader = shadersystem::FindShaderHandle(STRING_ID("SpotLightLightMatrices"));
		CheckIfValidShader(*newRenderer, newRenderer->mSpotLightLightMatrixShader, "SpotLightLightMatrices");


		newRenderer->mPointLightShadowShaders[0] = shadersystem::FindShaderHandle(STRING_ID("PointLightStaticShadowDepth"));
		CheckIfValidShader(*newRenderer, newRenderer->mPointLightShadowShaders[0], "PointLightStaticShadowDepth");

		newRenderer->mPointLightShadowShaders[1] = shadersystem::FindShaderHandle(STRING_ID("PointLightDynamicShadowDepth"));
		CheckIfValidShader(*newRenderer, newRenderer->mPointLightShadowShaders[1], "PointLightDynamicShadowDepth");

		newRenderer->mPointLightLightMatrixShader = shadersystem::FindShaderHandle(STRING_ID("PointLightLightMatrices"));
		CheckIfValidShader(*newRenderer, newRenderer->mPointLightLightMatrixShader, "PointLightLightMatrices");


		newRenderer->mDepthShaders[0] = shadersystem::FindShaderHandle(STRING_ID("StaticDepth"));
		CheckIfValidShader(*newRenderer, newRenderer->mDepthShaders[0], "StaticDepth");

		newRenderer->mDepthShaders[1] = shadersystem::FindShaderHandle(STRING_ID("DynamicDepth"));
		CheckIfValidShader(*newRenderer, newRenderer->mDepthShaders[1], "DynamicDepth");

		newRenderer->mShadowSplitComputeShader = shadersystem::FindShaderHandle(STRING_ID("CalculateCascades"));
		CheckIfValidShader(*newRenderer, newRenderer->mShadowSplitComputeShader, "CalculateCascades");

		newRenderer->mSDSMReduceZBoundsComputeShader = shadersystem::FindShaderHandle(STRING_ID("ReduceZBounds"));
		CheckIfValidShader(*newRenderer, newRenderer->mSDSMReduceZBoundsComputeShader, "ReduceZBounds");

		newRenderer->mSDSMCalculateLogPartitionsComputeShader = shadersystem::FindShaderHandle(STRING_ID("CalculateLogPartitions"));
		CheckIfValidShader(*newRenderer, newRenderer->mSDSMCalculateLogPartitionsComputeShader, "CalculateLogPartitions");

		newRenderer->mAmbientOcclusionShader = shadersystem::FindShaderHandle(STRING_ID("GroundTruthAO"));
		CheckIfValidShader(*newRenderer, newRenderer->mAmbientOcclusionShader, "GroundTruthAO");

		newRenderer->mDenoiseShader = shadersystem::FindShaderHandle(STRING_ID("Denoise"));
		CheckIfValidShader(*newRenderer, newRenderer->mDenoiseShader, "Denoise");

		newRenderer->mAmbientOcclusionTemporalDenoiseShader = shadersystem::FindShaderHandle(STRING_ID("GTAOTemporalDenoise"));
		CheckIfValidShader(*newRenderer, newRenderer->mAmbientOcclusionTemporalDenoiseShader, "GTAOTemporalDenoise");

		newRenderer->mCreateClusterComputeShader = shadersystem::FindShaderHandle(STRING_ID("CalculateClusters"));
		CheckIfValidShader(*newRenderer, newRenderer->mCreateClusterComputeShader, "CalculateClusters");

		newRenderer->mMarkActiveClusterTilesComputeShader = shadersystem::FindShaderHandle(STRING_ID("MarkActiveClusters"));
		CheckIfValidShader(*newRenderer, newRenderer->mMarkActiveClusterTilesComputeShader, "MarkActiveClusters");

		newRenderer->mFindUniqueClustersComputeShader = shadersystem::FindShaderHandle(STRING_ID("FindUniqueClusters"));
		CheckIfValidShader(*newRenderer, newRenderer->mFindUniqueClustersComputeShader, "FindUniqueClusters");

		newRenderer->mAssignLightsToClustersComputeShader = shadersystem::FindShaderHandle(STRING_ID("ClustersCullLights"));
		CheckIfValidShader(*newRenderer, newRenderer->mAssignLightsToClustersComputeShader, "ClustersCullLights");

		newRenderer->mTransparentCompositeShader = shadersystem::FindShaderHandle(STRING_ID("TransparentComposite"));
		CheckIfValidShader(*newRenderer, newRenderer->mTransparentCompositeShader, "TransparentComposite");

		newRenderer->mSSRShader = shadersystem::FindShaderHandle(STRING_ID("SSR"));
		CheckIfValidShader(*newRenderer, newRenderer->mSSRShader, "SSR");

		newRenderer->mSSRConeTraceShader = shadersystem::FindShaderHandle(STRING_ID("ConeTracedSSR"));
		CheckIfValidShader(*newRenderer, newRenderer->mSSRConeTraceShader, "ConeTracedSSR");

		newRenderer->mVerticalBlurShader = shadersystem::FindShaderHandle(STRING_ID("VerticalBlur"));
		CheckIfValidShader(*newRenderer, newRenderer->mVerticalBlurShader, "VerticalBlur");

		newRenderer->mHorizontalBlurShader = shadersystem::FindShaderHandle(STRING_ID("HorizontalBlur"));
		CheckIfValidShader(*newRenderer, newRenderer->mHorizontalBlurShader, "HorizontalBlur");

		newRenderer->mBloomDownSampleShader = shadersystem::FindShaderHandle(STRING_ID("DownSamplePreFilter"));
		CheckIfValidShader(*newRenderer, newRenderer->mBloomDownSampleShader, "DownSamplePreFilter");

		newRenderer->mBloomBlurPreFilterShader = shadersystem::FindShaderHandle(STRING_ID("BlurPrefilter"));
		CheckIfValidShader(*newRenderer, newRenderer->mBloomBlurPreFilterShader, "BlurPrefilter");

		newRenderer->mBloomUpSampleShader = shadersystem::FindShaderHandle(STRING_ID("UpSampleBlur"));
		CheckIfValidShader(*newRenderer, newRenderer->mBloomUpSampleShader, "UpSampleBlur");

		newRenderer->mFXAAShader = shadersystem::FindShaderHandle(STRING_ID("FXAA"));
		CheckIfValidShader(*newRenderer, newRenderer->mFXAAShader, "FXAA");

		newRenderer->mSMAAEdgeDetectionShader = shadersystem::FindShaderHandle(STRING_ID("SMAAEdgeDetection"));
		CheckIfValidShader(*newRenderer, newRenderer->mSMAAEdgeDetectionShader, "SMAAEdgeDetection");

		newRenderer->mSMAABlendingWeightShader = shadersystem::FindShaderHandle(STRING_ID("SMAABlendingWeightCalculation"));
		CheckIfValidShader(*newRenderer, newRenderer->mSMAABlendingWeightShader, "SMAABlendingWeightCalculation");

		newRenderer->mSMAANeighborhoodBlendingShader = shadersystem::FindShaderHandle(STRING_ID("SMAANeighborhoodBlending"));
		CheckIfValidShader(*newRenderer, newRenderer->mSMAANeighborhoodBlendingShader, "SMAANeighborhoodBlending");

		newRenderer->mSMAANeighborhoodBlendingReprojectionShader = shadersystem::FindShaderHandle(STRING_ID("SMAANeighborhoodBlendingReprojection"));
		CheckIfValidShader(*newRenderer, newRenderer->mSMAANeighborhoodBlendingReprojectionShader, "SMAANeighborhoodBlendingReprojection");

		newRenderer->mSMAAReprojectResolveShader = shadersystem::FindShaderHandle(STRING_ID("SMAAReprojectResolve"));
		CheckIfValidShader(*newRenderer, newRenderer->mSMAAReprojectResolveShader, "SMAAReprojectResolve");

		newRenderer->mSMAASeparateShader = shadersystem::FindShaderHandle(STRING_ID("SMAASeparate"));
		CheckIfValidShader(*newRenderer, newRenderer->mSMAASeparateShader, "SMAASeparate");

		newRenderer->mPassThroughShader = shadersystem::FindShaderHandle(STRING_ID("PassThrough"));
		CheckIfValidShader(*newRenderer, newRenderer->mPassThroughShader, "PassThrough");

		newRenderer->mMSAAResolveNearestShader = shadersystem::FindShaderHandle(STRING_ID("MSAAResolveNearest"));
		CheckIfValidShader(*newRenderer, newRenderer->mMSAAResolveNearestShader, "MSAAResolveNearest");

#ifdef R2_EDITOR
		newRenderer->mEntityColorShader[0] = shadersystem::FindShaderHandle(STRING_ID("StaticEntityColor"));
		CheckIfValidShader(*newRenderer, newRenderer->mEntityColorShader[0], "StaticEntityColor");

		newRenderer->mEntityColorShader[1] = shadersystem::FindShaderHandle(STRING_ID("DynamicEntityColor"));
		CheckIfValidShader(*newRenderer, newRenderer->mEntityColorShader[1], "DynamicEntityColor");


		newRenderer->mGridShader = shadersystem::FindShaderHandle(STRING_ID("GridShader"));
		CheckIfValidShader(*newRenderer, newRenderer->mGridShader, "GridShader");
#endif





	//	newRenderer->mSDSMReduceBoundsComputeShader = shadersystem::FindShaderHandle(STRING_ID("ReduceBounds"));
	//	CheckIfValidShader(*newRenderer, newRenderer->mSDSMReduceBoundsComputeShader, "ReduceBounds");

	//	newRenderer->mSDSMCalculateCustomPartitionsComputeShader = shadersystem::FindShaderHandle(STRING_ID("CalculateCustomPartitions"));
	//	CheckIfValidShader(*newRenderer, newRenderer->mSDSMCalculateCustomPartitionsComputeShader, "CalculateCustomPartitions");
		

		newRenderer->mShadowSDSMComputeShader = shadersystem::FindShaderHandle(STRING_ID("CalculateCascadesSDSM"));
		CheckIfValidShader(*newRenderer, newRenderer->mShadowSDSMComputeShader, "CalculateCascadesSDSM");


		//@TODO(Serge): get rid of this - it's a giant hack

		

		newRenderer->mStaticDirectionLightBatchUniformLocation = rendererimpl::GetConstantLocation(newRenderer->mShadowDepthShaders[0], "directionLightBatch");
		newRenderer->mDynamicDirectionLightBatchUniformLocation = rendererimpl::GetConstantLocation(newRenderer->mShadowDepthShaders[1], "directionLightBatch");

		newRenderer->mStaticSpotLightBatchUniformLocation = rendererimpl::GetConstantLocation(newRenderer->mSpotLightShadowShaders[0], "spotLightBatch");
		newRenderer->mDynamicSpotLightBatchUniformLocation = rendererimpl::GetConstantLocation(newRenderer->mSpotLightShadowShaders[1], "spotLightBatch");

		newRenderer->mStaticPointLightBatchUniformLocation = rendererimpl::GetConstantLocation(newRenderer->mPointLightShadowShaders[0], "pointLightBatch");
		newRenderer->mDynamicPointLightBatchUniformLocation = rendererimpl::GetConstantLocation(newRenderer->mPointLightShadowShaders[1], "pointLightBatch");

		newRenderer->mVerticalBlurTextureContainerLocation = rendererimpl::GetConstantLocation(newRenderer->mVerticalBlurShader, "textureContainerToBlur");
		newRenderer->mVerticalBlurTexturePageLocation = rendererimpl::GetConstantLocation(newRenderer->mVerticalBlurShader, "texturePage");
		newRenderer->mVerticalBlurTextureLodLocation = rendererimpl::GetConstantLocation(newRenderer->mVerticalBlurShader, "textureLod");

		newRenderer->mHorizontalBlurTextureContainerLocation = rendererimpl::GetConstantLocation(newRenderer->mHorizontalBlurShader, "textureContainerToBlur");
		newRenderer->mHorizontalBlurTexturePageLocation = rendererimpl::GetConstantLocation(newRenderer->mHorizontalBlurShader, "texturePage");
		newRenderer->mHorizontalBlurTextureLodLocation = rendererimpl::GetConstantLocation(newRenderer->mHorizontalBlurShader, "textureLod");

		newRenderer->mSMAASeparateTextureContainerLocation = rendererimpl::GetConstantLocation(newRenderer->mSMAASeparateShader, "inputTextureContainer");
		newRenderer->mSMAASeparateTexturePageLocation = rendererimpl::GetConstantLocation(newRenderer->mSMAASeparateShader, "inputTexturePage");
		newRenderer->mSMAASeparateTextureLodLocation = rendererimpl::GetConstantLocation(newRenderer->mSMAASeparateShader, "inputTextureLod");

		newRenderer->mPassThroughTextureContainerLocation = rendererimpl::GetConstantLocation(newRenderer->mPassThroughShader, "inputTextureContainer");
		newRenderer->mPassThroughTexturePageLocation = rendererimpl::GetConstantLocation(newRenderer->mPassThroughShader, "inputTexturePage");
		newRenderer->mPassThroughTextureLodLocation = rendererimpl::GetConstantLocation(newRenderer->mPassThroughShader, "inputTextureLod");
		
		newRenderer->mMSAAResolveNearestTextureContainerLocation = rendererimpl::GetConstantLocation(newRenderer->mMSAAResolveNearestShader, "inputTextureContainer");;
		newRenderer->mMSAAResolveNearestTexturePageLocation = rendererimpl::GetConstantLocation(newRenderer->mMSAAResolveNearestShader, "inputTexturePage");
		newRenderer->mMSAAResolveNearestTextureLodLocation = rendererimpl::GetConstantLocation(newRenderer->mMSAAResolveNearestShader, "inputTexturePage");

		newRenderer->mPreRenderBucket = MAKE_CMD_BUCKET(*rendererArena, key::Basic, key::DecodeBasicKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mPreRenderBucket != nullptr, "We couldn't create the command bucket!");
		newRenderer->mPostRenderBucket = MAKE_CMD_BUCKET(*rendererArena, key::Basic, key::DecodeBasicKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mPostRenderBucket != nullptr, "We couldn't create the command bucket!");

		newRenderer->mPrePostRenderCommandArena = MAKE_STACK_ARENA(*rendererArena, 2 * COMMAND_CAPACITY * cmd::LargestCommand() + COMMAND_AUX_MEMORY / 2); // I think we want more memory here because it needs to do more heavy ops
		R2_CHECK(newRenderer->mPrePostRenderCommandArena != nullptr, "We couldn't create the pre/post command arena!");

		newRenderer->mShadowBucket = MAKE_CMD_BUCKET(*rendererArena, key::ShadowKey, key::DecodeShadowKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mShadowBucket != nullptr, "We couldn't create the command bucket!");

		newRenderer->mDepthPrePassBucket = MAKE_CMD_BUCKET(*rendererArena, key::DepthKey, key::DecodeDepthKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mDepthPrePassBucket != nullptr, "We couldn't create the command bucket!");

		newRenderer->mDepthPrePassShadowBucket = MAKE_CMD_BUCKET(*rendererArena, key::DepthKey, key::DecodeDepthKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mDepthPrePassShadowBucket != nullptr, "We couldn't create the command bucket!");

		newRenderer->mShadowArena = MAKE_STACK_ARENA(*rendererArena, 4 * COMMAND_CAPACITY * cmd::LargestCommand() + COMMAND_AUX_MEMORY / 4);
		R2_CHECK(newRenderer->mShadowArena != nullptr, "We couldn't create the shadow stack arena for commands");

		newRenderer->mAmbientOcclusionBucket = MAKE_CMD_BUCKET(*rendererArena, key::DepthKey, key::DecodeDepthKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mAmbientOcclusionBucket != nullptr, "We couldn't create the command bucket!");

		newRenderer->mAmbientOcclusionDenoiseBucket = MAKE_CMD_BUCKET(*rendererArena, key::DepthKey, key::DecodeDepthKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mAmbientOcclusionDenoiseBucket != nullptr, "We couldn't create the command bucket!");

		newRenderer->mAmbientOcclusionTemporalDenoiseBucket = MAKE_CMD_BUCKET(*rendererArena, key::DepthKey, key::DecodeDepthKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mAmbientOcclusionTemporalDenoiseBucket != nullptr, "We couldn't create the command bucket!");

		newRenderer->mAmbientOcclusionArena = MAKE_STACK_ARENA(*rendererArena, 2 * COMMAND_CAPACITY * cmd::LargestCommand());
		R2_CHECK(newRenderer->mAmbientOcclusionArena != nullptr, "We couldn't create the ambient occlusion stack arena");

		newRenderer->mClustersBucket = MAKE_CMD_BUCKET(*rendererArena, key::Basic, key::DecodeBasicKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mClustersBucket != nullptr, "We couldn't create the clusters bucket");

		newRenderer->mSSRBucket = MAKE_CMD_BUCKET(*rendererArena, key::Basic, key::DecodeBasicKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mSSRBucket != nullptr, "We couldn't create the ssr bucket");

#ifdef R2_EDITOR
		newRenderer->mEditorPickingBucket = MAKE_CMD_BUCKET(*rendererArena, key::Basic, key::DecodeBasicKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mEditorPickingBucket != nullptr, "We couldn't create the mEditorPickingBucket bucket");
#endif

		auto size = CENG.DisplaySize();

		
		

		memcpy(newRenderer->mRenderTargetParams, renderTargetParams, sizeof(renderTargetParams));

		
		
		newRenderer->mRenderTargetsArena = MAKE_STACK_ARENA(*rendererArena, renderTargetArenaSize);

		// r2::SArray<RenderMaterialParams>*mRenderMaterialsToRender = nullptr;
		newRenderer->mRenderMaterialsToRender = MAKE_SARRAY(*rendererArena, RenderMaterialParams, NUM_RENDER_MATERIALS_TO_RENDER);

		newRenderer->mPreRenderStackArena = MAKE_STACK_ARENA(*rendererArena, PRE_RENDER_STACK_ARENA_SIZE);

		//@TODO(Serge): we need to get the scale, x and y offsets
		ResizeRenderSurface(*newRenderer, size.width, size.height, size.width, size.height, 1.0f, 1.0f, 0.0f, 0.0f); //@TODO(Serge): we need to get the scale, x and y offsets

		newRenderer->mCommandBucket = MAKE_CMD_BUCKET(*rendererArena, r2::draw::key::Basic, r2::draw::key::DecodeBasicKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mCommandBucket != nullptr, "We couldn't create the command bucket!");

		newRenderer->mTransparentBucket = MAKE_CMD_BUCKET(*rendererArena, r2::draw::key::Basic, r2::draw::key::DecodeBasicKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mTransparentBucket != nullptr, "We couldn't create the command bucket!");

		newRenderer->mFinalBucket = MAKE_CMD_BUCKET(*rendererArena, r2::draw::key::Basic, r2::draw::key::DecodeBasicKey, COMMAND_CAPACITY);
		R2_CHECK(newRenderer->mFinalBucket != nullptr, "We couldn't create the final command bucket!");

		newRenderer->mCommandArena = MAKE_STACK_ARENA(*rendererArena, 6 * COMMAND_CAPACITY * cmd::LargestCommand() + COMMAND_AUX_MEMORY/4);

		R2_CHECK(newRenderer->mCommandArena != nullptr, "We couldn't create the stack arena for commands");

		//Make the render batches
		{
			newRenderer->mRenderBatches = MAKE_SARRAY(*rendererArena, RenderBatch, NUM_DRAW_TYPES);

			for (s32 i = 0; i < NUM_DRAW_TYPES; ++i)
			{
				RenderBatch nextBatch;

				nextBatch.vertexBufferLayoutHandle = vb::InvalidVertexBufferLayoutHandle;
				nextBatch.subCommandsHandle = InvalidConstantConfigHandle;
				nextBatch.modelsHandle = InvalidConstantConfigHandle;
				nextBatch.materialsHandle = InvalidConstantConfigHandle;
				nextBatch.boneTransformOffsetsHandle = InvalidConstantConfigHandle;
				nextBatch.boneTransformsHandle = InvalidConstantConfigHandle;

				nextBatch.gpuModelRefs = MAKE_SARRAY(*rendererArena, const vb::GPUModelRef*, MAX_NUM_DRAWS);

				nextBatch.materialBatch.infos = MAKE_SARRAY(*rendererArena, MaterialBatch::Info, MAX_NUM_DRAWS);
				
				
				nextBatch.materialBatch.renderMaterialParams = MAKE_SARRAY(*rendererArena, RenderMaterialParams, MAX_NUM_DRAWS);
				nextBatch.materialBatch.shaderEffectPasses = MAKE_SARRAY(*rendererArena, ShaderEffectPasses, MAX_NUM_DRAWS);

				nextBatch.models = MAKE_SARRAY(*rendererArena, glm::mat4, MAX_NUM_DRAWS);
				nextBatch.drawState = MAKE_SARRAY(*rendererArena, cmd::DrawState, MAX_NUM_DRAWS);
				nextBatch.numInstances = MAKE_SARRAY(*rendererArena, u32, MAX_NUM_DRAWS);
				nextBatch.meshRenderData = MAKE_SARRAY(*rendererArena, MeshRenderData, MAX_NUMBER_OF_MODELS_LOADED_AT_ONE_TIME * AVG_NUM_OF_MESHES_PER_MODEL);
#ifdef R2_EDITOR
				nextBatch.entityIDs = MAKE_SARRAY(*rendererArena, u32, MAX_NUM_DRAWS);
				nextBatch.entityInstanceOffsetBatches = MAKE_SARRAY(*rendererArena, EntityInstanceBatchOffset, MAX_NUM_DRAWS);
				nextBatch.entityInstances = MAKE_SARRAY(*rendererArena, s32, MAX_NUM_DRAWS);
#endif

				if (i == DrawType::DYNAMIC)
				{
					nextBatch.boneTransforms = MAKE_SARRAY(*rendererArena, ShaderBoneTransform, MAX_NUM_BONES);
					nextBatch.useSameBoneTransformsForInstances = MAKE_SARRAY(*rendererArena, b32, MAX_NUM_DRAWS);
				}
				else
				{
					nextBatch.boneTransforms = nullptr;
				}

				r2::sarr::Push(*newRenderer->mRenderBatches, nextBatch);
			}
		}

		CreateRenderPasses(*newRenderer);

		newRenderer->mModelCache = modlche::Create(memoryAreaHandle, DefaultModelsMemorySize(), true, "Rendering Engine Default Models");
		if (!newRenderer->mModelCache)
		{
			R2_CHECK(false, "We couldn't init the default engine models");
			return false;
		}

		bool loadedModels = LoadEngineModels(*newRenderer);

		R2_CHECK(loadedModels, "We didn't load the models for the engine!");

		InitializeVertexLayouts(*newRenderer, STATIC_MODELS_VERTEX_LAYOUT_SIZE, ANIM_MODELS_VERTEX_LAYOUT_SIZE);

		
		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();
		TexturePacksCache& texturePacksCache = CENG.GetTexturePacksCache();

		asset::AssetLib& assetLib = CENG.GetAssetLib();

		//@TODO(Serge): Clean this up - I don't like how we have to know what the material + texture manifest names are in the renderer
		char materialsPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(R2_ENGINE_INTERNAL_MATERIALS_MANIFESTS_BIN, materialsPath, "engine_material_pack.mpak");//"engine_material_params_pack.mppk");

		const byte* materialManifestData = r2::asset::lib::GetManifestData(assetLib, r2::asset::Asset::GetAssetNameForFilePath(materialsPath, r2::asset::EngineAssetType::MATERIAL_PACK_MANIFEST));

		const flat::MaterialPack* engineMaterialPack = flat::GetMaterialPack(materialManifestData);

		texche::LoadMaterialTextures(texturePacksCache, engineMaterialPack);

		char texturePackPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(R2_ENGINE_INTERNAL_TEXTURES_MANIFESTS_BIN, texturePackPath, "engine_texture_pack.tman");

		//@TODO(Serge): this should be part of the TextureCache or AssetLib(maybe) - each manifest should know how many of each texture they have
		u32 totalNumberOfTextures = 0;
		u32 totalNumberOfTexturePacks = 0;
		u32 totalNumCubemaps = 0;
		u32 cacheSize = 0;
		r2::draw::texche::GetTexturePacksCacheSizes(texturePackPath, totalNumberOfTextures, totalNumberOfTexturePacks, totalNumCubemaps, cacheSize);

		r2::SArray<r2::draw::tex::Texture>* engineTextures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::Texture, totalNumberOfTextures);
		r2::SArray<r2::draw::tex::CubemapTexture>* engineCubemaps = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::CubemapTexture, totalNumCubemaps);

		texche::GetTexturesForMaterialPack(texturePacksCache, engineMaterialPack, engineTextures, engineCubemaps);

		//We want these to be set before we upload them since they might use the missing texture in UploadMaterialTextureParamsArray()
		newRenderer->mMissingTexture = *texche::GetAlbedoTextureForMaterialName(texturePacksCache, engineMaterialPack, STRING_ID("MissingTexture")); //gameAssetManager.GetAlbedoTextureForMaterialName(engineMaterialPack, STRING_ID("MissingTexture"));
		newRenderer->mBlueNoiseTexture = *texche::GetAlbedoTextureForMaterialName(texturePacksCache, engineMaterialPack, STRING_ID("BlueNoise64"));//gameAssetManager.GetAlbedoTextureForMaterialName(engineMaterialPack, STRING_ID("BlueNoise64"));

		newRenderer->mRenderMaterialCache->mMissingTexture = newRenderer->mMissingTexture;

		rmat::UploadMaterialTextureParamsArray(*newRenderer->mRenderMaterialCache, engineMaterialPack, engineTextures, engineCubemaps);

		FREE(engineCubemaps, *MEM_ENG_SCRATCH_PTR);
		FREE(engineTextures, *MEM_ENG_SCRATCH_PTR);

		newRenderer->mDefaultOutlineMaterialName.assetName.hashID = STRING_ID("Outline");
		newRenderer->mDefaultOutlineMaterialName.packName.hashID = engineMaterialPack->assetName()->assetName();
		
		newRenderer->mDefaultOutlineAlphaDiscardMaterialName.assetName.hashID = STRING_ID("OutlineAlphaDiscard");
		newRenderer->mDefaultOutlineAlphaDiscardMaterialName.packName.hashID = engineMaterialPack->assetName()->assetName();
#ifdef R2_ASSET_PIPELINE
		newRenderer->mDefaultOutlineMaterialName.assetName.assetNameString = "outline";
		newRenderer->mDefaultOutlineMaterialName.packName.assetNameString = engineMaterialPack->assetName()->stringName()->str();

		newRenderer->mDefaultOutlineAlphaDiscardMaterialName.assetName.assetNameString = "outline_alpha_discard";
		newRenderer->mDefaultOutlineAlphaDiscardMaterialName.packName.assetNameString = engineMaterialPack->assetName()->stringName()->str();
#endif

		



		newRenderer->mMissingTextureRenderMaterialParams = *rmat::GetGPURenderMaterial(*newRenderer->mRenderMaterialCache, STRING_ID("MissingTexture"));
		newRenderer->mBlueNoiseRenderMaterialParams = *rmat::GetGPURenderMaterial(*newRenderer->mRenderMaterialCache, STRING_ID("BlueNoise64"));


		//Init the SDSM shader shadow data
		{
			glm::vec2 blurSizeLightSpace = glm::vec2(0.0f, 0.0f);
			constexpr float maxFloat = std::numeric_limits<float>::max();
			glm::vec3 maxPartitionScale = glm::vec3(maxFloat, maxFloat, maxFloat);
			glm::vec4 partitionBorderLightSpace(blurSizeLightSpace.x, blurSizeLightSpace.y, 0.0f, 0.0f);

			newRenderer->mSDSMShaderParams.lightSpaceBorder = partitionBorderLightSpace;
			newRenderer->mSDSMShaderParams.maxScale = glm::vec4(maxPartitionScale, 0.0);
			newRenderer->mSDSMShaderParams.projMultSplitScaleZMultLambda = glm::vec4(1.5, 1, 9, 1.2);
			newRenderer->mSDSMShaderParams.dilationFactor = DILATION_FACTOR;
			newRenderer->mSDSMShaderParams.scatterTileDim = SCATTER_TILE_DIM;
			newRenderer->mSDSMShaderParams.reduceTileDim = REDUCE_TILE_DIM;
			newRenderer->mSDSMShaderParams.padding = 0;
			newRenderer->mSDSMShaderParams.splitScaleMultFadeFactor = glm::vec4(0.5, 2, 0, 0);
			newRenderer->mSDSMShaderParams.blueNoiseTexture = newRenderer->mBlueNoiseRenderMaterialParams.albedo.texture;
			newRenderer->mSDSMShaderParamsNeedUpdate = true;
		}

		//Init the SSRParams
		{

			tex::TextureFormat textureFormat;
			textureFormat.width = 4;
			textureFormat.height = 4;
			textureFormat.mipLevels = 1;
			textureFormat.internalformat = tex::COLOR_FORMAT_R8;

			newRenderer->mSSRDitherTexture = tex::CreateTexture(textureFormat, 1, false);
			u8 textureData[] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };

			tex::TexSubImage2D(newRenderer->mSSRDitherTexture, 0, 0, 0, textureFormat, &textureData[0]);

			newRenderer->mSSRParams.mSSRDitherTexture = texsys::GetTextureAddress(newRenderer->mSSRDitherTexture);

			newRenderer->mSSRParams.mSSRStride = 10.0f;
			newRenderer->mSSRParams.mSSRThickness = 0.01f;
			newRenderer->mSSRParams.mSSRRayMarchIterations = 96;
			newRenderer->mSSRParams.mSSRStrideZCutoff = 36.0f;
			newRenderer->mSSRParams.mSSRMaxDistance = 10.0f;
			
			newRenderer->mSSRParams.mSSRDitherTilingFactor = 7.0f;
			newRenderer->mSSRParams.mSSRRoughnessMips = 0;
			newRenderer->mSSRParams.mSSRConeTracingSteps = 7;
			newRenderer->mSSRParams.mSSRMaxFadeDistance = 10;
			newRenderer->mSSRParams.mSSRFadeScreenStart = 0.1;
			newRenderer->mSSRParams.mSSRFadeScreenEnd = 0.9;

			newRenderer->mSSRNeedsUpdate = true;
		}

		//Set default bloom params
		{
			newRenderer->mBloomParams.mBloomThreshold = 1.0f;
			newRenderer->mBloomParams.mBloomKnee = 0.1f;
			newRenderer->mBloomParams.mBloomIntensity = 0.05f;
			newRenderer->mBloomParams.mBloomFilterSize = 0.005f;
		}

		{
			if (newRenderer->mSMAAAreaTexture.container == nullptr)
			{
				tex::TextureFormat textureFormat;
				textureFormat.width = AREATEX_WIDTH;
				textureFormat.height = AREATEX_HEIGHT;
				textureFormat.mipLevels = 1;
				textureFormat.internalformat = tex::COLOR_FORMAT_R8G8_UNORM;

				newRenderer->mSMAAAreaTexture = tex::CreateTexture(textureFormat, 1, false);

				tex::TexSubImage2D(newRenderer->mSMAAAreaTexture, 0, 0, 0, textureFormat, areaTexBytes);

				newRenderer->mAAParams.mSMAAAreaTexture = texsys::GetTextureAddress(newRenderer->mSMAAAreaTexture);
			}

			if (newRenderer->mSMAASearchTexture.container == nullptr)
			{
				tex::TextureFormat textureFormat;
				textureFormat.width = SEARCHTEX_WIDTH;
				textureFormat.height = SEARCHTEX_HEIGHT;
				textureFormat.mipLevels = 1;
				textureFormat.internalformat = tex::COLOR_FORMAT_R8_UNORM;

				newRenderer->mSMAASearchTexture = tex::CreateTexture(textureFormat, 1, false);

				tex::TexSubImage2D(newRenderer->mSMAASearchTexture, 0, 0, 0, textureFormat, searchTexBytes);

				newRenderer->mAAParams.mSMAASearchTexture = texsys::GetTextureAddress(newRenderer->mSMAASearchTexture);
			}
		}

		//@NOTE(Serge): this always has to be after the initialize vertex layouts and after we upload the render materials
		UploadEngineModels(*newRenderer);

		return newRenderer;
	}

	void Update(Renderer& renderer)
	{
		
		
	}

	void Render(Renderer& renderer, float alpha)
	{
		R2_CHECK(renderer.mnoptrRenderCam != nullptr, "We should have a proper camera before we render");
		UpdateCamera(renderer, *renderer.mnoptrRenderCam);

		renderer.mShaderVectors.jitter = glm::vec4(GetJitter(renderer, renderer.mShaderVectors.frame, false), GetJitter(renderer, renderer.mShaderVectors.frame - 1, false));

		//UpdateFrameCounter(renderer, renderer.mFrameCounter);
		if (renderer.mFlags.IsSet(RENDERER_FLAG_NEEDS_CLUSTER_VOLUME_TILE_UPDATE))
		{
			float logFarOverNear = glm::log2(renderer.mnoptrRenderCam->farPlane / renderer.mnoptrRenderCam->nearPlane);
			float logNear = glm::log2(renderer.mnoptrRenderCam->nearPlane);
			renderer.mShaderVectors.clusterScaleBias = glm::vec2(renderer.mShaderVectors.clusterTileSizes.z / logFarOverNear, -(renderer.mShaderVectors.clusterTileSizes.z * logNear) / logFarOverNear);

			//UpdateClusterScaleBias(renderer, glm::vec2(renderer.mClusterTileSizes.z / logFarOverNear, -(renderer.mClusterTileSizes.z * logNear) / logFarOverNear));
			//UpdateClusterTileSizes(renderer);
		}

		if (renderer.mFlags.IsSet(RENDERER_FLAG_NEEDS_ALL_SURFACES_UPLOAD))
		{
			UploadAllSurfaces(renderer);
		}
		else
		{
			UploadSwappingSurfaces(renderer);
		}

		UpdateShaderVectors(renderer);
		//	UpdateJitter(renderer);

		UpdateBloomDataIfNeeded(renderer);

		if (renderer.mOutputMerger == OUTPUT_FXAA)
		{
			UpdateFXAADataIfNeeded(renderer);
		}
		else if (renderer.mOutputMerger == OUTPUT_SMAA_X1 ||
			renderer.mOutputMerger == OUTPUT_SMAA_T2X)
		{

			float smaaCameraWeight = 1.0f;

			//@HACK(Serge): Stupid BS needed to make sure that the Temporal resolve doesn't ghost weirdly when camera moves backwards or faces a different direction
			//Maybe there's an actual way to fix this but I'm not sure since if we move backwards or turn the camera, we don't have enough info to fill the buffer correctly...
			//Maybe motion blur could fix this?
			{
				float dotResult = glm::dot(renderer.mSMAALastCameraFacingDirection, renderer.mnoptrRenderCam->facing);
				glm::vec3 diff = renderer.mnoptrRenderCam->position - renderer.mSMAALastCameraPosition;
				if (diff != glm::vec3(0))
				{
					diff = glm::normalize(diff);
				}
				float dotResult2 = glm::dot(diff, renderer.mnoptrRenderCam->facing);
				if (!math::NearEq(dotResult, 1.0f) ||
					(diff != glm::vec3(0) && !math::NearEq(dotResult2, 1.0f)))
				{
					//if camera moved around then turn off the temporal resolve
					smaaCameraWeight = 0.0f;
				}
			}


			renderer.mAAParams.mSMAASampleIndices = SMAASubSampleIndices(renderer, (renderer.mShaderVectors.frame % 2));
			renderer.mAAParams.mSMAACameraMovementWeight = smaaCameraWeight;

			renderer.mSMAANeedsUpdate = true;

			UpdateSMAADataIfNeeded(renderer);
		}

		UpdateLighting(renderer);



#ifdef R2_DEBUG
		DebugPreRender(renderer);
#endif

		UpdateClusters(renderer);
		UpdateSSRDataIfNeeded(renderer);

		UpdateColorCorrectionIfNeeded(renderer);

		BloomRenderPass(renderer);


		PreRender(renderer);
		


	//	if (r2::sarr::Size(*s_optrRenderer->finalBatch.subcommands) == 0)
		{
			//@NOTE: this is here because we need to wait till the user of the code makes all of the handles
	//		SetupFinalBatchInternal();
		}

	//	AddFinalBatchInternal();
		//const u64 numEntries = r2::sarr::Size(*s_optrRenderer->mCommandBucket->entries);

		//for (u64 i = 0; i < numEntries; ++i)
		//{
		//	const r2::draw::CommandBucket<r2::draw::key::Basic>::Entry& entry = r2::sarr::At(*s_optrRenderer->mCommandBucket->entries, i);
		//	printf("entry - key: %llu, data: %p, func: %p\n", entry.aKey.keyValue, entry.data, entry.func);
		//}

		//printf("================================================\n");
		cmdbkt::Sort(*renderer.mDepthPrePassBucket, key::CompareDepthKey);
		cmdbkt::Sort(*renderer.mDepthPrePassShadowBucket, key::CompareDepthKey);
		cmdbkt::Sort(*renderer.mClustersBucket, key::CompareBasicKey);
		cmdbkt::Sort(*renderer.mAmbientOcclusionBucket, key::CompareDepthKey);
		cmdbkt::Sort(*renderer.mAmbientOcclusionDenoiseBucket, key::CompareDepthKey);
		cmdbkt::Sort(*renderer.mAmbientOcclusionTemporalDenoiseBucket, key::CompareDepthKey);
		cmdbkt::Sort(*renderer.mPreRenderBucket, r2::draw::key::CompareBasicKey);
		cmdbkt::Sort(*renderer.mShadowBucket, key::CompareShadowKey);
		cmdbkt::Sort(*renderer.mCommandBucket, r2::draw::key::CompareBasicKey);
		cmdbkt::Sort(*renderer.mTransparentBucket, r2::draw::key::CompareBasicKey);
		cmdbkt::Sort(*renderer.mSSRBucket, r2::draw::key::CompareBasicKey);
#ifdef R2_EDITOR
		cmdbkt::Sort(*renderer.mEditorPickingBucket, r2::draw::key::CompareBasicKey);
#endif
		cmdbkt::Sort(*renderer.mFinalBucket, r2::draw::key::CompareBasicKey);
		cmdbkt::Sort(*renderer.mPostRenderBucket, r2::draw::key::CompareBasicKey);
#ifdef R2_DEBUG
		cmdbkt::Sort(*renderer.mPreDebugCommandBucket, r2::draw::key::CompareDebugKey);
		cmdbkt::Sort(*renderer.mDebugCommandBucket, r2::draw::key::CompareDebugKey);
		cmdbkt::Sort(*renderer.mPostDebugCommandBucket, r2::draw::key::CompareDebugKey);
#endif
		//for (u64 i = 0; i < numEntries; ++i)
		//{
		//	const r2::draw::CommandBucket<r2::draw::key::Basic>::Entry* entry = r2::sarr::At(*s_optrRenderer->mCommandBucket->sortedEntries, i);
		//	printf("sorted - key: %llu, data: %p, func: %p\n", entry->aKey.keyValue, entry->data, entry->func);
		//}

		cmdbkt::Submit(*renderer.mPreRenderBucket);
		
		cmdbkt::Submit(*renderer.mDepthPrePassBucket);
		cmdbkt::Submit(*renderer.mDepthPrePassShadowBucket);
		cmdbkt::Submit(*renderer.mClustersBucket);
		cmdbkt::Submit(*renderer.mAmbientOcclusionBucket);
		cmdbkt::Submit(*renderer.mAmbientOcclusionDenoiseBucket);
		cmdbkt::Submit(*renderer.mAmbientOcclusionTemporalDenoiseBucket);
		cmdbkt::Submit(*renderer.mShadowBucket);

#ifdef R2_EDITOR
		cmdbkt::Submit(*renderer.mEditorPickingBucket);
#endif

		cmdbkt::Submit(*renderer.mCommandBucket); 
		
		cmdbkt::Submit(*renderer.mSSRBucket);

		//We want the debug submits to be after we use the depth buffer for all passes since we need to write to it
		//for debug shapes to look correct
#ifdef R2_DEBUG
		cmdbkt::Submit(*renderer.mPreDebugCommandBucket);
		cmdbkt::Submit(*renderer.mDebugCommandBucket);
#endif
		cmdbkt::Submit(*renderer.mTransparentBucket);

		cmdbkt::Submit(*renderer.mFinalBucket);
		cmdbkt::Submit(*renderer.mPostRenderBucket);

#ifdef R2_DEBUG
		cmdbkt::Submit(*renderer.mPostDebugCommandBucket);

		ClearDebugRenderData(renderer);
#endif

		cmdbkt::ClearAll(*renderer.mPreRenderBucket);
		cmdbkt::ClearAll(*renderer.mAmbientOcclusionBucket);
		cmdbkt::ClearAll(*renderer.mAmbientOcclusionDenoiseBucket);
		cmdbkt::ClearAll(*renderer.mAmbientOcclusionTemporalDenoiseBucket);
		cmdbkt::ClearAll(*renderer.mDepthPrePassBucket);
		cmdbkt::ClearAll(*renderer.mDepthPrePassShadowBucket);
		cmdbkt::ClearAll(*renderer.mClustersBucket);
		cmdbkt::ClearAll(*renderer.mTransparentBucket);
		cmdbkt::ClearAll(*renderer.mCommandBucket);
		cmdbkt::ClearAll(*renderer.mSSRBucket);
		cmdbkt::ClearAll(*renderer.mShadowBucket);
		cmdbkt::ClearAll(*renderer.mFinalBucket);
		cmdbkt::ClearAll(*renderer.mPostRenderBucket);
		
#ifdef R2_EDITOR
		cmdbkt::ClearAll(*renderer.mEditorPickingBucket);
#endif

		ClearRenderBatches(renderer);

		//This is kinda bad but... speed... 
		RESET_ARENA(*renderer.mCommandArena);
		RESET_ARENA(*renderer.mPrePostRenderCommandArena);
		RESET_ARENA(*renderer.mShadowArena);
		RESET_ARENA(*renderer.mAmbientOcclusionArena);

		
		renderer.mFlags.Clear();

		SwapRenderTargetsHistoryIfNecessary(renderer);

		//remember the previous projection/view matrices

		renderer.mShaderMatrices.prevProjection = renderer.mnoptrRenderCam->proj;
		renderer.mShaderMatrices.prevView = renderer.mnoptrRenderCam->view;
		renderer.mShaderMatrices.prevVPMatrix = renderer.mnoptrRenderCam->vp;

		//renderer.prevProj = renderer.mnoptrRenderCam->proj;
	//	renderer.prevView = renderer.mnoptrRenderCam->view;
		//renderer.prevVP = renderer.mnoptrRenderCam->vp;

		renderer.mSMAALastCameraFacingDirection = renderer.mnoptrRenderCam->facing;
		renderer.mSMAALastCameraPosition = renderer.mnoptrRenderCam->position;

		++renderer.mShaderVectors.frame;
	}

	void Shutdown(Renderer* renderer)
	{
		if (renderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		r2::mem::LinearArena* arena = renderer->mSubAreaArena;

#ifdef R2_DEBUG

		FREE(renderer->mDebugVertices, *arena);
		FREE(renderer->modelDrawBatchSubCommands, *arena);

		FREE(renderer->genericModelBatchRenderOffsets, *arena);

		FREE(renderer->linesDrawBatchSubCommands, *arena);

		FREE(renderer->linesBatchRenderOffsets, *arena);

		for (s32 i = NUM_DEBUG_DRAW_TYPES - 1; i >= 0; --i)
		{
			DebugRenderBatch& debugRenderBatch = r2::sarr::At(*renderer->mDebugRenderBatches, i);


			FREE(debugRenderBatch.meshPasses, *arena);
			FREE(debugRenderBatch.shaderHandles, *arena);

			if ((DebugDrawType)i == DDT_LINES)
			{
				FREE(debugRenderBatch.vertices, *arena);
			}

			else if ((DebugDrawType)i == DDT_MODELS_GENERIC)
			{
				FREE(debugRenderBatch.debugModelTypesToDraw, *arena);
			}
			else
			{
				R2_CHECK(false, "Unsupported");
			}

			FREE(debugRenderBatch.numInstances, *arena);
			FREE(debugRenderBatch.drawFlags, *arena);
			//FREE(debugRenderBatch.colors, *arena);
			FREE(debugRenderBatch.debugRenderConstants, *arena);
		}

		FREE(renderer->mDebugRenderBatches, *arena);
		FREE(renderer->mDebugCommandArena, *arena);
		FREE_CMD_BUCKET(*arena, key::DebugKey, renderer->mDebugCommandBucket);
		FREE_CMD_BUCKET(*arena, key::DebugKey, renderer->mPostDebugCommandBucket);
		FREE_CMD_BUCKET(*arena, key::DebugKey, renderer->mPreDebugCommandBucket);

#endif
		for (int i = NUM_DRAW_TYPES - 1; i >= 0; --i)
		{
			RenderBatch& nextBatch = r2::sarr::At(*renderer->mRenderBatches, i);
			
			if (i == DrawType::DYNAMIC)
			{
				FREE(nextBatch.useSameBoneTransformsForInstances, *arena);
				FREE(nextBatch.boneTransforms, *arena);
			}

#ifdef R2_EDITOR
			FREE(nextBatch.entityInstances, *arena);
			FREE(nextBatch.entityInstanceOffsetBatches, *arena);
			FREE(nextBatch.entityIDs, *arena);
#endif
			FREE(nextBatch.meshRenderData, *arena);
			FREE(nextBatch.numInstances, *arena);
			FREE(nextBatch.drawState, *arena);
			FREE(nextBatch.models, *arena);

			FREE(nextBatch.materialBatch.shaderEffectPasses, *arena);
			FREE(nextBatch.materialBatch.renderMaterialParams, *arena);

			FREE(nextBatch.materialBatch.infos, *arena);
			FREE(nextBatch.gpuModelRefs, *arena);

		}

		DestroyRenderPasses(*renderer);

		FREE(renderer->mRenderBatches, *arena);

		DestroyRenderSurfaces(*renderer);
		
		FREE(renderer->mCommandArena, *arena);

		FREE(renderer->mAmbientOcclusionArena, *arena);

		FREE(renderer->mShadowArena, *arena);

		FREE(renderer->mRenderMaterialsToRender, *arena);

		FREE(renderer->mPreRenderStackArena, *arena);

		FREE(renderer->mRenderTargetsArena, *arena);

		FREE(renderer->mPrePostRenderCommandArena, *arena);

		FREE_CMD_BUCKET(*arena, r2::draw::key::Basic, renderer->mFinalBucket);
		FREE_CMD_BUCKET(*arena, key::Basic, renderer->mTransparentBucket);
		FREE_CMD_BUCKET(*arena, r2::draw::key::Basic, renderer->mCommandBucket);

#ifdef R2_EDITOR
		FREE_CMD_BUCKET(*arena, key::Basic, renderer->mEditorPickingBucket);
#endif

		FREE_CMD_BUCKET(*arena, key::Basic, renderer->mSSRBucket);
		FREE_CMD_BUCKET(*arena, key::Basic, renderer->mClustersBucket);
		FREE_CMD_BUCKET(*arena, key::DepthKey, renderer->mAmbientOcclusionTemporalDenoiseBucket);
		FREE_CMD_BUCKET(*arena, key::DepthKey, renderer->mAmbientOcclusionDenoiseBucket);
		FREE_CMD_BUCKET(*arena, key::DepthKey, renderer->mAmbientOcclusionBucket);
		FREE_CMD_BUCKET(*arena, key::DepthKey, renderer->mDepthPrePassShadowBucket);
		FREE_CMD_BUCKET(*arena, key::DepthKey, renderer->mDepthPrePassBucket);
		FREE_CMD_BUCKET(*arena, key::ShadowKey, renderer->mShadowBucket);
		FREE_CMD_BUCKET(*arena, key::Basic, renderer->mPostRenderBucket);
		FREE_CMD_BUCKET(*arena, key::Basic, renderer->mPreRenderBucket);
		

		modlche::Shutdown(renderer->mModelCache);
		FREE(renderer->mDefaultModelHandles, *arena);

		r2::draw::tex::UnloadFromGPU(renderer->mSSRDitherTexture);

		//if (renderer->mOutputMerger == OUTPUT_SMAA_X1 || renderer->mOutputMerger == OUTPUT_SMAA_T2X)
		{
			r2::draw::tex::UnloadFromGPU(renderer->mSMAAAreaTexture);
			r2::draw::tex::UnloadFromGPU(renderer->mSMAASearchTexture);
		}

		//Needs to be here - we may need to move the texture system out of the renderer for this to go
	//	mat::UnloadAllMaterialTexturesFromGPU(*renderer->mnoptrMaterialSystem);



		lightsys::DestroyLightSystem(*arena, renderer->mLightSystem);
	
		r2::draw::rmat::Shutdown(renderer->mRenderMaterialCache);

		r2::draw::vb::FreeVertexBufferLayoutSystem(renderer->mVertexBufferLayoutSystem);
		r2::draw::texsys::Shutdown();

		renderer->mSubAreaArena = nullptr;

		FREE(renderer->mConstantBufferHandles, *arena);
		FREE(renderer->mConstantBufferData, *arena);
		FREE(renderer->mVertexBufferLayoutHandles, *arena);

		FREE(renderer->mConstantLayouts, *arena);
		FREE(renderer->mEngineModelRefs, *arena);

		FREE(renderer, *arena);

		FREE_EMPLACED_ARENA(arena);
	}

	//Setup code
	void SetClearColor(const glm::vec4& color)
	{
		r2::draw::rendererimpl::SetClearColor(color);
	}

	void SetClearDepth(float color)
	{
		rendererimpl::SetDepthClearColor(color);
	}

	void InitializeVertexLayouts(Renderer& renderer, u32 staticModelLayoutSize, u32 animatedModelLayoutSize)
	{
		AddStaticModelLayout(renderer, { staticModelLayoutSize }, staticModelLayoutSize);
		AddAnimatedModelLayout(renderer, { animatedModelLayoutSize, animatedModelLayoutSize }, animatedModelLayoutSize);

		renderer.mVPMatricesConfigHandle = AddConstantBufferLayout(renderer, r2::draw::ConstantBufferLayout::Type::Small, {
			{r2::draw::ShaderDataType::Mat4, "projection"},
			{r2::draw::ShaderDataType::Mat4, "view"},
			{r2::draw::ShaderDataType::Mat4, "skyboxView"},
			{r2::draw::ShaderDataType::Mat4, "InverseProjection"},
			{r2::draw::ShaderDataType::Mat4, "InverseView"},
			{r2::draw::ShaderDataType::Mat4, "VPMatrix"},
			{r2::draw::ShaderDataType::Mat4, "prevProjection"},
			{r2::draw::ShaderDataType::Mat4, "prevView"},
			{r2::draw::ShaderDataType::Mat4, "prevVPMatrix"}
		});

		renderer.mVectorsConfigHandle = AddConstantBufferLayout(renderer, r2::draw::ConstantBufferLayout::Type::Small, {
			{r2::draw::ShaderDataType::Float4, "CameraPosTimeW"},
			{r2::draw::ShaderDataType::Float4, "Exposure"},
			{r2::draw::ShaderDataType::Float4, "ShadowMapSizes"},
			{r2::draw::ShaderDataType::Float4, "fovAspectResolutionXY"},
			{r2::draw::ShaderDataType::UInt64, "Frame"},
			{r2::draw::ShaderDataType::Float2, "clusterScaleBias"},
			{r2::draw::ShaderDataType::UInt4, "tileSizes"},
			{r2::draw::ShaderDataType::Float4, "jitter"}
		});

		AddSurfacesLayout(renderer);

		renderer.mSDSMParamsConfigHandle = AddConstantBufferLayout(renderer, r2::draw::ConstantBufferLayout::Type::Small, {
			{r2::draw::ShaderDataType::Float4, "lightSpaceBorder"},
			{r2::draw::ShaderDataType::Float4, "maxScale"},
			{r2::draw::ShaderDataType::Float4, "projMultSplitScaleZMultLambda"},
			{r2::draw::ShaderDataType::Float, "dialationFactor"},
			{r2::draw::ShaderDataType::UInt, "scatterTileDim"},
			{r2::draw::ShaderDataType::UInt, "reduceTileDim"},
			{r2::draw::ShaderDataType::UInt, "padding"},
			{r2::draw::ShaderDataType::Float4, "splitScaleMultFadeFactor"},
			{r2::draw::ShaderDataType::Struct, "BlueNoiseTexture"}
		});


		r2::draw::ConstantBufferLayoutConfiguration ssrParams
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		ssrParams.layout.InitForSSR();

		r2::sarr::Push(*renderer.mConstantLayouts, ssrParams);

		renderer.mSSRConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		

		//renderer.mSSRConfigHandle = AddConstantBufferLayout(renderer, ConstantBufferLayout::Type::Small, {
		//	{r2::draw::ShaderDataType::Float, "ssr_stride"},
		//	{r2::draw::ShaderDataType::Float, "ssr_ssThickness"},
		//	{r2::draw::ShaderDataType::Int, "ssr_rayMarchIterations"},
		//	{r2::draw::ShaderDataType::Float, "ssr_strideZCutoff"},
		//	{r2::draw::ShaderDataType::Struct, "ssr_ditherTexture"},
		//	{r2::draw::ShaderDataType::Float, "ssr_ditherTilingFactor"},
		//	{r2::draw::ShaderDataType::Int, "ssr_roughnessMips"},
		//	{r2::draw::ShaderDataType::Int, "ssr_coneTracingSteps"},
		//	{r2::draw::ShaderDataType::Float, "ssr_maxFadeDistance"},
		//	{r2::draw::ShaderDataType::Float, "ssr_fadeStart"},
		//	{r2::draw::ShaderDataType::Float, "ssr_fadeEnd"},
		//	{r2::draw::ShaderDataType::Float, "ssr_maxDistance"},
		//});

		renderer.mBloomConfigHandle = AddConstantBufferLayout(renderer, ConstantBufferLayout::Type::Small, {
			{r2::draw::ShaderDataType::Float4, "bloom_Filter"},
			{r2::draw::ShaderDataType::UInt4, "bloom_resolutions"},
			{r2::draw::ShaderDataType::Float4, "bloom_filterRadiusXYIntensity"},
			{r2::draw::ShaderDataType::UInt64, "bloomTextureToDownSample"},
			{r2::draw::ShaderDataType::Float, "bloomTexturePage"},
			{r2::draw::ShaderDataType::Float, "bloomTextureMipLevel"}
		});



		r2::draw::ConstantBufferLayoutConfiguration aaParams
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		aaParams.layout.InitForAntiAliasing();

		r2::sarr::Push(*renderer.mConstantLayouts, aaParams);

		renderer.mAAConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;



		/*renderer.mAAConfigHandle = AddConstantBufferLayout(renderer, ConstantBufferLayout::Type::Small, {
			{r2::draw::ShaderDataType::Struct, "inputTexture"},
			{r2::draw::ShaderDataType::Float, "fxaa_lumaThreshold"},
			{r2::draw::ShaderDataType::Float, "fxaa_lumaMulReduce"},
			{r2::draw::ShaderDataType::Float, "fxaa_lumaMinReduce"},
			{r2::draw::ShaderDataType::Float, "fxaa_maxSpan"},
			{r2::draw::ShaderDataType::Float2, "fxaa_texelStep"},
			{r2::draw::ShaderDataType::Float, "smaa_lumaThreshold"},
			{r2::draw::ShaderDataType::Int, "smaa_maxSearchSteps"},
			{r2::draw::ShaderDataType::Struct, "smaUpda_areaTexture"},
			{r2::draw::ShaderDataType::Struct, "smaa_searchTexture"},
			{r2::draw::ShaderDataType::Int4, "smaa_subSampleIndices"},
			{r2::draw::ShaderDataType::Int, "smaa_cornerRounding"},
			{r2::draw::ShaderDataType::Int, "smaa_maxSearchStepsDiag"},
			{r2::draw::ShaderDataType::Float, "smaa_cameraMovementWeight"}
		});*/

		//renderer.mColorCorrectionConfigHandle = AddConstantBufferLayout(renderer, ConstantBufferLayout::Type::Small, {
		//	{r2::draw::ShaderDataType::Float, "cc_constrast"},
		//	{r2::draw::ShaderDataType::Float, "cc_brightness"},
		//	{r2::draw::ShaderDataType::Float, "cc_saturation"},
		//	{r2::draw::ShaderDataType::Float, "cc_gamma"},

		//	{r2::draw::ShaderDataType::Float, "cc_filmGrainStrength"},
		//	{r2::draw::ShaderDataType::Float, "cg_halfColX"},
		//	{r2::draw::ShaderDataType::Float, "cg_halfColY"},
		//	{r2::draw::ShaderDataType::Float, "cg_numColors"},

		//	{r2::draw::ShaderDataType::Struct, "cg_LUTTexture"},

		//	{r2::draw::ShaderDataType::Float, "cg_contribution"}
		//});

		r2::draw::ConstantBufferLayoutConfiguration colorCorrectionParams
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		colorCorrectionParams.layout.InitForColorCorrection();

		r2::sarr::Push(*renderer.mConstantLayouts, colorCorrectionParams);

		renderer.mColorCorrectionConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;



		AddModelsLayout(renderer, r2::draw::ConstantBufferLayout::Type::Big);

		AddSubCommandsLayout(renderer);
		AddMaterialLayout(renderer);
		
		//Maybe these should automatically be added by the animated models layout
		AddBoneTransformsLayout(renderer);

		AddBoneTransformOffsetsLayout(renderer);

		AddLightingLayout(renderer);
	

#ifdef R2_DEBUG
		//Only add the vertex layout here
		AddDebugDrawLayout(renderer);
#endif
		//@NOTE(Serge): we always have to add the debug constant layouts so everything always matches in the shaders
		AddDebugColorsLayout(renderer);
		AddDebugModelSubCommandsLayout(renderer);
		AddDebugLineSubCommandsLayout(renderer);

		AddShadowDataLayout(renderer);

		AddMaterialOffsetsLayout(renderer);
		AddClusterVolumesLayout(renderer);
		AddDispatchComputeIndirectLayout(renderer);

		AddMeshDataLayout(renderer);


		bool success = GenerateLayouts(renderer);
		R2_CHECK(success, "We couldn't create the buffer layouts!");
	}

	bool GenerateLayouts(Renderer& renderer)
	{


		bool success = //GenerateBufferLayouts(renderer, renderer.mVertexLayouts) &&
		GenerateConstantBuffers(renderer, renderer.mConstantLayouts);


		R2_CHECK(success, "We didn't properly generate the layouts!");

		if (success)
		{
			//setup the layouts for the render batches
			

#ifdef R2_DEBUG
			DebugRenderBatch& debugLinesRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_LINES);
			debugLinesRenderBatch.vertexBufferLayoutHandle = renderer.mDebugLinesVertexConfigHandle;
			debugLinesRenderBatch.renderDebugConstantsConfigHandle = renderer.mDebugRenderConstantsConfigHandle;
			debugLinesRenderBatch.subCommandsConstantConfigHandle = renderer.mDebugLinesSubCommandsConfigHandle;

			DebugRenderBatch& debugModelGenericRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_MODELS_GENERIC);
			debugModelGenericRenderBatch.vertexBufferLayoutHandle = renderer.mDebugModelVertexConfigHandle;
			debugModelGenericRenderBatch.renderDebugConstantsConfigHandle = renderer.mDebugRenderConstantsConfigHandle;
			debugModelGenericRenderBatch.subCommandsConstantConfigHandle = renderer.mDebugModelSubCommandsConfigHandle;

#endif

			for (s32 i = 0; i < DrawType::NUM_DRAW_TYPES; ++i)
			{
				RenderBatch& batch = r2::sarr::At(*renderer.mRenderBatches, i);

				batch.materialsHandle = r2::sarr::At(*renderer.mConstantBufferHandles, renderer.mMaterialConfigHandle);
				batch.modelsHandle = r2::sarr::At(*renderer.mConstantBufferHandles, renderer.mModelConfigHandle);
				batch.subCommandsHandle = r2::sarr::At(*renderer.mConstantBufferHandles, renderer.mSubcommandsConfigHandle);
				batch.materialOffsetsHandle = r2::sarr::At(*renderer.mConstantBufferHandles, renderer.mMaterialOffsetsConfigHandle);

				if (i == DrawType::STATIC)
				{
					batch.vertexBufferLayoutHandle = renderer.mStaticVertexModelConfigHandle;
				}
				else if (i == DrawType::DYNAMIC)
				{
					batch.vertexBufferLayoutHandle = renderer.mAnimVertexModelConfigHandle;
					batch.boneTransformsHandle = renderer.mBoneTransformsConfigHandle;
					batch.boneTransformOffsetsHandle = renderer.mBoneTransformOffsetsConfigHandle;
				}
			}
		}

		return success;
	}

	bool GenerateConstantBuffers(Renderer& renderer, const r2::SArray<r2::draw::ConstantBufferLayoutConfiguration>* constantBufferConfigs)
	{

		if (r2::sarr::Size(*renderer.mConstantBufferHandles) > 0)
		{
			R2_CHECK(false, "We have already generated the constant buffer handles!");
			return false;
		}

		if (!renderer.mConstantBufferData)
		{
			R2_CHECK(false, "We have already generated the constant buffer data!");
			return false;
		}

		R2_CHECK(constantBufferConfigs != nullptr, "constant buffer configs cannot be null");

		if (constantBufferConfigs == nullptr)
		{
			return false;
		}

		if (r2::sarr::Size(*constantBufferConfigs) > MAX_BUFFER_LAYOUTS)
		{
			R2_CHECK(false, "Trying to configure more buffer constant buffer handles than we have space for!");
			return false;
		}

		const u64 numConstantBuffers = r2::sarr::Size(*constantBufferConfigs);

		r2::draw::rendererimpl::GenerateContantBuffers(static_cast<u32>(numConstantBuffers), renderer.mConstantBufferHandles->mData);
		renderer.mConstantBufferHandles->mSize = numConstantBuffers;

		for (u64 i = 0; i < numConstantBuffers; ++i)
		{
			ConstantBufferData constData;
			const ConstantBufferLayoutConfiguration& config = r2::sarr::At(*constantBufferConfigs, i);
			auto handle = r2::sarr::At(*renderer.mConstantBufferHandles, i);

			constData.handle = handle;
			constData.type = config.layout.GetType();
			constData.isPersistent = config.layout.GetFlags().IsSet(CB_FLAG_MAP_PERSISTENT);
			constData.bufferSize = config.layout.GetSize();

			r2::shashmap::Set(*renderer.mConstantBufferData, handle, constData);
		}

		r2::draw::rendererimpl::SetupConstantBufferConfigs(constantBufferConfigs, renderer.mConstantBufferHandles->mData);

		return true;
	}

	vb::VertexBufferLayoutHandle AddStaticModelLayout(Renderer& renderer, const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize)
	{
		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return vb::InvalidVertexBufferLayoutHandle;
		}

		auto numVertexLayouts = vertexLayoutSizes.size();
		r2::draw::BufferLayoutConfiguration layoutConfig;

		layoutConfig.layout = BufferLayout(
			{
				{
				{r2::draw::ShaderDataType::Float3, "aPos"},
				{r2::draw::ShaderDataType::Float3, "aNormal"},
				{r2::draw::ShaderDataType::Float3, "aTangent"},
				{r2::draw::ShaderDataType::Float3, "aTexCoord0"},
				{r2::draw::ShaderDataType::Float3, "aTexCoord1"},

				}
			}
		);

		size_t i = 0;
		for (u64 layoutSize : vertexLayoutSizes)
		{
			layoutConfig.vertexBufferConfigs[i] =
			{
				(u32)layoutSize,
				r2::draw::VertexDrawTypeStatic
			};
			++i;
		}

		layoutConfig.indexBufferConfig =
		{
			(u32)indexSize,
			r2::draw::VertexDrawTypeStatic
		};

		layoutConfig.useDrawIDs = true;
		layoutConfig.maxDrawCount = MAX_NUM_DRAWS;
		layoutConfig.numVertexConfigs = numVertexLayouts;

		//r2::sarr::Push(*renderer.mVertexLayouts, layoutConfig);


		vb::VertexBufferLayoutHandle staticVertexModelHandle = vbsys::AddVertexBufferLayout(*renderer.mVertexBufferLayoutSystem, layoutConfig);

		r2::sarr::Push(*renderer.mVertexBufferLayoutHandles, staticVertexModelHandle);

		renderer.mStaticVertexModelConfigHandle = staticVertexModelHandle;//r2::sarr::Size(*renderer.mVertexLayouts) - 1;

#ifdef R2_DEBUG
		renderer.mDebugModelVertexConfigHandle = renderer.mStaticVertexModelConfigHandle;
#endif
		return renderer.mStaticVertexModelConfigHandle;
	}

	vb::VertexBufferLayoutHandle AddAnimatedModelLayout(Renderer& renderer, const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize)
	{

		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return vb::InvalidVertexBufferLayoutHandle;
		}

		R2_CHECK(vertexLayoutSizes.size() == 2, "Only support 2 vertex layouts for Animated Models right now");

		auto numVertexLayouts = vertexLayoutSizes.size();
		r2::draw::BufferLayoutConfiguration layoutConfig;

		layoutConfig.layout = BufferLayout(
			{
				{r2::draw::ShaderDataType::Float3, "aPos", 0},
				{r2::draw::ShaderDataType::Float3, "aNormal", 0},
				{r2::draw::ShaderDataType::Float3, "aTangent", 0},
				{r2::draw::ShaderDataType::Float3, "aTexCoord0", 0},
				{r2::draw::ShaderDataType::Float3, "aTexCoord1", 0},
				{r2::draw::ShaderDataType::Float4, "aBoneWeights", 1},
				{r2::draw::ShaderDataType::Int4,   "aBoneIDs", 1}
			}
		);

		size_t i = 0;
		for (u64 layoutSize : vertexLayoutSizes)
		{
			layoutConfig.vertexBufferConfigs[i] =
			{
				(u32)layoutSize,
				r2::draw::VertexDrawTypeStatic
			};
			++i;
		}

		layoutConfig.indexBufferConfig =
		{
			(u32)indexSize,
			r2::draw::VertexDrawTypeStatic
		};

		layoutConfig.useDrawIDs = true;
		layoutConfig.maxDrawCount = MAX_NUM_DRAWS;
		layoutConfig.numVertexConfigs = numVertexLayouts;


		vb::VertexBufferLayoutHandle animatedVertexModelHandle = vbsys::AddVertexBufferLayout(*renderer.mVertexBufferLayoutSystem, layoutConfig);

		r2::sarr::Push(*renderer.mVertexBufferLayoutHandles, animatedVertexModelHandle);

		renderer.mAnimVertexModelConfigHandle = animatedVertexModelHandle;

		return renderer.mAnimVertexModelConfigHandle;
	}
#ifdef R2_DEBUG
	vb::VertexBufferLayoutHandle AddDebugDrawLayout(Renderer& renderer)
	{
		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return vb::InvalidVertexBufferLayoutHandle;
		}

		if (renderer.mDebugLinesVertexConfigHandle != vb::InvalidVertexBufferLayoutHandle)
		{
			R2_CHECK(false, "We have already added the debug vertex layout!");
			return vb::InvalidVertexBufferLayoutHandle;
		}

		r2::draw::BufferLayoutConfiguration layoutConfig;

		layoutConfig.layout = BufferLayout(
			{
				{r2::draw::ShaderDataType::Float3, "aPos", 0}
			}
		);

		layoutConfig.vertexBufferConfigs[0] =
		{
			(u32)Megabytes(8),
			r2::draw::VertexDrawTypeStatic
		};

		layoutConfig.indexBufferConfig =
		{
			EMPTY_BUFFER,
			r2::draw::VertexDrawTypeStatic
		};

		layoutConfig.useDrawIDs = true;
		layoutConfig.maxDrawCount = MAX_NUM_DRAWS;
		layoutConfig.numVertexConfigs = 1;


		vb::VertexBufferLayoutHandle debugVertexModelHandle = vbsys::AddVertexBufferLayout(*renderer.mVertexBufferLayoutSystem, layoutConfig);

		r2::sarr::Push(*renderer.mVertexBufferLayoutHandles, debugVertexModelHandle);

		renderer.mDebugLinesVertexConfigHandle = debugVertexModelHandle;

		return renderer.mDebugLinesVertexConfigHandle;
	}
#endif

	ConstantConfigHandle AddConstantBufferLayout(Renderer& renderer, ConstantBufferLayout::Type type, const std::initializer_list<ConstantBufferElement>& elements)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		ConstantBufferFlags flags = 0;
		CreateConstantBufferFlags createFlags = 0;
		if (type > ConstantBufferLayout::Type::Small)
		{
			flags = r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT | CB_FLAG_MAP_COHERENT;
			createFlags = r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE;
		}

		r2::draw::ConstantBufferLayoutConfiguration constConfig =
		{
			{
				type,
				flags,
				createFlags,
				elements
			},
			 r2::draw::VertexDrawTypeDynamic
		};
		
	

		r2::sarr::Push(*renderer.mConstantLayouts, constConfig);

		return r2::sarr::Size(*renderer.mConstantLayouts) - 1;
	}

	ConstantConfigHandle AddMaterialLayout(Renderer& renderer)
	{

		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration materials
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		materials.layout.InitForMaterials(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT | CB_FLAG_MAP_COHERENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_DRAWS);

		r2::sarr::Push(*renderer.mConstantLayouts, materials);

		renderer.mMaterialConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mMaterialConfigHandle;
	}

	ConstantConfigHandle AddModelsLayout(Renderer& renderer, ConstantBufferLayout::Type type)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		renderer.mModelConfigHandle = AddConstantBufferLayout(renderer, type, {
			{
				r2::draw::ShaderDataType::Mat4, "models", MAX_NUM_DRAWS}
			});

		r2::sarr::At(*renderer.mConstantLayouts, renderer.mModelConfigHandle).layout.SetBufferMult(ConstantBufferLayout::RING_BUFFER_MULTIPLIER);

		return renderer.mModelConfigHandle;
	}

	ConstantConfigHandle AddSubCommandsLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration subCommands
		{
			//layout
			{},
			r2::draw::VertexDrawTypeDynamic
		};

		subCommands.layout.InitForSubCommands(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT | CB_FLAG_MAP_COHERENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_DRAWS);

		r2::sarr::Push(*renderer.mConstantLayouts, subCommands);

		renderer.mSubcommandsConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mSubcommandsConfigHandle;
	}


	ConstantConfigHandle AddDebugLineSubCommandsLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration subCommands
		{
			//layout
			{},
			r2::draw::VertexDrawTypeDynamic
		};

		subCommands.layout.InitForDebugSubCommands(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT | CB_FLAG_MAP_COHERENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_DRAWS);

		r2::sarr::Push(*renderer.mConstantLayouts, subCommands);

		renderer.mDebugLinesSubCommandsConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mDebugLinesSubCommandsConfigHandle;
	}

	ConstantConfigHandle AddDebugModelSubCommandsLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration subCommands
		{
			//layout
			{},
			r2::draw::VertexDrawTypeDynamic
		};

		subCommands.layout.InitForSubCommands(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT | CB_FLAG_MAP_COHERENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_DRAWS);

		r2::sarr::Push(*renderer.mConstantLayouts, subCommands);

		renderer.mDebugModelSubCommandsConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mDebugModelSubCommandsConfigHandle;

	}

	ConstantConfigHandle AddDebugColorsLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration debugRenderConstantsLayout
		{
			//layout
			{
			},
			r2::draw::VertexDrawTypeDynamic
		};

		debugRenderConstantsLayout.layout.InitForDebugRenderConstants(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT | CB_FLAG_MAP_COHERENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_DRAWS * 2);

		r2::sarr::Push(*renderer.mConstantLayouts, debugRenderConstantsLayout);

		renderer.mDebugRenderConstantsConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mDebugRenderConstantsConfigHandle;
	}

	ConstantConfigHandle AddBoneTransformsLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration boneTransforms
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		boneTransforms.layout.InitForBoneTransforms(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT | CB_FLAG_MAP_COHERENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_BONES);

		r2::sarr::Push(*renderer.mConstantLayouts, boneTransforms);

		renderer.mBoneTransformsConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;
		return renderer.mBoneTransformsConfigHandle;
	}

	ConstantConfigHandle AddBoneTransformOffsetsLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration boneTransformOffsets
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		boneTransformOffsets.layout.InitForBoneTransformOffsets(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT | CB_FLAG_MAP_COHERENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_BONES);

		r2::sarr::Push(*renderer.mConstantLayouts, boneTransformOffsets);

		renderer.mBoneTransformOffsetsConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mBoneTransformOffsetsConfigHandle;
	}

	ConstantConfigHandle AddLightingLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration lighting
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		lighting.layout.InitForLighting();

		r2::sarr::Push(*renderer.mConstantLayouts, lighting);

		renderer.mLightingConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mLightingConfigHandle;
	}

	ConstantConfigHandle AddShadowDataLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration shadowData
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		shadowData.layout.InitForShadowData();

		r2::sarr::Push(*renderer.mConstantLayouts, shadowData);

		renderer.mShadowDataConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mShadowDataConfigHandle;
	}

	ConstantConfigHandle AddMaterialOffsetsLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration materialOffsets
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		materialOffsets.layout.InitForMaterialOffsets(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT | CB_FLAG_MAP_COHERENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_DRAWS);

		r2::sarr::Push(*renderer.mConstantLayouts, materialOffsets);

		renderer.mMaterialOffsetsConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mMaterialOffsetsConfigHandle;
	}

	ConstantConfigHandle AddClusterVolumesLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration clusterVolumes
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		clusterVolumes.layout.InitForClusterAABBs(
			r2::draw::CB_FLAG_WRITE | CB_FLAG_MAP_COHERENT, 0,
			r2::util::NextPowerOfTwo64Bit(renderer.mShaderVectors.clusterTileSizes.x * renderer.mShaderVectors.clusterTileSizes.y * renderer.mShaderVectors.clusterTileSizes.z));

		r2::sarr::Push(*renderer.mConstantLayouts, clusterVolumes);

		renderer.mClusterVolumesConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mClusterVolumesConfigHandle;
	}

	ConstantConfigHandle AddDispatchComputeIndirectLayout(Renderer& renderer)
	{
		const u32 MAX_DISPATCH_COMPUTE_INDIRECT_CALLS = 16;
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration dispatchComputeConfig
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		dispatchComputeConfig.layout.InitForDispatchComputeIndirect(
			r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_COHERENT,
			0,
			MAX_DISPATCH_COMPUTE_INDIRECT_CALLS);

		r2::sarr::Push(*renderer.mConstantLayouts, dispatchComputeConfig);

		renderer.mDispatchComputeConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mDispatchComputeConfigHandle;
	}

	ConstantConfigHandle AddMeshDataLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration meshData
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		meshData.layout.InitForMeshData(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT | CB_FLAG_MAP_COHERENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, sizeof(glm::mat4)*2, MAX_NUM_DRAWS);

		r2::sarr::Push(*renderer.mConstantLayouts, meshData);

		renderer.mMeshDataConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mMeshDataConfigHandle;
	}

	ConstantConfigHandle AddSurfacesLayout(Renderer& renderer)
	{
		if (renderer.mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		r2::draw::ConstantBufferLayoutConfiguration surfaces
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		surfaces.layout.InitForSurfaces(renderer.mRenderTargetParams);

		r2::sarr::Push(*renderer.mConstantLayouts, surfaces);

		renderer.mSurfacesConfigHandle = r2::sarr::Size(*renderer.mConstantLayouts) - 1;

		return renderer.mSurfacesConfigHandle;

	}

	u64 MemorySize(u64 renderTargetsMemorySize)
	{
		u32 boundsChecking = 0;
#if defined(R2_DEBUG ) || defined(R2_RELEASE)
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();

		u64 memorySize =
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::draw::Renderer), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::CommandBucket<r2::draw::key::Basic>::MemorySize(COMMAND_CAPACITY), ALIGNMENT, headerSize, boundsChecking) * 6 +
#ifdef R2_EDITOR
			r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::CommandBucket<r2::draw::key::Basic>::MemorySize(COMMAND_CAPACITY), ALIGNMENT, headerSize, boundsChecking) +
#endif // R2_EDITOR


			r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::CommandBucket<r2::draw::key::ShadowKey>::MemorySize(COMMAND_CAPACITY), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::CommandBucket<r2::draw::key::DepthKey>::MemorySize(COMMAND_CAPACITY), ALIGNMENT, headerSize, boundsChecking) * 5 +
			
			renderTargetsMemorySize +

			LightSystem::MemorySize(ALIGNMENT, headerSize, boundsChecking) +

			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray< vb::VertexBufferLayoutHandle>::MemorySize(NUM_VERTEX_BUFFER_LAYOUT_TYPES), ALIGNMENT, headerSize, boundsChecking) +

			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ConstantBufferHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ConstantBufferLayoutConfiguration>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<ConstantBufferData>::MemorySize(MAX_BUFFER_LAYOUTS * r2::SHashMap<ConstantBufferData>::LoadFactorMultiplier()), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(RenderPass), ALIGNMENT, headerSize, boundsChecking) * NUM_RENDER_PASSES +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking) +

			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(cmd::LargestCommand(), ALIGNMENT, stackHeaderSize, boundsChecking) * COMMAND_CAPACITY * 4 +
			r2::mem::utils::GetMaxMemoryForAllocation(COMMAND_AUX_MEMORY / 4, ALIGNMENT, headerSize, boundsChecking) +

			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(cmd::LargestCommand(), ALIGNMENT, stackHeaderSize, boundsChecking) * COMMAND_CAPACITY * 2 +
			r2::mem::utils::GetMaxMemoryForAllocation(COMMAND_AUX_MEMORY / 2, ALIGNMENT, headerSize, boundsChecking) +

			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(cmd::LargestCommand(), ALIGNMENT, stackHeaderSize, boundsChecking) * COMMAND_CAPACITY * 2 +
			r2::mem::utils::GetMaxMemoryForAllocation(COMMAND_AUX_MEMORY / 4, ALIGNMENT, headerSize, boundsChecking) +

#ifdef R2_EDITOR
			r2::mem::utils::GetMaxMemoryForAllocation(cmd::LargestCommand(), ALIGNMENT, stackHeaderSize, boundsChecking) * COMMAND_CAPACITY  +
#endif

			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(cmd::LargestCommand(), ALIGNMENT, stackHeaderSize, boundsChecking) * COMMAND_CAPACITY *3 + //@TODO(Serge): this is a wild over-estimate - only need 2 DrawBatch commands

			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(PRE_RENDER_STACK_ARENA_SIZE, ALIGNMENT, headerSize, boundsChecking) +

			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<RenderMaterialParams>::MemorySize(NUM_RENDER_MATERIALS_TO_RENDER), ALIGNMENT, headerSize, boundsChecking) +

			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ModelHandle>::MemorySize(MAX_DEFAULT_MODELS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<vb::GPUModelRefHandle>::MemorySize(NUM_DEFAULT_MODELS), ALIGNMENT, headerSize, boundsChecking) +

			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<RenderBatch>::MemorySize(DrawType::NUM_DRAW_TYPES), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(RenderBatch::MemorySize(MAX_NUM_DRAWS, MAX_NUM_DRAWS, MAX_NUMBER_OF_MODELS_LOADED_AT_ONE_TIME * AVG_NUM_OF_MESHES_PER_MODEL, MAX_NUM_BONES, ALIGNMENT, headerSize, boundsChecking), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(RenderBatch::MemorySize(MAX_NUM_DRAWS, MAX_NUM_DRAWS, MAX_NUMBER_OF_MODELS_LOADED_AT_ONE_TIME * AVG_NUM_OF_MESHES_PER_MODEL, 0, ALIGNMENT, headerSize, boundsChecking), ALIGNMENT, headerSize, boundsChecking) * (NUM_DRAW_TYPES - 1)

#ifdef R2_DEBUG
			+ r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<DebugRenderBatch>::MemorySize(DebugDrawType::NUM_DEBUG_DRAW_TYPES), ALIGNMENT, headerSize, boundsChecking) * 2

			+ r2::mem::utils::GetMaxMemoryForAllocation(DebugRenderBatch::MemorySize(MAX_NUM_DRAWS, true, MAX_NUM_DRAWS, ALIGNMENT, headerSize, boundsChecking), ALIGNMENT, headerSize, boundsChecking)
			+ r2::mem::utils::GetMaxMemoryForAllocation(DebugRenderBatch::MemorySize(MAX_NUM_DRAWS, false, MAX_NUM_DRAWS, ALIGNMENT, headerSize, boundsChecking), ALIGNMENT, headerSize, boundsChecking)
			+ r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::CommandBucket<r2::draw::key::DebugKey>::MemorySize(COMMAND_CAPACITY), ALIGNMENT, headerSize, boundsChecking) * 3
			+ r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking)
			+ r2::mem::utils::GetMaxMemoryForAllocation(cmd::LargestCommand(), ALIGNMENT, stackHeaderSize, boundsChecking) * COMMAND_CAPACITY 
			+ r2::mem::utils::GetMaxMemoryForAllocation(DEBUG_COMMAND_AUX_MEMORY, ALIGNMENT, headerSize, boundsChecking)


			+ r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<BatchRenderOffsets>::MemorySize(MAX_NUM_DRAWS), ALIGNMENT, headerSize, boundsChecking)
			+ r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<cmd::DrawDebugBatchSubCommand>::MemorySize(MAX_NUM_DRAWS), ALIGNMENT, headerSize, boundsChecking)
			+ r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<BatchRenderOffsets>::MemorySize(MAX_NUM_DRAWS), ALIGNMENT, headerSize, boundsChecking)

			+ r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<cmd::DrawBatchSubCommand>::MemorySize(MAX_NUM_DRAWS), ALIGNMENT, headerSize, boundsChecking)
			//r2::SArray<DebugVertex>* mDebugVertices = nullptr;
			+ r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<DebugVertex>::MemorySize(MAX_NUM_DRAWS*2), ALIGNMENT, headerSize, boundsChecking)
#endif
			; //end of sizes

		return r2::mem::utils::GetMaxMemoryForAllocation(memorySize, ALIGNMENT);
	}

	const r2::SArray<ConstantBufferHandle>* GetConstantBufferHandles(Renderer& renderer)
	{
		return renderer.mConstantBufferHandles;
	}

	template<typename T,  class CMD, class ARENA>
	CMD* AddCommand(ARENA& arena, r2::draw::CommandBucket<T>& bucket, T key, u64 auxMemory)
	{
		return r2::draw::cmdbkt::AddCommand<T, r2::mem::StackArena, CMD>(arena, bucket, key, auxMemory);
	}

	template<class CMDTOAPPENDTO, class CMD, class ARENA>
	CMD* AppendCommand(ARENA& arena, CMDTOAPPENDTO* cmdToAppendTo, u64 auxMemory)
	{
		return r2::draw::cmdbkt::AppendCommand<CMDTOAPPENDTO, CMD, r2::mem::StackArena>(arena, cmdToAppendTo, auxMemory);
	}

	const r2::draw::Model* GetDefaultModel(Renderer& renderer, r2::draw::DefaultModel defaultModel)
	{
		if (renderer.mModelCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		auto modelHandle = r2::sarr::At(*renderer.mDefaultModelHandles, defaultModel);
		return modlche::GetModel(renderer.mModelCache, modelHandle);
	}

	const r2::SArray<vb::GPUModelRefHandle>* GetDefaultModelRefs(Renderer& renderer)
	{
		if (renderer.mEngineModelRefs == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		return renderer.mEngineModelRefs;
	}

	vb::GPUModelRefHandle GetDefaultModelRef(Renderer& renderer, r2::draw::DefaultModel defaultModel)
	{
		if (renderer.mEngineModelRefs == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return vb::InvalidGPUModelRefHandle;
		}

		return r2::sarr::At(*renderer.mEngineModelRefs, defaultModel);
	}

	vb::GPUModelRefHandle GetModelRefHandleForModelAssetName(Renderer& renderer, const r2::asset::AssetName& modelAssetName)
	{
		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return vb::InvalidGPUModelRefHandle;
		}

		return vbsys::GetModelRefHandle(*renderer.mVertexBufferLayoutSystem, modelAssetName);
	}

	const RenderMaterialParams& GetMissingTextureRenderMaterialParam(Renderer& renderer)
	{
		return renderer.mMissingTextureRenderMaterialParams;
	}

	const tex::Texture* GetMissingTexture(Renderer* renderer)
	{
		return &renderer->mMissingTexture;
	}

	const RenderMaterialParams& GetBlueNoise64TextureMaterialParam(Renderer& renderer)
	{
		return renderer.mBlueNoiseRenderMaterialParams;
	}

	const tex::Texture* GetBlueNoise64Texture(Renderer* renderer)
	{
		return &renderer->mBlueNoiseTexture;
	}

	r2::mat::MaterialName GetDefaultOutlineMaterialName(Renderer& renderer, bool alphaDiscard)
	{
		if (alphaDiscard)
		{
			return renderer.mDefaultOutlineAlphaDiscardMaterialName;
		}

		return renderer.mDefaultOutlineMaterialName;
	}

	void UploadEngineModels(Renderer& renderer)
	{
		const r2::draw::Model* quadModel = GetDefaultModel(renderer, r2::draw::QUAD);
		const r2::draw::Model* sphereModel = GetDefaultModel(renderer, r2::draw::SPHERE);
		const r2::draw::Model* cubeModel = GetDefaultModel(renderer, r2::draw::CUBE);
		const r2::draw::Model* cylinderModel = GetDefaultModel(renderer, r2::draw::CYLINDER);
		const r2::draw::Model* coneModel = GetDefaultModel(renderer, r2::draw::CONE);
		const r2::draw::Model* fullScreenTriangleModel = GetDefaultModel(renderer, r2::draw::FULLSCREEN_TRIANGLE);

		r2::SArray<const r2::draw::Model*>* modelsToUpload = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const r2::draw::Model*, MAX_DEFAULT_MODELS);
		r2::sarr::Push(*modelsToUpload, quadModel);
		r2::sarr::Push(*modelsToUpload, cubeModel);
		r2::sarr::Push(*modelsToUpload, sphereModel);
		r2::sarr::Push(*modelsToUpload, coneModel);
		r2::sarr::Push(*modelsToUpload, cylinderModel);
		r2::sarr::Push(*modelsToUpload, fullScreenTriangleModel);

		UploadModels(renderer, *modelsToUpload, *renderer.mEngineModelRefs);

		FREE(modelsToUpload, *MEM_ENG_SCRATCH_PTR);


		//r2::sarr::Push(*modelsToUpload, fullScreenTriangleModel);
		 

	//	

		//@NOTE: because we can now re-use meshes for other models, we can re-use the CUBE mesh for the SKYBOX model
		//r2::sarr::Push(*renderer.mEngineModelRefs, GetDefaultModelRef(renderer, CUBE));
		//r2::sarr::Push(*s_optrRenderer->mEngineModelRefs, UploadModel(fullScreenTriangleModel));
	//	r2::sarr::Push(*s_optrRenderer->mEngineModelRefs, GetDefaultModelRef(QUAD));
	}

	vb::GPUModelRefHandle UploadModel(Renderer& renderer, const Model* model)
	{
		if (!renderer.mVertexBufferLayoutSystem || !renderer.mVertexBufferLayoutHandles)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return vb::InvalidGPUModelRefHandle;
		}

		vb::GPUModelRefHandle cachedHandle = vbsys::GetModelRefHandle(*renderer.mVertexBufferLayoutSystem, *model);

		if (vbsys::IsModelRefHandleValid(*renderer.mVertexBufferLayoutSystem, cachedHandle))
		{
			return cachedHandle;
		}

		vb::VertexBufferLayoutHandle configHandleToUse = renderer.mStaticVertexModelConfigHandle;

		if (model->optrBoneData > 0)
		{
			configHandleToUse = renderer.mAnimVertexModelConfigHandle;
		}

		vb::GPUModelRefHandle result = vbsys::UploadModelToVertexBuffer(*renderer.mVertexBufferLayoutSystem, configHandleToUse, *model, renderer.mPreRenderBucket, renderer.mPrePostRenderCommandArena);

		vb::GPUModelRef* modelRef = vbsys::GetGPUModelRefPtr(*renderer.mVertexBufferLayoutSystem, result);

		R2_CHECK(modelRef != nullptr, "?");
		//now resolve the material name
						
		const auto numMaterialNames = r2::sarr::Size(*model->optrMaterialNames);

		r2::sarr::Append(*modelRef->materialNames, *model->optrMaterialNames);
		
		return result;
	}

	void UploadModels(Renderer& renderer, const r2::SArray<const Model*>& models, r2::SArray<vb::GPUModelRefHandle>& modelRefs)
	{
		if (!renderer.mVertexBufferLayoutSystem || !renderer.mVertexBufferLayoutHandles)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}
		
		if (r2::sarr::IsEmpty(models))
		{
			return;
		}

		//built in limitation - all models should be either animated or static 
		bool isAnimated = r2::sarr::At(models, 0)->optrBoneData > 0;
#ifdef R2_DEBUG
		//@NOTE: verify that all models are one or the other 
		const auto numModelsToCheck = r2::sarr::Size(models);

		for (u32 i = 1; i < numModelsToCheck; ++i)
		{
			bool animatedModel = r2::sarr::At(models, i)->optrBoneData > 0;
			R2_CHECK(animatedModel == isAnimated, "These always need to be the same!");
		}
#endif

		//this one is a bit weird now since we need all of them to be not loaded or all to be loaded
		vb::GPUModelRefHandle cachedHandle = vbsys::GetModelRefHandle(*renderer.mVertexBufferLayoutSystem, *r2::sarr::At(models, 0));
		if (vbsys::IsModelRefHandleValid(*renderer.mVertexBufferLayoutSystem, cachedHandle))
		{
			r2::sarr::Push(modelRefs, cachedHandle);
			for (u32 i = 1; i < r2::sarr::Size(models); ++i)
			{
				cachedHandle = vbsys::GetModelRefHandle(*renderer.mVertexBufferLayoutSystem, *r2::sarr::At(models, i));
				R2_CHECK(vbsys::IsModelRefHandleValid(*renderer.mVertexBufferLayoutSystem, cachedHandle), "This must be the case now");

				r2::sarr::Push(modelRefs, cachedHandle);
			}

			return;
		}

		const auto startingModelRefOffset = r2::sarr::Size(modelRefs);

		vb::VertexBufferLayoutHandle configHandleToUse = renderer.mStaticVertexModelConfigHandle;
		if (isAnimated)
		{
			configHandleToUse = renderer.mAnimVertexModelConfigHandle;
		}

		vbsys::UploadAllModels(*renderer.mVertexBufferLayoutSystem, configHandleToUse, models, modelRefs, renderer.mPreRenderBucket, renderer.mPrePostRenderCommandArena);
		
		const auto numModelRefs = r2::sarr::Size(modelRefs);

		for (u32 i = startingModelRefOffset; i < numModelRefs; ++i)
		{
			vb::GPUModelRefHandle result = r2::sarr::At(modelRefs, i);

			vb::GPUModelRef* modelRef = vbsys::GetGPUModelRefPtr(*renderer.mVertexBufferLayoutSystem, result);

			const Model* model = r2::sarr::At(models, i - startingModelRefOffset);

			const auto numMaterialNames = r2::sarr::Size(*model->optrMaterialNames);

			r2::sarr::Append(*modelRef->materialNames, *model->optrMaterialNames);
		}
	}

	void UnloadModel(Renderer& renderer, const vb::GPUModelRefHandle& modelRefHandle)
	{
		if (!renderer.mVertexBufferLayoutSystem || !renderer.mVertexBufferLayoutHandles)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		bool unloaded = vbsys::UnloadModelFromVertexBuffer(*renderer.mVertexBufferLayoutSystem, modelRefHandle);

		R2_CHECK(unloaded, "Failed to unload: %ll", modelRefHandle);
	}

	void UnloadStaticModelRefHandles(Renderer& renderer, const r2::SArray<vb::GPUModelRefHandle>* handles)
	{
		if (!renderer.mVertexBufferLayoutSystem || !renderer.mVertexBufferLayoutHandles)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		if (!handles)
		{
			R2_CHECK(false, "Passed in an empty handles array");
			return;
		}

		bool unloaded = vbsys::UnloadAllModelRefHandles(*renderer.mVertexBufferLayoutSystem, renderer.mStaticVertexModelConfigHandle, handles);

		R2_CHECK(unloaded, "Failed to unload model ref handles from static vertex buffer layout");
	}

	void UnloadAnimModelRefHandles(Renderer& renderer, const r2::SArray<vb::GPUModelRefHandle>* handles)
	{
		if (!renderer.mVertexBufferLayoutSystem || !renderer.mVertexBufferLayoutHandles)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		bool unloaded = vbsys::UnloadAllModelRefHandles(*renderer.mVertexBufferLayoutSystem, renderer.mAnimVertexModelConfigHandle, handles);

		R2_CHECK(unloaded, "Failed to unload model ref handles from anim vertex buffer layout");
	}

	void UnloadAllStaticModels(Renderer& renderer)
	{
		if (!renderer.mVertexBufferLayoutSystem || !renderer.mVertexBufferLayoutHandles)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		R2_CHECK(false, "We probably don't want to do this since this will unload the default models - leaving the option open for later when we move the default models somewhere else");

		bool unloaded = vbsys::UnloadAllModelsFromVertexBuffer(*renderer.mVertexBufferLayoutSystem, renderer.mStaticVertexModelConfigHandle);

		R2_CHECK(unloaded, "Failed to unload model ref handles from static vertex buffer layout");
	}

	void UnloadAllAnimModels(Renderer& renderer)
	{
		if (!renderer.mVertexBufferLayoutSystem || !renderer.mVertexBufferLayoutHandles)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		bool unloaded = vbsys::UnloadAllModelsFromVertexBuffer(*renderer.mVertexBufferLayoutSystem, renderer.mAnimVertexModelConfigHandle);

		R2_CHECK(unloaded, "Failed to unload model ref handles from static vertex buffer layout");
	}

	bool IsModelLoaded(const Renderer& renderer, const vb::GPUModelRefHandle& modelRefHandle)
	{
		if (!renderer.mVertexBufferLayoutSystem || !renderer.mVertexBufferLayoutHandles)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return false;
		}

		return vbsys::IsModelRefHandleValid(*renderer.mVertexBufferLayoutSystem, modelRefHandle);
	}

	bool IsModelRefHandleValid(const Renderer& renderer, const vb::GPUModelRefHandle& modelRefHandle)
	{
		if (!renderer.mVertexBufferLayoutSystem || !renderer.mVertexBufferLayoutHandles)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return false;
		}

		return vbsys::IsModelRefHandleValid(*renderer.mVertexBufferLayoutSystem, modelRefHandle);
	}

	r2::draw::RenderMaterialCache* GetRenderMaterialCache(Renderer& renderer)
	{
		return renderer.mRenderMaterialCache;
	}

	void AddFillConstantBufferCommandFull(Renderer& renderer, ConstantBufferHandle handle, const void* data, u64 size, u64 offset)
	{
		r2::draw::key::Basic fillKey;
		//@TODO(Serge): fix this or pass it in
		fillKey.keyValue = 0;

		ConstantBufferData* constBufferData = GetConstData(renderer, handle);

		R2_CHECK(constBufferData != nullptr, "We couldn't find the constant buffer handle!");

		u64 numConstantBufferHandles = r2::sarr::Size(*renderer.mConstantBufferHandles);
		u64 constBufferIndex = 0;
		bool found = false;
		for (; constBufferIndex < numConstantBufferHandles; ++constBufferIndex)
		{
			if (handle == r2::sarr::At(*renderer.mConstantBufferHandles, constBufferIndex))
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			R2_CHECK(false, "Couldn't find the constant buffer so we can upload the data");
			return;
		}

		const ConstantBufferLayoutConfiguration& config = r2::sarr::At(*renderer.mConstantLayouts, constBufferIndex);
		
		r2::draw::cmd::FillConstantBuffer* fillConstantBufferCommand = cmdbkt::AddCommand<key::Basic, mem::StackArena, cmd::FillConstantBuffer>(*renderer.mPrePostRenderCommandArena, *renderer.mPreRenderBucket, fillKey, size);

		r2::draw::cmd::FillConstantBufferCommand(fillConstantBufferCommand, handle, constBufferData->type, constBufferData->isPersistent, data, size, offset);
	}

	u64 AddFillConstantBufferCommandForData(Renderer& renderer, ConstantBufferHandle handle, u64 elementIndex, const void* data)
	{
		r2::draw::key::Basic fillKey;
		//@TODO(Serge): fix this or pass it in
		fillKey.keyValue = 0;

		ConstantBufferData* constBufferData = GetConstData(renderer, handle);

		R2_CHECK(constBufferData != nullptr, "We couldn't find the constant buffer handle!");

		u64 numConstantBufferHandles = r2::sarr::Size(*renderer.mConstantBufferHandles);
		u64 constBufferIndex = 0;
		bool found = false;
		for (; constBufferIndex < numConstantBufferHandles; ++constBufferIndex)
		{
			if (handle == r2::sarr::At(*renderer.mConstantBufferHandles, constBufferIndex))
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			R2_CHECK(false, "Couldn't find the constant buffer so we can upload the data");
			return 0;
		}
		
		const ConstantBufferLayoutConfiguration& config = r2::sarr::At(*renderer.mConstantLayouts, constBufferIndex);

		r2::draw::cmd::FillConstantBuffer* fillConstantBufferCommand = cmdbkt::AddCommand<key::Basic, mem::StackArena, cmd::FillConstantBuffer>(*renderer.mPrePostRenderCommandArena, *renderer.mPreRenderBucket, fillKey, config.layout.GetElements().at(elementIndex).size);
		
		return r2::draw::cmd::FillConstantBufferCommand(fillConstantBufferCommand, handle, constBufferData->type, constBufferData->isPersistent, data, config.layout.GetElements().at(elementIndex).size, config.layout.GetElements().at(elementIndex).offset);
	}

	u64 AddZeroConstantBufferCommand(Renderer& renderer, ConstantBufferHandle handle, u64 elementIndex)
	{
		r2::draw::key::Basic fillKey;
		//@TODO(Serge): fix this or pass it in
		fillKey.keyValue = 0;

		ConstantBufferData* constBufferData = GetConstData(renderer, handle);

		R2_CHECK(constBufferData != nullptr, "We couldn't find the constant buffer handle!");

		u64 numConstantBufferHandles = r2::sarr::Size(*renderer.mConstantBufferHandles);
		u64 constBufferIndex = 0;
		bool found = false;
		for (; constBufferIndex < numConstantBufferHandles; ++constBufferIndex)
		{
			if (handle == r2::sarr::At(*renderer.mConstantBufferHandles, constBufferIndex))
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			R2_CHECK(false, "Couldn't find the constant buffer so we can upload the data");
			return 0;
		}

		const ConstantBufferLayoutConfiguration& config = r2::sarr::At(*renderer.mConstantLayouts, constBufferIndex);
		r2::draw::cmd::FillConstantBuffer* fillConstantBufferCommand = cmdbkt::AddCommand<key::Basic, mem::StackArena, cmd::FillConstantBuffer>(*renderer.mPrePostRenderCommandArena, *renderer.mPreRenderBucket, fillKey, config.layout.GetElements().at(elementIndex).size);
		return cmd::ZeroConstantBufferCommand(fillConstantBufferCommand, handle, constBufferData->type, constBufferData->isPersistent, config.layout.GetElements().at(elementIndex).size, config.layout.GetElements().at(elementIndex).offset);

	}

	void UpdateSceneLighting(Renderer& renderer, const r2::draw::LightSystem& lightSystem)
	{

		ConstantBufferHandle lightBufferHandle = r2::sarr::At(*renderer.mConstantBufferHandles, renderer.mLightingConfigHandle);

		AddFillConstantBufferCommandFull(renderer, lightBufferHandle, &lightSystem.mSceneLighting, sizeof(lightSystem.mSceneLighting), 0);
	}

	template<class ARENA, typename T>
	r2::draw::cmd::FillConstantBuffer* AddFillConstantBufferCommand(ARENA& arena, CommandBucket<T>& bucket, T key, u64 auxMemory)
	{
		return r2::draw::renderer::AddCommand<T, r2::draw::cmd::FillConstantBuffer, ARENA>(arena, bucket, key, auxMemory);
	}

	ConstantBufferData* GetConstData(Renderer& renderer, ConstantBufferHandle handle)
	{
		ConstantBufferData defaultConstBufferData;
		ConstantBufferData& constData = r2::shashmap::Get(*renderer.mConstantBufferData, handle, defaultConstBufferData);

		if (constData.handle == EMPTY_BUFFER)
		{
			R2_CHECK(false, "We didn't find the constant buffer data associated with this handle: %u", handle);
			
			return nullptr;
		}

		return &constData;
	}

	ConstantBufferData* GetConstDataByConfigHandle(Renderer& renderer, ConstantConfigHandle handle)
	{
		auto constantBufferHandles = GetConstantBufferHandles(renderer);

		return GetConstData(renderer, r2::sarr::At(*constantBufferHandles, handle));
	}
	
	struct CameraDepth
	{
		u32 cameraDepth;
		u32 index;
	};

	struct DrawCommandData
	{
		//@TODO(Serge): Should this be the forward pass' shader? or should this be ShaderEffectPasses?
		ShaderEffectPasses shaderEffectPasses;
		cmd::DrawState drawState;
		b32 isDynamic = false;
		r2::SArray<cmd::DrawBatchSubCommand>* subCommands = nullptr;
		r2::SArray<CameraDepth>* cameraDepths = nullptr;
	};



	void PopulateRenderDataFromRenderBatch(
		Renderer& renderer,
		r2::SArray<void*>* tempAllocations,
		const RenderBatch& renderBatch,
		r2::SHashMap<DrawCommandData*>* shaderDrawCommandData,
		r2::SArray<RenderMaterialParams>* renderMaterials,
		r2::SArray<glm::uvec4>* materialOffsetsPerObject,
		u32& materialOffset,
		u32 baseInstanceOffset,
		u32 drawCommandBatchSize, 
		u32& meshOffset)
	{
		const u64 numModels = r2::sarr::Size(*renderBatch.gpuModelRefs);
		u32 numModelInstances = 0;

		for (u64 modelIndex = 0; modelIndex < numModels; ++modelIndex)
		{
			const vb::GPUModelRef* modelRef = r2::sarr::At(*renderBatch.gpuModelRefs, modelIndex);
			const cmd::DrawState& drawState = r2::sarr::At(*renderBatch.drawState, modelIndex);
			u32 drawStateHash = utils::HashBytes32(&drawState, sizeof(drawState));

			const glm::mat4& modelMatrix = r2::sarr::At(*renderBatch.models, modelIndex);
			const u32 numMeshRefs = r2::sarr::Size(*modelRef->meshEntries);
			const u32 numInstances = r2::sarr::At(*renderBatch.numInstances, modelIndex);

			const MaterialBatch::Info& materialBatchInfo = r2::sarr::At(*renderBatch.materialBatch.infos, modelIndex);

			//@TODO(Serge): somehow make this fast for each mesh - dunno how to do that right now
			u32 cameraDepthToModel = GetCameraDepth(renderer, {}, modelMatrix);

			u32 entityID = 0;
			EntityInstanceBatchOffset entityInstanceBatchOffset;
#ifdef R2_EDITOR
			if (!r2::sarr::IsEmpty(*renderBatch.entityIDs))
			{
				entityID = r2::sarr::At(*renderBatch.entityIDs, modelIndex);
				entityInstanceBatchOffset = r2::sarr::At(*renderBatch.entityInstanceOffsetBatches, modelIndex);
			}
#endif

			for (u32 i = 0; i < numInstances; i++)
			{
				u32 entityInstance = 0;
#ifdef R2_EDITOR
				if (entityID != 0 && entityInstanceBatchOffset.start >= 0)
				{
					R2_CHECK(numInstances == entityInstanceBatchOffset.numInstances, "Should always be the case");
					entityInstance = r2::sarr::At(*renderBatch.entityInstances, entityInstanceBatchOffset.start + i);
				}
#endif
				r2::sarr::Push(*materialOffsetsPerObject, glm::uvec4(materialOffset, entityID, entityInstance, meshOffset));
			}
			
			materialOffset += materialBatchInfo.numMaterials;

			//@NOTE(Serge): for engine Models (non-gltf models) we should also include a mesh 
			if (modelRef->numGLTFMeshes == 0)
			{
				meshOffset += 1;
			}
			else
			{
				meshOffset += modelRef->numGLTFMeshes;
			}

			for (u32 meshRefIndex = 0; meshRefIndex < numMeshRefs; ++meshRefIndex)
			{
				const vb::MeshEntry& meshRef = r2::sarr::At(*modelRef->meshEntries, meshRefIndex);
				
				const ShaderEffectPasses& shaderEffectPasses = r2::sarr::At(*renderBatch.materialBatch.shaderEffectPasses, meshRef.materialIndex + materialBatchInfo.start); 

				//Here we're just going to base it on which layer / mesh pass we care about
				ShaderHandle shaderHandleToUse = r2::draw::InvalidShader;
				ShaderHandle depthShaderHandleToUse = r2::draw::InvalidShader;

				DrawLayer drawLayerToUse = drawState.layer;

				flat::eMeshPass meshPass = meshPass = flat::eMeshPass_FORWARD;
				if (drawState.layer == DL_TRANSPARENT)
				{
					meshPass = flat::eMeshPass_TRANSPARENT;
				}
				
				shaderHandleToUse = modelRef->isAnimated ? shaderEffectPasses.meshPasses[meshPass].dynamicShaderHandle : shaderEffectPasses.meshPasses[meshPass].staticShaderHandle;
				
				if (shaderHandleToUse == 0 && meshPass != flat::eMeshPass_TRANSPARENT)
				{
					shaderHandleToUse = modelRef->isAnimated ? shaderEffectPasses.meshPasses[flat::eMeshPass_TRANSPARENT].dynamicShaderHandle : shaderEffectPasses.meshPasses[flat::eMeshPass_TRANSPARENT].staticShaderHandle;
					if (shaderHandleToUse != 0)
					{
						drawLayerToUse = DL_TRANSPARENT;
					}
				}

				R2_CHECK(shaderHandleToUse != r2::draw::InvalidShader, "We don't have a proper shader?");

				depthShaderHandleToUse = modelRef->isAnimated ? shaderEffectPasses.meshPasses[flat::eMeshPass_DEPTH].dynamicShaderHandle : shaderEffectPasses.meshPasses[flat::eMeshPass_DEPTH].staticShaderHandle;

				//@TODO(Serge): add this back when we have proper shader effects setup
				//R2_CHECK(depthShaderHandleToUse != r2::draw::InvalidShader, "We don't have a proper shader?");

				key::SortBatchKey commandKey = key::GenerateSortBatchKey(drawLayerToUse, shaderHandleToUse, depthShaderHandleToUse, drawStateHash);

				DrawCommandData* defaultDrawCommandData = nullptr;

				DrawCommandData* drawCommandData = r2::shashmap::Get(*shaderDrawCommandData, commandKey.keyValue, defaultDrawCommandData);

				if (drawCommandData == defaultDrawCommandData)
				{
					drawCommandData = ALLOC(DrawCommandData, *renderer.mPreRenderStackArena);

					r2::sarr::Push(*tempAllocations, (void*)drawCommandData);

					R2_CHECK(drawCommandData != nullptr, "We couldn't allocate a drawCommandData!");

					drawCommandData->shaderEffectPasses = shaderEffectPasses;
					drawCommandData->isDynamic = modelRef->isAnimated;
					drawCommandData->drawState = drawState;
					drawCommandData->drawState.layer = drawLayerToUse;
					drawCommandData->subCommands = MAKE_SARRAY(*renderer.mPreRenderStackArena, cmd::DrawBatchSubCommand, drawCommandBatchSize);
					drawCommandData->cameraDepths = MAKE_SARRAY(*renderer.mPrePostRenderCommandArena, CameraDepth, drawCommandBatchSize);
					
					r2::sarr::Push(*tempAllocations, (void*)drawCommandData->cameraDepths);
					r2::sarr::Push(*tempAllocations, (void*)drawCommandData->subCommands);
					
					r2::shashmap::Set(*shaderDrawCommandData, commandKey.keyValue, drawCommandData);
				}

				R2_CHECK(drawCommandData != nullptr, "This shouldn't be nullptr!");

				r2::draw::cmd::DrawBatchSubCommand subCommand;
				subCommand.baseInstance = baseInstanceOffset + numModelInstances;
				subCommand.baseVertex = meshRef.gpuVertexEntry.start;
				subCommand.firstIndex = meshRef.gpuIndexEntry.start;
				subCommand.instanceCount = numInstances;
				subCommand.count = meshRef.gpuIndexEntry.size;

				r2::sarr::Push(*drawCommandData->subCommands, subCommand);


				CameraDepth cameraDepth;
				cameraDepth.cameraDepth = cameraDepthToModel;
				cameraDepth.index = r2::sarr::Size(*drawCommandData->cameraDepths);

				r2::sarr::Push(*drawCommandData->cameraDepths, cameraDepth);
			}

			numModelInstances += numInstances;
		}
	}

	void PreRender(Renderer& renderer)
	{
	//	PROFILE_SCOPE("PreRender");
		//PreRender should be setting up the batches to render
		static int MAX_NUM_GEOMETRY_SHADER_INVOCATIONS = shader::GetMaxNumberOfGeometryShaderInvocations();
		const s32 numDirectionLights = renderer.mLightSystem->mSceneLighting.mNumDirectionLights;
		const s32 numSpotLights = renderer.mLightSystem->mSceneLighting.mNumSpotLights;
		const s32 numPointLights = renderer.mLightSystem->mSceneLighting.mNumPointLights;

		const auto numShadowCastingDirectionLights = renderer.mLightSystem->mSceneLighting.numShadowCastingDirectionLights;
		const auto numShadowCastingPointLights = renderer.mLightSystem->mSceneLighting.numShadowCastingPointLights;
		const auto numShadowCastingSpotLights = renderer.mLightSystem->mSceneLighting.numShadowCastingSpotLights;

		const r2::SArray<r2::draw::ConstantBufferHandle>* constHandles = r2::draw::renderer::GetConstantBufferHandles(renderer);

		BufferLayoutHandle animVertexBufferLayoutHandle = vbsys::GetBufferLayoutHandle(*renderer.mVertexBufferLayoutSystem, r2::sarr::At(*renderer.mVertexBufferLayoutHandles, VBL_ANIMATED));
		BufferLayoutHandle staticVertexBufferLayoutHandle = vbsys::GetBufferLayoutHandle(*renderer.mVertexBufferLayoutSystem, r2::sarr::At(*renderer.mVertexBufferLayoutHandles, VBL_STATIC));
		BufferLayoutHandle finalBatchVertexBufferLayoutHandle = vbsys::GetBufferLayoutHandle(*renderer.mVertexBufferLayoutSystem, r2::sarr::At(*renderer.mVertexBufferLayoutHandles, VBL_FINAL));

		const RenderBatch& staticRenderBatch = r2::sarr::At(*renderer.mRenderBatches, DrawType::STATIC);
		const RenderBatch& dynamicRenderBatch = r2::sarr::At(*renderer.mRenderBatches, DrawType::DYNAMIC);
		const u64 numStaticModels = r2::sarr::Size(*staticRenderBatch.gpuModelRefs);
		const u64 numDynamicModels = r2::sarr::Size(*dynamicRenderBatch.gpuModelRefs);

		//We can calculate exactly how many materials there are
		u64 totalSubCommands = 0;
		u64 staticDrawCommandBatchSize = 0;
		u64 dynamicDrawCommandBatchSize = 0;

		u32 numStaticInstances = 0;
		u32 numDynamicInstances = 0;

		for (u64 i = 0; i < numStaticModels; ++i)
		{
			const vb::GPUModelRef* modelRef = r2::sarr::At(*staticRenderBatch.gpuModelRefs, i);

			totalSubCommands += r2::sarr::Size(*modelRef->meshEntries);

			numStaticInstances += r2::sarr::At(*staticRenderBatch.numInstances, i);
		}

		staticDrawCommandBatchSize = totalSubCommands;

		for (u64 i = 0; i < numDynamicModels; ++i)
		{
			const vb::GPUModelRef* modelRef = r2::sarr::At(*dynamicRenderBatch.gpuModelRefs, i);

			R2_CHECK(modelRef->isAnimated, "This should be animated if it's dynamic");

			numDynamicInstances += r2::sarr::At(*dynamicRenderBatch.numInstances, i);

			totalSubCommands += r2::sarr::Size(*modelRef->meshEntries);
		}

		r2::SArray<void*>* tempAllocations = MAKE_SARRAY(*renderer.mPreRenderStackArena, void*, 100 + numDynamicModels + numStaticModels); //@TODO(Serge): measure how many allocations

		r2::SArray<glm::ivec4>* boneOffsets = MAKE_SARRAY(*renderer.mPreRenderStackArena, glm::ivec4, numDynamicInstances);

		r2::sarr::Push(*tempAllocations, (void*)boneOffsets);
		u32 boneOffset = 0;

		for (u64 i = 0; i < numDynamicModels; ++i)
		{
			const vb::GPUModelRef* modelRef = r2::sarr::At(*dynamicRenderBatch.gpuModelRefs, i);

			u32 numInstances = r2::sarr::At(*dynamicRenderBatch.numInstances, i);
			b32 useSameBoneTransformsForInstances = r2::sarr::At(*dynamicRenderBatch.useSameBoneTransformsForInstances, i);

			if (useSameBoneTransformsForInstances)
			{
				for (u32 j = 0; j < numInstances; ++j)
				{
					r2::sarr::Push(*boneOffsets, glm::ivec4(boneOffset, 0, 0, 0));
				}
				
				boneOffset += modelRef->numBones;
			}
			else
			{
				for (u32 j = 0; j < numInstances; ++j)
				{
					r2::sarr::Push(*boneOffsets, glm::ivec4(boneOffset, 0, 0, 0));

					boneOffset += modelRef->numBones;
				}
			}
		}

		dynamicDrawCommandBatchSize = totalSubCommands - staticDrawCommandBatchSize;

		//@Threading: If we want to thread this in the future, we can call PopulateRenderDataFromRenderBatch from different threads provided they have their own temp allocators
		//			  You will need to add jobs(or whatever) to dealloc the memory after we merge and create the prerenderbucket which might be tricky since we'll have to make sure the proper threads free the memory

		r2::SArray<RenderMaterialParams>* renderMaterials = renderer.mRenderMaterialsToRender;//MAKE_SARRAY(*renderer.mPreRenderStackArena, RenderMaterialParams, (numStaticModels + numDynamicModels) * AVG_NUM_OF_MESHES_PER_MODEL);
		
		r2::SArray<glm::uvec4>* materialOffsetsPerObject = MAKE_SARRAY(*renderer.mPreRenderStackArena, glm::uvec4, numDynamicInstances + numStaticInstances);
		
		r2::sarr::Push(*tempAllocations, (void*)materialOffsetsPerObject);

		r2::SHashMap<DrawCommandData*>* shaderDrawCommandData = MAKE_SHASHMAP(*renderer.mPreRenderStackArena, DrawCommandData*, totalSubCommands * r2::SHashMap<DrawCommandData>::LoadFactorMultiplier());

		r2::sarr::Push(*tempAllocations, (void*)shaderDrawCommandData);

		u32 materialOffset = 0;
		u32 meshOffset = 0;
		PopulateRenderDataFromRenderBatch(renderer, tempAllocations, dynamicRenderBatch, shaderDrawCommandData, renderMaterials, materialOffsetsPerObject, materialOffset, 0, dynamicDrawCommandBatchSize, meshOffset);
		PopulateRenderDataFromRenderBatch(renderer, tempAllocations, staticRenderBatch, shaderDrawCommandData, renderMaterials, materialOffsetsPerObject, materialOffset, numDynamicInstances, staticDrawCommandBatchSize, meshOffset);

		r2::sarr::Append(*renderMaterials, *dynamicRenderBatch.materialBatch.renderMaterialParams);
		r2::sarr::Append(*renderMaterials, *staticRenderBatch.materialBatch.renderMaterialParams);

		key::Basic basicKey;
		basicKey.keyValue = 0;

		const u64 numModels =  numDynamicInstances + numStaticInstances + 1; //+1 for the final batch model
		const u64 finalBatchModelOffset = numModels - 1;
		const u64 modelsMemorySize = numModels * sizeof(glm::mat4);

		glm::mat4 finalBatchModelMat = glm::mat4(1.0f);

		cmd::FillConstantBuffer* modelsCmd = AddFillConstantBufferCommand<mem::StackArena, key::Basic>(*renderer.mPrePostRenderCommandArena, *renderer.mPreRenderBucket, basicKey, modelsMemorySize);

		char* modelsAuxMemory = cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(modelsCmd);
		 
		const u64 dynamicModelsMemorySize = numDynamicInstances * sizeof(glm::mat4);
		const u64 staticModelsMemorySize = numStaticInstances * sizeof(glm::mat4);

		memcpy(modelsAuxMemory, dynamicRenderBatch.models->mData, dynamicModelsMemorySize);
		memcpy(mem::utils::PointerAdd(modelsAuxMemory, dynamicModelsMemorySize), staticRenderBatch.models->mData, staticModelsMemorySize);
		memcpy(mem::utils::PointerAdd(modelsAuxMemory, dynamicModelsMemorySize + staticModelsMemorySize), glm::value_ptr(finalBatchModelMat), sizeof(glm::mat4));

		auto modelsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mModelConfigHandle);

		//fill out constCMD
		{
			ConstantBufferData* modelConstData = GetConstData(renderer, modelsConstantBufferHandle);

			modelsCmd->data = modelsAuxMemory;
			modelsCmd->dataSize = modelsMemorySize;
			modelsCmd->offset = modelConstData->currentOffset;
			modelsCmd->constantBufferHandle = modelsConstantBufferHandle;

			modelsCmd->isPersistent = modelConstData->isPersistent;
			modelsCmd->type = modelConstData->type;

			modelConstData->AddDataSize(modelsMemorySize);
		}


		cmd::FillConstantBuffer* prevFillCMD = modelsCmd;

		const u64 numRenderMaterials = r2::sarr::Size(*renderMaterials);
		const u64 materialsDataSize = sizeof(r2::draw::RenderMaterialParams) * numRenderMaterials;
		auto materialsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mMaterialConfigHandle);

		cmd::FillConstantBuffer* materialsCMD = nullptr;

		if (materialsDataSize > 0)
		{
			
			materialsCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, prevFillCMD, materialsDataSize);

			ConstantBufferData* materialsConstData = GetConstData(renderer, materialsConstantBufferHandle);

			FillConstantBufferCommand(
				materialsCMD,
				materialsConstantBufferHandle,
				materialsConstData->type,
				materialsConstData->isPersistent,
				renderMaterials->mData,
				materialsDataSize,
				materialsConstData->currentOffset);

			materialsConstData->AddDataSize(materialsDataSize);

			prevFillCMD = materialsCMD;
		}

		const u64 numMaterialOffsets = r2::sarr::Size(*materialOffsetsPerObject);
		const u64 materialOffsetsDataSize = sizeof(glm::uvec4) * numMaterialOffsets;
		auto materialOffsetsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mMaterialOffsetsConfigHandle);

		cmd::FillConstantBuffer* materialOffsetsCMD = nullptr;
		
		
		if (materialOffsetsDataSize > 0)
		{
			materialOffsetsCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, prevFillCMD, materialOffsetsDataSize);

			ConstantBufferData* materialOffsetsConstData = GetConstData(renderer, materialOffsetsConstantBufferHandle);

			FillConstantBufferCommand(
				materialOffsetsCMD,
				materialOffsetsConstantBufferHandle,
				materialOffsetsConstData->type,
				materialOffsetsConstData->isPersistent,
				materialOffsetsPerObject->mData,
				materialOffsetsDataSize,
				materialOffsetsConstData->currentOffset);

			materialOffsetsConstData->AddDataSize(materialOffsetsDataSize);

			prevFillCMD = materialOffsetsCMD;
		}

		cmd::FillConstantBuffer* meshDataCMD = nullptr;

		const u32 numDynamicMeshDatas = r2::sarr::Size(*dynamicRenderBatch.meshRenderData);
		const u32 numStaticMeshDatas = r2::sarr::Size(*staticRenderBatch.meshRenderData);

		const u32 numMeshDatas = numDynamicMeshDatas + numStaticMeshDatas;
		const u32 meshDataSize = sizeof(MeshRenderData) * numMeshDatas;
		auto meshDataConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mMeshDataConfigHandle);

		if (materialOffsetsDataSize > 0 && meshDataSize > 0)
		{
			meshDataCMD = AddFillConstantBufferCommand<mem::StackArena, key::Basic>(*renderer.mPrePostRenderCommandArena, *renderer.mPreRenderBucket, basicKey, meshDataSize);

			char* meshDataAuxMemory = cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(meshDataCMD);

			const u64 dynamicMeshMemorySize = numDynamicMeshDatas * sizeof(MeshRenderData);
			const u64 staticMeshMemorySize = numStaticMeshDatas * sizeof(MeshRenderData);

			memcpy(meshDataAuxMemory, dynamicRenderBatch.meshRenderData->mData, dynamicMeshMemorySize);
			memcpy(mem::utils::PointerAdd(meshDataAuxMemory, dynamicMeshMemorySize), staticRenderBatch.meshRenderData->mData, staticMeshMemorySize);

			ConstantBufferData* meshConstData = GetConstData(renderer, meshDataConstantBufferHandle);

			meshDataCMD->data = meshDataAuxMemory;
			meshDataCMD->dataSize = meshDataSize;
			meshDataCMD->offset = meshConstData->currentOffset;
			meshDataCMD->constantBufferHandle = meshDataConstantBufferHandle;

			meshDataCMD->isPersistent = meshConstData->isPersistent;
			meshDataCMD->type = meshConstData->type;

			meshConstData->AddDataSize(meshDataSize);

			prevFillCMD = meshDataCMD;
		}

		if (dynamicRenderBatch.boneTransforms && r2::sarr::Size(*dynamicRenderBatch.boneTransforms) > 0)
		{
			const u64 boneTransformMemorySize = dynamicRenderBatch.boneTransforms->mSize * sizeof(ShaderBoneTransform);
			r2::draw::cmd::FillConstantBuffer* boneTransformsCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, prevFillCMD, boneTransformMemorySize);

			auto boneTransformsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mBoneTransformsConfigHandle);
			ConstantBufferData* boneXFormConstData = GetConstData(renderer, boneTransformsConstantBufferHandle);

			FillConstantBufferCommand(
				boneTransformsCMD,
				boneTransformsConstantBufferHandle,
				boneXFormConstData->type,
				boneXFormConstData->isPersistent,
				dynamicRenderBatch.boneTransforms->mData,
				boneTransformMemorySize,
				boneXFormConstData->currentOffset);

			boneXFormConstData->AddDataSize(boneTransformMemorySize);

			u64 boneTransformOffsetsDataSize = boneOffsets->mSize * sizeof(glm::ivec4);
			cmd::FillConstantBuffer* boneTransformOffsetsCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, boneTransformsCMD, boneTransformOffsetsDataSize);

			auto boneTransformOffsetsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mBoneTransformOffsetsConfigHandle);

			ConstantBufferData* boneXFormOffsetsConstData = GetConstData(renderer, boneTransformOffsetsConstantBufferHandle);
			FillConstantBufferCommand(
				boneTransformOffsetsCMD,
				boneTransformOffsetsConstantBufferHandle,
				boneXFormOffsetsConstData->type,
				boneXFormOffsetsConstData->isPersistent,
				boneOffsets->mData,
				boneTransformOffsetsDataSize,
				boneXFormOffsetsConstData->currentOffset);

			boneXFormOffsetsConstData->AddDataSize(boneTransformOffsetsDataSize);

			prevFillCMD = boneTransformOffsetsCMD;
		}

		//@NOTE(Serge): can we get away with just one of these?
		r2::SArray<BatchRenderOffsets>* staticRenderBatchesOffsets = MAKE_SARRAY(*renderer.mPreRenderStackArena, BatchRenderOffsets, staticDrawCommandBatchSize);//@NOTE: pretty sure this is an overestimate - could reduce to save mem
		r2::SArray<BatchRenderOffsets>* dynamicRenderBatchesOffsets = MAKE_SARRAY(*renderer.mPreRenderStackArena, BatchRenderOffsets, dynamicDrawCommandBatchSize);

		r2::sarr::Push(*tempAllocations, (void*)staticRenderBatchesOffsets);
		r2::sarr::Push(*tempAllocations, (void*)dynamicRenderBatchesOffsets);

		cmd::FillConstantBuffer* subCommandsCMD = nullptr;

		const u64 subCommandsMemorySize = sizeof(cmd::DrawBatchSubCommand) * (totalSubCommands + 1); //+ 1 for final batch

		subCommandsCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, prevFillCMD, subCommandsMemorySize);

		char* subCommandsAuxMemory = cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(subCommandsCMD);

		u64 subCommandsMemoryOffset = 0;
		u32 subCommandsOffset = 0;

		auto hashIter = r2::shashmap::Begin(*shaderDrawCommandData);

		for (; hashIter != r2::shashmap::End(*shaderDrawCommandData); ++hashIter)
		{
			DrawCommandData* drawCommandData = hashIter->value;
			if (drawCommandData != nullptr)
			{
				//sort the subcommands here by camera depth

				//non transparency - sort front to back
				//@TODO(Serge): I'm not sure this does all of the requisite sorting - what about sorting between DrawCommandData?
				if (drawCommandData->shaderEffectPasses.meshPasses[flat::eMeshPass_TRANSPARENT].IsInvalid())
				{
					std::sort(r2::sarr::Begin(*drawCommandData->cameraDepths), r2::sarr::End(*drawCommandData->cameraDepths), [](const CameraDepth& s1, const CameraDepth& s2) {
						return s1.cameraDepth < s2.cameraDepth;
						});
				}
				

				const u32 numSubCommandsInBatch = static_cast<u32>(r2::sarr::Size(*drawCommandData->subCommands));

				for (u32 i = 0; i < numSubCommandsInBatch; ++i)
				{
					u32 index = r2::sarr::At(*drawCommandData->cameraDepths, i).index;

					memcpy(mem::utils::PointerAdd(subCommandsAuxMemory, subCommandsMemoryOffset), &r2::sarr::At(*drawCommandData->subCommands, index), sizeof(cmd::DrawBatchSubCommand));
					subCommandsMemoryOffset += sizeof(cmd::DrawBatchSubCommand);
				}

				BatchRenderOffsets batchOffsets;

				batchOffsets.shaderEffectPasses = drawCommandData->shaderEffectPasses;
				batchOffsets.subCommandsOffset = subCommandsOffset;
				batchOffsets.numSubCommands = numSubCommandsInBatch;
				batchOffsets.isDynamic = drawCommandData->isDynamic;

				memcpy(&batchOffsets.drawState, &drawCommandData->drawState, sizeof(cmd::DrawState));

				batchOffsets.cameraDepth = 0; //@TODO(Serge): I think it's zero because camera depth doesn't apply well to big batches of stuff
				batchOffsets.blendingFunctionKeyValue = key::GetBlendingFunctionKeyValue(drawCommandData->drawState.blendState);
				

				R2_CHECK(batchOffsets.numSubCommands > 0, "We should have a count!");

				if (drawCommandData->isDynamic)
				{
					r2::sarr::Push(*dynamicRenderBatchesOffsets, batchOffsets);
				}
				else
				{
					r2::sarr::Push(*staticRenderBatchesOffsets, batchOffsets);
				}

				subCommandsOffset += numSubCommandsInBatch;
			}
		}

		//Make our final batch data here
		BatchRenderOffsets finalBatchOffsets;
		{
			const vb::GPUModelRef* fullScreenTriangleModelRef = vbsys::GetGPUModelRef(*renderer.mVertexBufferLayoutSystem, GetDefaultModelRef(renderer, FULLSCREEN_TRIANGLE));// r2::sarr::At(*renderer.mModelRefs, GetDefaultModelRef(renderer, FULLSCREEN_TRIANGLE));

			cmd::DrawBatchSubCommand finalBatchSubcommand;
			finalBatchSubcommand.baseInstance = finalBatchModelOffset;
			finalBatchSubcommand.baseVertex = r2::sarr::At(*fullScreenTriangleModelRef->meshEntries, 0).gpuVertexEntry.start;
			finalBatchSubcommand.firstIndex = r2::sarr::At(*fullScreenTriangleModelRef->meshEntries, 0).gpuIndexEntry.start;
			finalBatchSubcommand.instanceCount = 1;
			finalBatchSubcommand.count = r2::sarr::At(*fullScreenTriangleModelRef->meshEntries, 0).gpuIndexEntry.size;
			memcpy(mem::utils::PointerAdd(subCommandsAuxMemory, subCommandsMemoryOffset), &finalBatchSubcommand, sizeof(cmd::DrawBatchSubCommand));

			finalBatchOffsets.drawState.layer = DL_SCREEN;
			finalBatchOffsets.numSubCommands = 1;
			finalBatchOffsets.subCommandsOffset = subCommandsOffset;
			finalBatchOffsets.isDynamic = false;

			finalBatchOffsets.shaderEffectPasses.meshPasses[flat::eMeshPass_FORWARD].staticShaderHandle = renderer.mFinalCompositeShaderHandle;
			
			subCommandsMemoryOffset += sizeof(cmd::DrawBatchSubCommand);
			subCommandsOffset += 1;
		}


		auto subCommandsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mSubcommandsConfigHandle);

		ConstantBufferData* subCommandsConstData = GetConstData(renderer, subCommandsConstantBufferHandle);

		subCommandsCMD->constantBufferHandle = subCommandsConstantBufferHandle;
		subCommandsCMD->data = subCommandsAuxMemory;
		subCommandsCMD->dataSize = subCommandsMemorySize;
		subCommandsCMD->offset = subCommandsConstData->currentOffset;
		subCommandsCMD->type = subCommandsConstData->type;
		subCommandsCMD->isPersistent = subCommandsConstData->isPersistent;

		subCommandsConstData->AddDataSize(subCommandsMemorySize);

		//check to see if we need to rebuild the cluster volume tiles
		if (renderer.mFlags.IsSet(RENDERER_FLAG_NEEDS_CLUSTER_VOLUME_TILE_UPDATE))
		{
			key::Basic clusterKey = key::GenerateBasicKey(0, 0, DL_COMPUTE, 0, 0, renderer.mCreateClusterComputeShader);
			
			cmd::DispatchCompute* createClusterTilesCMD = AddCommand<key::Basic, cmd::DispatchCompute, mem::StackArena>(*renderer.mPrePostRenderCommandArena, *renderer.mPreRenderBucket, clusterKey, 0);
			createClusterTilesCMD->numGroupsX = renderer.mShaderVectors.clusterTileSizes.x;
			createClusterTilesCMD->numGroupsY = renderer.mShaderVectors.clusterTileSizes.y;
			createClusterTilesCMD->numGroupsZ = renderer.mShaderVectors.clusterTileSizes.z;
		}

		//PostRenderBucket commands
		{
			cmd::CompleteConstantBuffer* completeModelsCMD = AddCommand<key::Basic, cmd::CompleteConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, *renderer.mPostRenderBucket, basicKey, 0);
			completeModelsCMD->constantBufferHandle = modelsConstantBufferHandle;
			completeModelsCMD->count = numModels;

			cmd::CompleteConstantBuffer* completeMaterialsCMD = AppendCommand <cmd::CompleteConstantBuffer, cmd::CompleteConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, completeModelsCMD, 0);

			completeMaterialsCMD->constantBufferHandle = materialsConstantBufferHandle;
			completeMaterialsCMD->count = numRenderMaterials;

			cmd::CompleteConstantBuffer* completeMaterialOffsetsCMD = AppendCommand<cmd::CompleteConstantBuffer, cmd::CompleteConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, completeMaterialsCMD, 0);

			completeMaterialOffsetsCMD->constantBufferHandle = materialOffsetsConstantBufferHandle;
			completeMaterialOffsetsCMD->count = numMaterialOffsets;

			cmd::CompleteConstantBuffer* completeMeshDataCMD = AppendCommand<cmd::CompleteConstantBuffer, cmd::CompleteConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, completeMaterialOffsetsCMD, 0);

			completeMaterialOffsetsCMD->constantBufferHandle = meshDataConstantBufferHandle;
			completeMaterialOffsetsCMD->count = numMeshDatas;

			cmd::CompleteConstantBuffer* prevCompleteCMD = completeMeshDataCMD;

			if (dynamicRenderBatch.boneTransforms && r2::sarr::Size(*dynamicRenderBatch.boneTransforms) > 0)
			{
				auto boneTransformsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mBoneTransformsConfigHandle);

				cmd::CompleteConstantBuffer* completeBoneTransformsCMD = AppendCommand <cmd::CompleteConstantBuffer, cmd::CompleteConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, prevCompleteCMD, 0);

				completeBoneTransformsCMD->constantBufferHandle = boneTransformsConstantBufferHandle;
				completeBoneTransformsCMD->count = r2::sarr::Size(*dynamicRenderBatch.boneTransforms);

				auto boneTransformOffsetsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mBoneTransformOffsetsConfigHandle);

				cmd::CompleteConstantBuffer* completeBoneTransformsOffsetsCMD = AppendCommand <cmd::CompleteConstantBuffer, cmd::CompleteConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, completeBoneTransformsCMD, 0);

				completeBoneTransformsOffsetsCMD->constantBufferHandle = boneTransformOffsetsConstantBufferHandle;
				completeBoneTransformsOffsetsCMD->count = r2::sarr::Size(*boneOffsets);

				prevCompleteCMD = completeBoneTransformsOffsetsCMD;
			}

			cmd::CompleteConstantBuffer* completeSubCommandsCMD = AppendCommand <cmd::CompleteConstantBuffer, cmd::CompleteConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, prevCompleteCMD, 0);

			completeSubCommandsCMD->constantBufferHandle = subCommandsConstantBufferHandle;
			completeSubCommandsCMD->count = subCommandsOffset;
		}
		
		ClearSurfaceOptions clearGBufferOptions;
		clearGBufferOptions.shouldClear = true;
		clearGBufferOptions.flags = cmd::CLEAR_COLOR_BUFFER | cmd::CLEAR_STENCIL_BUFFER;
		
		const u64 numStaticDrawBatches = r2::sarr::Size(*staticRenderBatchesOffsets);

		ShaderHandle clearShaderHandle = InvalidShader;

		if (numStaticDrawBatches > 0)
		{
			const auto& batchOffset = r2::sarr::At(*staticRenderBatchesOffsets, 0);

			clearShaderHandle = batchOffset.shaderEffectPasses.meshPasses[flat::eMeshPass_FORWARD].staticShaderHandle;
			if (batchOffset.drawState.layer == DL_TRANSPARENT)
			{
				clearShaderHandle = batchOffset.shaderEffectPasses.meshPasses[flat::eMeshPass_TRANSPARENT].staticShaderHandle;
			} 
		}

		ClearSurfaceOptions clearDepthOptions;
		clearDepthOptions.shouldClear = true;
		clearDepthOptions.flags = cmd::CLEAR_DEPTH_BUFFER;

		ClearSurfaceOptions clearDepthStencilOptions;
		clearDepthStencilOptions.shouldClear = true;
		clearDepthStencilOptions.flags = cmd::CLEAR_DEPTH_BUFFER | cmd::CLEAR_STENCIL_BUFFER;

		key::Basic clearKey = key::GenerateBasicKey(0, 0, DL_CLEAR, 0, 0, clearShaderHandle);
		key::ShadowKey shadowClearKey = key::GenerateShadowKey(key::ShadowKey::CLEAR, 0, 0, false, light::LightType::LT_DIRECTIONAL_LIGHT, 0);
		key::DepthKey depthClearKey = key::GenerateDepthKey(key::DepthKey::NORMAL, 0, r2::draw::renderer::GetDepthShaderHandle(false), DrawLayer::DL_WORLD);

		BeginRenderPass<key::DepthKey>(renderer, RPT_ZPREPASS, clearDepthStencilOptions, *renderer.mDepthPrePassBucket, depthClearKey, *renderer.mShadowArena);
		BeginRenderPass<key::DepthKey>(renderer, RPT_ZPREPASS_SHADOWS, clearDepthOptions, *renderer.mDepthPrePassShadowBucket, depthClearKey, *renderer.mShadowArena);
		
		key::Basic clusterKey = key::GenerateBasicKey(0, 0, DL_CLEAR, 0, 0, 0);
		BeginRenderPass<key::Basic>(renderer, RPT_CLUSTERS, clearGBufferOptions, *renderer.mClustersBucket, clusterKey, *renderer.mCommandArena);

		

		ClearSurfaceOptions shadowClearOptions;
		shadowClearOptions.shouldClear = true;
		shadowClearOptions.flags = cmd::CLEAR_DEPTH_BUFFER;
		

		BeginRenderPass<key::ShadowKey>(renderer, RPT_SHADOWS, shadowClearOptions, *renderer.mShadowBucket, shadowClearKey, *renderer.mShadowArena);

		//@NOTE(Serge): directional light here is intentional for sort order
		key::ShadowKey pointLightShadowKey = key::GenerateShadowKey(key::ShadowKey::POINT_LIGHT, 0, 0, false, light::LightType::LT_DIRECTIONAL_LIGHT, 0);
		BeginRenderPass<key::ShadowKey>(renderer, RPT_POINTLIGHT_SHADOWS, clearDepthOptions, *renderer.mShadowBucket, pointLightShadowKey, *renderer.mShadowArena);
		BeginRenderPass<key::Basic>(renderer, RPT_GBUFFER, clearGBufferOptions, *renderer.mCommandBucket, clearKey, *renderer.mCommandArena);

		ClearSurfaceOptions transparentClearOptions;
		transparentClearOptions.useNormalClear = false;
		transparentClearOptions.numClearParams = 2;
		transparentClearOptions.shouldClear = true;
		transparentClearOptions.clearParams[0].bufferIndex = 0;
		transparentClearOptions.clearParams[0].bufferFlags = COLOR;
		transparentClearOptions.clearParams[0].clearValue[0] = 0.0f;
		transparentClearOptions.clearParams[0].clearValue[1] = 0.0f;
		transparentClearOptions.clearParams[0].clearValue[2] = 0.0f;
		transparentClearOptions.clearParams[0].clearValue[3] = 0.0f;

		//memset(transparentClearOptions.clearParams[0].clearValue, 0, sizeof(float) * 4); 
		transparentClearOptions.clearParams[1].bufferIndex = 1;
		transparentClearOptions.clearParams[1].bufferFlags = COLOR;
		transparentClearOptions.clearParams[1].clearValue[0] = 1.0f;
		transparentClearOptions.clearParams[1].clearValue[1] = 1.0f;
		transparentClearOptions.clearParams[1].clearValue[2] = 1.0f;
		transparentClearOptions.clearParams[1].clearValue[3] = 1.0f;
		//memset(transparentClearOptions.clearParams[1].clearValue, 1, sizeof(float) * 4);


		BeginRenderPass<key::Basic>(renderer, RPT_TRANSPARENT, transparentClearOptions, *renderer.mTransparentBucket, clearKey, *renderer.mCommandArena);
		
		

#ifdef R2_EDITOR
		ClearSurfaceOptions clearEditorPickingOptions;
		clearEditorPickingOptions.shouldClear = true;
		clearEditorPickingOptions.flags = cmd::CLEAR_COLOR_BUFFER ;
		key::Basic clearEditorPickingKey = key::GenerateBasicKey(0, 0, DL_CLEAR, 0, 0, renderer.mEntityColorShader[0]);

		BeginRenderPass<key::Basic>(renderer, RPT_EDITOR_PICKING, clearEditorPickingOptions, *renderer.mEditorPickingBucket, clearEditorPickingKey, *renderer.mCommandArena);
#endif

		

		//@NOTE(Serge): we need to figure out if we even need to separate the static and dynamic draw batches anymore
		//				like having to do this twice is pretty dumb... We would need to add the bufferLayoutHandle and isDynamic (to choose the proper shader) to the BatchOffsets

		for (u64 i = 0; i < numStaticDrawBatches; ++i)
		{
			const auto& batchOffset = r2::sarr::At(*staticRenderBatchesOffsets, i);

			ShaderHandle shaderToUse = batchOffset.shaderEffectPasses.meshPasses[flat::eMeshPass_FORWARD].staticShaderHandle;
			if (batchOffset.drawState.layer == DL_TRANSPARENT)
			{
				shaderToUse = batchOffset.shaderEffectPasses.meshPasses[flat::eMeshPass_TRANSPARENT].staticShaderHandle;
			}

			key::Basic key = key::GenerateBasicKey(key::Basic::FSL_GAME, 0, batchOffset.drawState.layer, batchOffset.blendingFunctionKeyValue, batchOffset.cameraDepth, shaderToUse);

			cmd::DrawBatch* drawBatch = nullptr;

			if (batchOffset.drawState.layer != DL_TRANSPARENT)
			{
				drawBatch = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mCommandBucket, key, 0);
				drawBatch->state.depthFunction = EQUAL;
				cmd::SetDefaultBlendState(drawBatch->state.blendState);
			}
			else
			{
				//@TODO(Serge): change to mTransparentBucket
				drawBatch = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mTransparentBucket, key, 0);
				drawBatch->state.depthFunction = LESS;
				
				//now setup all of the blend state to make transparency work
				SetDefaultTransparencyBlendState(drawBatch->state.blendState);
			}

			drawBatch->state.depthWriteEnabled = false;
			drawBatch->batchHandle = subCommandsConstantBufferHandle;
			drawBatch->bufferLayoutHandle = staticVertexBufferLayoutHandle;
			drawBatch->numSubCommands = batchOffset.numSubCommands;
			R2_CHECK(drawBatch->numSubCommands > 0, "We should have a count!");
			drawBatch->startCommandIndex = batchOffset.subCommandsOffset;
			drawBatch->primitiveType = PrimitiveType::TRIANGLES;
			drawBatch->subCommands = nullptr;
			drawBatch->state.depthEnabled = batchOffset.drawState.depthEnabled;
			drawBatch->state.cullState = batchOffset.drawState.cullState;
			
			drawBatch->state.polygonOffsetEnabled = false;
			drawBatch->state.polygonOffset = glm::vec2(0);
			drawBatch->state.stencilState = batchOffset.drawState.stencilState;
			
			
			if (batchOffset.drawState.layer == DL_SKYBOX)
			{
				drawBatch->state.depthFunction = LEQUAL;
			}

#ifdef R2_EDITOR
			//@TODO(Serge): make a shader for the skybox for picking
			if (batchOffset.drawState.layer != DL_SKYBOX)
			{
				key::Basic staticEditorPickingKey = key::GenerateBasicKey(key::Basic::FSL_GAME, 0, batchOffset.drawState.layer, 0, batchOffset.cameraDepth, renderer.mEntityColorShader[0]);

				cmd::DrawBatch* editorPickingDrawBatchCMD = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mEditorPickingBucket, staticEditorPickingKey, 0);
				editorPickingDrawBatchCMD->state.depthFunction = EQUAL;

				editorPickingDrawBatchCMD->state.depthWriteEnabled = false;
				editorPickingDrawBatchCMD->batchHandle = subCommandsConstantBufferHandle;
				editorPickingDrawBatchCMD->bufferLayoutHandle = staticVertexBufferLayoutHandle;
				editorPickingDrawBatchCMD->numSubCommands = batchOffset.numSubCommands;
				R2_CHECK(editorPickingDrawBatchCMD->numSubCommands > 0, "We should have a count!");
				editorPickingDrawBatchCMD->startCommandIndex = batchOffset.subCommandsOffset;
				editorPickingDrawBatchCMD->primitiveType = PrimitiveType::TRIANGLES;
				editorPickingDrawBatchCMD->subCommands = nullptr;
				editorPickingDrawBatchCMD->state.depthEnabled = batchOffset.drawState.depthEnabled;
				editorPickingDrawBatchCMD->state.cullState = batchOffset.drawState.cullState;

				editorPickingDrawBatchCMD->state.polygonOffsetEnabled = false;
				editorPickingDrawBatchCMD->state.polygonOffset = glm::vec2(0);

				cmd::SetDefaultStencilState(editorPickingDrawBatchCMD->state.stencilState);
				cmd::SetDefaultBlendState(editorPickingDrawBatchCMD->state.blendState);
			}
			
#endif


			if (batchOffset.drawState.layer != DL_SKYBOX)
			{
				key::ShadowKey directionShadowKey = key::GenerateShadowKey(key::ShadowKey::NORMAL, 0, 0, false, light::LightType::LT_DIRECTIONAL_LIGHT, batchOffset.cameraDepth);

				//I guess we need to loop here to submit all of the draws for each light...

				//@TODO(Serge): we don't loop over each cascade in the geometry shader for some reason so we have to do this extra divide by NUM_FRUSTUM_SPLITS, we should probably loop in there and update

				const u32 numDirectionShadowBatchesNeeded = static_cast<u32>(glm::max(glm::ceil((float)numShadowCastingDirectionLights / ((float)MAX_NUM_GEOMETRY_SHADER_INVOCATIONS / (float)cam::NUM_FRUSTUM_SPLITS)), numShadowCastingDirectionLights > 0 ? 1.0f : 0.0f));

				for (u32 i = 0; i < numDirectionShadowBatchesNeeded; ++i)
				{
					cmd::ConstantUint* directionLightBatchIndexUpdateCMD = AddCommand<key::ShadowKey, cmd::ConstantUint, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, directionShadowKey, 0);
					directionLightBatchIndexUpdateCMD->value = i;
					directionLightBatchIndexUpdateCMD->uniformLocation = renderer.mStaticDirectionLightBatchUniformLocation;

					cmd::DrawBatch* shadowDrawBatch = AppendCommand<cmd::ConstantUint, cmd::DrawBatch, mem::StackArena>(*renderer.mShadowArena, directionLightBatchIndexUpdateCMD, 0);

					shadowDrawBatch->batchHandle = subCommandsConstantBufferHandle;
					shadowDrawBatch->bufferLayoutHandle = staticVertexBufferLayoutHandle;
					shadowDrawBatch->numSubCommands = batchOffset.numSubCommands;
					R2_CHECK(shadowDrawBatch->numSubCommands > 0, "We should have a count!");
					shadowDrawBatch->startCommandIndex = batchOffset.subCommandsOffset;
					shadowDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
					shadowDrawBatch->subCommands = nullptr;
					shadowDrawBatch->state.depthEnabled = true;

					shadowDrawBatch->state.depthFunction = LESS;
					shadowDrawBatch->state.depthWriteEnabled = true;
					shadowDrawBatch->state.polygonOffsetEnabled = false;
					shadowDrawBatch->state.polygonOffset = glm::vec2(0.0, 0.0);

					cmd::SetDefaultCullState(shadowDrawBatch->state.cullState);
					shadowDrawBatch->state.cullState.cullFace = CULL_FACE_FRONT;

					cmd::SetDefaultStencilState(shadowDrawBatch->state.stencilState);

					cmd::SetDefaultBlendState(shadowDrawBatch->state.blendState);
				}

				const u32 numSpotLightShadowBatchesNeeded = static_cast<u32>(glm::max(glm::ceil((float)numShadowCastingSpotLights / (float)MAX_NUM_GEOMETRY_SHADER_INVOCATIONS), numShadowCastingSpotLights > 0 ? 1.0f : 0.0f));

				key::ShadowKey spotLightShadowKey = key::GenerateShadowKey(key::ShadowKey::NORMAL, 0, 0, false, light::LightType::LT_SPOT_LIGHT, batchOffset.cameraDepth);

				for (u32 i = 0; i < numSpotLightShadowBatchesNeeded; ++i)
				{
					cmd::ConstantUint* spotLightBatchIndexUpdateCMD = AddCommand<key::ShadowKey, cmd::ConstantUint, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, spotLightShadowKey, 0);
					spotLightBatchIndexUpdateCMD->value = i;
					spotLightBatchIndexUpdateCMD->uniformLocation = renderer.mStaticSpotLightBatchUniformLocation;

					cmd::DrawBatch* shadowDrawBatch = AppendCommand<cmd::ConstantUint, cmd::DrawBatch, mem::StackArena>(*renderer.mShadowArena, spotLightBatchIndexUpdateCMD, 0);

					shadowDrawBatch->batchHandle = subCommandsConstantBufferHandle;
					shadowDrawBatch->bufferLayoutHandle = staticVertexBufferLayoutHandle;
					shadowDrawBatch->numSubCommands = batchOffset.numSubCommands;
					R2_CHECK(shadowDrawBatch->numSubCommands > 0, "We should have a count!");
					shadowDrawBatch->startCommandIndex = batchOffset.subCommandsOffset;
					shadowDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
					shadowDrawBatch->subCommands = nullptr;
					shadowDrawBatch->state.depthEnabled = true;
					shadowDrawBatch->state.depthFunction = LESS;
					shadowDrawBatch->state.depthWriteEnabled = true;
					shadowDrawBatch->state.polygonOffsetEnabled = false;
					shadowDrawBatch->state.polygonOffset = glm::vec2(0);

					cmd::SetDefaultCullState(shadowDrawBatch->state.cullState);
					shadowDrawBatch->state.cullState.cullFace = CULL_FACE_FRONT;

					cmd::SetDefaultStencilState(shadowDrawBatch->state.stencilState);

					cmd::SetDefaultBlendState(shadowDrawBatch->state.blendState);

				}

				const u32 numPointLightShadowBatchesNeeded = static_cast<u32>(glm::max(glm::ceil((float)numShadowCastingPointLights / (float)MAX_NUM_GEOMETRY_SHADER_INVOCATIONS), numShadowCastingPointLights > 0 ? 1.0f : 0.0f));

				key::ShadowKey plShadowKey = key::GenerateShadowKey(key::ShadowKey::POINT_LIGHT, 0, 0, false, light::LightType::LT_POINT_LIGHT, batchOffset.cameraDepth);

				for (u32 i = 0; i < numPointLightShadowBatchesNeeded; ++i)
				{
					cmd::ConstantUint* pointLightBatchIndexUpdateCMD = AddCommand<key::ShadowKey, cmd::ConstantUint, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, plShadowKey, 0);
					pointLightBatchIndexUpdateCMD->value = i;
					pointLightBatchIndexUpdateCMD->uniformLocation = renderer.mStaticPointLightBatchUniformLocation;

					cmd::DrawBatch* shadowDrawBatch = AppendCommand<cmd::ConstantUint, cmd::DrawBatch, mem::StackArena>(*renderer.mShadowArena, pointLightBatchIndexUpdateCMD, 0);

					shadowDrawBatch->batchHandle = subCommandsConstantBufferHandle;
					shadowDrawBatch->bufferLayoutHandle = staticVertexBufferLayoutHandle;
					shadowDrawBatch->numSubCommands = batchOffset.numSubCommands;
					R2_CHECK(shadowDrawBatch->numSubCommands > 0, "We should have a count!");
					shadowDrawBatch->startCommandIndex = batchOffset.subCommandsOffset;
					shadowDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
					shadowDrawBatch->subCommands = nullptr;
					shadowDrawBatch->state.depthEnabled = true;
					shadowDrawBatch->state.depthFunction = LESS;
					shadowDrawBatch->state.depthWriteEnabled = true;
					shadowDrawBatch->state.polygonOffsetEnabled = false;
					shadowDrawBatch->state.polygonOffset = glm::vec2(0);

					cmd::SetDefaultCullState(shadowDrawBatch->state.cullState);
					shadowDrawBatch->state.cullState.cullFace = CULL_FACE_FRONT;

					cmd::SetDefaultStencilState(shadowDrawBatch->state.stencilState);

					cmd::SetDefaultBlendState(shadowDrawBatch->state.blendState);
				}

				key::DepthKey zppKey = key::GenerateDepthKey(key::DepthKey::NORMAL, 0, batchOffset.shaderEffectPasses.meshPasses[flat::eMeshPass_DEPTH].staticShaderHandle, r2::draw::DrawLayer::DL_WORLD);

				if (batchOffset.drawState.layer != DL_TRANSPARENT)
				{
					cmd::DrawBatch* zppDrawBatch = AddCommand<key::DepthKey, cmd::DrawBatch, mem::StackArena>(*renderer.mShadowArena, *renderer.mDepthPrePassBucket, zppKey, 0);
					zppDrawBatch->batchHandle = subCommandsConstantBufferHandle;
					zppDrawBatch->bufferLayoutHandle = staticVertexBufferLayoutHandle;
					zppDrawBatch->numSubCommands = batchOffset.numSubCommands;
					R2_CHECK(zppDrawBatch->numSubCommands > 0, "We should have a count!");
					zppDrawBatch->startCommandIndex = batchOffset.subCommandsOffset;
					zppDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
					zppDrawBatch->subCommands = nullptr;
					zppDrawBatch->state.depthEnabled = true;
					zppDrawBatch->state.depthFunction = LESS;

					zppDrawBatch->state.depthWriteEnabled = true;
					if (batchOffset.drawState.blendState.blendingEnabled)
					{
						zppDrawBatch->state.depthWriteEnabled = false;
					}

					zppDrawBatch->state.polygonOffsetEnabled = false;
					zppDrawBatch->state.polygonOffset = glm::vec2(0);

					cmd::SetDefaultCullState(zppDrawBatch->state.cullState);
					zppDrawBatch->state.cullState.cullFace = CULL_FACE_FRONT;

					cmd::SetDefaultStencilState(zppDrawBatch->state.stencilState);

					cmd::SetDefaultBlendState(zppDrawBatch->state.blendState);

					cmd::DrawBatch* zppShadowsDrawBatch = AddCommand<key::DepthKey, cmd::DrawBatch, mem::StackArena>(*renderer.mShadowArena, *renderer.mDepthPrePassShadowBucket, zppKey, 0);
					zppShadowsDrawBatch->batchHandle = subCommandsConstantBufferHandle;
					zppShadowsDrawBatch->bufferLayoutHandle = staticVertexBufferLayoutHandle;
					zppShadowsDrawBatch->numSubCommands = batchOffset.numSubCommands;
					R2_CHECK(zppShadowsDrawBatch->numSubCommands > 0, "We should have a count!");
					zppShadowsDrawBatch->startCommandIndex = batchOffset.subCommandsOffset;
					zppShadowsDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
					zppShadowsDrawBatch->subCommands = nullptr;
					zppShadowsDrawBatch->state.depthEnabled = true;//TODO(Serge): fix with proper draw state

					zppShadowsDrawBatch->state.depthFunction = LESS;

					zppShadowsDrawBatch->state.depthWriteEnabled = true;

					zppShadowsDrawBatch->state.polygonOffsetEnabled = false;
					zppShadowsDrawBatch->state.polygonOffset = glm::vec2(0);

					cmd::SetDefaultCullState(zppShadowsDrawBatch->state.cullState);
					zppShadowsDrawBatch->state.cullState.cullFace = CULL_FACE_FRONT;

					cmd::SetDefaultStencilState(zppShadowsDrawBatch->state.stencilState);

					cmd::SetDefaultBlendState(zppShadowsDrawBatch->state.blendState);
				}
			}
		}

		

		const u64 numDynamicDrawBatches = r2::sarr::Size(*dynamicRenderBatchesOffsets);
		for (u64 i = 0; i < numDynamicDrawBatches; ++i)
		{
			const auto& batchOffset = r2::sarr::At(*dynamicRenderBatchesOffsets, i);

			ShaderHandle shaderToUse = batchOffset.shaderEffectPasses.meshPasses[flat::eMeshPass_FORWARD].dynamicShaderHandle;
			if (batchOffset.drawState.layer == DL_TRANSPARENT)
			{
				shaderToUse = batchOffset.shaderEffectPasses.meshPasses[flat::eMeshPass_TRANSPARENT].dynamicShaderHandle;
			}

			key::Basic key = key::GenerateBasicKey(key::Basic::FSL_GAME, 0, batchOffset.drawState.layer, batchOffset.blendingFunctionKeyValue, batchOffset.cameraDepth, shaderToUse);

			cmd::DrawBatch* drawBatch = nullptr;

			if (batchOffset.drawState.layer != DL_TRANSPARENT)
			{
				drawBatch = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mCommandBucket, key, 0);
				drawBatch->state.depthFunction = EQUAL;
				
				cmd::SetDefaultBlendState(drawBatch->state.blendState);
			}
			else
			{
				drawBatch = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mTransparentBucket, key, 0);
				drawBatch->state.depthFunction = LESS;
				
				SetDefaultTransparencyBlendState(drawBatch->state.blendState);
			}

			drawBatch->state.depthWriteEnabled = false;
			drawBatch->batchHandle = subCommandsConstantBufferHandle;
			drawBatch->bufferLayoutHandle = animVertexBufferLayoutHandle;
			drawBatch->numSubCommands = batchOffset.numSubCommands;
			R2_CHECK(drawBatch->numSubCommands > 0, "We should have a count!");
			drawBatch->startCommandIndex = batchOffset.subCommandsOffset;
			drawBatch->primitiveType = PrimitiveType::TRIANGLES;
			drawBatch->subCommands = nullptr;
			drawBatch->state.depthEnabled = batchOffset.drawState.depthEnabled;
			drawBatch->state.cullState = batchOffset.drawState.cullState;

			drawBatch->state.polygonOffsetEnabled = false;
			drawBatch->state.polygonOffset = glm::vec2(0);
			drawBatch->state.stencilState = batchOffset.drawState.stencilState;

#ifdef R2_EDITOR
			key::Basic dynamicEditorPickingKey = key::GenerateBasicKey(key::Basic::FSL_GAME, 0, batchOffset.drawState.layer, 0, batchOffset.cameraDepth, renderer.mEntityColorShader[1]);

			cmd::DrawBatch* editorPickingDrawBatchCMD = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mEditorPickingBucket, dynamicEditorPickingKey, 0);
			editorPickingDrawBatchCMD->state.depthFunction = EQUAL;

			editorPickingDrawBatchCMD->state.depthWriteEnabled = false;
			editorPickingDrawBatchCMD->batchHandle = subCommandsConstantBufferHandle;
			editorPickingDrawBatchCMD->bufferLayoutHandle = animVertexBufferLayoutHandle;
			editorPickingDrawBatchCMD->numSubCommands = batchOffset.numSubCommands;
			R2_CHECK(editorPickingDrawBatchCMD->numSubCommands > 0, "We should have a count!");
			editorPickingDrawBatchCMD->startCommandIndex = batchOffset.subCommandsOffset;
			editorPickingDrawBatchCMD->primitiveType = PrimitiveType::TRIANGLES;
			editorPickingDrawBatchCMD->subCommands = nullptr;
			editorPickingDrawBatchCMD->state.depthEnabled = batchOffset.drawState.depthEnabled;
			editorPickingDrawBatchCMD->state.cullState = batchOffset.drawState.cullState;

			editorPickingDrawBatchCMD->state.polygonOffsetEnabled = false;
			editorPickingDrawBatchCMD->state.polygonOffset = glm::vec2(0);

			cmd::SetDefaultStencilState(editorPickingDrawBatchCMD->state.stencilState);
			cmd::SetDefaultBlendState(editorPickingDrawBatchCMD->state.blendState);
#endif


			//memcpy(&drawBatch->state.blendState, &batchOffset.drawState.blendState, sizeof(BlendState));
		//	cmd::SetDefaultBlendState(drawBatch->state.blendState);


			//if (!batchOffset.drawState.blendState.blendingEnabled)
			{
				key::ShadowKey directionShadowKey = key::GenerateShadowKey(key::ShadowKey::NORMAL, 0, 0, true, light::LightType::LT_DIRECTIONAL_LIGHT, batchOffset.cameraDepth);

				//@TODO(Serge): we don't loop over each cascade in the geometry shader for some reason so we have to do this extra divide by NUM_FRUSTUM_SPLITS, we should probably loop in there and update
				const u32 numDirectionShadowBatchesNeeded = static_cast<u32>(glm::max(glm::ceil((float)numShadowCastingDirectionLights / ((float)MAX_NUM_GEOMETRY_SHADER_INVOCATIONS / (float)cam::NUM_FRUSTUM_SPLITS)), numShadowCastingDirectionLights > 0 ? 1.0f : 0.0f));

				for (u32 i = 0; i < numDirectionShadowBatchesNeeded; ++i)
				{
					cmd::ConstantUint* directionLightBatchIndexUpdateCMD = AddCommand<key::ShadowKey, cmd::ConstantUint, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, directionShadowKey, 0);
					directionLightBatchIndexUpdateCMD->value = i;
					directionLightBatchIndexUpdateCMD->uniformLocation = renderer.mDynamicDirectionLightBatchUniformLocation;

					cmd::DrawBatch* shadowDrawBatch = AppendCommand<cmd::ConstantUint, cmd::DrawBatch, mem::StackArena>(*renderer.mShadowArena, directionLightBatchIndexUpdateCMD, 0);

					shadowDrawBatch->batchHandle = subCommandsConstantBufferHandle;
					shadowDrawBatch->bufferLayoutHandle = animVertexBufferLayoutHandle;
					shadowDrawBatch->numSubCommands = batchOffset.numSubCommands;
					R2_CHECK(shadowDrawBatch->numSubCommands > 0, "We should have a count!");
					shadowDrawBatch->startCommandIndex = batchOffset.subCommandsOffset;
					shadowDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
					shadowDrawBatch->subCommands = nullptr;
					shadowDrawBatch->state.depthEnabled = true;
					shadowDrawBatch->state.depthFunction = LESS;
					shadowDrawBatch->state.depthWriteEnabled = true;
					shadowDrawBatch->state.polygonOffsetEnabled = false;
					shadowDrawBatch->state.polygonOffset = glm::vec2(0);

					cmd::SetDefaultCullState(shadowDrawBatch->state.cullState);
					shadowDrawBatch->state.cullState.cullFace = CULL_FACE_FRONT;

					cmd::SetDefaultStencilState(shadowDrawBatch->state.stencilState);

					cmd::SetDefaultBlendState(shadowDrawBatch->state.blendState);
				}

				const u32 numSpotLightShadowBatchesNeeded = static_cast<u32>(glm::max(glm::ceil((float)numShadowCastingSpotLights / (float)MAX_NUM_GEOMETRY_SHADER_INVOCATIONS), numShadowCastingSpotLights > 0 ? 1.0f : 0.0f));

				key::ShadowKey spotLightShadowKey = key::GenerateShadowKey(key::ShadowKey::NORMAL, 0, 0, true, light::LightType::LT_SPOT_LIGHT, batchOffset.cameraDepth);

				for (u32 i = 0; i < numSpotLightShadowBatchesNeeded; ++i)
				{
					cmd::ConstantUint* spotLightBatchIndexUpdateCMD = AddCommand<key::ShadowKey, cmd::ConstantUint, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, spotLightShadowKey, 0);
					spotLightBatchIndexUpdateCMD->value = i;
					spotLightBatchIndexUpdateCMD->uniformLocation = renderer.mDynamicSpotLightBatchUniformLocation;

					cmd::DrawBatch* shadowDrawBatch = AppendCommand<cmd::ConstantUint, cmd::DrawBatch, mem::StackArena>(*renderer.mShadowArena, spotLightBatchIndexUpdateCMD, 0);

					shadowDrawBatch->batchHandle = subCommandsConstantBufferHandle;
					shadowDrawBatch->bufferLayoutHandle = animVertexBufferLayoutHandle;
					shadowDrawBatch->numSubCommands = batchOffset.numSubCommands;
					R2_CHECK(shadowDrawBatch->numSubCommands > 0, "We should have a count!");
					shadowDrawBatch->startCommandIndex = batchOffset.subCommandsOffset;
					shadowDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
					shadowDrawBatch->subCommands = nullptr;
					shadowDrawBatch->state.depthEnabled = true;
					shadowDrawBatch->state.depthFunction = LESS;
					shadowDrawBatch->state.depthWriteEnabled = true;
					shadowDrawBatch->state.polygonOffsetEnabled = false;
					shadowDrawBatch->state.polygonOffset = glm::vec2(0);

					cmd::SetDefaultCullState(shadowDrawBatch->state.cullState);
					shadowDrawBatch->state.cullState.cullFace = CULL_FACE_FRONT;

					cmd::SetDefaultStencilState(shadowDrawBatch->state.stencilState);

					cmd::SetDefaultBlendState(shadowDrawBatch->state.blendState);
				}

				const u32 numPointLightShadowBatchesNeeded = static_cast<u32>(glm::max(glm::ceil((float)numShadowCastingPointLights / (float)MAX_NUM_GEOMETRY_SHADER_INVOCATIONS), numShadowCastingPointLights > 0 ? 1.0f : 0.0f));

				key::ShadowKey plShadowKey = key::GenerateShadowKey(key::ShadowKey::POINT_LIGHT, 0, 0, true, light::LightType::LT_POINT_LIGHT, batchOffset.cameraDepth);

				for (u32 i = 0; i < numPointLightShadowBatchesNeeded; ++i)
				{
					cmd::ConstantUint* pointLightBatchIndexUpdateCMD = AddCommand<key::ShadowKey, cmd::ConstantUint, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, plShadowKey, 0);
					pointLightBatchIndexUpdateCMD->value = i;
					pointLightBatchIndexUpdateCMD->uniformLocation = renderer.mDynamicPointLightBatchUniformLocation;

					cmd::DrawBatch* shadowDrawBatch = AppendCommand<cmd::ConstantUint, cmd::DrawBatch, mem::StackArena>(*renderer.mShadowArena, pointLightBatchIndexUpdateCMD, 0);

					shadowDrawBatch->batchHandle = subCommandsConstantBufferHandle;
					shadowDrawBatch->bufferLayoutHandle = animVertexBufferLayoutHandle;
					shadowDrawBatch->numSubCommands = batchOffset.numSubCommands;
					R2_CHECK(shadowDrawBatch->numSubCommands > 0, "We should have a count!");
					shadowDrawBatch->startCommandIndex = batchOffset.subCommandsOffset;
					shadowDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
					shadowDrawBatch->subCommands = nullptr;
					shadowDrawBatch->state.depthEnabled = true;
					shadowDrawBatch->state.depthFunction = LESS;
					shadowDrawBatch->state.depthWriteEnabled = true;
					shadowDrawBatch->state.polygonOffsetEnabled = false;
					shadowDrawBatch->state.polygonOffset = glm::vec2(0);

					cmd::SetDefaultCullState(shadowDrawBatch->state.cullState);
					shadowDrawBatch->state.cullState.cullFace = CULL_FACE_FRONT;

					cmd::SetDefaultStencilState(shadowDrawBatch->state.stencilState);

					cmd::SetDefaultBlendState(shadowDrawBatch->state.blendState);
				}

				key::DepthKey zppKey = key::GenerateDepthKey(key::DepthKey::NORMAL, 0, batchOffset.shaderEffectPasses.meshPasses[flat::eMeshPass_DEPTH].dynamicShaderHandle, r2::draw::DrawLayer::DL_CHARACTER);

				if (batchOffset.drawState.layer != DL_TRANSPARENT)
				{
					cmd::DrawBatch* zppDrawBatch = AddCommand<key::DepthKey, cmd::DrawBatch, mem::StackArena>(*renderer.mShadowArena, *renderer.mDepthPrePassBucket, zppKey, 0);
					zppDrawBatch->batchHandle = subCommandsConstantBufferHandle;
					zppDrawBatch->bufferLayoutHandle = animVertexBufferLayoutHandle;
					zppDrawBatch->numSubCommands = batchOffset.numSubCommands;
					R2_CHECK(zppDrawBatch->numSubCommands > 0, "We should have a count!");
					zppDrawBatch->startCommandIndex = batchOffset.subCommandsOffset;
					zppDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
					zppDrawBatch->subCommands = nullptr;
					zppDrawBatch->state.depthEnabled = true;

					zppDrawBatch->state.depthWriteEnabled = true;
					if (batchOffset.drawState.blendState.blendingEnabled)
					{
						zppDrawBatch->state.depthWriteEnabled = false;
					}

					zppDrawBatch->state.depthFunction = LESS;

					zppDrawBatch->state.polygonOffsetEnabled = false;
					zppDrawBatch->state.polygonOffset = glm::vec2(0);

					cmd::SetDefaultCullState(zppDrawBatch->state.cullState);
					zppDrawBatch->state.cullState.cullFace = CULL_FACE_FRONT;

					cmd::SetDefaultStencilState(zppDrawBatch->state.stencilState);

					cmd::SetDefaultBlendState(zppDrawBatch->state.blendState);


					//@TODO(Serge): figure out a better way to do this - the issue is the z reduce will screw up if we're not drawing any static meshes (unlikely to happen often but could if we do say an animation tool)
					bool shouldIncludeAnimatedModelsInZPPShadows = (numStaticDrawBatches == 0);

					if (numStaticDrawBatches == 1)
					{
						const auto& staticBatchOffset = r2::sarr::At(*staticRenderBatchesOffsets, 0);
						shouldIncludeAnimatedModelsInZPPShadows = staticBatchOffset.drawState.layer == DL_SKYBOX;
					}

					if (shouldIncludeAnimatedModelsInZPPShadows)
					{
						cmd::DrawBatch* zppShadowsDrawBatch = AddCommand<key::DepthKey, cmd::DrawBatch, mem::StackArena>(*renderer.mShadowArena, *renderer.mDepthPrePassShadowBucket, zppKey, 0);
						zppShadowsDrawBatch->batchHandle = subCommandsConstantBufferHandle;
						zppShadowsDrawBatch->bufferLayoutHandle = animVertexBufferLayoutHandle;
						zppShadowsDrawBatch->numSubCommands = batchOffset.numSubCommands;
						R2_CHECK(zppShadowsDrawBatch->numSubCommands > 0, "We should have a count!");
						zppShadowsDrawBatch->startCommandIndex = batchOffset.subCommandsOffset;
						zppShadowsDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
						zppShadowsDrawBatch->subCommands = nullptr;
						zppShadowsDrawBatch->state.depthEnabled = true;//TODO(Serge): fix with proper draw state

						zppShadowsDrawBatch->state.depthFunction = LESS;

						zppShadowsDrawBatch->state.depthWriteEnabled = true;

						zppShadowsDrawBatch->state.polygonOffsetEnabled = false;
						zppShadowsDrawBatch->state.polygonOffset = glm::vec2(0);

						cmd::SetDefaultCullState(zppShadowsDrawBatch->state.cullState);
						zppShadowsDrawBatch->state.cullState.cullFace = CULL_FACE_FRONT;

						cmd::SetDefaultStencilState(zppShadowsDrawBatch->state.stencilState);

						cmd::SetDefaultBlendState(zppShadowsDrawBatch->state.blendState);
					}
				}
			}
		}

		key::Basic copyGBufferKey = key::GenerateBasicKey(0, 0, DL_SCREEN, 0, 0, 0, 0);

		cmd::CopyRenderTargetColorTexture* copyGBufferCMD = AddCommand<key::Basic, cmd::CopyRenderTargetColorTexture, mem::StackArena>(*renderer.mCommandArena, *renderer.mCommandBucket, copyGBufferKey, 0);

		const auto gbufferConvolvedColorAttachment = r2::sarr::At(*renderer.mRenderTargets[RTS_CONVOLVED_GBUFFER].colorAttachments, 0);

		copyGBufferCMD->frameBufferID = renderer.mRenderTargets[RTS_GBUFFER].frameBufferID;
		copyGBufferCMD->attachment = 0;
		copyGBufferCMD->toTextureID = gbufferConvolvedColorAttachment.texture[gbufferConvolvedColorAttachment.currentTexture].container->texId;
		copyGBufferCMD->xOffset = 0;
		copyGBufferCMD->yOffset = 0;
		copyGBufferCMD->layer = gbufferConvolvedColorAttachment.texture[gbufferConvolvedColorAttachment.currentTexture].sliceIndex;
		copyGBufferCMD->mipLevel = 0;
		copyGBufferCMD->x = 0;
		copyGBufferCMD->y = 0;
		copyGBufferCMD->width = renderer.mRenderTargets[RTS_GBUFFER].width;
		copyGBufferCMD->height = renderer.mRenderTargets[RTS_GBUFFER].height;

		//previous texture
		const auto previousTexture = (gbufferConvolvedColorAttachment.currentTexture + 1) % gbufferConvolvedColorAttachment.numTextures;//@NOTE only works for 2 textures atm


		key::Basic transparentCompositeClearKey = key::GenerateBasicKey(key::Basic::FSL_EFFECT, 0, DL_TRANSPARENT, 0, 0, renderer.mTransparentCompositeShader, 0);
		key::Basic transparentCompositeDrawKey = key::GenerateBasicKey(key::Basic::FSL_EFFECT, 0, DL_TRANSPARENT, 0, 0, renderer.mTransparentCompositeShader, 1);

		ClearSurfaceOptions transparentCompositeClearOptions;
		transparentCompositeClearOptions.shouldClear = false;


		BeginRenderPass<key::Basic>(renderer, RPT_TRANSPARENT_COMPOSITE, transparentCompositeClearOptions, *renderer.mTransparentBucket, transparentCompositeClearKey, *renderer.mCommandArena);

		cmd::DrawBatch* transparentCompositeCMD = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mTransparentBucket, transparentCompositeDrawKey, 0);
		transparentCompositeCMD->batchHandle = subCommandsConstantBufferHandle;
		transparentCompositeCMD->bufferLayoutHandle = finalBatchVertexBufferLayoutHandle;
		transparentCompositeCMD->numSubCommands = finalBatchOffsets.numSubCommands;
		R2_CHECK(transparentCompositeCMD->numSubCommands > 0, "We should have a count!");
		transparentCompositeCMD->startCommandIndex = finalBatchOffsets.subCommandsOffset;
		transparentCompositeCMD->primitiveType = PrimitiveType::TRIANGLES;
		transparentCompositeCMD->subCommands = nullptr;
		transparentCompositeCMD->state.depthEnabled = true;
		transparentCompositeCMD->state.depthFunction = LESS;
		transparentCompositeCMD->state.depthWriteEnabled = false;
		transparentCompositeCMD->state.polygonOffsetEnabled = false;
		transparentCompositeCMD->state.polygonOffset = glm::vec2(0);

		cmd::SetDefaultStencilState(transparentCompositeCMD->state.stencilState);
		cmd::SetDefaultCullState(transparentCompositeCMD->state.cullState);
		
		//cmd::SetDefaultBlendState(transparentCompositeCMD->state)
		SetDefaultTransparentCompositeBlendState(transparentCompositeCMD->state.blendState);


#ifdef R2_EDITOR
		EndRenderPass<key::Basic>(renderer, RPT_EDITOR_PICKING, *renderer.mEditorPickingBucket);
#endif

		EndRenderPass<key::Basic>(renderer, RPT_TRANSPARENT_COMPOSITE, *renderer.mTransparentBucket);

		

		const auto gbufferColorAttachment = r2::sarr::At(*renderer.mRenderTargets[RTS_GBUFFER].colorAttachments, 0);

		//DO THE MIP VERTICAL BLUR HERE 
		for (u32 mipLevel = 1; mipLevel < gbufferConvolvedColorAttachment.format.mipLevels; ++mipLevel)
		{
			key::Basic mipKey = key::GenerateBasicKey(0, 0, DL_SCREEN, 0, 0, renderer.mVerticalBlurShader, mipLevel);

			cmd::SetRenderTargetMipLevel* setRenderTargetMip = AddCommand<key::Basic, cmd::SetRenderTargetMipLevel, mem::StackArena>(*renderer.mCommandArena, *renderer.mCommandBucket, mipKey, 0);

			cmd::FillSetRenderTargetMipLevelCommandWithTextureIndex(renderer.mRenderTargets[RTS_CONVOLVED_GBUFFER], mipLevel, previousTexture, *setRenderTargetMip);

			cmd::SetTexture* setColorBufferTexture = AppendCommand<cmd::SetRenderTargetMipLevel, cmd::SetTexture, mem::StackArena>(*renderer.mCommandArena, setRenderTargetMip, 0);

			setColorBufferTexture->textureContainerUniformLocation = renderer.mVerticalBlurTextureContainerLocation;
			setColorBufferTexture->textureContainer = gbufferColorAttachment.texture[gbufferColorAttachment.currentTexture].container->handle;
			setColorBufferTexture->texturePageUniformLocation = renderer.mVerticalBlurTexturePageLocation;
			setColorBufferTexture->texturePage = gbufferColorAttachment.texture[gbufferColorAttachment.currentTexture].sliceIndex;
			setColorBufferTexture->textureLodUniformLocation = renderer.mVerticalBlurTextureLodLocation;
			setColorBufferTexture->textureLod = 0.0f;

			cmd::DrawBatch* drawMipBatch = AppendCommand<cmd::SetTexture, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, setColorBufferTexture, 0);
			drawMipBatch->batchHandle = subCommandsConstantBufferHandle;
			drawMipBatch->bufferLayoutHandle = finalBatchVertexBufferLayoutHandle;
			drawMipBatch->numSubCommands = finalBatchOffsets.numSubCommands;
			R2_CHECK(drawMipBatch->numSubCommands > 0, "We should have a count!");
			drawMipBatch->startCommandIndex = finalBatchOffsets.subCommandsOffset;
			drawMipBatch->primitiveType = PrimitiveType::TRIANGLES;
			drawMipBatch->subCommands = nullptr;
			drawMipBatch->state.depthEnabled = false;
			drawMipBatch->state.depthFunction = LESS;
			drawMipBatch->state.depthWriteEnabled = false;
			drawMipBatch->state.polygonOffsetEnabled = false;
			drawMipBatch->state.polygonOffset = glm::vec2(0);

			cmd::SetDefaultCullState(drawMipBatch->state.cullState);

			cmd::SetDefaultStencilState(drawMipBatch->state.stencilState);

			cmd::SetDefaultBlendState(drawMipBatch->state.blendState);
		}

		//DO THE MIP HORIZONTAL BLUR HERE 
		for (u32 mipLevel = 1; mipLevel < gbufferConvolvedColorAttachment.format.mipLevels; ++mipLevel)
		{
			key::Basic mipKey = key::GenerateBasicKey(0, 0, DL_SCREEN, 0, 1, renderer.mHorizontalBlurShader, mipLevel);

			cmd::SetRenderTargetMipLevel* setRenderTargetMip = AddCommand<key::Basic, cmd::SetRenderTargetMipLevel, mem::StackArena>(*renderer.mCommandArena, *renderer.mCommandBucket, mipKey, 0);

			cmd::FillSetRenderTargetMipLevelCommandWithTextureIndex(renderer.mRenderTargets[RTS_CONVOLVED_GBUFFER], mipLevel, gbufferConvolvedColorAttachment.currentTexture, *setRenderTargetMip);

			cmd::SetTexture* setColorBufferTexture = AppendCommand<cmd::SetRenderTargetMipLevel, cmd::SetTexture, mem::StackArena>(*renderer.mCommandArena, setRenderTargetMip, 0);

			setColorBufferTexture->textureContainerUniformLocation = renderer.mVerticalBlurTextureContainerLocation;
			setColorBufferTexture->textureContainer = gbufferConvolvedColorAttachment.texture[previousTexture].container->handle;
			setColorBufferTexture->texturePageUniformLocation = renderer.mVerticalBlurTexturePageLocation;
			setColorBufferTexture->texturePage = gbufferConvolvedColorAttachment.texture[previousTexture].sliceIndex;
			setColorBufferTexture->textureLodUniformLocation = renderer.mVerticalBlurTextureLodLocation;
			setColorBufferTexture->textureLod = static_cast<float>(mipLevel);

			cmd::DrawBatch* drawMipBatch = AppendCommand<cmd::SetTexture, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, setColorBufferTexture, 0);
			drawMipBatch->batchHandle = subCommandsConstantBufferHandle;
			drawMipBatch->bufferLayoutHandle = finalBatchVertexBufferLayoutHandle;
			drawMipBatch->numSubCommands = finalBatchOffsets.numSubCommands;
			R2_CHECK(drawMipBatch->numSubCommands > 0, "We should have a count!");
			drawMipBatch->startCommandIndex = finalBatchOffsets.subCommandsOffset;
			drawMipBatch->primitiveType = PrimitiveType::TRIANGLES;
			drawMipBatch->subCommands = nullptr;
			drawMipBatch->state.depthEnabled = false;
			drawMipBatch->state.depthFunction = LESS;
			drawMipBatch->state.depthWriteEnabled = false;
			drawMipBatch->state.polygonOffsetEnabled = false;
			drawMipBatch->state.polygonOffset = glm::vec2(0);

			cmd::SetDefaultCullState(drawMipBatch->state.cullState);

			cmd::SetDefaultStencilState(drawMipBatch->state.stencilState);

			cmd::SetDefaultBlendState(drawMipBatch->state.blendState);
		}
	

		EndRenderPass(renderer, RPT_ZPREPASS, *renderer.mDepthPrePassBucket);
		EndRenderPass(renderer, RPT_SHADOWS, *renderer.mShadowBucket);
		EndRenderPass(renderer, RPT_POINTLIGHT_SHADOWS, *renderer.mShadowBucket);
		EndRenderPass(renderer, RPT_GBUFFER, *renderer.mCommandBucket);
		EndRenderPass(renderer, RPT_ZPREPASS_SHADOWS, *renderer.mDepthPrePassShadowBucket);
		EndRenderPass(renderer, RPT_CLUSTERS, *renderer.mClustersBucket);

		ClearSurfaceOptions clearCompositeOptions;
		clearCompositeOptions.shouldClear = true;
		clearCompositeOptions.flags = cmd::CLEAR_COLOR_BUFFER;


		key::DepthKey aoKey = key::GenerateDepthKey(key::DepthKey::COMPUTE, 0, renderer.mAmbientOcclusionShader, DL_SCREEN);
		BeginRenderPass<key::DepthKey>(renderer, RPT_AMBIENT_OCCLUSION, clearCompositeOptions, *renderer.mAmbientOcclusionBucket, aoKey, *renderer.mAmbientOcclusionArena);
		cmd::DrawBatch* aoDrawBatch = AddCommand<key::DepthKey, cmd::DrawBatch, mem::StackArena>(*renderer.mAmbientOcclusionArena, *renderer.mAmbientOcclusionBucket, aoKey, 0);
		aoDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		aoDrawBatch->bufferLayoutHandle = finalBatchVertexBufferLayoutHandle;
		aoDrawBatch->numSubCommands = finalBatchOffsets.numSubCommands;
		R2_CHECK(aoDrawBatch->numSubCommands > 0, "We should have a count!");
		aoDrawBatch->startCommandIndex = finalBatchOffsets.subCommandsOffset;
		aoDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		aoDrawBatch->subCommands = nullptr;
		aoDrawBatch->state.depthEnabled = false;
		aoDrawBatch->state.depthFunction = LESS;
		aoDrawBatch->state.depthWriteEnabled = false;
		aoDrawBatch->state.polygonOffsetEnabled = false;
		aoDrawBatch->state.polygonOffset = glm::vec2(0);

		cmd::SetDefaultStencilState(aoDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(aoDrawBatch->state.blendState);
		cmd::SetDefaultCullState(aoDrawBatch->state.cullState);

		EndRenderPass(renderer, RPT_AMBIENT_OCCLUSION, *renderer.mAmbientOcclusionBucket);

		key::DepthKey aoDenoiseKey = key::GenerateDepthKey(key::DepthKey::COMPUTE, 0, renderer.mDenoiseShader, DL_SCREEN);
		BeginRenderPass<key::DepthKey>(renderer, RPT_AMBIENT_OCCLUSION_DENOISE, clearCompositeOptions, *renderer.mAmbientOcclusionDenoiseBucket, aoDenoiseKey, *renderer.mAmbientOcclusionArena);
		cmd::DrawBatch* aoDenoiseDrawBatch = AddCommand<key::DepthKey, cmd::DrawBatch, mem::StackArena>(*renderer.mAmbientOcclusionArena, *renderer.mAmbientOcclusionDenoiseBucket, aoDenoiseKey, 0);
		aoDenoiseDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		aoDenoiseDrawBatch->bufferLayoutHandle = finalBatchVertexBufferLayoutHandle;
		aoDenoiseDrawBatch->numSubCommands = finalBatchOffsets.numSubCommands;
		R2_CHECK(aoDenoiseDrawBatch->numSubCommands > 0, "We should have a count!");
		aoDenoiseDrawBatch->startCommandIndex = finalBatchOffsets.subCommandsOffset;
		aoDenoiseDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		aoDenoiseDrawBatch->subCommands = nullptr;
		aoDenoiseDrawBatch->state.depthEnabled = false;
		aoDenoiseDrawBatch->state.depthFunction = LESS;
		aoDenoiseDrawBatch->state.depthWriteEnabled = false;
		aoDenoiseDrawBatch->state.polygonOffsetEnabled = false;
		aoDenoiseDrawBatch->state.polygonOffset = glm::vec2(0);
		
		cmd::SetDefaultCullState(aoDenoiseDrawBatch->state.cullState);

		cmd::SetDefaultStencilState(aoDenoiseDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(aoDenoiseDrawBatch->state.blendState);

		EndRenderPass(renderer, RPT_AMBIENT_OCCLUSION_DENOISE, *renderer.mAmbientOcclusionDenoiseBucket);



		key::DepthKey aoTemporalDenoiseKey = key::GenerateDepthKey(key::DepthKey::COMPUTE, 0, renderer.mAmbientOcclusionTemporalDenoiseShader, DL_SCREEN);

		BeginRenderPass<key::DepthKey>(renderer, RPT_AMBIENT_OCCLUSION_TEMPORAL_DENOISE, clearCompositeOptions, *renderer.mAmbientOcclusionTemporalDenoiseBucket, aoTemporalDenoiseKey, *renderer.mAmbientOcclusionArena);
		cmd::DrawBatch* aoTemporalDenoiseDrawBatch = AddCommand<key::DepthKey, cmd::DrawBatch, mem::StackArena>(*renderer.mAmbientOcclusionArena, *renderer.mAmbientOcclusionTemporalDenoiseBucket, aoTemporalDenoiseKey, 0);
		aoTemporalDenoiseDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		aoTemporalDenoiseDrawBatch->bufferLayoutHandle = finalBatchVertexBufferLayoutHandle;
		aoTemporalDenoiseDrawBatch->numSubCommands = finalBatchOffsets.numSubCommands;
		R2_CHECK(aoTemporalDenoiseDrawBatch->numSubCommands > 0, "We should have a count!");
		aoTemporalDenoiseDrawBatch->startCommandIndex = finalBatchOffsets.subCommandsOffset;
		aoTemporalDenoiseDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		aoTemporalDenoiseDrawBatch->subCommands = nullptr;
		aoTemporalDenoiseDrawBatch->state.depthEnabled = false;
		aoTemporalDenoiseDrawBatch->state.depthFunction = LESS;
		aoTemporalDenoiseDrawBatch->state.depthWriteEnabled = false;
		aoTemporalDenoiseDrawBatch->state.polygonOffsetEnabled = false;
		aoTemporalDenoiseDrawBatch->state.polygonOffset = glm::vec2(0);

		cmd::SetDefaultCullState(aoTemporalDenoiseDrawBatch->state.cullState);
		cmd::SetDefaultStencilState(aoTemporalDenoiseDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(aoTemporalDenoiseDrawBatch->state.blendState);

		EndRenderPass(renderer, RPT_AMBIENT_OCCLUSION_TEMPORAL_DENOISE, *renderer.mAmbientOcclusionTemporalDenoiseBucket);


		key::Basic ssrBatchClearKey = key::GenerateBasicKey(0, 0, DL_CLEAR, 0, 0, renderer.mSSRShader);
		key::Basic ssrDrawKey = key::GenerateBasicKey(0, 0, DL_SCREEN, 0, 0, renderer.mSSRShader);

		BeginRenderPass<key::Basic>(renderer, RPT_SSR, clearCompositeOptions, *renderer.mSSRBucket, ssrBatchClearKey, *renderer.mCommandArena);
		cmd::DrawBatch* ssrDrawBatch = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mSSRBucket, ssrDrawKey, 0);
		ssrDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		ssrDrawBatch->bufferLayoutHandle = finalBatchVertexBufferLayoutHandle;
		ssrDrawBatch->numSubCommands = finalBatchOffsets.numSubCommands;
		R2_CHECK(ssrDrawBatch->numSubCommands > 0, "We should have a count!");
		ssrDrawBatch->startCommandIndex = finalBatchOffsets.subCommandsOffset;
		ssrDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		ssrDrawBatch->subCommands = nullptr;
		ssrDrawBatch->state.depthEnabled = false;
		ssrDrawBatch->state.depthFunction = LESS;
		ssrDrawBatch->state.depthWriteEnabled = false;
		ssrDrawBatch->state.polygonOffsetEnabled = false;
		ssrDrawBatch->state.polygonOffset = glm::vec2(0);
		
		cmd::SetDefaultCullState(ssrDrawBatch->state.cullState);
		cmd::SetDefaultStencilState(ssrDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(ssrDrawBatch->state.blendState);

		key::Basic ssrConeTracedDrawKey = key::GenerateBasicKey(0, 0, DL_SCREEN, 0, 1, renderer.mSSRConeTraceShader, 0);

		cmd::SetRenderTargetMipLevel* setSSRConeTracedRenderTarget = AddCommand<key::Basic, cmd::SetRenderTargetMipLevel, mem::StackArena>(*renderer.mCommandArena, *renderer.mSSRBucket, ssrConeTracedDrawKey, 0);

		RenderPass* ssrRenderPass = GetRenderPass(renderer, RPT_SSR);
		R2_CHECK(ssrRenderPass != nullptr, "Should be not nullptr");

		cmd::FillSetRenderTargetMipLevelCommand(renderer.mRenderTargets[RTS_SSR_CONE_TRACED], 0, *setSSRConeTracedRenderTarget, ssrRenderPass->config);

		cmd::DrawBatch* ssrConeTracedBatch = AppendCommand<cmd::SetRenderTargetMipLevel, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, setSSRConeTracedRenderTarget, 0);
		ssrConeTracedBatch->batchHandle = subCommandsConstantBufferHandle;
		ssrConeTracedBatch->bufferLayoutHandle = finalBatchVertexBufferLayoutHandle;
		ssrConeTracedBatch->numSubCommands = finalBatchOffsets.numSubCommands;
		R2_CHECK(ssrConeTracedBatch->numSubCommands > 0, "We should have a count!");
		ssrConeTracedBatch->startCommandIndex = finalBatchOffsets.subCommandsOffset;
		ssrConeTracedBatch->primitiveType = PrimitiveType::TRIANGLES;
		ssrConeTracedBatch->subCommands = nullptr;
		ssrConeTracedBatch->state.depthEnabled = false;
		ssrConeTracedBatch->state.depthFunction = LESS;
		ssrConeTracedBatch->state.depthWriteEnabled = false;
		ssrConeTracedBatch->state.polygonOffsetEnabled = false;
		ssrConeTracedBatch->state.polygonOffset = glm::vec2(0);

		cmd::SetDefaultCullState(ssrConeTracedBatch->state.cullState);
		cmd::SetDefaultStencilState(ssrConeTracedBatch->state.stencilState);
		cmd::SetDefaultBlendState(ssrConeTracedBatch->state.blendState);

		EndRenderPass(renderer, RPT_SSR, *renderer.mSSRBucket);

		//@NOTE: need to clear the bloom render target
		//key::Basic clearBloomKey = key::GenerateBasicKey(0, 0, DL_CLEAR, 0, 0, 0);

		//BeginRenderPass<key::Basic>(renderer, RPT_BLOOM, clearGBufferOptions, *renderer.mCommandBucket, clearBloomKey, *renderer.mCommandArena);
		//EndRenderPass<key::Basic>(renderer, RPT_BLOOM, *renderer.mCommandBucket);

		key::Basic finalBatchClearKey = key::GenerateBasicKey(0, 0, DL_CLEAR, 0, 0, finalBatchOffsets.shaderEffectPasses.meshPasses[flat::eMeshPass_FORWARD].staticShaderHandle);

		BeginRenderPass<key::Basic>(renderer, RPT_FINAL_COMPOSITE, clearCompositeOptions, *renderer.mFinalBucket, finalBatchClearKey, *renderer.mCommandArena);

		key::Basic finalBatchKey = key::GenerateBasicKey(0, 0, finalBatchOffsets.drawState.layer, 0, 0, finalBatchOffsets.shaderEffectPasses.meshPasses[flat::eMeshPass_FORWARD].staticShaderHandle);

		cmd::DrawBatch* finalDrawBatch = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mFinalBucket, finalBatchKey, 0); //@TODO(Serge): we should have mFinalBucket have it's own arena instead of renderer.mCommandArena
		finalDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		finalDrawBatch->bufferLayoutHandle = finalBatchVertexBufferLayoutHandle;
		finalDrawBatch->numSubCommands = finalBatchOffsets.numSubCommands;
		R2_CHECK(finalDrawBatch->numSubCommands > 0, "We should have a count!");
		finalDrawBatch->startCommandIndex = finalBatchOffsets.subCommandsOffset;
		finalDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		finalDrawBatch->subCommands = nullptr;
		finalDrawBatch->state.depthEnabled = false;
		finalDrawBatch->state.depthFunction = LESS;
		finalDrawBatch->state.depthWriteEnabled = true; //needs to be set for some reason even though depth isn't enabled?
		finalDrawBatch->state.polygonOffsetEnabled = false;
		finalDrawBatch->state.polygonOffset = glm::vec2(0);
		
		cmd::SetDefaultCullState(finalDrawBatch->state.cullState);
		cmd::SetDefaultStencilState(finalDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(finalDrawBatch->state.blendState);

		EndRenderPass(renderer, RPT_FINAL_COMPOSITE, *renderer.mFinalBucket);

		switch (renderer.mOutputMerger)
		{
		case OUTPUT_NO_AA:
			NonAARenderPass(renderer, subCommandsConstantBufferHandle, finalBatchVertexBufferLayoutHandle, finalBatchOffsets.numSubCommands, finalBatchOffsets.subCommandsOffset);
			break;
		case OUTPUT_FXAA:
			FXAARenderPass(renderer, subCommandsConstantBufferHandle, finalBatchVertexBufferLayoutHandle, finalBatchOffsets.numSubCommands, finalBatchOffsets.subCommandsOffset);
			break;
		case OUTPUT_SMAA_X1:
			SMAAx1RenderPass(renderer, subCommandsConstantBufferHandle, finalBatchVertexBufferLayoutHandle, finalBatchOffsets.numSubCommands, finalBatchOffsets.subCommandsOffset, renderer.mSMAANeighborhoodBlendingShader, 0);
			SMAAx1OutputRenderPass(renderer, subCommandsConstantBufferHandle, finalBatchVertexBufferLayoutHandle, finalBatchOffsets.numSubCommands, finalBatchOffsets.subCommandsOffset, renderer.mPassThroughShader, 0);
			break;
		case OUTPUT_SMAA_T2X:
			SMAAx1RenderPass(renderer, subCommandsConstantBufferHandle, finalBatchVertexBufferLayoutHandle, finalBatchOffsets.numSubCommands, finalBatchOffsets.subCommandsOffset, renderer.mSMAANeighborhoodBlendingReprojectionShader, 0);
			SMAAxT2SOutputRenderPass(renderer, subCommandsConstantBufferHandle, finalBatchVertexBufferLayoutHandle, finalBatchOffsets.numSubCommands, finalBatchOffsets.subCommandsOffset, renderer.mSMAAReprojectResolveShader, 0);
			break;
		default:
			R2_CHECK(false, "Unsupported output merger type!");
		}

		r2::sarr::Clear(*renderer.mRenderMaterialsToRender);

		RESET_ARENA(*renderer.mPreRenderStackArena);
	}

	void ClearRenderBatches(Renderer& renderer)
	{
		u32 numRenderBatches = r2::sarr::Size(*renderer.mRenderBatches);

		for (u32 i = 0; i < numRenderBatches; ++i)
		{
			RenderBatch& batch = r2::sarr::At(*renderer.mRenderBatches, i);

			r2::sarr::Clear(*batch.gpuModelRefs);
			r2::sarr::Clear(*batch.materialBatch.infos);
			r2::sarr::Clear(*batch.materialBatch.renderMaterialParams);
			r2::sarr::Clear(*batch.materialBatch.shaderEffectPasses);
			//r2::sarr::Clear(*batch.materialBatch.materialHandles);
			
#ifdef R2_EDITOR
			r2::sarr::Clear(*batch.entityIDs);
			r2::sarr::Clear(*batch.entityInstanceOffsetBatches);
			r2::sarr::Clear(*batch.entityInstances);
#endif

			r2::sarr::Clear(*batch.models);
			r2::sarr::Clear(*batch.drawState);
			r2::sarr::Clear(*batch.numInstances);
			r2::sarr::Clear(*batch.meshRenderData);

			if (batch.boneTransforms)
			{
				r2::sarr::Clear(*batch.boneTransforms);
				r2::sarr::Clear(*batch.useSameBoneTransformsForInstances);
			}
		}
	}

	RenderTarget* GetRenderTarget(Renderer& renderer, RenderTargetSurface surface)
	{
		if (surface == RTS_EMPTY)
		{
			return nullptr;
		}

		if (surface >= NUM_RENDER_TARGET_SURFACES)
		{
			R2_CHECK(false, "We should have a render target surface passed in!");
			return nullptr;
		}

		return &renderer.mRenderTargets[surface];
	}

	RenderPass* GetRenderPass(Renderer& renderer, RenderPassType pass)
	{
		if (pass == RPT_NONE || pass == NUM_RENDER_PASSES)
		{
			R2_CHECK(false, "Passed in an empty pass?");
			return nullptr;
		}

		return renderer.mRenderPasses[pass];
	}

	void CreateRenderPasses(Renderer& renderer)
	{
		RenderPassConfig passConfig;
		passConfig.primitiveType = PrimitiveType::TRIANGLES;
		passConfig.flags = 0;

		renderer.mRenderPasses[RPT_ZPREPASS] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_ZPREPASS, passConfig, {}, RTS_ZPREPASS, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_ZPREPASS_SHADOWS] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_ZPREPASS_SHADOWS, passConfig, { }, RTS_ZPREPASS_SHADOWS, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_CLUSTERS] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_CLUSTERS, passConfig, { RTS_ZPREPASS }, RTS_EMPTY, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_AMBIENT_OCCLUSION] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_AMBIENT_OCCLUSION, passConfig, { RTS_ZPREPASS_SHADOWS }, RTS_AMBIENT_OCCLUSION, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_AMBIENT_OCCLUSION_DENOISE] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_AMBIENT_OCCLUSION_DENOISE, passConfig, { RTS_AMBIENT_OCCLUSION, RTS_ZPREPASS_SHADOWS }, RTS_AMBIENT_OCCLUSION_DENOISED, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_AMBIENT_OCCLUSION_TEMPORAL_DENOISE] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_AMBIENT_OCCLUSION_TEMPORAL_DENOISE, passConfig, { RTS_AMBIENT_OCCLUSION_DENOISED, RTS_ZPREPASS_SHADOWS }, RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_GBUFFER] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_GBUFFER, passConfig, {RTS_SHADOWS, RTS_POINTLIGHT_SHADOWS, RTS_ZPREPASS, RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED }, RTS_GBUFFER, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_SHADOWS] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_SHADOWS, passConfig, {RTS_ZPREPASS_SHADOWS}, RTS_SHADOWS, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_POINTLIGHT_SHADOWS] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_POINTLIGHT_SHADOWS, passConfig, {}, RTS_POINTLIGHT_SHADOWS, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_SSR] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_SSR, passConfig, { RTS_NORMAL, RTS_ZPREPASS, RTS_SPECULAR, RTS_CONVOLVED_GBUFFER }, RTS_SSR, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_FINAL_COMPOSITE] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_FINAL_COMPOSITE, passConfig, { RTS_NORMAL, RTS_SPECULAR, RTS_GBUFFER, RTS_SSR, RTS_SSR_CONE_TRACED, RTS_BLOOM, RTS_BLOOM_BLUR, RTS_BLOOM_UPSAMPLE, RTS_ZPREPASS, RTS_TRANSPARENT_ACCUM, RTS_TRANSPARENT_REVEAL }, RTS_COMPOSITE, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_SMAA_EDGE_DETECTION] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_SMAA_EDGE_DETECTION, passConfig, { RTS_COMPOSITE }, RTS_SMAA_EDGE_DETECTION, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_SMAA_BLENDING_WEIGHT] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_SMAA_BLENDING_WEIGHT, passConfig, { RTS_SMAA_EDGE_DETECTION }, RTS_SMAA_BLENDING_WEIGHT, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_SMAA_NEIGHBORHOOD_BLENDING] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_SMAA_NEIGHBORHOOD_BLENDING, passConfig, { RTS_SMAA_BLENDING_WEIGHT, RTS_COMPOSITE }, RTS_SMAA_NEIGHBORHOOD_BLENDING, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_TRANSPARENT] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_TRANSPARENT, passConfig, { RTS_SHADOWS, RTS_POINTLIGHT_SHADOWS, RTS_ZPREPASS, RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED }, RTS_TRANSPARENT_ACCUM, __FILE__, __LINE__, "");

		RenderPassConfig transparentCompositePassConfig;
		transparentCompositePassConfig.primitiveType = PrimitiveType::TRIANGLES;
		transparentCompositePassConfig.flags = RPC_OUTPUT_FIRST_COLOR_ONLY;

		renderer.mRenderPasses[RPT_TRANSPARENT_COMPOSITE] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_TRANSPARENT_COMPOSITE, transparentCompositePassConfig, { RTS_TRANSPARENT_ACCUM, RTS_TRANSPARENT_REVEAL }, RTS_GBUFFER, __FILE__, __LINE__, "");
		renderer.mRenderPasses[RPT_OUTPUT] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_OUTPUT, passConfig, { RTS_COMPOSITE, RTS_SMAA_NEIGHBORHOOD_BLENDING, RTS_ZPREPASS, RTS_SMAA_EDGE_DETECTION, RTS_SMAA_BLENDING_WEIGHT, RTS_ZPREPASS_SHADOWS }, RTS_OUTPUT, __FILE__, __LINE__, "");

#ifdef R2_EDITOR
		renderer.mRenderPasses[RPT_EDITOR_PICKING] = rp::CreateRenderPass<r2::mem::LinearArena>(*renderer.mSubAreaArena, RPT_EDITOR_PICKING, passConfig, {}, RTS_EDITOR_PICKING, __FILE__, __LINE__, "");
#endif
	}

	void DestroyRenderPasses(Renderer& renderer)
	{
#ifdef R2_EDITOR
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_EDITOR_PICKING]);
#endif
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_OUTPUT]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_TRANSPARENT_COMPOSITE]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_TRANSPARENT]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_SMAA_NEIGHBORHOOD_BLENDING]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_SMAA_BLENDING_WEIGHT]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_SMAA_EDGE_DETECTION]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_FINAL_COMPOSITE]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_SSR]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_POINTLIGHT_SHADOWS]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_SHADOWS]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_GBUFFER]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_AMBIENT_OCCLUSION_TEMPORAL_DENOISE]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_AMBIENT_OCCLUSION_DENOISE]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_AMBIENT_OCCLUSION]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_CLUSTERS]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_ZPREPASS_SHADOWS]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_ZPREPASS]);
	}

	template <class T>
	void BeginRenderPass(Renderer& renderer, RenderPassType renderPassType, const ClearSurfaceOptions& clearOptions, r2::draw::CommandBucket<T>& commandBucket, T key, mem::StackArena& arena)
	{
		RenderPass* renderPass = GetRenderPass(renderer, renderPassType);

		R2_CHECK(renderPass != nullptr, "This should never be null");

		const auto numInputTextures = renderPass->numRenderInputTargets;

		RenderTarget* renderTarget = GetRenderTarget(renderer, renderPass->renderOutputTargetHandle );

		cmd::SetRenderTargetMipLevel* setRenderTargetCMD = nullptr;
		cmd::Clear* clearCMD = nullptr;
		cmd::ClearBuffers* clearBuffersCMD = nullptr;

		if (renderTarget)
		{
			setRenderTargetCMD = AddCommand<T, cmd::SetRenderTargetMipLevel, mem::StackArena>(arena, commandBucket, key, 0);

			cmd::FillSetRenderTargetMipLevelCommand(*renderTarget, 0, *setRenderTargetCMD, renderPass->config);

			if (clearOptions.shouldClear)
			{
				if (clearOptions.useNormalClear)
				{
					clearCMD = AppendCommand<cmd::SetRenderTargetMipLevel, cmd::Clear, mem::StackArena>(arena, setRenderTargetCMD, 0);
					clearCMD->flags = clearOptions.flags;
				}
				else
				{
					clearBuffersCMD = AppendCommand<cmd::SetRenderTargetMipLevel, cmd::ClearBuffers, mem::StackArena>(arena, setRenderTargetCMD, 0);

					clearBuffersCMD->renderTargetHandle = renderTarget->frameBufferID;
					clearBuffersCMD->numBuffers = clearOptions.numClearParams;
					memcpy(clearBuffersCMD->clearParams, clearOptions.clearParams, sizeof(clearOptions.clearParams));
				}
			}
		}
	}

	template<class T>
	void EndRenderPass(Renderer& renderer, RenderPassType renderPass, r2::draw::CommandBucket<T>& commandBucket)
	{
		cmdbkt::Close(commandBucket);
	}

	void SwapRenderTargetsHistoryIfNecessary(Renderer& renderer)
	{
		for (int i = 0; i < NUM_RENDER_TARGET_SURFACES; ++i)
		{
			rt::SwapTexturesIfNecessary(renderer.mRenderTargets[i]);
		}
	}

	void UploadSwappingSurfaces(Renderer& renderer)
	{
		for (u32 i = RTS_GBUFFER; i < RTS_OUTPUT; ++i)
		{
			RenderTarget* rt = GetRenderTarget(renderer, (RenderTargetSurface)i);
			R2_CHECK(rt != nullptr, "Should never be nullptr!");

			u32 offset = GetRenderPassTargetOffset(renderer, (RenderTargetSurface)i);

			int currentTexture = 0;

			tex::TextureAddress surfaceTextureAddress[2];
			int numTextureToUpload = 1;
			bool shouldAddFillCommand = false;

			if (rt->colorAttachments)
			{
				const auto& attachment = r2::sarr::At(*rt->colorAttachments, 0);

				shouldAddFillCommand = attachment.textureAttachmentFormat.swapping;

				if (shouldAddFillCommand)
				{
					currentTexture = attachment.currentTexture;

					surfaceTextureAddress[0] = texsys::GetTextureAddress(attachment.texture[currentTexture]);

					if (attachment.textureAttachmentFormat.uploadAllTextures)
					{
						numTextureToUpload++;

						currentTexture = (currentTexture + 1) % attachment.numTextures;

						surfaceTextureAddress[1] = texsys::GetTextureAddress(attachment.texture[currentTexture]);
					}
				}
				
			}
			else if (rt->depthAttachments)
			{
				const auto& attachment = r2::sarr::At(*rt->depthAttachments, 0);

				shouldAddFillCommand = attachment.textureAttachmentFormat.swapping;

				if (shouldAddFillCommand)
				{
					currentTexture = attachment.currentTexture;

					surfaceTextureAddress[0] = texsys::GetTextureAddress(attachment.texture[currentTexture]);

					if (attachment.textureAttachmentFormat.uploadAllTextures)
					{
						numTextureToUpload++;

						currentTexture = (currentTexture + 1) % attachment.numTextures;

						surfaceTextureAddress[1] = texsys::GetTextureAddress(attachment.texture[currentTexture]);
					}
				}
				
			}
			else if (rt->depthStencilAttachments)
			{
				const auto& attachment = r2::sarr::At(*rt->depthStencilAttachments, 0);

				shouldAddFillCommand = attachment.textureAttachmentFormat.swapping;

				if (shouldAddFillCommand)
				{
					currentTexture = attachment.currentTexture;

					surfaceTextureAddress[0] = texsys::GetTextureAddress(attachment.texture[currentTexture]);

					if (attachment.textureAttachmentFormat.uploadAllTextures)
					{
						numTextureToUpload++;

						currentTexture = (currentTexture + 1) % attachment.numTextures;

						surfaceTextureAddress[1] = texsys::GetTextureAddress(attachment.texture[currentTexture]);
					}
				}
			}
			else
			{
				R2_CHECK(false, "?");
			}
			
			if (shouldAddFillCommand)
			{
				key::Basic fillKey;
				fillKey.keyValue = 0;

				//add new command to upload
				cmd::FillConstantBuffer* uploadSurfacesCMD = AddCommand<key::Basic, cmd::FillConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, *renderer.mPreRenderBucket, fillKey, sizeof(tex::TextureAddress) * numTextureToUpload);

				ConstantBufferHandle surfaceBufferHandle = r2::sarr::At(*renderer.mConstantBufferHandles, renderer.mSurfacesConfigHandle);

				ConstantBufferData* constBufferData = GetConstData(renderer, surfaceBufferHandle);

				FillConstantBufferCommand(uploadSurfacesCMD, surfaceBufferHandle, constBufferData->type, constBufferData->isPersistent, &surfaceTextureAddress[0], sizeof(tex::TextureAddress) * numTextureToUpload, offset);
			}
		
		}
	}

	void SetDefaultTransparencyBlendState(BlendState& blendState)
	{
		blendState.blendingEnabled = true;
		blendState.blendEquation = BLEND_EQUATION_ADD;
		blendState.numBlendFunctions = 2;

		//first one is the accum
		blendState.blendFunctions[0].blendDrawBuffer = 0;
		blendState.blendFunctions[0].sfactor = ONE;
		blendState.blendFunctions[0].dfactor = ONE;

		//next is the reveal
		blendState.blendFunctions[1].blendDrawBuffer = 1;
		blendState.blendFunctions[1].sfactor = ZERO;
		blendState.blendFunctions[1].dfactor = ONE_MINUS_SRC_COLOR;
	}

	void SetDefaultTransparentCompositeBlendState(BlendState& blendState)
	{
		blendState.blendingEnabled = true;
		blendState.blendEquation = BLEND_EQUATION_ADD;
		blendState.numBlendFunctions = 1;

		blendState.blendFunctions[0].blendDrawBuffer = 0;
		blendState.blendFunctions[0].sfactor = SRC_ALPHA;
		blendState.blendFunctions[0].dfactor = ONE_MINUS_SRC_ALPHA;
	}

	//void UpdateRenderTargetsIfNecessary(Renderer& renderer)
	//{
	//	for (int i = 0; i < NUM_RENDER_TARGET_SURFACES; ++i)Up
	//	{
	//		rt::UpdateRenderTargetIfNecessary(renderer.mRenderTargets[i]);
	//	}
	//}

	void UpdateShaderMatrices(Renderer& renderer)
	{
		ConstantBufferHandle vpMatricesBufferHandle = r2::sarr::At(*renderer.mConstantBufferHandles, renderer.mVPMatricesConfigHandle);
		AddFillConstantBufferCommandFull(renderer, vpMatricesBufferHandle, &renderer.mShaderMatrices, sizeof(renderer.mShaderMatrices), 0);
	}

	void UpdateShaderVectors(Renderer& renderer)
	{
		ConstantBufferHandle vectorsBufferHandle = r2::sarr::At(*renderer.mConstantBufferHandles, renderer.mVectorsConfigHandle);
		AddFillConstantBufferCommandFull(renderer, vectorsBufferHandle, &renderer.mShaderVectors, sizeof(renderer.mShaderVectors), 0);
	}

	void ClearShadowData(Renderer& renderer)
	{
		const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles(renderer);

		static constexpr Partition sEmptyPartitions = { glm::vec4(0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF), glm::vec4(0) };
		static constexpr UPartition sEmptyPartitionsU = { glm::uvec4(0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF), glm::uvec4(0) };


		r2::draw::renderer::AddFillConstantBufferCommandForData(renderer, r2::sarr::At(*constantBufferHandles, renderer.mShadowDataConfigHandle), 0, &sEmptyPartitions);
		r2::draw::renderer::AddFillConstantBufferCommandForData(renderer, r2::sarr::At(*constantBufferHandles, renderer.mShadowDataConfigHandle), 1, &sEmptyPartitionsU);
//		r2::draw::renderer::AddFillConstantBufferCommandForData(renderer, r2::sarr::At(*constantBufferHandles, renderer.mShadowDataConfigHandle), 2, &sEmptyBoundsUInt);
	}


	void UpdateShadowMapPages(Renderer& renderer)
	{
		const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles(renderer);
		auto handle = r2::sarr::At(*constantBufferHandles, renderer.mShadowDataConfigHandle);

		r2::draw::renderer::AddFillConstantBufferCommandForData(renderer, handle, 5, renderer.mLightSystem->mShadowMapPages.mSpotLightShadowMapPages);
		r2::draw::renderer::AddFillConstantBufferCommandForData(renderer, handle, 6, renderer.mLightSystem->mShadowMapPages.mPointLightShadowMapPages);
		r2::draw::renderer::AddFillConstantBufferCommandForData(renderer, handle, 7, renderer.mLightSystem->mShadowMapPages.mDirectionLightShadowMapPages);
	}

	void ClearAllShadowMapPages(Renderer& renderer)
	{
		lightsys::ClearShadowMapPages(*renderer.mLightSystem);

		renderer.mFlags.Set(eRendererFlags::RENDERER_FLAG_NEEDS_SHADOW_MAPS_REFRESH);
	}

	void AssignShadowMapPagesForAllLights(Renderer& renderer)
	{
		RenderTarget* shadowRenderTarget = GetRenderTarget(renderer, RTS_SHADOWS);
		RenderTarget* pointlightShadowRenderTarget = GetRenderTarget(renderer, RTS_POINTLIGHT_SHADOWS);

		R2_CHECK(shadowRenderTarget != nullptr, "We should have a valid render target for shadows to do this operation");
		R2_CHECK((shadowRenderTarget->depthAttachments != nullptr || shadowRenderTarget->colorAttachments != nullptr), "We should have a depth attachment here");

		R2_CHECK(pointlightShadowRenderTarget != nullptr, "We should have a valid render target for shadows to do this operation");
		R2_CHECK(pointlightShadowRenderTarget->depthAttachments != nullptr, "We should have a depth attachment here");

		for (u32 i = 0; i < renderer.mLightSystem->mSceneLighting.numShadowCastingDirectionLights; ++i)
		{
			u32 lightIndex = renderer.mLightSystem->mSceneLighting.mShadowCastingDirectionLights[i];
			const DirectionLight& light = renderer.mLightSystem->mSceneLighting.mDirectionLights[lightIndex];
			R2_CHECK(light.lightProperties.lightID >= 0, "We should have a valid lightID");

			if (light.lightProperties.castsShadowsUseSoftShadows.x > 0)
			{
				float sliceIndex = rt::AddTexturePagesToAttachment(*shadowRenderTarget, rt::DEPTH, light::NUM_DIRECTIONLIGHT_LAYERS);

				renderer.mLightSystem->mShadowMapPages.mDirectionLightShadowMapPages[light.lightProperties.lightID] = sliceIndex;
			}
		}

		for (u32 i = 0; i < renderer.mLightSystem->mSceneLighting.numShadowCastingSpotLights; ++i)
		{
			u32 lightIndex = renderer.mLightSystem->mSceneLighting.mShadowCastingSpotLights[i];

			const SpotLight& light = renderer.mLightSystem->mSceneLighting.mSpotLights[lightIndex];
			R2_CHECK(light.lightProperties.lightID >= 0, "We should have a valid lightID");

			if (light.lightProperties.castsShadowsUseSoftShadows.x > 0)
			{
				float sliceIndex = rt::AddTexturePagesToAttachment(*shadowRenderTarget, rt::DEPTH, light::NUM_SPOTLIGHT_LAYERS);

				renderer.mLightSystem->mShadowMapPages.mSpotLightShadowMapPages[light.lightProperties.lightID] = sliceIndex;
			}
			
		}

		for (u32 i = 0; i < renderer.mLightSystem->mSceneLighting.numShadowCastingPointLights; ++i)
		{
			u32 lightIndex = renderer.mLightSystem->mSceneLighting.mShadowCastingPointLights[i];

			const PointLight& light = renderer.mLightSystem->mSceneLighting.mPointLights[lightIndex];
			R2_CHECK(light.lightProperties.lightID >= 0, "We should have a valid lightID");

			if (light.lightProperties.castsShadowsUseSoftShadows.x > 0)
			{
				float sliceIndex = rt::AddTexturePagesToAttachment(*pointlightShadowRenderTarget, rt::DEPTH_CUBEMAP, light::NUM_POINTLIGHT_LAYERS);

				renderer.mLightSystem->mShadowMapPages.mPointLightShadowMapPages[light.lightProperties.lightID] = sliceIndex;
			}
			
		}

		renderer.mFlags.Set(eRendererFlags::RENDERER_FLAG_NEEDS_SHADOW_MAPS_REFRESH);
	}

	void AssignShadowMapPagesForSpotLight(Renderer& renderer, const SpotLight& spotLight)
	{
		if (spotLight.lightProperties.castsShadowsUseSoftShadows.x == 0)
		{
			return;
		}

		RenderTarget* renderTarget = GetRenderTarget(renderer, RTS_SHADOWS);

		R2_CHECK(renderTarget != nullptr, "We should have a valid render target for shadows to do this operation");
		R2_CHECK((renderTarget->depthAttachments != nullptr || renderTarget->colorAttachments != nullptr), "We should have a depth attachment here");

		R2_CHECK(spotLight.lightProperties.lightID >= 0, "We should have a valid lightID");

		float sliceIndex = rt::AddTexturePagesToAttachment(*renderTarget, rt::DEPTH, light::NUM_SPOTLIGHT_LAYERS);

		renderer.mLightSystem->mShadowMapPages.mSpotLightShadowMapPages[spotLight.lightProperties.lightID] = sliceIndex;

		renderer.mFlags.Set(eRendererFlags::RENDERER_FLAG_NEEDS_SHADOW_MAPS_REFRESH);
	}

	void RemoveShadowMapPagesForSpotLight(Renderer& renderer, const SpotLight& spotLight)
	{
		if (spotLight.lightProperties.castsShadowsUseSoftShadows.x == 0)
		{
			return;
		}

		RenderTarget* renderTarget = GetRenderTarget(renderer, RTS_SHADOWS);

		R2_CHECK(renderTarget != nullptr, "We should have a valid render target for shadows to do this operation");
		R2_CHECK((renderTarget->depthAttachments != nullptr || renderTarget->colorAttachments != nullptr), "We should have a depth attachment here");

		R2_CHECK(spotLight.lightProperties.lightID >= 0, "We should have a valid lightID");

		rt::RemoveTexturePagesFromAttachment(*renderTarget, rt::DEPTH, renderer.mLightSystem->mShadowMapPages.mSpotLightShadowMapPages[spotLight.lightProperties.lightID], light::NUM_SPOTLIGHT_LAYERS);

		renderer.mLightSystem->mShadowMapPages.mSpotLightShadowMapPages[spotLight.lightProperties.lightID] = -1;

		renderer.mFlags.Set(eRendererFlags::RENDERER_FLAG_NEEDS_SHADOW_MAPS_REFRESH);
	}

	void AssignShadowMapPagesForPointLight(Renderer& renderer, const PointLight& pointLight)
	{
		if (pointLight.lightProperties.castsShadowsUseSoftShadows.x == 0)
		{
			return;
		}

		RenderTarget* renderTarget = GetRenderTarget(renderer, RTS_POINTLIGHT_SHADOWS);

		R2_CHECK(renderTarget != nullptr, "We should have a valid render target for shadows to do this operation");
		R2_CHECK(renderTarget->depthAttachments != nullptr, "We should have a depth attachment here");

		R2_CHECK(pointLight.lightProperties.lightID >= 0, "We should have a valid lightID");

		float sliceIndex = rt::AddTexturePagesToAttachment(*renderTarget, rt::DEPTH_CUBEMAP, light::NUM_POINTLIGHT_LAYERS);

		renderer.mLightSystem->mShadowMapPages.mPointLightShadowMapPages[pointLight.lightProperties.lightID] = sliceIndex;

		renderer.mFlags.Set(eRendererFlags::RENDERER_FLAG_NEEDS_SHADOW_MAPS_REFRESH);
	}

	void RemoveShadowMapPagesForPointLight(Renderer& renderer, const PointLight& pointLight)
	{
		if (pointLight.lightProperties.castsShadowsUseSoftShadows.x == 0)
		{
			return;
		}

		RenderTarget* renderTarget = GetRenderTarget(renderer, RTS_POINTLIGHT_SHADOWS);

		R2_CHECK(renderTarget != nullptr, "We should have a valid render target for shadows to do this operation");
		R2_CHECK(renderTarget->depthAttachments != nullptr, "We should have a depth attachment here");

		R2_CHECK(pointLight.lightProperties.lightID >= 0, "We should have a valid lightID");


		rt::RemoveTexturePagesFromAttachment(*renderTarget, rt::DEPTH_CUBEMAP, renderer.mLightSystem->mShadowMapPages.mPointLightShadowMapPages[pointLight.lightProperties.lightID], light::NUM_POINTLIGHT_LAYERS);

		renderer.mLightSystem->mShadowMapPages.mPointLightShadowMapPages[pointLight.lightProperties.lightID] = -1;

		renderer.mFlags.Set(eRendererFlags::RENDERER_FLAG_NEEDS_SHADOW_MAPS_REFRESH);
	}

	void AssignShadowMapPagesForDirectionLight(Renderer& renderer, const DirectionLight& directionLight)
	{
		if (directionLight.lightProperties.castsShadowsUseSoftShadows.x == 0)
		{
			return;
		}

		RenderTarget* renderTarget = GetRenderTarget(renderer, RTS_SHADOWS);

		R2_CHECK(renderTarget != nullptr, "We should have a valid render target for shadows to do this operation");
		R2_CHECK((renderTarget->depthAttachments != nullptr || renderTarget->colorAttachments != nullptr), "We should have a depth attachment here");

		R2_CHECK(directionLight.lightProperties.lightID >= 0, "We should have a valid lightID");

		float sliceIndex = rt::AddTexturePagesToAttachment(*renderTarget, rt::DEPTH, light::NUM_DIRECTIONLIGHT_LAYERS);

		renderer.mLightSystem->mShadowMapPages.mDirectionLightShadowMapPages[directionLight.lightProperties.lightID] = sliceIndex;

		renderer.mFlags.Set(eRendererFlags::RENDERER_FLAG_NEEDS_SHADOW_MAPS_REFRESH);
	}

	void RemoveShadowMapPagesForDirectionLight(Renderer& renderer, const DirectionLight& directionLight)
	{
		if (directionLight.lightProperties.castsShadowsUseSoftShadows.x == 0)
		{
			return;
		}

		RenderTarget* renderTarget = GetRenderTarget(renderer, RTS_SHADOWS);

		R2_CHECK(renderTarget != nullptr, "We should have a valid render target for shadows to do this operation");
		R2_CHECK((renderTarget->depthAttachments != nullptr || renderTarget->colorAttachments != nullptr), "We should have a depth attachment here");

		R2_CHECK(directionLight.lightProperties.lightID >= 0, "We should have a valid lightID");

		rt::RemoveTexturePagesFromAttachment(*renderTarget, rt::DEPTH, renderer.mLightSystem->mShadowMapPages.mDirectionLightShadowMapPages[directionLight.lightProperties.lightID], light::NUM_DIRECTIONLIGHT_LAYERS);

		renderer.mLightSystem->mShadowMapPages.mDirectionLightShadowMapPages[directionLight.lightProperties.lightID] = -1;

		renderer.mFlags.Set(eRendererFlags::RENDERER_FLAG_NEEDS_SHADOW_MAPS_REFRESH);
	}

	void UpdateShadowsForDirectionLight(Renderer& renderer, DirectionLightHandle dirLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		
		auto light = GetDirectionLightConstPtr(renderer, dirLightHandle);

		if (light->lightProperties.castsShadowsUseSoftShadows.x > 0)
		{
			AssignShadowMapPagesForDirectionLight(renderer, *light);
		}
		else
		{
			RemoveShadowMapPagesForDirectionLight(renderer, *light);
		}
	}

	void UpdateShadowsForPointLight(Renderer& renderer, PointLightHandle pointLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");

		auto light = GetPointLightConstPtr(renderer, pointLightHandle);

		if (light->lightProperties.castsShadowsUseSoftShadows.x > 0)
		{
			AssignShadowMapPagesForPointLight(renderer, *light);
		}
		else
		{
			RemoveShadowMapPagesForPointLight(renderer, *light);
		}
	}

	void UpdateShadowsForSpotLight(Renderer& renderer, SpotLightHandle spotLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");

		auto light = GetSpotLightConstPtr(renderer, spotLightHandle);

		if (light->lightProperties.castsShadowsUseSoftShadows.x > 0)
		{
			AssignShadowMapPagesForSpotLight(renderer, *light);
		}
		else
		{
			RemoveShadowMapPagesForSpotLight(renderer, *light);
		}
	}

	void UpdateSDSMShaderParams(Renderer& renderer)
	{
		const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles(renderer);
		auto sdsmConstantBufferHandle = r2::sarr::At(*constantBufferHandles, renderer.mSDSMParamsConfigHandle);
		r2::draw::renderer::AddFillConstantBufferCommandFull(renderer, sdsmConstantBufferHandle, &renderer.mSDSMShaderParams, sizeof(renderer.mSDSMShaderParams), 0);
	}

	void ClearActiveClusters(Renderer& renderer)
	{
		const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles(renderer);
		auto clustersConstantBufferHandle = r2::sarr::At(*constantBufferHandles, renderer.mClusterVolumesConfigHandle);

		auto dispatchComputeConstantBufferHandle = r2::sarr::At(*constantBufferHandles, renderer.mDispatchComputeConfigHandle);

		r2::draw::renderer::AddZeroConstantBufferCommand(renderer, clustersConstantBufferHandle, 0);
		//r2::draw::renderer::AddZeroConstantBufferCommand(renderer, clustersConstantBufferHandle, 1);
		r2::draw::renderer::AddZeroConstantBufferCommand(renderer, clustersConstantBufferHandle, 2);
		r2::draw::renderer::AddZeroConstantBufferCommand(renderer, clustersConstantBufferHandle, 3);
		r2::draw::renderer::AddZeroConstantBufferCommand(renderer, clustersConstantBufferHandle, 4);
	
		r2::draw::renderer::AddZeroConstantBufferCommand(renderer, dispatchComputeConstantBufferHandle, 0);
	}

	void UpdateClusters(Renderer& renderer)
	{
		ClearActiveClusters(renderer);
		
		if (renderer.mLightSystem->mSceneLighting.mNumPointLights == 0 &&
			renderer.mLightSystem->mSceneLighting.mNumSpotLights == 0)
		{
			return;
		}

		key::Basic dispatchMarkActiveClustersKey = key::GenerateBasicKey(0, 0, DL_COMPUTE, 0, 0, renderer.mMarkActiveClusterTilesComputeShader, 0);
		key::Basic dispatchFindUniqueClustersKey = key::GenerateBasicKey(0, 0, DL_COMPUTE, 0, 1, renderer.mFindUniqueClustersComputeShader, 1);
		key::Basic dispatchAssignLightsToClustersKey = key::GenerateBasicKey(0, 0, DL_COMPUTE, 0, 2, renderer.mAssignLightsToClustersComputeShader, 2);

		R2_CHECK(dispatchMarkActiveClustersKey.keyValue < dispatchFindUniqueClustersKey.keyValue&&
			dispatchFindUniqueClustersKey.keyValue < dispatchAssignLightsToClustersKey.keyValue, "These should be properly ordered!");

		cmd::DispatchCompute* markActiveClusters = AddCommand<key::Basic, cmd::DispatchCompute, mem::StackArena>(*renderer.mCommandArena, *renderer.mClustersBucket, dispatchMarkActiveClustersKey, 0);
		markActiveClusters->numGroupsX = renderer.mResolutionSize.width / 20;
		markActiveClusters->numGroupsY = renderer.mResolutionSize.height / 20;
		markActiveClusters->numGroupsZ = 1;

		cmd::Barrier* barrierCMD = AppendCommand<cmd::DispatchCompute, cmd::Barrier, mem::StackArena>(*renderer.mCommandArena, markActiveClusters, 0);
		barrierCMD->flags = cmd::SHADER_STORAGE_BARRIER_BIT;

		cmd::DispatchCompute* findUniqueClusters = AddCommand<key::Basic, cmd::DispatchCompute, mem::StackArena>(*renderer.mCommandArena, *renderer.mClustersBucket, dispatchFindUniqueClustersKey, 0);
		findUniqueClusters->numGroupsX = 3;
		findUniqueClusters->numGroupsY = 3;
		findUniqueClusters->numGroupsZ = 3;

		cmd::Barrier* barrierCMD2 = AppendCommand<cmd::DispatchCompute, cmd::Barrier, mem::StackArena>(*renderer.mCommandArena, findUniqueClusters, 0);
		barrierCMD2->flags = cmd::ALL_BARRIER_BITS;

		const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles(renderer);

		ConstantBufferHandle dispatchComputeConstantBufferHandle = r2::sarr::At(*constantBufferHandles, renderer.mDispatchComputeConfigHandle);

		cmd::DispatchComputeIndirect* assignLightsCMD = AddCommand<key::Basic, cmd::DispatchComputeIndirect, mem::StackArena>(*renderer.mCommandArena, *renderer.mClustersBucket, dispatchAssignLightsToClustersKey, 0);
		assignLightsCMD->dispatchIndirectBuffer = dispatchComputeConstantBufferHandle;
		assignLightsCMD->offset = 0;

		/*cmd::Barrier* barrierCMD3 = AppendCommand<cmd::DispatchComputeIndirect, cmd::Barrier, mem::StackArena>(*renderer.mCommandArena, assignLightsCMD, 0);
		barrierCMD3->flags = cmd::SHADER_STORAGE_BARRIER_BIT;*/
	}

	void UpdateSSRDataIfNeeded(Renderer& renderer)
	{
		if (renderer.mSSRNeedsUpdate)
		{
			renderer.mSSRNeedsUpdate = false;

			const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles(renderer);
			auto ssrConstantBufferHandle = r2::sarr::At(*constantBufferHandles, renderer.mSSRConfigHandle);

			AddFillConstantBufferCommandFull(renderer, ssrConstantBufferHandle, &renderer.mSSRParams, sizeof(renderer.mSSRParams), 0);
		}
	}

	void BloomRenderPass(Renderer& renderer)
	{
		const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles(renderer);

		auto bloomConstantBufferHandle = r2::sarr::At(*constantBufferHandles, renderer.mBloomConfigHandle);

		ConstantBufferData* constBufferData = GetConstData(renderer, bloomConstantBufferHandle);

		R2_CHECK(constBufferData != nullptr, "We couldn't find the constant buffer handle!");

		u64 numConstantBufferHandles = r2::sarr::Size(*renderer.mConstantBufferHandles);
		u64 constBufferIndex = 0;
		bool found = false;
		for (; constBufferIndex < numConstantBufferHandles; ++constBufferIndex)
		{
			if (bloomConstantBufferHandle == r2::sarr::At(*renderer.mConstantBufferHandles, constBufferIndex))
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			R2_CHECK(false, "Couldn't find the constant buffer so we can upload the data");
			return;
		}

		const ConstantBufferLayoutConfiguration& config = r2::sarr::At(*renderer.mConstantLayouts, constBufferIndex);

		
		const auto& gbufferColorAttachment = r2::sarr::At(*renderer.mRenderTargets[RTS_GBUFFER].colorAttachments, 0);
		const auto gbufferTexture = gbufferColorAttachment.texture[gbufferColorAttachment.currentTexture];

		const auto& bloomColorAttachment = r2::sarr::At(*renderer.mRenderTargets[RTS_BLOOM].colorAttachments, 0);
		const auto bloomTexture = bloomColorAttachment.texture[bloomColorAttachment.currentTexture];
		
		const auto& bloomBlurColorAttachment = r2::sarr::At(*renderer.mRenderTargets[RTS_BLOOM_BLUR].colorAttachments, 0);
		const auto& bloomBlurTexture = bloomBlurColorAttachment.texture[bloomBlurColorAttachment.currentTexture];

		const auto& bloomUpSampleColorAttachment = r2::sarr::At(*renderer.mRenderTargets[RTS_BLOOM_UPSAMPLE].colorAttachments, 0);
		const auto bloomUpSampleTexture = bloomUpSampleColorAttachment.texture[bloomUpSampleColorAttachment.currentTexture];

		R2_CHECK(gbufferColorAttachment.format.internalformat == bloomColorAttachment.format.internalformat, "These should be the same!");

		u32 imageWidth = gbufferColorAttachment.format.width;
		u32 imageHeight = gbufferColorAttachment.format.height;

		//figure out how many mips to loop over

		u32 numMips = 0;

		constexpr u32 WARP_SIZE = 32;

		struct MipSize
		{

			u32 imageWidth;
			u32 imageHeight;
		};

		r2::SArray<MipSize>* mipSizes = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, MipSize, bloomColorAttachment.format.mipLevels);

		while (imageWidth > WARP_SIZE && imageHeight > WARP_SIZE)
		{
			++numMips;

			imageWidth /= 2;
			imageHeight /= 2;

			r2::sarr::Push(*mipSizes, { imageWidth, imageHeight });
		}


		for (u32 i = 0; i < numMips; ++i)
		{
			const MipSize& mipSize = r2::sarr::At(*mipSizes, i);
			u64 inputTextureHandle = bloomTexture.container->handle;
			float inputTextureLayer = bloomTexture.sliceIndex;
			u32 inputTextureFormat = bloomColorAttachment.format.internalformat;
			float inputMipLevel = i == 0 ? 0 : i - 1;

			if (i == 0)
			{
				inputTextureHandle = gbufferTexture.container->handle;
				inputTextureLayer = gbufferTexture.sliceIndex;
				inputTextureFormat = gbufferColorAttachment.format.internalformat;
			}

			key::Basic dispatchBloomDownSampleKey = key::GenerateBasicKey(0, 0, DL_EFFECT, 0, 0, renderer.mBloomDownSampleShader, i+1);

			cmd::FillConstantBuffer* fillResolutionCMD = AddCommand<key::Basic, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, *renderer.mCommandBucket, dispatchBloomDownSampleKey, config.layout.GetElements().at(1).size);

			

			glm::uvec4 bloomResolution = glm::uvec4(mipSize.imageWidth * 2, mipSize.imageHeight* 2, mipSize.imageWidth , mipSize.imageHeight);
			
			cmd::FillConstantBufferCommand(fillResolutionCMD, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, glm::value_ptr(bloomResolution), config.layout.GetElements().at(1).size, config.layout.GetElements().at(1).offset);


			cmd::FillConstantBuffer* fillTextureHandleCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, fillResolutionCMD, config.layout.GetElements().at(3).size);
			cmd::FillConstantBufferCommand(fillTextureHandleCMD, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, &inputTextureHandle, config.layout.GetElements().at(3).size, config.layout.GetElements().at(3).offset);

			cmd::FillConstantBuffer* fillTexturePageCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, fillTextureHandleCMD, config.layout.GetElements().at(4).size);
			cmd::FillConstantBufferCommand(fillTexturePageCMD, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, &inputTextureLayer, config.layout.GetElements().at(4).size, config.layout.GetElements().at(4).offset);

			cmd::FillConstantBuffer* fillTextureMipLevel = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, fillTexturePageCMD, config.layout.GetElements().at(5).size);
			cmd::FillConstantBufferCommand(fillTextureMipLevel, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, &inputMipLevel, config.layout.GetElements().at(5).size, config.layout.GetElements().at(5).offset);

			cmd::BindImageTexture* bindOutputImageTextureCMD = AppendCommand<cmd::FillConstantBuffer, cmd::BindImageTexture, r2::mem::StackArena>(*renderer.mCommandArena, fillTextureMipLevel, 0);
			bindOutputImageTextureCMD->textureUnit = 0;
			bindOutputImageTextureCMD->texture = bloomTexture.container->texId;
			bindOutputImageTextureCMD->mipLevel = i;
			bindOutputImageTextureCMD->layered = true;
			bindOutputImageTextureCMD->layer = bloomTexture.sliceIndex;
			bindOutputImageTextureCMD->format = bloomColorAttachment.format.internalformat;
			bindOutputImageTextureCMD->access = tex::WRITE_ONLY;

			cmd::DispatchCompute* dispatchBloomDownSamplePreFilterCMD = AppendCommand<cmd::BindImageTexture, cmd::DispatchCompute, r2::mem::StackArena>(*renderer.mCommandArena, bindOutputImageTextureCMD, 0);

			dispatchBloomDownSamplePreFilterCMD->numGroupsX = glm::max(static_cast<u32>(glm::ceil(float(mipSize.imageWidth) / float(WARP_SIZE))), 1u);
			dispatchBloomDownSamplePreFilterCMD->numGroupsY = glm::max(static_cast<u32>(glm::ceil(float(mipSize.imageHeight) / float(WARP_SIZE))), 1u);
			dispatchBloomDownSamplePreFilterCMD->numGroupsZ = 1;

		}

		for (u32 i = 0; i < numMips; ++i)
		{
			const MipSize& mipSize = r2::sarr::At(*mipSizes, i);

			key::Basic dispatchBloomDownSampleKey = key::GenerateBasicKey(0, 0, DL_EFFECT, 0, 1, renderer.mBloomBlurPreFilterShader, i);

			cmd::FillConstantBuffer* fillResolutionCMD = AddCommand<key::Basic, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, *renderer.mCommandBucket, dispatchBloomDownSampleKey, config.layout.GetElements().at(1).size);

			glm::uvec4 bloomResolution = glm::uvec4(mipSize.imageWidth * 2, mipSize.imageHeight * 2, mipSize.imageWidth, mipSize.imageHeight);

			cmd::FillConstantBufferCommand(fillResolutionCMD, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, glm::value_ptr(bloomResolution), config.layout.GetElements().at(1).size, config.layout.GetElements().at(1).offset);


			cmd::FillConstantBuffer* fillTextureHandleCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, fillResolutionCMD, config.layout.GetElements().at(3).size);
			cmd::FillConstantBufferCommand(fillTextureHandleCMD, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, &bloomTexture.container->handle, config.layout.GetElements().at(3).size, config.layout.GetElements().at(3).offset);

			cmd::FillConstantBuffer* fillTexturePageCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, fillTextureHandleCMD, config.layout.GetElements().at(4).size);
			cmd::FillConstantBufferCommand(fillTexturePageCMD, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, &bloomTexture.sliceIndex, config.layout.GetElements().at(4).size, config.layout.GetElements().at(4).offset);

			float mip = i;

			cmd::FillConstantBuffer* fillTextureMipLevel = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, fillTexturePageCMD, config.layout.GetElements().at(5).size);
			cmd::FillConstantBufferCommand(fillTextureMipLevel, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, &mip, config.layout.GetElements().at(5).size, config.layout.GetElements().at(5).offset);

			cmd::BindImageTexture* bindOutputImageTextureCMD = AppendCommand<cmd::FillConstantBuffer, cmd::BindImageTexture, r2::mem::StackArena>(*renderer.mCommandArena, fillTextureMipLevel, 0);
			bindOutputImageTextureCMD->textureUnit = 0;
			bindOutputImageTextureCMD->texture = bloomBlurTexture.container->texId;
			bindOutputImageTextureCMD->mipLevel = i;
			bindOutputImageTextureCMD->layered = true;
			bindOutputImageTextureCMD->layer = bloomBlurTexture.sliceIndex;
			bindOutputImageTextureCMD->format = bloomBlurColorAttachment.format.internalformat;
			bindOutputImageTextureCMD->access = tex::WRITE_ONLY;

			cmd::DispatchCompute* dispatchBloomBlurPreFilterCMD = AppendCommand<cmd::BindImageTexture, cmd::DispatchCompute, r2::mem::StackArena>(*renderer.mCommandArena, bindOutputImageTextureCMD, 0);

			dispatchBloomBlurPreFilterCMD->numGroupsX = glm::max(static_cast<u32>(glm::ceil(float(mipSize.imageWidth) / float(WARP_SIZE))), 1u);
			dispatchBloomBlurPreFilterCMD->numGroupsY = glm::max(static_cast<u32>(glm::ceil(float(mipSize.imageHeight) / float(WARP_SIZE))), 1u);
			dispatchBloomBlurPreFilterCMD->numGroupsZ = 1;


		}

		u32 pass = 0;
		for (s32 i = numMips - 1; i > 0; --i)
		{
			const MipSize& mipSize = r2::sarr::At(*mipSizes, i);
			const MipSize& nextMipSize = r2::sarr::At(*mipSizes, i - 1);

			key::Basic dispatchBloomUpSampleKey = key::GenerateBasicKey(0, 0, DL_EFFECT, 0, 2, renderer.mBloomUpSampleShader, pass);

			cmd::FillConstantBuffer* fillResolutionCMD = AddCommand<key::Basic, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, *renderer.mCommandBucket, dispatchBloomUpSampleKey, config.layout.GetElements().at(1).size);

			glm::uvec4 bloomResolution = glm::uvec4(mipSize.imageWidth, mipSize.imageHeight, nextMipSize.imageWidth, nextMipSize.imageHeight);

			cmd::FillConstantBufferCommand(fillResolutionCMD, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, glm::value_ptr(bloomResolution), config.layout.GetElements().at(1).size, config.layout.GetElements().at(1).offset);


			cmd::FillConstantBuffer* fillFilterCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, fillResolutionCMD, config.layout.GetElements().at(2).size);

			//Why are we doing this?
			glm::vec4 bloomFilterRadius = glm::vec4(renderer.mBloomParams.mBloomFilterSize, renderer.mBloomParams.mBloomFilterSize, renderer.mBloomParams.mBloomIntensity, 0);

			cmd::FillConstantBufferCommand(fillFilterCMD, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, glm::value_ptr(bloomFilterRadius), config.layout.GetElements().at(2).size, config.layout.GetElements().at(2).offset);

			u64 inputTexture1Handle = bloomUpSampleTexture.container->handle;
			float inputTexture1Layer = bloomUpSampleTexture.sliceIndex;
			float inputTexture1MipLevel = i;
			u32 inputTexture1Format = bloomUpSampleColorAttachment.format.internalformat;

			if (i == numMips - 1)
			{
				inputTexture1Handle = bloomBlurTexture.container->handle;
				inputTexture1Layer = bloomBlurTexture.sliceIndex;
				inputTexture1Format = bloomBlurColorAttachment.format.internalformat;
			}


			cmd::FillConstantBuffer* fillTextureHandleCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, fillFilterCMD, config.layout.GetElements().at(3).size);
			cmd::FillConstantBufferCommand(fillTextureHandleCMD, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, &inputTexture1Handle, config.layout.GetElements().at(3).size, config.layout.GetElements().at(3).offset);

			cmd::FillConstantBuffer* fillTexturePageCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, fillTextureHandleCMD, config.layout.GetElements().at(4).size);
			cmd::FillConstantBufferCommand(fillTexturePageCMD, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, &inputTexture1Layer, config.layout.GetElements().at(4).size, config.layout.GetElements().at(4).offset);

			cmd::FillConstantBuffer* fillTextureMipLevel = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, r2::mem::StackArena>(*renderer.mCommandArena, fillTexturePageCMD, config.layout.GetElements().at(5).size);
			cmd::FillConstantBufferCommand(fillTextureMipLevel, bloomConstantBufferHandle, constBufferData->type, constBufferData->isPersistent, &inputTexture1MipLevel, config.layout.GetElements().at(5).size, config.layout.GetElements().at(5).offset);

			cmd::BindImageTexture* bindInputImageTexture2CMD = AppendCommand<cmd::FillConstantBuffer, cmd::BindImageTexture, r2::mem::StackArena>(*renderer.mCommandArena, fillTextureMipLevel, 0);
			bindInputImageTexture2CMD->textureUnit = 0;
			bindInputImageTexture2CMD->texture = bloomBlurTexture.container->texId;
			bindInputImageTexture2CMD->mipLevel = i-1;
			bindInputImageTexture2CMD->layered = true;
			bindInputImageTexture2CMD->layer = bloomBlurTexture.sliceIndex;
			bindInputImageTexture2CMD->format = bloomBlurColorAttachment.format.internalformat;
			bindInputImageTexture2CMD->access = tex::READ_ONLY;

			cmd::BindImageTexture* bindOutputImageTextureCMD = AppendCommand<cmd::BindImageTexture, cmd::BindImageTexture, r2::mem::StackArena>(*renderer.mCommandArena, bindInputImageTexture2CMD, 0);
			bindOutputImageTextureCMD->textureUnit = 1;
			bindOutputImageTextureCMD->texture = bloomUpSampleTexture.container->texId;
			bindOutputImageTextureCMD->mipLevel = i-1;
			bindOutputImageTextureCMD->layered = true;
			bindOutputImageTextureCMD->layer = bloomUpSampleTexture.sliceIndex;
			bindOutputImageTextureCMD->format = bloomUpSampleColorAttachment.format.internalformat;
			bindOutputImageTextureCMD->access = tex::WRITE_ONLY;

			cmd::DispatchCompute* dispatchBloomUpSamplePreFilterCMD = AppendCommand<cmd::BindImageTexture, cmd::DispatchCompute, r2::mem::StackArena>(*renderer.mCommandArena, bindOutputImageTextureCMD, 0);

			dispatchBloomUpSamplePreFilterCMD->numGroupsX = glm::max(static_cast<u32>(glm::ceil(float(nextMipSize.imageWidth) / float(WARP_SIZE))), 1u);
			dispatchBloomUpSamplePreFilterCMD->numGroupsY = glm::max(static_cast<u32>(glm::ceil(float(nextMipSize.imageHeight) / float(WARP_SIZE))), 1u);
			dispatchBloomUpSamplePreFilterCMD->numGroupsZ = 1;

			++pass;
		}

		FREE(mipSizes, *MEM_ENG_SCRATCH_PTR);
	}

	void UpdateBloomDataIfNeeded(Renderer& renderer)
	{
		if (renderer.mBloomParamsNeedsUpdate)
		{
			const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles(renderer);

			auto bloomConstantBufferHandle = r2::sarr::At(*constantBufferHandles, renderer.mBloomConfigHandle);

			glm::vec4 bloomFilterData = glm::vec4(renderer.mBloomParams.mBloomThreshold, renderer.mBloomParams.mBloomThreshold - renderer.mBloomParams.mBloomKnee, 2.0f * renderer.mBloomParams.mBloomKnee, 0.25f / (renderer.mBloomParams.mBloomKnee + 0.00001f));

			r2::draw::renderer::AddFillConstantBufferCommandForData(renderer, bloomConstantBufferHandle, 0, glm::value_ptr(bloomFilterData));
		
			renderer.mBloomParamsNeedsUpdate = false;
		}
	}

	void FXAARenderPass(Renderer& renderer, ConstantBufferHandle subCommandsConstantBufferHandle, BufferLayoutHandle bufferLayoutHandle, u32 numSubCommands, u32 startCommandIndex)
	{
		ClearSurfaceOptions clearOptions;
		clearOptions.shouldClear = true;
		clearOptions.flags = cmd::CLEAR_COLOR_BUFFER;
		
		key::Basic outputClearKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, 0, DL_CLEAR, 0, 0, renderer.mFXAAShader);

		BeginRenderPass<key::Basic>(renderer, RPT_OUTPUT, clearOptions, *renderer.mFinalBucket, outputClearKey, *renderer.mCommandArena);

		key::Basic outputBatchKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, 0, DL_SCREEN, 0, 0, renderer.mFXAAShader);

		cmd::DrawBatch* outputDrawBatch = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mFinalBucket, outputBatchKey, 0); //@TODO(Serge): we should have mFinalBucket have it's own arena instead of renderer.mCommandArena
		outputDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		outputDrawBatch->bufferLayoutHandle = bufferLayoutHandle;
		outputDrawBatch->numSubCommands = numSubCommands;
		R2_CHECK(outputDrawBatch->numSubCommands > 0, "We should have a count!");
		outputDrawBatch->startCommandIndex = startCommandIndex;
		outputDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		outputDrawBatch->subCommands = nullptr;
		outputDrawBatch->state.depthEnabled = false;
		outputDrawBatch->state.depthFunction = LESS;
		outputDrawBatch->state.depthWriteEnabled = true; //needs to be set for some reason even though depth isn't enabled?
		outputDrawBatch->state.polygonOffsetEnabled = false;
		outputDrawBatch->state.polygonOffset = glm::vec2(0);

		cmd::SetDefaultCullState(outputDrawBatch->state.cullState);
		cmd::SetDefaultStencilState(outputDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(outputDrawBatch->state.blendState);


		EndRenderPass(renderer, RPT_OUTPUT, *renderer.mFinalBucket);
	}

	void UpdateFXAADataIfNeeded(Renderer& renderer)
	{
		if (renderer.mFXAANeedsUpdate)
		{
			renderer.mFXAANeedsUpdate = false;

			const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles(renderer);
			auto fxaaConstantBufferHandle = r2::sarr::At(*constantBufferHandles, renderer.mAAConfigHandle);

			AddFillConstantBufferCommandFull(renderer, fxaaConstantBufferHandle, &renderer.mAAParams, sizeof(renderer.mAAParams), 0);
		}
	}

	void UpdateSMAADataIfNeeded(Renderer& renderer)
	{
		const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles(renderer);
		auto smaaConstantBufferHandle = r2::sarr::At(*constantBufferHandles, renderer.mAAConfigHandle);

		if (renderer.mSMAANeedsUpdate)
		{
			renderer.mSMAANeedsUpdate = false;
			AddFillConstantBufferCommandFull(renderer, smaaConstantBufferHandle, &renderer.mAAParams, sizeof(renderer.mAAParams), 0);
		}
	}

	glm::vec2 SMAAGetJitter(const Renderer& renderer, u64 frameIndex)
	{
		if (renderer.mOutputMerger == OUTPUT_SMAA_T2X)
		{
			static glm::vec2 jitters[] = { {-0.25, 0.25}, {0.25, -0.25} };
			return jitters[frameIndex];
		}

		return glm::vec2(0);
	}

	glm::ivec4 SMAASubSampleIndices(const Renderer& renderer, u64 frameIndex)
	{
		if (renderer.mOutputMerger == OUTPUT_SMAA_T2X)
		{
			static glm::ivec4 subsampleIndices[] = { {1, 1, 1, 0}, {2, 2, 2, 0} };

			return subsampleIndices[frameIndex];
		}

		return glm::ivec4(0);
	}

	//@TODO(Serge): refactor so that it only does SMAAx1 (instead of handling both current cases) and takes in which pass/frame buffer it needs, along with proper keys to sort the command bucket
	void SMAAx1RenderPass(
		Renderer& renderer,
		ConstantBufferHandle subCommandsConstantBufferHandle,
		BufferLayoutHandle bufferLayoutHandle,
		u32 numSubCommands,
		u32 startCommandIndex,
		ShaderHandle neighborhoodBlendingShader,
		u32 pass)
	{
		R2_CHECK(pass >= 0 && pass <= 1, "Pass should be in the range [0,1]");

		ClearSurfaceOptions clearOptions;
		clearOptions.shouldClear = true;
		clearOptions.flags = cmd::CLEAR_COLOR_BUFFER | cmd::CLEAR_STENCIL_BUFFER;

		constexpr u32 numberOfStages = 4;

		u32 edgeDetectionPass = pass * numberOfStages + 0;

		//edge detection pass
		key::Basic edgeDetectionClearKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, edgeDetectionPass, DL_CLEAR, 0, edgeDetectionPass, renderer.mSMAAEdgeDetectionShader);

		BeginRenderPass<key::Basic>(renderer, RPT_SMAA_EDGE_DETECTION, clearOptions, *renderer.mFinalBucket, edgeDetectionClearKey, *renderer.mCommandArena);

		key::Basic edgeDetectionBatchKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, edgeDetectionPass, DL_SCREEN, 0, edgeDetectionPass, renderer.mSMAAEdgeDetectionShader);

		cmd::DrawBatch* edgeDetectionDrawBatch = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mFinalBucket, edgeDetectionBatchKey, 0); //@TODO(Serge): we should have mFinalBucket have it's own arena instead of renderer.mCommandArena
		edgeDetectionDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		edgeDetectionDrawBatch->bufferLayoutHandle = bufferLayoutHandle;
		edgeDetectionDrawBatch->numSubCommands = numSubCommands;
		R2_CHECK(edgeDetectionDrawBatch->numSubCommands > 0, "We should have a count!");
		edgeDetectionDrawBatch->startCommandIndex = startCommandIndex;
		edgeDetectionDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		edgeDetectionDrawBatch->subCommands = nullptr;
		edgeDetectionDrawBatch->state.depthEnabled = false;
		edgeDetectionDrawBatch->state.depthFunction = LESS;
		edgeDetectionDrawBatch->state.depthWriteEnabled = true; //needs to be set for some reason even though depth isn't enabled?
		edgeDetectionDrawBatch->state.polygonOffsetEnabled = false;
		edgeDetectionDrawBatch->state.polygonOffset = glm::vec2(0);

		cmd::SetDefaultCullState(edgeDetectionDrawBatch->state.cullState);
		cmd::SetDefaultStencilState(edgeDetectionDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(edgeDetectionDrawBatch->state.blendState);

		edgeDetectionDrawBatch->state.stencilState.op.stencilFail = r2::draw::KEEP;
		edgeDetectionDrawBatch->state.stencilState.op.depthFail = r2::draw::KEEP;
		edgeDetectionDrawBatch->state.stencilState.op.depthAndStencilPass = r2::draw::REPLACE;

		edgeDetectionDrawBatch->state.stencilState.stencilEnabled = true;
		edgeDetectionDrawBatch->state.stencilState.stencilWriteEnabled = true;
		edgeDetectionDrawBatch->state.stencilState.func.func = r2::draw::ALWAYS;
		edgeDetectionDrawBatch->state.stencilState.func.ref = 1;
		edgeDetectionDrawBatch->state.stencilState.func.mask = 0xFF;

		EndRenderPass(renderer, RPT_SMAA_EDGE_DETECTION, *renderer.mFinalBucket);

		clearOptions.flags = cmd::CLEAR_COLOR_BUFFER;

		//blending weights pass

		u32 blendingWeightPass = pass * numberOfStages + 1;

		key::Basic blendingWeightClearKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, blendingWeightPass, DL_CLEAR, 0, blendingWeightPass, renderer.mSMAABlendingWeightShader);

		BeginRenderPass<key::Basic>(renderer, RPT_SMAA_BLENDING_WEIGHT, clearOptions, *renderer.mFinalBucket, blendingWeightClearKey, *renderer.mCommandArena);

		key::Basic blendingWeightBatchKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, blendingWeightPass, DL_SCREEN, 0, blendingWeightPass, renderer.mSMAABlendingWeightShader);

		cmd::DrawBatch* blendingWeightDrawBatch = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mFinalBucket, blendingWeightBatchKey, 0); //@TODO(Serge): we should have mFinalBucket have it's own arena instead of renderer.mCommandArena
		blendingWeightDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		blendingWeightDrawBatch->bufferLayoutHandle = bufferLayoutHandle;
		blendingWeightDrawBatch->numSubCommands = numSubCommands;
		R2_CHECK(blendingWeightDrawBatch->numSubCommands > 0, "We should have a count!");
		blendingWeightDrawBatch->startCommandIndex = startCommandIndex;
		blendingWeightDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		blendingWeightDrawBatch->subCommands = nullptr;
		blendingWeightDrawBatch->state.depthEnabled = false;
		blendingWeightDrawBatch->state.depthFunction = LESS;
		blendingWeightDrawBatch->state.depthWriteEnabled = true; //needs to be set for some reason even though depth isn't enabled?
		blendingWeightDrawBatch->state.polygonOffsetEnabled = false;
		blendingWeightDrawBatch->state.polygonOffset = glm::vec2(0);

		cmd::SetDefaultCullState(blendingWeightDrawBatch->state.cullState);
		cmd::SetDefaultStencilState(blendingWeightDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(blendingWeightDrawBatch->state.blendState);

		blendingWeightDrawBatch->state.stencilState.op.stencilFail = r2::draw::KEEP;
		blendingWeightDrawBatch->state.stencilState.op.depthFail = r2::draw::KEEP;
		blendingWeightDrawBatch->state.stencilState.op.depthAndStencilPass = r2::draw::REPLACE;

		blendingWeightDrawBatch->state.stencilState.stencilEnabled = true;
		blendingWeightDrawBatch->state.stencilState.stencilWriteEnabled = false;
		blendingWeightDrawBatch->state.stencilState.func.func = r2::draw::EQUAL;
		blendingWeightDrawBatch->state.stencilState.func.ref = 1;
		blendingWeightDrawBatch->state.stencilState.func.mask = 0xFF;


		EndRenderPass(renderer, RPT_SMAA_BLENDING_WEIGHT, *renderer.mFinalBucket);

		//neighborhood blending pass
		key::Basic neighborhoodBlendingClearKey;
		key::Basic neighborhoodBlendingBatchKey;

		u32 neighborhoodBlendingPass = pass * numberOfStages + 2;

		neighborhoodBlendingClearKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, neighborhoodBlendingPass, DL_CLEAR, 0, neighborhoodBlendingPass, neighborhoodBlendingShader);
		neighborhoodBlendingBatchKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, neighborhoodBlendingPass, DL_SCREEN, 0, neighborhoodBlendingPass, neighborhoodBlendingShader);

		BeginRenderPass<key::Basic>(renderer, RPT_SMAA_NEIGHBORHOOD_BLENDING, clearOptions, *renderer.mFinalBucket, neighborhoodBlendingClearKey, *renderer.mCommandArena);

		cmd::DrawBatch* neighborhoodBlendingDrawBatch = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mFinalBucket, neighborhoodBlendingBatchKey, 0); //@TODO(Serge): we should have mFinalBucket have it's own arena instead of renderer.mCommandArena
		neighborhoodBlendingDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		neighborhoodBlendingDrawBatch->bufferLayoutHandle = bufferLayoutHandle;
		neighborhoodBlendingDrawBatch->numSubCommands = numSubCommands;
		R2_CHECK(neighborhoodBlendingDrawBatch->numSubCommands > 0, "We should have a count!");
		neighborhoodBlendingDrawBatch->startCommandIndex = startCommandIndex;
		neighborhoodBlendingDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		neighborhoodBlendingDrawBatch->subCommands = nullptr;
		neighborhoodBlendingDrawBatch->state.depthEnabled = false;
		neighborhoodBlendingDrawBatch->state.depthFunction = LESS;
		neighborhoodBlendingDrawBatch->state.depthWriteEnabled = true; //needs to be set for some reason even though depth isn't enabled?
		neighborhoodBlendingDrawBatch->state.polygonOffsetEnabled = false;
		neighborhoodBlendingDrawBatch->state.polygonOffset = glm::vec2(0);

		cmd::SetDefaultCullState(neighborhoodBlendingDrawBatch->state.cullState);
		cmd::SetDefaultStencilState(neighborhoodBlendingDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(neighborhoodBlendingDrawBatch->state.blendState);

		EndRenderPass(renderer, RPT_SMAA_NEIGHBORHOOD_BLENDING, *renderer.mFinalBucket);
	}

	void SMAAx1OutputRenderPass(Renderer& renderer, ConstantBufferHandle subCommandsConstantBufferHandle, BufferLayoutHandle bufferLayoutHandle, u32 numSubCommands, u32 startCommandIndex, ShaderHandle outputShader, u32 pass)
	{
		ClearSurfaceOptions clearOptions;
		clearOptions.shouldClear = true;
		clearOptions.flags = cmd::CLEAR_COLOR_BUFFER;

		constexpr u32 numberOfStages = 4;

		u32 outputPass = pass * numberOfStages + 3;

		key::Basic outputClearKey;
		key::Basic outputBatchKey;

		outputClearKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, outputPass, DL_CLEAR, 0, outputPass, outputShader);
		outputBatchKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, outputPass, DL_SCREEN, 0, outputPass, outputShader);

		BeginRenderPass<key::Basic>(renderer, RPT_OUTPUT, clearOptions, *renderer.mFinalBucket, outputClearKey, *renderer.mCommandArena);

		cmd::DrawBatch* outputDrawBatch = nullptr;

		const auto& compositeColorAttachment = r2::sarr::At(*renderer.mRenderTargets[RTS_SMAA_NEIGHBORHOOD_BLENDING].colorAttachments, 0);

		const auto textureAddress = tex::GetTextureAddress(compositeColorAttachment.texture[compositeColorAttachment.currentTexture]);

		cmd::SetTexture* setCompositeTexture = AddCommand<key::Basic, cmd::SetTexture, mem::StackArena>(*renderer.mCommandArena, *renderer.mFinalBucket, outputBatchKey, 0);
		setCompositeTexture->textureContainerUniformLocation = renderer.mPassThroughTextureContainerLocation;
		setCompositeTexture->textureContainer = textureAddress.containerHandle;
		setCompositeTexture->texturePageUniformLocation = renderer.mPassThroughTexturePageLocation;
		setCompositeTexture->texturePage = textureAddress.texPage;
		setCompositeTexture->textureLodUniformLocation = renderer.mPassThroughTextureLodLocation;
		setCompositeTexture->textureLod = 0;

		outputDrawBatch = AppendCommand<cmd::SetTexture, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, setCompositeTexture, 0);

		outputDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		outputDrawBatch->bufferLayoutHandle = bufferLayoutHandle;
		outputDrawBatch->numSubCommands = numSubCommands;
		R2_CHECK(outputDrawBatch->numSubCommands > 0, "We should have a count!");
		outputDrawBatch->startCommandIndex = startCommandIndex;
		outputDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		outputDrawBatch->subCommands = nullptr;
		outputDrawBatch->state.depthEnabled = false;
		outputDrawBatch->state.depthFunction = LESS;
		outputDrawBatch->state.depthWriteEnabled = true; //needs to be set for some reason even though depth isn't enabled?
		outputDrawBatch->state.polygonOffsetEnabled = false;
		outputDrawBatch->state.polygonOffset = glm::vec2(0);

		cmd::SetDefaultCullState(outputDrawBatch->state.cullState);
		cmd::SetDefaultStencilState(outputDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(outputDrawBatch->state.blendState);

		EndRenderPass(renderer, RPT_OUTPUT, *renderer.mFinalBucket);
	}

	void SMAAxT2SOutputRenderPass(Renderer& renderer, ConstantBufferHandle subCommandsConstantBufferHandle, BufferLayoutHandle bufferLayoutHandle, u32 numSubCommands, u32 startCommandIndex, ShaderHandle outputShader, u32 pass)
	{
		ClearSurfaceOptions clearOptions;
		clearOptions.shouldClear = true;
		clearOptions.flags = cmd::CLEAR_COLOR_BUFFER;

		constexpr u32 numberOfStages = 4;

		u32 outputPass = pass * numberOfStages + 3;

		key::Basic outputClearKey;
		key::Basic outputBatchKey;

		outputClearKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, outputPass, DL_CLEAR, 0, outputPass, outputShader);
		outputBatchKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, outputPass, DL_SCREEN, 0, outputPass, outputShader);

		BeginRenderPass<key::Basic>(renderer, RPT_OUTPUT, clearOptions, *renderer.mFinalBucket, outputClearKey, *renderer.mCommandArena);

		cmd::DrawBatch* outputDrawBatch = nullptr;
		outputDrawBatch = AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mFinalBucket, outputBatchKey, 0);
		outputDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		outputDrawBatch->bufferLayoutHandle = bufferLayoutHandle;
		outputDrawBatch->numSubCommands = numSubCommands;
		R2_CHECK(outputDrawBatch->numSubCommands > 0, "We should have a count!");
		outputDrawBatch->startCommandIndex = startCommandIndex;
		outputDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		outputDrawBatch->subCommands = nullptr;
		outputDrawBatch->state.depthEnabled = false;
		outputDrawBatch->state.depthFunction = LESS;
		outputDrawBatch->state.depthWriteEnabled = true; //needs to be set for some reason even though depth isn't enabled?
		outputDrawBatch->state.polygonOffsetEnabled = false;
		outputDrawBatch->state.polygonOffset = glm::vec2(0);

		cmd::SetDefaultCullState(outputDrawBatch->state.cullState);
		cmd::SetDefaultStencilState(outputDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(outputDrawBatch->state.blendState);

		EndRenderPass(renderer, RPT_OUTPUT, *renderer.mFinalBucket);
	}

	void NonAARenderPass(Renderer& renderer, ConstantBufferHandle subCommandsConstantBufferHandle, BufferLayoutHandle bufferLayoutHandle, u32 numSubCommands, u32 startCommandIndex)
	{
		ClearSurfaceOptions clearOptions;
		clearOptions.shouldClear = true;
		clearOptions.flags = cmd::CLEAR_COLOR_BUFFER;

		key::Basic outputClearKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, 0, DL_CLEAR, 0, 0, renderer.mPassThroughShader);

		BeginRenderPass<key::Basic>(renderer, RPT_OUTPUT, clearOptions, *renderer.mFinalBucket, outputClearKey, *renderer.mCommandArena);

		key::Basic outputBatchKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, 0, DL_SCREEN, 0, 0, renderer.mPassThroughShader);

		const auto& compositeColorAttachment = r2::sarr::At(*renderer.mRenderTargets[RTS_COMPOSITE].colorAttachments, 0);

		const auto textureAddress = tex::GetTextureAddress(compositeColorAttachment.texture[compositeColorAttachment.currentTexture]);

		cmd::SetTexture* setCompositeTexture = AddCommand<key::Basic, cmd::SetTexture, mem::StackArena>(*renderer.mCommandArena, *renderer.mFinalBucket, outputBatchKey, 0);
		setCompositeTexture->textureContainerUniformLocation = renderer.mPassThroughTextureContainerLocation;
		setCompositeTexture->textureContainer = textureAddress.containerHandle;
		setCompositeTexture->texturePageUniformLocation = renderer.mPassThroughTexturePageLocation;
		setCompositeTexture->texturePage = textureAddress.texPage;
		setCompositeTexture->textureLodUniformLocation = renderer.mPassThroughTextureLodLocation;
		setCompositeTexture->textureLod = 0;

		cmd::DrawBatch* outputDrawBatch = AppendCommand<cmd::SetTexture, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, setCompositeTexture, 0);//AddCommand<key::Basic, cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mFinalBucket, outputBatchKey, 0);
		outputDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		outputDrawBatch->bufferLayoutHandle = bufferLayoutHandle;
		outputDrawBatch->numSubCommands = numSubCommands;
		R2_CHECK(outputDrawBatch->numSubCommands > 0, "We should have a count!");
		outputDrawBatch->startCommandIndex = startCommandIndex;
		outputDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		outputDrawBatch->subCommands = nullptr;
		outputDrawBatch->state.depthEnabled = false;
		outputDrawBatch->state.depthFunction = LESS;
		outputDrawBatch->state.depthWriteEnabled = true; //needs to be set for some reason even though depth isn't enabled?
		outputDrawBatch->state.polygonOffsetEnabled = false;
		outputDrawBatch->state.polygonOffset = glm::vec2(0);

		cmd::SetDefaultCullState(outputDrawBatch->state.cullState);
		cmd::SetDefaultStencilState(outputDrawBatch->state.stencilState);
		cmd::SetDefaultBlendState(outputDrawBatch->state.blendState);

		EndRenderPass(renderer, RPT_OUTPUT, *renderer.mFinalBucket);
	}

	void CheckIfValidShader(Renderer& renderer, ShaderHandle shader, const char* name)
	{
		R2_CHECK(shader != 0, "Shader: %s is invalid!", name);
	}

	void UpdateCamera(Renderer& renderer, Camera& camera)
	{
		if (renderer.mOutputMerger == OUTPUT_SMAA_T2X ||
			renderer.mOutputMerger == OUTPUT_SMAA_X1)
		{
			u64 frameIndex = (renderer.mShaderVectors.frame % 2);

			glm::vec2 smaaCameraJitter = SMAAGetJitter(renderer, frameIndex);
			glm::vec2 jitter = glm::vec2( smaaCameraJitter.x / static_cast<float>(renderer.mCompositeSize.width), smaaCameraJitter.y / static_cast<float>(renderer.mCompositeSize.height));

			cam::SetCameraJitter(camera, jitter);
		}
		else
		{
			cam::SetCameraJitter(camera, glm::vec2(0));
		}

		renderer.mShaderMatrices.projection = camera.proj;
		renderer.mShaderMatrices.view = camera.view;
		renderer.mShaderMatrices.skyboxView = glm::mat4(glm::mat3(camera.view));
		renderer.mShaderMatrices.inverseProjection = camera.invProj;
		renderer.mShaderMatrices.inverseView = camera.invView;
		renderer.mShaderMatrices.vpMatrix = camera.vp;
		
		UpdateShaderMatrices(renderer);

		renderer.mShaderVectors.cameraPosTimeW = glm::vec4(camera.position, CENG.GetTicks() / 1000.0f);
		renderer.mShaderVectors.exposureNearFar = glm::vec4(camera.exposure, camera.nearPlane, camera.farPlane, 0);

		R2_CHECK(cam::NUM_FRUSTUM_SPLITS == 4, "Change to not be a vec4 if cam::NUM_FRUSTUM_SPLITS is >");

		renderer.mShaderVectors.shadowMapSizes = glm::vec4(light::SHADOW_MAP_SIZE, light::SHADOW_MAP_SIZE, light::SHADOW_MAP_SIZE, light::SHADOW_MAP_SIZE);
		renderer.mShaderVectors.fovAspectResXResY = glm::vec4(camera.fov, camera.aspectRatio, renderer.mResolutionSize.width, renderer.mResolutionSize.height);
	}

	void DrawModel(
		Renderer& renderer,
		u32 entityID,
		const r2::SArray<s32>& entityInstances,
		const DrawParameters& drawParameters,
		const vb::GPUModelRefHandle& modelRefHandle,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<r2::mat::MaterialName>& materialNames,
		const r2::SArray<ShaderBoneTransform>* boneTransforms)
	{
		if (renderer.mRenderBatches == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		if (renderer.mStaticVertexModelConfigHandle == vb::InvalidVertexBufferLayoutHandle ||
			renderer.mAnimVertexModelConfigHandle == vb::InvalidVertexBufferLayoutHandle)
		{
			R2_CHECK(false, "We haven't generated the layouts yet!");
			return;
		}

		const vb::GPUModelRef* gpuModelRef = vbsys::GetGPUModelRef(*renderer.mVertexBufferLayoutSystem, modelRefHandle);
		R2_CHECK(gpuModelRef != nullptr, "Failed to get the GPUModelRef for handle: %llu", modelRefHandle);

		R2_CHECK(gpuModelRef->numMaterials == r2::sarr::Size(materialNames) || ((r2::sarr::Size(materialNames) == 1) && gpuModelRef->numMaterials >= 1), "These must be the same");

		DrawType drawType = STATIC;

		if (gpuModelRef->isAnimated && !boneTransforms)
		{
			R2_CHECK(false, "Submitted an animated model with no bone transforms!");
		}

		if (gpuModelRef->isAnimated)
		{
			drawType = DYNAMIC;
		}
		
		//@NOTE(Serge): maybe we should pass the mesh data in
		auto gameAssetManager = CENG.GetGameAssetManager();
		const Model* model = nullptr;

		if (r2::draw::modlche::HasModel(renderer.mModelCache, { gpuModelRef->assetName.hashID, renderer.mModelCache->mModelCache->GetSlot() }))
		{
			model = GetDefaultModel(gpuModelRef->assetName);
		}
		else
		{
			model = gameAssetManager.GetAssetDataConst<Model>(gpuModelRef->assetName);
		}

		R2_CHECK(model != nullptr, "Should never happen");

		RenderBatch& batch = r2::sarr::At(*renderer.mRenderBatches, drawType);

		if (model->optrGLTFMeshInfos)
		{
			const auto numMeshInfos = r2::sarr::Size(*model->optrGLTFMeshInfos);
			for (u32 i = 0; i < numMeshInfos; ++i)
			{
				MeshRenderData meshRenderData;
				const auto& gltfMeshInfo = r2::sarr::At(*model->optrGLTFMeshInfos, i);
				meshRenderData.globalInvTransform = gltfMeshInfo.meshGlobalInv;
				meshRenderData.globalTransform = gltfMeshInfo.meshGlobal;

				r2::sarr::Push(*batch.meshRenderData, meshRenderData);
			}
		}
		else
		{
			MeshRenderData meshRenderData = {};
			r2::sarr::Push(*batch.meshRenderData, meshRenderData);
		}

		r2::sarr::Push(*batch.gpuModelRefs, gpuModelRef);

#ifdef R2_EDITOR
		r2::sarr::Push(*batch.entityIDs, entityID);
		EntityInstanceBatchOffset entityInstanceBatchOffset;
		entityInstanceBatchOffset.start = r2::sarr::Size(*batch.entityInstances);
		entityInstanceBatchOffset.numInstances = r2::sarr::Size(entityInstances);
		r2::sarr::Push(*batch.entityInstanceOffsetBatches, entityInstanceBatchOffset);
		r2::sarr::Append(*batch.entityInstances, entityInstances);
#endif

		r2::sarr::Append(*batch.models, modelMatrices);

		r2::sarr::Push(*batch.numInstances, static_cast<u32>( r2::sarr::Size(modelMatrices)));

		cmd::DrawState state;

		memset(&state, 0, sizeof(cmd::DrawState));

		state.depthEnabled = drawParameters.flags.IsSet(DEPTH_TEST);
		state.layer = drawParameters.layer;
		state.stencilState = drawParameters.stencilState;
		state.cullState = drawParameters.cullState;
		memcpy(&state.blendState, &drawParameters.blendState, sizeof(BlendState));

		r2::sarr::Push(*batch.drawState, state);

		bool isMaterialSizesSame = r2::sarr::Size(materialNames) == gpuModelRef->numMaterials;

		MaterialBatch::Info materialBatchInfo;

		materialBatchInfo.start = r2::sarr::Size(*batch.materialBatch.renderMaterialParams);
		materialBatchInfo.numMaterials = gpuModelRef->numMaterials;

		r2::sarr::Push(*batch.materialBatch.infos, materialBatchInfo);

		r2::draw::RenderMaterialCache* renderMaterialCache = renderer.mRenderMaterialCache;

		if (isMaterialSizesSame)
		{
			for (u32 i = 0; i < materialBatchInfo.numMaterials; ++i)
			{
				const r2::mat::MaterialName& materialName = r2::sarr::At(materialNames, i);

				r2::draw::RenderMaterialParams* renderMaterial = nullptr;
				r2::draw::ShaderEffectPasses shaderEffectPasses;

				bool result = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, materialName, &renderMaterial, shaderEffectPasses);
				R2_CHECK(result, "Should always succeed");

				r2::sarr::Push(*batch.materialBatch.renderMaterialParams, *renderMaterial);
				r2::sarr::Push(*batch.materialBatch.shaderEffectPasses, shaderEffectPasses);
			}
		}
		else
		{
			R2_CHECK(r2::sarr::Size(materialNames) == 1, "You should only have 1 material in this case");

			const r2::mat::MaterialName& materialName = r2::sarr::At(materialNames, 0);

			r2::draw::RenderMaterialParams* renderMaterial = nullptr;
			r2::draw::ShaderEffectPasses shaderEffectPasses;

			bool result = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, materialName, &renderMaterial, shaderEffectPasses);
			R2_CHECK(result, "Should always succeed");

			for (u32 i = 0; i < gpuModelRef->numMaterials; ++i)
			{
				r2::sarr::Push(*batch.materialBatch.renderMaterialParams, *renderMaterial);
				r2::sarr::Push(*batch.materialBatch.shaderEffectPasses, shaderEffectPasses);
			}
		}

		if (drawType == DYNAMIC)
		{
			b32 useSameBoneTransformsForInstances = drawParameters.flags.IsSet(USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
			R2_CHECK(boneTransforms != nullptr && r2::sarr::Size(*boneTransforms) > 0, "bone transforms should be valid");
			r2::sarr::Push(*batch.useSameBoneTransformsForInstances, useSameBoneTransformsForInstances);
			r2::sarr::Append(*batch.boneTransforms, *boneTransforms);
		}
	}

	void DrawModels(
		Renderer& renderer,
		const r2::SArray<u32>& entities,
		const r2::SArray<s32>& entityInstances,
		const r2::SArray<u32>& numEntityInstancesPerEntity,
		const DrawParameters& drawParameters,
		const r2::SArray<vb::GPUModelRefHandle>& modelRefHandles,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<u32>& numInstancesPerModel,
		const r2::SArray<r2::mat::MaterialName>& materialNames,
		const r2::SArray<ShaderBoneTransform>* boneTransforms) 
	{
		if (renderer.mRenderBatches == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		const auto numInstancesInArray = r2::sarr::Size(numInstancesPerModel);
		const auto numModelMatrices = r2::sarr::Size(modelMatrices);
		const auto numModelRefs = r2::sarr::Size(modelRefHandles);

		R2_CHECK(numModelRefs > 0, "We should have model refs here!");

		R2_CHECK(numModelRefs <= numModelMatrices, "We must have at least as many model matrices as model refs!");
		R2_CHECK(numModelRefs == numInstancesInArray, "These must be the same!");

#if R2_DEBUG
		u32 numInstancesInTotal = 0;

		for (u64 i = 0; i < numInstancesInArray; ++i)
		{
			numInstancesInTotal += static_cast<u32>(r2::sarr::At(numInstancesPerModel, i));
		}

		R2_CHECK(numInstancesInTotal == numModelMatrices, "We should have the same amount of instances as there are model matrices");

#endif

		r2::SArray<const vb::GPUModelRef*>* modelRefArray = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const vb::GPUModelRef*, numModelRefs);
		auto gameAssetManager = CENG.GetGameAssetManager();

		//get all the model refs
		for (u64 i = 0; i < numModelRefs; ++i)
		{
			const vb::GPUModelRef* gpuModelRef = vbsys::GetGPUModelRef(*renderer.mVertexBufferLayoutSystem, r2::sarr::At(modelRefHandles, i));

			r2::sarr::Push(*modelRefArray, gpuModelRef);
		}

#if defined(R2_DEBUG)
		//check to see if what we're being given is all one type
		if (!boneTransforms)
		{
			for (u64 i = 0; i < numModelRefs; ++i)
			{
				const auto modelRef = r2::sarr::At(*modelRefArray, i);

				R2_CHECK(modelRef->isAnimated == false, "modelRef at index: %llu should not be animated!", i);
			}
		}
		else 
		{
			const auto numBoneTransforms = r2::sarr::Size(*boneTransforms);
			u32 numBonesInTotal = 0;
			bool useSameBoneTransformsForInstances = drawParameters.flags.IsSet(USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);

			for (u64 i = 0; i < numModelRefs; ++i)
			{
				const auto modelRef = r2::sarr::At(*modelRefArray, i);

				R2_CHECK(modelRef->isAnimated == true, "modelRef at index: %llu should be animated!", i);

				u32 numInstancesForModel = r2::sarr::At(numInstancesPerModel, i);

				if (useSameBoneTransformsForInstances)
				{
					numBonesInTotal += modelRef->numBones;
				}
				else
				{
					numBonesInTotal += (modelRef->numBones * numInstancesForModel);
				}
			}

			R2_CHECK(numBonesInTotal == numBoneTransforms, "These should match!");
		}


		u64 numMaterialsForGPUModelRefs = 0;
		for (u64 i = 0; i < numModelRefs; ++i)
		{
			const vb::GPUModelRef* modelRef = r2::sarr::At(*modelRefArray, i);
			numMaterialsForGPUModelRefs += modelRef->numMaterials;
		}

		R2_CHECK(r2::sarr::Size(materialNames) == numMaterialsForGPUModelRefs, "These should be the same in this case");

#endif

		DrawType drawType = (!boneTransforms) ? DrawType::STATIC : DrawType::DYNAMIC;

		RenderBatch& batch = r2::sarr::At(*renderer.mRenderBatches, drawType);


		//@NOTE(Serge): maybe we should pass the mesh data in
		for (u32 i = 0; i < numModelRefs; ++i)
		{
			const auto gpuModelRef = r2::sarr::At(*modelRefArray, i);
			const Model* model = nullptr;

			if (r2::draw::modlche::HasModel(renderer.mModelCache, { gpuModelRef->assetName.hashID, renderer.mModelCache->mModelCache->GetSlot() }))
			{
				model = GetDefaultModel(gpuModelRef->assetName);
			}
			else
			{
				model = gameAssetManager.GetAssetDataConst<Model>(gpuModelRef->assetName);
			}

			R2_CHECK(model != nullptr, "Should never happen");

			RenderBatch& batch = r2::sarr::At(*renderer.mRenderBatches, drawType);

			if (model->optrGLTFMeshInfos)
			{
				const auto numMeshInfos = r2::sarr::Size(*model->optrGLTFMeshInfos);
				for (u32 j = 0; j < numMeshInfos; ++j)
				{
					MeshRenderData meshRenderData;
					const auto& gltfMeshInfo = r2::sarr::At(*model->optrGLTFMeshInfos, j);
					meshRenderData.globalInvTransform = gltfMeshInfo.meshGlobalInv;
					meshRenderData.globalTransform = gltfMeshInfo.meshGlobal;

					r2::sarr::Push(*batch.meshRenderData, meshRenderData);
				}
			}
			else
			{
				MeshRenderData meshRenderData = {};
				r2::sarr::Push(*batch.meshRenderData, meshRenderData);
			}
		}

#ifdef R2_EDITOR
		R2_CHECK(r2::sarr::Size(numEntityInstancesPerEntity) == r2::sarr::Size(entities), "These should always be the same");

		r2::sarr::Append(*batch.entityIDs, entities);

		auto startEntityInstances = r2::sarr::Size(*batch.entityInstances);
		for (u32 i = 0; i < r2::sarr::Size(numEntityInstancesPerEntity); ++i)
		{
			EntityInstanceBatchOffset entityInstanceBatchOffset;
			entityInstanceBatchOffset.start = startEntityInstances;
			entityInstanceBatchOffset.numInstances = r2::sarr::At(numEntityInstancesPerEntity, i);

			startEntityInstances += entityInstanceBatchOffset.numInstances;

			r2::sarr::Push(*batch.entityInstanceOffsetBatches, entityInstanceBatchOffset);
		}
		
		r2::sarr::Append(*batch.entityInstances, entityInstances);
#endif

		r2::sarr::Append(*batch.gpuModelRefs, *modelRefArray);

		r2::sarr::Append(*batch.models, modelMatrices);

		r2::sarr::Append(*batch.numInstances, numInstancesPerModel);

		const u64 numFlags = numInstancesInArray;

		r2::SArray<cmd::DrawState>* tempDrawState = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, cmd::DrawState, numFlags);

		for (u64 i = 0; i < numFlags; ++i)
		{
			cmd::DrawState state;
			memset(&state, 0, sizeof(cmd::DrawState));

			state.depthEnabled = drawParameters.flags.IsSet(eDrawFlags::DEPTH_TEST);
			state.layer = drawParameters.layer;
			state.stencilState = drawParameters.stencilState;
			state.cullState = drawParameters.cullState;
			memcpy(&state.blendState, &drawParameters.blendState, sizeof(BlendState));

			r2::sarr::Push(*tempDrawState, state);
		}

		r2::sarr::Append(*batch.drawState, *tempDrawState);

		FREE(tempDrawState, *MEM_ENG_SCRATCH_PTR);

		u32 materialOffset = 0;
		for (u32 i = 0; i < numModelRefs; i++)
		{
			const vb::GPUModelRef* modelRef = r2::sarr::At(*modelRefArray, i);

			MaterialBatch::Info info;
			info.start = r2::sarr::Size(*batch.materialBatch.renderMaterialParams);
			info.numMaterials = modelRef->numMaterials;

			r2::sarr::Push(*batch.materialBatch.infos, info);

			for (u32 j = 0; j < info.numMaterials; ++j)
			{
				const r2::mat::MaterialName& materialName = r2::sarr::At(materialNames, j + materialOffset);

				r2::draw::RenderMaterialParams* renderMaterial = nullptr;
				r2::draw::ShaderEffectPasses shaderEffectPasses;
				r2::draw::rmat::GetGPURenderMaterial(*renderer.mRenderMaterialCache, materialName, &renderMaterial, shaderEffectPasses);

				r2::sarr::Push(*batch.materialBatch.renderMaterialParams, *renderMaterial);
				r2::sarr::Push(*batch.materialBatch.shaderEffectPasses, shaderEffectPasses);
			}

			materialOffset += info.numMaterials;
		}

		if (drawType == DYNAMIC)
		{
			R2_CHECK(boneTransforms != nullptr && r2::sarr::Size(*boneTransforms) > 0, "bone transforms should be valid");

			r2::SArray<b32>* useSameBoneTransformsForInstances = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, b32, numInstancesInArray);

			r2::sarr::Fill(*useSameBoneTransformsForInstances, static_cast<b32>( drawParameters.flags.IsSet(USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES)));

			r2::sarr::Append(*batch.useSameBoneTransformsForInstances, *useSameBoneTransformsForInstances);

			FREE(useSameBoneTransformsForInstances, *MEM_ENG_SCRATCH_PTR);

			r2::sarr::Append(*batch.boneTransforms, *boneTransforms);
		}

		FREE(modelRefArray, *MEM_ENG_SCRATCH_PTR);
	}

	ShaderHandle GetDepthShaderHandle(const Renderer& renderer, bool isDynamic)
	{
		u32 shaderIndex = isDynamic ? 1 : 0;
		return renderer.mDepthShaders[shaderIndex];
	}

	ShaderHandle GetShadowDepthShaderHandle(const Renderer& renderer, bool isDynamic, light::LightType lightType)
	{
		u32 shaderIndex = isDynamic ? 1 : 0;

		if (lightType == light::LightType::LT_DIRECTIONAL_LIGHT)
		{
			return renderer.mShadowDepthShaders[shaderIndex];
		}
		else if (lightType == light::LightType::LT_SPOT_LIGHT)
		{
			return renderer.mSpotLightShadowShaders[shaderIndex];
		}
		else if (lightType == light::LightType::LT_POINT_LIGHT)
		{
			return renderer.mPointLightShadowShaders[shaderIndex];
		}
		else
		{
			R2_CHECK(false, "No other light types are implemented yet!");
		}
	}

#ifdef R2_DEBUG

	void CreateGenericDebugSubCommands(Renderer& renderer, const DebugRenderBatch& debugRenderBatch, u32 numDebugObjectsToDraw, u32 instanceOffset, r2::SHashMap<DebugDrawCommandData*>* debugDrawCommandDataHashMap, bool isLines)
	{
		//@NOTE: this isn't at all thread safe! 
		if (renderer.mDebugCommandBucket == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		R2_CHECK(debugRenderBatch.drawFlags != nullptr, "We don't have any draw flags!");
		if (isLines)
		{
			R2_CHECK(r2::sarr::Size(*debugRenderBatch.numInstances) == r2::sarr::Size(*debugRenderBatch.drawFlags),
				"These should all be equal");
		}
		else
		{
			R2_CHECK(r2::sarr::Size(*debugRenderBatch.numInstances) == r2::sarr::Size(*debugRenderBatch.debugModelTypesToDraw) &&
				r2::sarr::Size(*debugRenderBatch.debugModelTypesToDraw) == r2::sarr::Size(*debugRenderBatch.drawFlags),
				"These should all be equal");
		}


		u64 debugConstantsOffset = 0;

		for (u64 i = 0; i < numDebugObjectsToDraw; ++i)
		{
			const DrawFlags& flags = r2::sarr::At(*debugRenderBatch.drawFlags, i);
			u32 numInstances = r2::sarr::At(*debugRenderBatch.numInstances, i);
			ShaderHandle shaderHandle = r2::sarr::At(*debugRenderBatch.shaderHandles, i);
			flat::eMeshPass meshPass = r2::sarr::At(*debugRenderBatch.meshPasses, i);

			auto sortKey = r2::draw::key::GenerateSortBatchKey(r2::draw::DL_DEBUG, shaderHandle, meshPass, flags.GetRawValue()); //@NOTE(Serge): currently don't care of the depth shader

			DebugModelType modelType = DebugModelType::DEBUG_LINE;
				
			if (!isLines)
			{
				modelType = r2::sarr::At(*debugRenderBatch.debugModelTypesToDraw, i);
			}	

			DebugDrawCommandData* emptyDebugDrawCommandData = nullptr;

			DebugDrawCommandData* debugDrawCommandData = r2::shashmap::Get(*debugDrawCommandDataHashMap, sortKey.keyValue, emptyDebugDrawCommandData);

			if (debugDrawCommandData == emptyDebugDrawCommandData)
			{
				//currently don't have one for this configuration
				debugDrawCommandData = ALLOC(DebugDrawCommandData, *renderer.mPreRenderStackArena);

				debugDrawCommandData->depthEnabled = flags.IsSet(eDrawFlags::DEPTH_TEST);
				debugDrawCommandData->primitiveType = isLines ? PrimitiveType::LINES : PrimitiveType::TRIANGLES;
				debugDrawCommandData->filled = flags.IsSet(eDrawFlags::FILL_MODEL);
				debugDrawCommandData->shaderHandle = shaderHandle;
				debugDrawCommandData->meshPass = meshPass;
				debugDrawCommandData->drawLayer = DL_DEBUG; //@TODO(Serge): change to real draw layer

				if (!isLines)
				{
					debugDrawCommandData->debugModelDrawBatchCommands = MAKE_SARRAY(*renderer.mPreRenderStackArena, cmd::DrawBatchSubCommand, MAX_NUM_DRAWS); //@NOTE(Serge): not sure if sizing is correct here
				}
				else
				{
					debugDrawCommandData->debugLineDrawBatchCommands = MAKE_SARRAY(*renderer.mPreRenderStackArena, cmd::DrawDebugBatchSubCommand, MAX_NUM_DRAWS);
				}

				r2::shashmap::Set(*debugDrawCommandDataHashMap, sortKey.keyValue, debugDrawCommandData);
			}

			if (isLines)
			{
				r2::draw::cmd::DrawDebugBatchSubCommand subCommand;
				subCommand.baseInstance = i + instanceOffset;
				subCommand.instanceCount = 1;
				subCommand.count = 2;

				subCommand.firstVertex = i * 2;
				r2::sarr::Push(*debugDrawCommandData->debugLineDrawBatchCommands, subCommand);
			}
			else
			{
				const vb::GPUModelRef* modelRef = vbsys::GetGPUModelRef(*renderer.mVertexBufferLayoutSystem, GetDefaultModelRef(renderer, static_cast<DefaultModel>(modelType)));

				const auto numMeshRefs = r2::sarr::Size(*modelRef->meshEntries);

				for (u64 j = 0; j < numMeshRefs; ++j)
				{
					r2::draw::cmd::DrawBatchSubCommand subCommand;

					const vb::MeshEntry& meshEntry = r2::sarr::At(*modelRef->meshEntries, j);

					subCommand.baseInstance = debugConstantsOffset + instanceOffset;
					subCommand.baseVertex = meshEntry.gpuVertexEntry.start;
					subCommand.firstIndex = meshEntry.gpuIndexEntry.start;
					subCommand.instanceCount = numInstances;
					subCommand.count = meshEntry.gpuIndexEntry.size;

					r2::sarr::Push(*debugDrawCommandData->debugModelDrawBatchCommands, subCommand);
				}
			}
			
			debugConstantsOffset += numInstances;
		}
	}

	void PopulateGenericDebugRenderBatchesOffsets(
		const r2::SHashMap<DebugDrawCommandData*>* debugDrawCommandMap,
		u32& subCommandsOffset,
		bool isLines,
		r2::SArray<r2::draw::cmd::DrawBatchSubCommand>* drawBatchSubCommands,
		r2::SArray<r2::draw::cmd::DrawDebugBatchSubCommand>* linesBatchSubCommands,
		r2::SArray<BatchRenderOffsets>* debugRenderBatchesOffsets)
	{
		auto hashIter = r2::shashmap::Begin(*debugDrawCommandMap);

		for (; hashIter != r2::shashmap::End(*debugDrawCommandMap); ++hashIter)
		{
			DebugDrawCommandData* drawCommandData = hashIter->value;
			if (drawCommandData != nullptr)
			{
				u32 numSubCommandsInBatch = 0;

				if (drawCommandData->debugModelDrawBatchCommands != nullptr && !isLines)
				{
					numSubCommandsInBatch = static_cast<u32>(r2::sarr::Size(*drawCommandData->debugModelDrawBatchCommands));

					if (numSubCommandsInBatch > 0)
					{
						r2::sarr::Append(*drawBatchSubCommands, *drawCommandData->debugModelDrawBatchCommands);
					}
				}
				else if(drawCommandData->debugLineDrawBatchCommands != nullptr && isLines)
				{
					numSubCommandsInBatch = static_cast<u32>(r2::sarr::Size(*drawCommandData->debugLineDrawBatchCommands));

					if (numSubCommandsInBatch > 0)
					{
						r2::sarr::Append(*linesBatchSubCommands, *drawCommandData->debugLineDrawBatchCommands);
					}
				}
				else
				{
					continue;
				}

				if (numSubCommandsInBatch > 0)
				{
					BatchRenderOffsets offsets;
					offsets.drawState.layer = drawCommandData->drawLayer;
					offsets.primitiveType = drawCommandData->primitiveType;
					offsets.drawState.depthEnabled = drawCommandData->depthEnabled;
					offsets.shaderEffectPasses.meshPasses[drawCommandData->meshPass].staticShaderHandle = drawCommandData->shaderHandle;
					offsets.numSubCommands = numSubCommandsInBatch;
					offsets.subCommandsOffset = subCommandsOffset;
					offsets.drawState.depthEnabled = drawCommandData->depthEnabled;
					offsets.drawState.depthWriteEnabled = drawCommandData->depthEnabled; //@TODO(Serge): maybe we should have a separate bool for this
					offsets.blendingFunctionKeyValue = drawCommandData->meshPass == flat::eMeshPass_FORWARD ? key::TR_OPAQUE : key::TR_ADDITIVE;
					
					subCommandsOffset += offsets.numSubCommands;

					r2::sarr::Push(*debugRenderBatchesOffsets, offsets);
				}
			}
		}		
	}


	void FillPreDebugCommandBucketModelData(Renderer& renderer, r2::SArray<cmd::DrawBatchSubCommand>* drawBatchSubCommands, ConstantConfigHandle subCommandsConstantConfigHandle)
	{
		R2_CHECK(drawBatchSubCommands != nullptr, "Should never happen");

		const auto subCommandsMemorySize = sizeof(cmd::DrawBatchSubCommand)* drawBatchSubCommands->mSize;

		key::DebugKey preDrawKey;
		preDrawKey.keyValue = 0;

		cmd::FillConstantBuffer* subCommandsCMD = nullptr;

		subCommandsCMD = cmdbkt::AddCommand<r2::draw::key::DebugKey, r2::mem::StackArena, cmd::FillConstantBuffer>(*renderer.mDebugCommandArena, *renderer.mPreDebugCommandBucket, preDrawKey, subCommandsMemorySize);

		char* subCommandsAuxMemory = cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(subCommandsCMD);

		memcpy(mem::utils::PointerAdd(subCommandsAuxMemory, 0), drawBatchSubCommands->mData, subCommandsMemorySize);

		const r2::SArray<r2::draw::ConstantBufferHandle>* constHandles = r2::draw::renderer::GetConstantBufferHandles(renderer);
		auto subCommandsConstantBufferHandle = r2::sarr::At(*constHandles, subCommandsConstantConfigHandle);

		ConstantBufferData* subCommandsConstData = GetConstData(renderer, subCommandsConstantBufferHandle);

		subCommandsCMD->constantBufferHandle = subCommandsConstantBufferHandle;
		subCommandsCMD->data = subCommandsAuxMemory;
		subCommandsCMD->dataSize = subCommandsMemorySize;
		subCommandsCMD->offset = subCommandsConstData->currentOffset;
		subCommandsCMD->type = subCommandsConstData->type;
		subCommandsCMD->isPersistent = subCommandsConstData->isPersistent;

		subCommandsConstData->AddDataSize(subCommandsMemorySize);
	}

	void FillPreDebugCommandBucketVertexData(
		Renderer& renderer,
		r2::SArray<DebugVertex>* debugVertices,
		r2::SArray<cmd::DrawDebugBatchSubCommand>* drawBatchSubCommands,
		vb::VertexBufferLayoutHandle vertexBufferLayoutHandle,
		ConstantConfigHandle subCommandsConstantConfigHandle)
	{
		R2_CHECK(debugVertices != nullptr, "Should never happen");

		key::DebugKey preDrawKey;
		preDrawKey.keyValue = 0;

		cmd::FillVertexBuffer* fillVertexBufferCMD = nullptr;

		const auto subCommandsMemorySize = sizeof(cmd::DrawDebugBatchSubCommand) * drawBatchSubCommands->mSize;
		const auto vertexMemorySize = sizeof(DebugVertex) * debugVertices->mSize;

		VertexBufferHandle vboHandle = vbsys::GetVertexBufferHandle(*renderer.mVertexBufferLayoutSystem, vertexBufferLayoutHandle, 0);

		u64 vOffset = 0;

		fillVertexBufferCMD = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::FillVertexBuffer>(*renderer.mDebugCommandArena, *renderer.mPreDebugCommandBucket, preDrawKey, vertexMemorySize);

		char* vertexAuxMemory = cmdpkt::GetAuxiliaryMemory<cmd::FillVertexBuffer>(fillVertexBufferCMD);

		memcpy(mem::utils::PointerAdd(vertexAuxMemory, 0), debugVertices->mData, vertexMemorySize);

		fillVertexBufferCMD->vertexBufferHandle = vboHandle;
		fillVertexBufferCMD->offset = 0;
		fillVertexBufferCMD->dataSize = vertexMemorySize;
		fillVertexBufferCMD->data = vertexAuxMemory;

		cmd::FillConstantBuffer* subCommandsCMD = subCommandsCMD = cmdbkt::AppendCommand<cmd::FillVertexBuffer, cmd::FillConstantBuffer, mem::StackArena>(*renderer.mDebugCommandArena, fillVertexBufferCMD, subCommandsMemorySize);

		char* subCommandsAuxMemory = cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(subCommandsCMD);

		memcpy(mem::utils::PointerAdd(subCommandsAuxMemory, 0), drawBatchSubCommands->mData, subCommandsMemorySize);

		const r2::SArray<r2::draw::ConstantBufferHandle>* constHandles = r2::draw::renderer::GetConstantBufferHandles(renderer);
		auto subCommandsConstantBufferHandle = r2::sarr::At(*constHandles, subCommandsConstantConfigHandle);

		ConstantBufferData* subCommandsConstData = GetConstData(renderer, subCommandsConstantBufferHandle);

		subCommandsCMD->constantBufferHandle = subCommandsConstantBufferHandle;
		subCommandsCMD->data = subCommandsAuxMemory;
		subCommandsCMD->dataSize = subCommandsMemorySize;
		subCommandsCMD->offset = subCommandsConstData->currentOffset;
		subCommandsCMD->type = subCommandsConstData->type;
		subCommandsCMD->isPersistent = subCommandsConstData->isPersistent;

		subCommandsConstData->AddDataSize(subCommandsMemorySize);
	}

	void FillDebugDrawCommands(Renderer& renderer, r2::SArray<BatchRenderOffsets>* debugBatchOffsets, const BufferLayoutHandle& debugBufferLayoutHandle, ConstantBufferHandle subCommandsBufferHandle, bool debugLines)
	{
		const u64 numDebugModelBatchOffsets = r2::sarr::Size(*debugBatchOffsets);

		for (u64 i = 0; i < numDebugModelBatchOffsets; ++i)
		{
			const auto& batchOffset = r2::sarr::At(*debugBatchOffsets, i);

			bool transparentBatch = batchOffset.shaderEffectPasses.meshPasses[flat::eMeshPass_TRANSPARENT].staticShaderHandle != InvalidShader;

			ShaderHandle shaderToUse = batchOffset.shaderEffectPasses.meshPasses[flat::eMeshPass_FORWARD].staticShaderHandle;
			if (transparentBatch)
			{
				shaderToUse = batchOffset.shaderEffectPasses.meshPasses[flat::eMeshPass_TRANSPARENT].staticShaderHandle;
			}

			key::DebugKey key = key::GenerateDebugKey(DL_DEBUG, batchOffset.drawState.depthEnabled, batchOffset.blendingFunctionKeyValue, batchOffset.primitiveType, shaderToUse);

			if (debugLines)
			{

				cmd::DrawDebugBatch* drawBatch = nullptr;
				if (!transparentBatch)
				{
					drawBatch = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::DrawDebugBatch>(*renderer.mDebugCommandArena, *renderer.mDebugCommandBucket, key, 0);
				}
				else
				{
					key::Basic transparentKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, 0, r2::draw::DrawLayer::DL_DEBUG, r2::draw::key::TR_ADDITIVE, 0, shaderToUse);

					drawBatch = cmdbkt::AddCommand<key::Basic, mem::StackArena, cmd::DrawDebugBatch>(*renderer.mDebugCommandArena, *renderer.mTransparentBucket, transparentKey, 0);
				}

				drawBatch->batchHandle = subCommandsBufferHandle;
				drawBatch->bufferLayoutHandle = debugBufferLayoutHandle;
				drawBatch->numSubCommands = batchOffset.numSubCommands;
				R2_CHECK(drawBatch->numSubCommands > 0, "We should have a count!");
				drawBatch->startCommandIndex = batchOffset.subCommandsOffset;
				drawBatch->primitiveType = batchOffset.primitiveType;
				drawBatch->subCommands = nullptr;
				drawBatch->state.depthEnabled = batchOffset.drawState.depthEnabled;
				drawBatch->state.depthFunction = LESS;
				drawBatch->state.depthWriteEnabled = false;
				drawBatch->state.polygonOffsetEnabled = false;
				drawBatch->state.polygonOffset = glm::vec2(0, 0);
				cmd::SetDefaultCullState(drawBatch->state.cullState);
				cmd::SetDefaultStencilState(drawBatch->state.stencilState);
				cmd::SetDefaultBlendState(drawBatch->state.blendState);
				if (transparentBatch)
				{
					SetDefaultTransparencyBlendState(drawBatch->state.blendState);
				}
			}
			else
			{
				cmd::DrawBatch* drawBatch = nullptr;
				if (!transparentBatch)
				{
					drawBatch = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::DrawBatch>(*renderer.mDebugCommandArena, *renderer.mDebugCommandBucket, key, 0);
				}
				else
				{
					key::Basic transparentKey = key::GenerateBasicKey(key::Basic::FSL_OUTPUT, 0, r2::draw::DrawLayer::DL_DEBUG, r2::draw::key::TR_ADDITIVE, 0, shaderToUse);

					drawBatch = cmdbkt::AddCommand<key::Basic, mem::StackArena, cmd::DrawBatch>(*renderer.mDebugCommandArena, *renderer.mTransparentBucket, transparentKey, 0);
				}

				drawBatch->batchHandle = subCommandsBufferHandle;
				drawBatch->bufferLayoutHandle = debugBufferLayoutHandle;
				drawBatch->numSubCommands = batchOffset.numSubCommands;
				R2_CHECK(drawBatch->numSubCommands > 0, "We should have a count!");
				drawBatch->startCommandIndex = batchOffset.subCommandsOffset;
				drawBatch->primitiveType = batchOffset.primitiveType;
				drawBatch->subCommands = nullptr;
				drawBatch->state.depthEnabled = batchOffset.drawState.depthEnabled;
				drawBatch->state.depthFunction = LESS;
				drawBatch->state.depthWriteEnabled = !transparentBatch;
				drawBatch->state.polygonOffsetEnabled = false;
				drawBatch->state.polygonOffset = glm::vec2(0, 0);
				cmd::SetDefaultCullState(drawBatch->state.cullState);
				cmd::SetDefaultStencilState(drawBatch->state.stencilState);
				cmd::SetDefaultBlendState(drawBatch->state.blendState);
				if (transparentBatch)
				{
					SetDefaultTransparencyBlendState(drawBatch->state.blendState);
				}
			}
		}
	}

	void DebugPreRender(Renderer& renderer)
	{
		const r2::SArray<r2::draw::ConstantBufferHandle>* constHandles = r2::draw::renderer::GetConstantBufferHandles(renderer);

		static const DebugRenderBatch& debugLinesRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_LINES);
		static const DebugRenderBatch& debugModelsGenericRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_MODELS_GENERIC);

		ConstantBufferHandle debugLinesSubCommandsBufferHandle = r2::sarr::At(*constHandles, debugLinesRenderBatch.subCommandsConstantConfigHandle);
		ConstantBufferHandle debugModelsGenericSubCommandsBufferHandle = r2::sarr::At(*constHandles, debugModelsGenericRenderBatch.subCommandsConstantConfigHandle);

		BufferLayoutHandle debugModelBufferLayoutHandle = vbsys::GetBufferLayoutHandle(*renderer.mVertexBufferLayoutSystem, r2::sarr::At(*renderer.mVertexBufferLayoutHandles, VBL_DEBUG_MODEL));
		BufferLayoutHandle debugLinesBufferLayoutHandle = vbsys::GetBufferLayoutHandle(*renderer.mVertexBufferLayoutSystem, r2::sarr::At(*renderer.mVertexBufferLayoutHandles, VBL_DEBUG_LINES));

		const auto numGenericModelsToDraw = r2::sarr::Size(*debugModelsGenericRenderBatch.numInstances);
		const auto numGenericDebugModelTypesToDraw = r2::sarr::Size(*debugModelsGenericRenderBatch.debugModelTypesToDraw);

		R2_CHECK(numGenericModelsToDraw == numGenericDebugModelTypesToDraw, "These should be the same!");

		u32 numGenericModelInstancesToDraw = 0;
		for (u32 i = 0; i < numGenericDebugModelTypesToDraw; ++i)
		{
			numGenericModelInstancesToDraw += static_cast<u32>(r2::sarr::At(*debugModelsGenericRenderBatch.numInstances, i));
		}

		u64 numLinesToDraw = r2::sarr::Size(*debugLinesRenderBatch.vertices) / 2;

		r2::SArray<DebugVertex>* debugVertices = nullptr;
		u64 totalNumLines = numLinesToDraw;
		if (totalNumLines > 0)
		{
			debugVertices = renderer.mDebugVertices;

			r2::sarr::Clear(*debugVertices);

			if(numLinesToDraw > 0)
				r2::sarr::Append(*debugVertices, *debugLinesRenderBatch.vertices);
		}

		const u64 totalObjectsToDraw =  numLinesToDraw  + numGenericModelInstancesToDraw;

		r2::SArray<DebugRenderConstants>* debugRenderConstants = nullptr;
		r2::SHashMap<DebugDrawCommandData*>* genericShaderDrawCommandData = nullptr;

		if (totalObjectsToDraw > 0)
		{
			genericShaderDrawCommandData = MAKE_SHASHMAP(*renderer.mPreRenderStackArena, DebugDrawCommandData*, (numLinesToDraw + numGenericDebugModelTypesToDraw) * r2::SHashMap<DrawCommandData>::LoadFactorMultiplier());
			debugRenderConstants = MAKE_SARRAY(*renderer.mPreRenderStackArena, DebugRenderConstants, totalObjectsToDraw);
		}
		else
		{
			RESET_ARENA(*renderer.mPreRenderStackArena);
			return; //there's nothing to do!
		}

		u32 modelSubCommandOffset = 0;
		u32 lineSubCommandOffset = 0;

		r2::SArray<BatchRenderOffsets>* linesBatchRenderOffsets = renderer.linesBatchRenderOffsets;
		r2::SArray<BatchRenderOffsets>* genericBatchRenderOffsets = renderer.genericModelBatchRenderOffsets;

		r2::SArray<cmd::DrawBatchSubCommand>* modelDrawBatchSubCommands = renderer.modelDrawBatchSubCommands;
		r2::SArray<cmd::DrawDebugBatchSubCommand>* linesDrawBatchSubCommands = renderer.linesDrawBatchSubCommands;

		r2::sarr::Clear(*linesBatchRenderOffsets);

		r2::sarr::Clear(*modelDrawBatchSubCommands);
		r2::sarr::Clear(*linesDrawBatchSubCommands);
		r2::sarr::Clear(*genericBatchRenderOffsets);

		u64 instanceCount = 0;

		if (numGenericModelsToDraw > 0)
		{
			r2::sarr::Append(*debugRenderConstants, *debugModelsGenericRenderBatch.debugRenderConstants);

			CreateGenericDebugSubCommands(renderer, debugModelsGenericRenderBatch, numGenericModelsToDraw, instanceCount, genericShaderDrawCommandData, false);

			instanceCount += numGenericModelInstancesToDraw;

			PopulateGenericDebugRenderBatchesOffsets(genericShaderDrawCommandData, modelSubCommandOffset, false, modelDrawBatchSubCommands, linesDrawBatchSubCommands, genericBatchRenderOffsets);
		}

		if (numLinesToDraw > 0)
		{
			r2::sarr::Append(*debugRenderConstants, *debugLinesRenderBatch.debugRenderConstants);

			CreateGenericDebugSubCommands(renderer, debugLinesRenderBatch, numLinesToDraw, instanceCount, genericShaderDrawCommandData, true);

			instanceCount += numLinesToDraw;

			PopulateGenericDebugRenderBatchesOffsets(genericShaderDrawCommandData, lineSubCommandOffset, true, modelDrawBatchSubCommands, linesDrawBatchSubCommands, linesBatchRenderOffsets);
		}

		key::DebugKey emptyDebugKey;
		emptyDebugKey.keyValue = 0;


		//Fill the pre and post buckets with the DebugRenderConstants data needed
		{
			const u64 renderDebugConstantsMemorySize = (totalObjectsToDraw) * sizeof(DebugRenderConstants);

			cmd::FillConstantBuffer* renderDebugConstantsCMD = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::FillConstantBuffer>(*renderer.mDebugCommandArena, *renderer.mPreDebugCommandBucket, emptyDebugKey, renderDebugConstantsMemorySize);

			char* renderDebugConstantsAuxMemory = cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(renderDebugConstantsCMD);

			memcpy(renderDebugConstantsAuxMemory, debugRenderConstants->mData, renderDebugConstantsMemorySize);

			auto renderDebugConstantsBufferHandle = r2::sarr::At(*constHandles, renderer.mDebugRenderConstantsConfigHandle);

			ConstantBufferData* renderDebugConstantsConstData = GetConstData(renderer, renderDebugConstantsBufferHandle);

			renderDebugConstantsCMD->constantBufferHandle = renderDebugConstantsBufferHandle;
			renderDebugConstantsCMD->data = renderDebugConstantsAuxMemory;
			renderDebugConstantsCMD->dataSize = renderDebugConstantsMemorySize;
			renderDebugConstantsCMD->offset = renderDebugConstantsConstData->currentOffset;
			renderDebugConstantsCMD->type = renderDebugConstantsConstData->type;
			renderDebugConstantsCMD->isPersistent = renderDebugConstantsConstData->isPersistent;

			renderDebugConstantsConstData->AddDataSize(renderDebugConstantsMemorySize);

			cmd::CompleteConstantBuffer* completeRenderConstantsCMD = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::CompleteConstantBuffer>(*renderer.mDebugCommandArena, *renderer.mPostDebugCommandBucket, emptyDebugKey, 0);

			completeRenderConstantsCMD->constantBufferHandle = renderDebugConstantsBufferHandle;
			completeRenderConstantsCMD->count = totalObjectsToDraw;


			if (modelDrawBatchSubCommands && !r2::sarr::IsEmpty(*modelDrawBatchSubCommands))
			{
				FillPreDebugCommandBucketModelData(renderer, modelDrawBatchSubCommands, debugModelsGenericRenderBatch.subCommandsConstantConfigHandle);
			}

			if (linesDrawBatchSubCommands && debugVertices)
			{
				FillPreDebugCommandBucketVertexData(renderer, debugVertices, linesDrawBatchSubCommands, debugLinesRenderBatch.vertexBufferLayoutHandle, debugLinesRenderBatch.subCommandsConstantConfigHandle);
			}

			if (modelSubCommandOffset > 0)
			{
				key::DebugKey postKey;
				postKey.keyValue = 0;
				cmd::CompleteConstantBuffer* completeSubCommandsCMD = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::CompleteConstantBuffer>(*renderer.mDebugCommandArena, *renderer.mPostDebugCommandBucket, postKey, 0);
				completeSubCommandsCMD->constantBufferHandle = debugModelsGenericSubCommandsBufferHandle;
				completeSubCommandsCMD->count = modelSubCommandOffset;
			}

			if (lineSubCommandOffset > 0)
			{
				key::DebugKey postKey;
				postKey.keyValue = 0;
				cmd::CompleteConstantBuffer* completeSubCommandsCMD = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::CompleteConstantBuffer>(*renderer.mDebugCommandArena, *renderer.mPostDebugCommandBucket, postKey, 0);
				completeSubCommandsCMD->constantBufferHandle = debugLinesSubCommandsBufferHandle;
				completeSubCommandsCMD->count = lineSubCommandOffset;
			}
		}

		if ( linesBatchRenderOffsets || genericBatchRenderOffsets)
		{
			key::DebugKey setDebugRenderTargetKey = key::GenerateDebugKey(DL_CLEAR, 0, 0, PrimitiveType::POINTS, 0); 

			cmd::SetRenderTargetMipLevel* setRenderTarget = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::SetRenderTargetMipLevel>(*renderer.mDebugCommandArena, *renderer.mDebugCommandBucket, setDebugRenderTargetKey, 0);

			RenderPass* gbufferRenderPass = GetRenderPass(renderer, RPT_GBUFFER);

			cmd::FillSetRenderTargetMipLevelCommand(renderer.mRenderTargets[RTS_GBUFFER], 0, *setRenderTarget, gbufferRenderPass->config);
		}

		if (genericBatchRenderOffsets)
		{
			FillDebugDrawCommands(renderer, genericBatchRenderOffsets, debugModelBufferLayoutHandle, debugModelsGenericSubCommandsBufferHandle, false);
		}
		
		if (linesBatchRenderOffsets)
		{
			FillDebugDrawCommands(renderer, linesBatchRenderOffsets, debugLinesBufferLayoutHandle, debugLinesSubCommandsBufferHandle, true);
		}

		RESET_ARENA(*renderer.mPreRenderStackArena);
	}

	void ClearDebugRenderData(Renderer& renderer)
	{
		for (u32 i = 0; i < NUM_DEBUG_DRAW_TYPES; ++i)
		{
			DebugRenderBatch& batch = r2::sarr::At(*renderer.mDebugRenderBatches, i);

			r2::sarr::Clear(*batch.debugRenderConstants);
			r2::sarr::Clear(*batch.drawFlags);
			r2::sarr::Clear(*batch.numInstances);
			
			if (batch.debugDrawType == DDT_LINES )
			{ 
				r2::sarr::Clear(*batch.vertices);
			}
			else if (batch.debugDrawType == DDT_MODELS_GENERIC)
			{
				r2::sarr::Clear(*batch.debugModelTypesToDraw);
			}
			else
			{
				R2_CHECK(false, "Unsupported");
			}

			r2::sarr::Clear(*batch.shaderHandles);
			r2::sarr::Clear(*batch.meshPasses);

		}

		cmdbkt::ClearAll(*renderer.mPreDebugCommandBucket);
		cmdbkt::ClearAll(*renderer.mDebugCommandBucket);
		cmdbkt::ClearAll(*renderer.mPostDebugCommandBucket);

		RESET_ARENA(*renderer.mDebugCommandArena);
	}

	void DrawDebugBones(Renderer& renderer, const r2::SArray<DebugBone>& bones, const glm::mat4& modelMatrix, const glm::vec4& color)
	{
		//@TODO(Serge): this is kind of dumb at the moment - we should find a better way to batch things
		r2::SArray<u64>* numBonesPerModel = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, 1);
		r2::SArray<glm::mat4>* modelMats = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, 1);

		r2::sarr::Push(*numBonesPerModel, r2::sarr::Size(bones));
		r2::sarr::Push(*modelMats, modelMatrix);

		DrawDebugBones(renderer, bones, *numBonesPerModel, *modelMats, color);

		FREE(modelMats, *MEM_ENG_SCRATCH_PTR);
		FREE(numBonesPerModel, *MEM_ENG_SCRATCH_PTR);
	}
	
	void DrawDebugBones(Renderer& renderer,
		const r2::SArray<DebugBone>& bones,
		const r2::SArray<u64>& numBonesPerModel,
		const r2::SArray<glm::mat4>& modelMats,
		const glm::vec4& color)
	{
		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		if (!renderer.mConstantBufferData)
		{
			R2_CHECK(false, "We haven't generated any constant buffers!");
			return;
		}

		if (renderer.mDebugLinesVertexConfigHandle == vb::InvalidVertexBufferLayoutHandle)
		{
			R2_CHECK(false, "We haven't setup a debug vertex configuration!");
			return;
		}

		R2_CHECK(r2::sarr::Size(modelMats) == r2::sarr::Size(numBonesPerModel), "These should be the same");

		const u32 numAnimModels = r2::sarr::Size(modelMats);

		u64 boneOffset = 0;
		for (u32 i = 0; i < numAnimModels; ++i)
		{
			const u64 numBonesForModel = r2::sarr::At(numBonesPerModel, i);
			const glm::mat4& modelMat = r2::sarr::At(modelMats, i);

			for (u64 j = 0; j < numBonesForModel; ++j)
			{
				const DebugBone& bone = r2::sarr::At(bones, j + boneOffset);
				DrawLine(renderer, bone.p0, bone.p1, modelMat, color, false);
			}

			boneOffset += numBonesForModel;
		}
	}

	void DrawQuad(Renderer& renderer, const glm::vec3& center, glm::vec2 scale, const glm::vec3& normal, const glm::vec4& color, bool filled, bool depthTest)
	{
		r2::SArray<glm::vec3>* centers = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 1);
		r2::SArray<glm::vec2>* scales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec2, 1);
		r2::SArray<glm::vec3>* normals = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 1);
		r2::SArray<glm::vec4>* colors = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec4, 1);

		r2::sarr::Push(*centers, center);
		r2::sarr::Push(*scales, scale);
		r2::sarr::Push(*normals, normal);
		r2::sarr::Push(*colors, color);

		DrawQuadInstanced(*centers, *scales, *normals, *colors, filled, depthTest);

		FREE(colors, *MEM_ENG_SCRATCH_PTR);
		FREE(normals, *MEM_ENG_SCRATCH_PTR);
		FREE(scales, *MEM_ENG_SCRATCH_PTR);
		FREE(centers, *MEM_ENG_SCRATCH_PTR);
	}

	void DrawSphere(Renderer& renderer, const glm::vec3& center, float radius, const glm::vec4& color, bool filled, bool depthTest)
	{
		r2::SArray<glm::vec3>* centers = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 1);
		r2::SArray<float>* radii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, 1);
		r2::SArray<glm::vec4>* colors = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec4, 1);

		r2::sarr::Push(*centers, center);
		r2::sarr::Push(*radii, radius);
		r2::sarr::Push(*colors, color);

		DrawSphereInstanced(renderer, *centers, *radii, *colors, filled, depthTest);

		FREE(colors, *MEM_ENG_SCRATCH_PTR);
		FREE(radii, *MEM_ENG_SCRATCH_PTR);
		FREE(centers, *MEM_ENG_SCRATCH_PTR);
	}

	void DrawCube(Renderer& renderer, const glm::vec3& center, float scale, const glm::vec4& color, bool filled, bool depthTest)
	{
		r2::SArray<glm::vec3>* centers = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 1);
		r2::SArray<float>* scales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, 1);
		r2::SArray<glm::vec4>* colors = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec4, 1);
		
		r2::sarr::Push(*centers, center);
		r2::sarr::Push(*scales, scale);
		r2::sarr::Push(*colors, color);

		DrawCubeInstanced(*centers, *scales, *colors, filled, depthTest);

		FREE(colors, *MEM_ENG_SCRATCH_PTR);
		FREE(scales, *MEM_ENG_SCRATCH_PTR);
		FREE(centers, *MEM_ENG_SCRATCH_PTR);
	}

	void DrawCylinder(Renderer& renderer, const glm::vec3& basePosition, const glm::vec3& dir, float radius, float height, const glm::vec4& color, bool filled, bool depthTest)
	{

		r2::SArray<glm::vec3>* basePositions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 1);
		r2::SArray<glm::vec3>* directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 1);
		r2::SArray<float>* radii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, 1);
		r2::SArray<float>* heights = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, 1);
		r2::SArray<glm::vec4>* colors = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec4, 1);

		r2::sarr::Push(*basePositions, basePosition);
		r2::sarr::Push(*directions, dir);
		r2::sarr::Push(*radii, radius);
		r2::sarr::Push(*heights, height);
		r2::sarr::Push(*colors, color);

		DrawCylinderInstanced(*basePositions, *directions, *radii, *heights, *colors, filled, depthTest);

		FREE(colors, *MEM_ENG_SCRATCH_PTR);
		FREE(heights, *MEM_ENG_SCRATCH_PTR);
		FREE(radii, *MEM_ENG_SCRATCH_PTR);
		FREE(directions, *MEM_ENG_SCRATCH_PTR);
		FREE(basePositions, *MEM_ENG_SCRATCH_PTR);
	}

	void DrawCone(Renderer& renderer, const glm::vec3& basePosition, const glm::vec3& dir, float radius, float height, const glm::vec4& color, bool filled, bool depthTest)
	{
		r2::SArray<glm::vec3>* basePositions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 1);
		r2::SArray<glm::vec3>* directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 1);
		r2::SArray<float>* radii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, 1);
		r2::SArray<float>* heights = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, 1);
		r2::SArray<glm::vec4>* colors = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec4, 1);

		r2::sarr::Push(*basePositions, basePosition);
		r2::sarr::Push(*directions, dir);
		r2::sarr::Push(*radii, radius);
		r2::sarr::Push(*heights, height);
		r2::sarr::Push(*colors, color);

		DrawConeInstanced(*basePositions, *directions, *radii, *heights, *colors, filled, depthTest);

		FREE(colors, *MEM_ENG_SCRATCH_PTR);
		FREE(heights, *MEM_ENG_SCRATCH_PTR);
		FREE(radii, *MEM_ENG_SCRATCH_PTR);
		FREE(directions, *MEM_ENG_SCRATCH_PTR);
		FREE(basePositions, *MEM_ENG_SCRATCH_PTR);
	}

	void DrawArrow(Renderer& renderer, const glm::vec3& basePosition, const glm::vec3& dir, float length, float headBaseRadius, const glm::vec4& color, bool filled, bool depthTest)
	{
		constexpr float ARROW_CONE_HEIGHT_FRACTION = 0.2;
		constexpr float ARROW_BASE_RADIUS_FRACTION = 0.2;

		float baseLength = (length - ARROW_CONE_HEIGHT_FRACTION);

		glm::vec3 ndir = glm::normalize(dir);
		glm::vec3 coneBasePos = ndir * baseLength + basePosition;
		float coneHeight = ARROW_CONE_HEIGHT_FRACTION;
		float baseRadius = ARROW_BASE_RADIUS_FRACTION * coneHeight/2.0f;

		DrawCylinder(renderer, basePosition, dir, baseRadius, baseLength, color, filled, depthTest);
		DrawCone(renderer, coneBasePos, dir, coneHeight/2.0f, coneHeight, color, filled, depthTest);
	}

	void DrawLine(Renderer& renderer, const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, bool depthTest)
	{
		DrawLine(renderer, p0, p1, glm::mat4(1.0f), color, depthTest);
	}

	void DrawLine(Renderer& renderer, const glm::vec3& p0, const glm::vec3& p1, const glm::mat4& modelMat, const glm::vec4& color, bool depthTest)
	{
		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		static DebugRenderBatch& debugLinesRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_LINES);
	//	static DebugRenderBatch& debugTransparentLinesRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_LINES_TRANSPARENT);
		
		DebugRenderBatch* debugRenderBatchToUse = &debugLinesRenderBatch;

		R2_CHECK(debugRenderBatchToUse != nullptr, "This should never happen");

		R2_CHECK(debugRenderBatchToUse->vertices != nullptr, "We haven't properly initialized the debug render batches!");

		r2::draw::DebugVertex v1{ p0 };
		r2::draw::DebugVertex v2{ p1 };

		DrawFlags flags;
		depthTest ? flags.Set(eDrawFlags::DEPTH_TEST) : flags.Remove(eDrawFlags::DEPTH_TEST);

		DebugRenderConstants constants;
		constants.color = color;
		constants.modelMatrix = modelMat;

		r2::sarr::Push(*debugRenderBatchToUse->debugRenderConstants, constants);
		r2::sarr::Push(*debugRenderBatchToUse->drawFlags, flags);
		r2::sarr::Push(*debugRenderBatchToUse->vertices, v1);
		r2::sarr::Push(*debugRenderBatchToUse->vertices, v2);
		r2::sarr::Push(*debugRenderBatchToUse->numInstances, 1u);
		r2::sarr::Push(*debugRenderBatchToUse->shaderHandles, color.a < 1.0f ? renderer.mDebugTransparentLineShaderHandle : renderer.mDebugLinesShaderHandle);
		r2::sarr::Push(*debugRenderBatchToUse->meshPasses, color.a < 1.0f ? flat::eMeshPass_TRANSPARENT : flat::eMeshPass_FORWARD);
	}

	void DrawTangentVectors(Renderer& renderer, DefaultModel defaultModel, const glm::mat4& transform)
	{
		const Model* model = GetDefaultModel(renderer, defaultModel);

		const u64 numMeshes = r2::sarr::Size(*model->optrMeshes);

		for (u64 i = 0; i < numMeshes; ++i)
		{
			const Mesh* mesh = r2::sarr::At(*model->optrMeshes, i);

			const u64 numVertices = r2::sarr::Size(*mesh->optrVertices);

			for (u64 v = 0; v < numVertices; ++v)
			{
				const draw::Vertex& vertex = r2::sarr::At(*mesh->optrVertices, v);

				//@TODO(Serge): all this matrix multiply is slow....
				glm::vec3 initialPosition = glm::vec3(transform * glm::vec4(vertex.position, 1));

				glm::vec3 normal = glm::normalize(glm::vec3(transform * glm::vec4(vertex.normal, 0)));
				glm::vec3 tangent = glm::normalize(glm::vec3(transform * glm::vec4(vertex.tangent, 0)));
				
				tangent = glm::normalize(tangent - dot(normal, tangent) * normal);
				glm::vec3 bitangent = glm::cross(normal, tangent);

				glm::vec3 offset = (normal * 0.015f);
				initialPosition += offset;

				DrawLine(renderer, initialPosition, initialPosition + normal * 0.3f, glm::vec4(0, 0, 1, 1), true);
				DrawLine(renderer, initialPosition, initialPosition + tangent * 0.3f, glm::vec4(1, 0, 0, 1), true);
				DrawLine(renderer, initialPosition, initialPosition + bitangent * 0.3f, glm::vec4(0, 1, 0, 1), true);
			}

		}
	}

	void DrawQuad(Renderer& renderer, const glm::mat4& mat, flat::eMeshPass meshPass, ShaderHandle shader, bool depthTest)
	{
		static DebugRenderBatch& genericDebugRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_MODELS_GENERIC);

		R2_CHECK(genericDebugRenderBatch.debugModelTypesToDraw != nullptr, "We haven't properly initialized the debug render batches!");

		DrawFlags flags;
		flags.Set(eDrawFlags::FILL_MODEL);
		depthTest ? flags.Set(eDrawFlags::DEPTH_TEST) : flags.Remove(eDrawFlags::DEPTH_TEST);

		DebugRenderConstants constants;
		constants.color = glm::vec4(1);
		constants.modelMatrix = mat;

		r2::sarr::Push(*genericDebugRenderBatch.debugRenderConstants, constants);

		r2::sarr::Push(*genericDebugRenderBatch.drawFlags, flags);
		r2::sarr::Push(*genericDebugRenderBatch.debugModelTypesToDraw, DEBUG_QUAD);
		r2::sarr::Push(*genericDebugRenderBatch.numInstances, static_cast<u32>(1));

		r2::sarr::Push(*genericDebugRenderBatch.shaderHandles, shader);
		r2::sarr::Push(*genericDebugRenderBatch.meshPasses, meshPass);
	}

	void DrawQuadInstanced(
		Renderer& renderer,
		const r2::SArray<glm::vec3>& centers,
		const r2::SArray<glm::vec2>& scales,
		const r2::SArray<glm::vec3>& normals,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest)
	{
		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		static DebugRenderBatch& genericDebugRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_MODELS_GENERIC);

		R2_CHECK(genericDebugRenderBatch.debugModelTypesToDraw != nullptr, "We haven't properly initialized the debug render batches!");


		R2_CHECK(r2::sarr::Size(centers) == r2::sarr::Size(scales) && r2::sarr::Size(centers) == r2::sarr::Size(colors) && r2::sarr::Size(centers) == r2::sarr::Size(normals),
			"There should be the same amount of elements in all arrays");

		const auto numCenters = r2::sarr::Size(centers);
		u32 numOpaqueInstances = 0;
		u32 numTransparentInstances = 0;

		glm::vec3 initialFacing = glm::vec3(0, 0, 1);

		for (u32 i = 0; i < numCenters; i++)
		{
			glm::vec3 position = r2::sarr::At(centers, i);
			glm::vec2 scale = r2::sarr::At(scales, i);
			glm::vec3 normal = glm::normalize(r2::sarr::At(normals, i));
			glm::vec4 color = r2::sarr::At(colors, i);

			DebugRenderBatch* debugRenderBatchToUse = &genericDebugRenderBatch;

			if (color.a < 1.0f)
			{
				numTransparentInstances++;
			}
			else
			{
				numOpaqueInstances++;
			}

			math::Transform t;
			t.position = position;
			t.scale = glm::vec3(scale.x, scale.y, 1.0f);

			const auto angleBetween = glm::angle(normal,initialFacing);
			
			glm::vec3 axis = glm::cross(glm::vec3(0, 0, 1), normal);

			if (math::NearEq(glm::abs(glm::dot(normal, initialFacing)), 1.0f))
			{
				axis = glm::vec3(1, 0, 0);
			}

			t.rotation = glm::angleAxis(angleBetween, axis);

			DebugRenderConstants constants;
			constants.color = color;
			constants.modelMatrix = r2::math::ToMatrix(t);
			r2::sarr::Push(*debugRenderBatchToUse->debugRenderConstants, constants);

			//r2::sarr::Push(*debugRenderBatchToUse->transforms, t);
			//r2::sarr::Push(*debugRenderBatchToUse->colors, color);
		}

		DrawFlags flags;
		filled ? flags.Set(eDrawFlags::FILL_MODEL) : flags.Remove(eDrawFlags::FILL_MODEL);
		depthTest ? flags.Set(eDrawFlags::DEPTH_TEST) : flags.Remove(eDrawFlags::DEPTH_TEST);

		if (numOpaqueInstances > 0)
		{
			r2::sarr::Push(*genericDebugRenderBatch.drawFlags, flags);
			r2::sarr::Push(*genericDebugRenderBatch.debugModelTypesToDraw, DEBUG_QUAD);
			r2::sarr::Push(*genericDebugRenderBatch.numInstances, static_cast<u32>(numOpaqueInstances));

			r2::sarr::Push(*genericDebugRenderBatch.shaderHandles, renderer.mDebugModelShaderHandle);
			r2::sarr::Push(*genericDebugRenderBatch.meshPasses, flat::eMeshPass::eMeshPass_FORWARD);
		}

		if(numTransparentInstances > 0)
		{
			r2::sarr::Push(*genericDebugRenderBatch.drawFlags, flags);
			r2::sarr::Push(*genericDebugRenderBatch.debugModelTypesToDraw, DEBUG_QUAD);
			r2::sarr::Push(*genericDebugRenderBatch.numInstances, static_cast<u32>(numTransparentInstances));

			r2::sarr::Push(*genericDebugRenderBatch.shaderHandles, renderer.mDebugTransparentModelShaderHandle);
			r2::sarr::Push(*genericDebugRenderBatch.meshPasses, flat::eMeshPass::eMeshPass_TRANSPARENT);
		}
	}

	void DrawSphereInstanced(
		Renderer& renderer,
		const r2::SArray<glm::vec3>& centers,
		const r2::SArray<float>& radii,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest)
	{
		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		static DebugRenderBatch& genericDebugRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_MODELS_GENERIC);

		R2_CHECK(genericDebugRenderBatch.debugModelTypesToDraw != nullptr, "We haven't properly initialized the debug render batches!");
		
		R2_CHECK(r2::sarr::Size(centers) == r2::sarr::Size(radii) && r2::sarr::Size(centers) == r2::sarr::Size(colors),
			"There should be the same amount of elements in all arrays");

		const auto numCenters = r2::sarr::Size(centers);
		u32 numOpaqueInstances = 0;
		u32 numTransparentInstances = 0;

		for (u32 i = 0; i < numCenters; i++)
		{
			const glm::vec3& center = r2::sarr::At(centers, i);
			float radius = r2::sarr::At(radii, i);
			const glm::vec4& color = r2::sarr::At(colors, i);

			DebugRenderBatch* debugRenderBatchToUse = &genericDebugRenderBatch;

			if (color.a < 1.0f)
			{
				numTransparentInstances++;
			}
			else
			{
				numOpaqueInstances++;
			}

			math::Transform t;
			t.position = center;
			t.scale = glm::vec3(radius);

			DebugRenderConstants constants;
			constants.color = color;
			constants.modelMatrix = math::ToMatrix(t);
			r2::sarr::Push(*debugRenderBatchToUse->debugRenderConstants, constants);

//			r2::sarr::Push(*debugRenderBatchToUse->transforms, t);
	//		r2::sarr::Push(*debugRenderBatchToUse->colors, color);
		}

		DrawFlags flags;
		filled ? flags.Set(eDrawFlags::FILL_MODEL) : flags.Remove(eDrawFlags::FILL_MODEL);
		depthTest ? flags.Set(eDrawFlags::DEPTH_TEST) : flags.Remove(eDrawFlags::DEPTH_TEST);

		if (numOpaqueInstances > 0)
		{
			r2::sarr::Push(*genericDebugRenderBatch.drawFlags, flags);
			r2::sarr::Push(*genericDebugRenderBatch.debugModelTypesToDraw, DEBUG_SPHERE);
			r2::sarr::Push(*genericDebugRenderBatch.numInstances, static_cast<u32>(numOpaqueInstances));

			r2::sarr::Push(*genericDebugRenderBatch.shaderHandles, renderer.mDebugModelShaderHandle);
			r2::sarr::Push(*genericDebugRenderBatch.meshPasses, flat::eMeshPass::eMeshPass_FORWARD);
		}
		
		if (numTransparentInstances > 0)
		{
			r2::sarr::Push(*genericDebugRenderBatch.drawFlags, flags);
			r2::sarr::Push(*genericDebugRenderBatch.debugModelTypesToDraw, DEBUG_SPHERE);
			r2::sarr::Push(*genericDebugRenderBatch.numInstances, static_cast<u32>(numTransparentInstances));

			r2::sarr::Push(*genericDebugRenderBatch.shaderHandles, renderer.mDebugTransparentModelShaderHandle);
			r2::sarr::Push(*genericDebugRenderBatch.meshPasses, flat::eMeshPass::eMeshPass_TRANSPARENT);
		}
	}

	void DrawCubeInstanced(
		Renderer& renderer,
		const r2::SArray<glm::vec3>& centers,
		const r2::SArray<float>& scales,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest)
	{
		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		static DebugRenderBatch& genericDebugRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_MODELS_GENERIC);

		R2_CHECK(genericDebugRenderBatch.debugModelTypesToDraw != nullptr, "We haven't properly initialized the debug render batches!");

		R2_CHECK(r2::sarr::Size(centers) == r2::sarr::Size(scales) && r2::sarr::Size(centers) == r2::sarr::Size(colors),
			"There should be the same amount of elements in all arrays");

		const auto numCenters = r2::sarr::Size(centers);
		u32 numOpaqueInstances = 0;
		u32 numTransparentInstances = 0;

		for (u64 i = 0; i < numCenters; i++)
		{
			const glm::vec3& center = r2::sarr::At(centers, i);
			float scale = r2::sarr::At(scales, i);
			const glm::vec4& color = r2::sarr::At(colors, i);

			DebugRenderBatch* debugRenderBatchToUse = &genericDebugRenderBatch;

			if (color.a < 1.0f)
			{
				numTransparentInstances++;
			}
			else
			{
				numOpaqueInstances++;
			}

			math::Transform t;
			t.position = center;
			t.scale = glm::vec3(scale);

			DebugRenderConstants constants;
			constants.color = color;
			constants.modelMatrix = math::ToMatrix(t);
			r2::sarr::Push(*debugRenderBatchToUse->debugRenderConstants, constants);

			//r2::sarr::Push(*debugRenderBatchToUse->transforms, t);
			//r2::sarr::Push(*debugRenderBatchToUse->colors, color);
		}

		DrawFlags flags;
		filled ? flags.Set(eDrawFlags::FILL_MODEL) : flags.Remove(eDrawFlags::FILL_MODEL);
		depthTest ? flags.Set(eDrawFlags::DEPTH_TEST) : flags.Remove(eDrawFlags::DEPTH_TEST);

		if (numOpaqueInstances > 0)
		{
			r2::sarr::Push(*genericDebugRenderBatch.drawFlags, flags);
			r2::sarr::Push(*genericDebugRenderBatch.debugModelTypesToDraw, DEBUG_CUBE);
			r2::sarr::Push(*genericDebugRenderBatch.numInstances, static_cast<u32>(numOpaqueInstances));

			r2::sarr::Push(*genericDebugRenderBatch.shaderHandles, renderer.mDebugModelShaderHandle);
			r2::sarr::Push(*genericDebugRenderBatch.meshPasses, flat::eMeshPass::eMeshPass_FORWARD);
		}
		
		if (numTransparentInstances)
		{
			r2::sarr::Push(*genericDebugRenderBatch.drawFlags, flags);
			r2::sarr::Push(*genericDebugRenderBatch.debugModelTypesToDraw, DEBUG_CUBE);
			r2::sarr::Push(*genericDebugRenderBatch.numInstances, static_cast<u32>(numTransparentInstances));

			r2::sarr::Push(*genericDebugRenderBatch.shaderHandles, renderer.mDebugTransparentModelShaderHandle);
			r2::sarr::Push(*genericDebugRenderBatch.meshPasses, flat::eMeshPass::eMeshPass_TRANSPARENT);
		}

	}

	void DrawCylinderInstanced(
		Renderer& renderer,
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& radii,
		const r2::SArray<float>& heights,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest)
	{
		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		static DebugRenderBatch& genericDebugRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_MODELS_GENERIC);

		R2_CHECK(genericDebugRenderBatch.debugModelTypesToDraw != nullptr, "We haven't properly initialized the debug render batches!");

		DrawFlags flags;
		filled ? flags.Set(eDrawFlags::FILL_MODEL) : flags.Remove(eDrawFlags::FILL_MODEL);
		depthTest ? flags.Set(eDrawFlags::DEPTH_TEST) : flags.Remove(eDrawFlags::DEPTH_TEST);

		R2_CHECK(
			r2::sarr::Size(basePositions) == r2::sarr::Size(directions) &&
			r2::sarr::Size(basePositions) == r2::sarr::Size(radii) &&
			r2::sarr::Size(basePositions) == r2::sarr::Size(heights) &&
			r2::sarr::Size(basePositions) == r2::sarr::Size(colors),
			"There should be the same amount of elements in all arrays");

		const auto numBasePositions = r2::sarr::Size(basePositions);
		u32 numOpaqueInstances = 0;
		u32 numTransparentInstances = 0;

		for (u64 i = 0; i < numBasePositions; i++)
		{
			const glm::vec3& basePosition = r2::sarr::At(basePositions, i);
			float radius = r2::sarr::At(radii, i);
			const glm::vec3& dir = r2::sarr::At(directions, i);
			float height = r2::sarr::At(heights, i);
			const glm::vec4& color = r2::sarr::At(colors, i);

			DebugRenderBatch* debugRenderBatchToUse = &genericDebugRenderBatch;

			if (color.a < 1.0f)
			{
				numTransparentInstances++;
			}
			else
			{
				numOpaqueInstances++;
			}

			math::Transform t;

			glm::vec3 ndir = glm::normalize(dir);
			glm::vec3 initialFacing = glm::vec3(0, 0, 1);

			glm::vec3 axis = glm::normalize(glm::cross(initialFacing, ndir));

			if (math::NearEq(glm::abs(glm::dot(ndir, initialFacing)), 1.0f))
			{
				axis = glm::vec3(1, 0, 0);
			}

			t.position = initialFacing * 0.5f * height;

			float angle = glm::acos(glm::dot(ndir, initialFacing));

			math::Transform r;
			r.rotation = glm::normalize(glm::rotate(r.rotation, angle, axis));

			math::Transform s;

			s.scale = glm::vec3(radius, radius, height);

			math::Transform t2;
			t2.position = -t.position;

			math::Transform transformToDraw = math::Combine(math::Combine(r, t), s);
			transformToDraw.position += basePosition;

			DebugRenderConstants constants;
			constants.color = color;
			constants.modelMatrix = math::ToMatrix(transformToDraw);

			r2::sarr::Push(*debugRenderBatchToUse->debugRenderConstants, constants);

	//		r2::sarr::Push(*debugRenderBatchToUse->transforms, transformToDraw);
		//	r2::sarr::Push(*debugRenderBatchToUse->colors, color);
		}

		if (numOpaqueInstances > 0)
		{
			r2::sarr::Push(*genericDebugRenderBatch.drawFlags, flags);
			r2::sarr::Push(*genericDebugRenderBatch.debugModelTypesToDraw, DEBUG_CYLINDER);
			r2::sarr::Push(*genericDebugRenderBatch.numInstances, static_cast<u32>(numOpaqueInstances));
			r2::sarr::Push(*genericDebugRenderBatch.shaderHandles, renderer.mDebugModelShaderHandle);
			r2::sarr::Push(*genericDebugRenderBatch.meshPasses, flat::eMeshPass::eMeshPass_FORWARD);

		}

		if (numTransparentInstances > 0)
		{
			r2::sarr::Push(*genericDebugRenderBatch.drawFlags, flags);
			r2::sarr::Push(*genericDebugRenderBatch.debugModelTypesToDraw, DEBUG_CYLINDER);
			r2::sarr::Push(*genericDebugRenderBatch.numInstances, static_cast<u32>(numTransparentInstances));

			r2::sarr::Push(*genericDebugRenderBatch.shaderHandles, renderer.mDebugTransparentLineShaderHandle);
			r2::sarr::Push(*genericDebugRenderBatch.meshPasses, flat::eMeshPass::eMeshPass_TRANSPARENT);
		}
	}

	void DrawConeInstanced(
		Renderer& renderer,
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& radii,
		const r2::SArray<float>& heights,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest)
	{
		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		static DebugRenderBatch& genericDebugRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_MODELS_GENERIC);

		R2_CHECK(genericDebugRenderBatch.debugModelTypesToDraw != nullptr, "We haven't properly initialized the debug render batches!");
		

		DrawFlags flags;
		filled ? flags.Set(eDrawFlags::FILL_MODEL) : flags.Remove(eDrawFlags::FILL_MODEL);
		depthTest ? flags.Set(eDrawFlags::DEPTH_TEST) : flags.Remove(eDrawFlags::DEPTH_TEST);

		R2_CHECK(
			r2::sarr::Size(basePositions) == r2::sarr::Size(directions) &&
			r2::sarr::Size(basePositions) == r2::sarr::Size(radii) &&
			r2::sarr::Size(basePositions) == r2::sarr::Size(heights) &&
			r2::sarr::Size(basePositions) == r2::sarr::Size(colors),
			"There should be the same amount of elements in all arrays");

		const auto numBasePositions = r2::sarr::Size(basePositions);
		u32 numOpaqueInstances = 0;
		u32 numTransparentInstances = 0;


		for (u64 i = 0; i < numBasePositions; i++)
		{
			const glm::vec3& basePosition = r2::sarr::At(basePositions, i);
			float radius = r2::sarr::At(radii, i);
			const glm::vec3& dir = r2::sarr::At(directions, i);
			float height = r2::sarr::At(heights, i);
			const glm::vec4& color = r2::sarr::At(colors, i);
			
			DebugRenderBatch* debugRenderBatchToUse = &genericDebugRenderBatch;

			if (color.a < 1.0f)
			{
				
				numTransparentInstances++;
			}
			else
			{

				numOpaqueInstances++;
			}

			math::Transform t;

			glm::vec3 ndir = glm::normalize(dir);
			glm::vec3 initialFacing = glm::vec3(0, 0, 1);

			glm::vec3 axis = glm::normalize(glm::cross(initialFacing, ndir));

			if (math::NearEq(glm::abs(glm::dot(ndir, initialFacing)), 1.0f))
			{
				axis = glm::vec3(1, 0, 0);
			}

			t.position = initialFacing * 0.5f * height;

			float angle = glm::acos(glm::dot(ndir, initialFacing));

			math::Transform r;
			r.rotation = glm::normalize(glm::rotate(r.rotation, angle, axis));

			math::Transform s;

			s.scale = glm::vec3(radius, radius, height);

			math::Transform t2;
			t2.position = -t.position;

			math::Transform transformToDraw = math::Combine(math::Combine(r, t), s);
			transformToDraw.position += basePosition;

			DebugRenderConstants constants;
			constants.color = color;
			constants.modelMatrix = math::ToMatrix(transformToDraw);

			r2::sarr::Push(*debugRenderBatchToUse->debugRenderConstants, constants);

		//	r2::sarr::Push(*debugRenderBatchToUse->transforms, transformToDraw);
		//	r2::sarr::Push(*debugRenderBatchToUse->colors, color);
		}

		if (numOpaqueInstances > 0)
		{
			r2::sarr::Push(*genericDebugRenderBatch.drawFlags, flags);
			r2::sarr::Push(*genericDebugRenderBatch.debugModelTypesToDraw, DEBUG_CONE);
			r2::sarr::Push(*genericDebugRenderBatch.numInstances, static_cast<u32>(numOpaqueInstances));

			r2::sarr::Push(*genericDebugRenderBatch.shaderHandles, renderer.mDebugModelShaderHandle);
			r2::sarr::Push(*genericDebugRenderBatch.meshPasses, flat::eMeshPass::eMeshPass_FORWARD);
		}

		if (numTransparentInstances > 0)
		{
			r2::sarr::Push(*genericDebugRenderBatch.drawFlags, flags);
			r2::sarr::Push(*genericDebugRenderBatch.debugModelTypesToDraw, DEBUG_CONE);
			r2::sarr::Push(*genericDebugRenderBatch.numInstances, static_cast<u32>(numTransparentInstances));

			r2::sarr::Push(*genericDebugRenderBatch.shaderHandles, renderer.mDebugTransparentModelShaderHandle);
			r2::sarr::Push(*genericDebugRenderBatch.meshPasses, flat::eMeshPass::eMeshPass_TRANSPARENT);
		}
	}

	void DrawArrowInstanced(
		Renderer& renderer,
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& lengths,
		const r2::SArray<float>& headBaseRadii,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest)
	{
		static constexpr float ARROW_CONE_HEIGHT_FRACTION = 0.2;
		static constexpr float ARROW_BASE_RADIUS_FRACTION = 0.2;
		
		if (renderer.mVertexBufferLayoutSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		R2_CHECK(
			r2::sarr::Size(basePositions) == r2::sarr::Size(directions) &&
			r2::sarr::Size(basePositions) == r2::sarr::Size(lengths) &&
			r2::sarr::Size(basePositions) == r2::sarr::Size(headBaseRadii) &&
			r2::sarr::Size(basePositions) == r2::sarr::Size(colors),
			"There should be the same amount of elements in all arrays");

		const auto numInstances = r2::sarr::Size(basePositions);

		r2::SArray<glm::vec3>* coneBasePositions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances);
		r2::SArray<float>* baseRadii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);
		r2::SArray<float>* baseLengths = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);
		r2::SArray<float>* coneHeights = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);
		r2::SArray<float>* newConeRadii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);

		for (u64 i = 0; i < numInstances; i++)
		{
			float length = r2::sarr::At(lengths, i);
			const glm::vec3& dir = r2::sarr::At(directions, i);
			const glm::vec3& basePosition = r2::sarr::At(basePositions, i);
			float headBaseRadius = r2::sarr::At(headBaseRadii, i);

			float baseLength = length - ARROW_CONE_HEIGHT_FRACTION;

			glm::vec3 ndir = glm::normalize(dir);
			glm::vec3 coneBasePos = ndir * baseLength + basePosition;
			float coneHeight = ARROW_CONE_HEIGHT_FRACTION;
			float baseRadius = ARROW_BASE_RADIUS_FRACTION * coneHeight/2.0f;

			r2::sarr::Push(*coneBasePositions, coneBasePos);
			r2::sarr::Push(*baseRadii, baseRadius);
			r2::sarr::Push(*coneHeights, coneHeight);
			r2::sarr::Push(*baseLengths, baseLength);
			r2::sarr::Push(*newConeRadii, coneHeight / 2.0f);
		}

		DrawConeInstanced(renderer, *coneBasePositions, directions, *newConeRadii, *coneHeights, colors, filled, depthTest);
		DrawCylinderInstanced(renderer, basePositions, directions, *baseRadii, *baseLengths, colors, filled, depthTest);
		
		FREE(newConeRadii, *MEM_ENG_SCRATCH_PTR);
		FREE(coneHeights, *MEM_ENG_SCRATCH_PTR);
		FREE(baseLengths, *MEM_ENG_SCRATCH_PTR);
		FREE(baseRadii, *MEM_ENG_SCRATCH_PTR);
		FREE(coneBasePositions, *MEM_ENG_SCRATCH_PTR);
	}

#endif //  R2_DEBUG

	const vb::GPUModelRef* GetGPUModelRef(const Renderer& renderer, const vb::GPUModelRefHandle& handle)
	{
		return vbsys::GetGPUModelRef(*renderer.mVertexBufferLayoutSystem, handle);
	}

	vb::VertexBufferLayoutSize GetStaticVertexBufferRemainingSize(const Renderer& renderer)
	{
		return vbsys::GetVertexBufferRemainingSize(*renderer.mVertexBufferLayoutSystem, renderer.mStaticVertexModelConfigHandle);
	}
	
	vb::VertexBufferLayoutSize GetAnimVertexBufferRemainingSize(const Renderer& renderer)
	{
		return vbsys::GetVertexBufferRemainingSize(*renderer.mVertexBufferLayoutSystem, renderer.mAnimVertexModelConfigHandle);
	}

	vb::VertexBufferLayoutSize GetStaticVertexBufferSize(const Renderer& renderer)
	{
		return vbsys::GetVertexBufferSize(*renderer.mVertexBufferLayoutSystem, renderer.mStaticVertexModelConfigHandle);
	}

	vb::VertexBufferLayoutSize GetAnimVertexBufferSize(const Renderer& renderer)
	{
		return vbsys::GetVertexBufferSize(*renderer.mVertexBufferLayoutSystem, renderer.mAnimVertexModelConfigHandle);
	}

	vb::VertexBufferLayoutSize GetStaticVertexBufferCapacity(const Renderer& renderer)
	{
		return vbsys::GetVertexBufferCapacity(*renderer.mVertexBufferLayoutSystem, renderer.mStaticVertexModelConfigHandle);
	}

	vb::VertexBufferLayoutSize GetAnimVertexBufferCapacity(const Renderer& renderer)
	{
		return vbsys::GetVertexBufferCapacity(*renderer.mVertexBufferLayoutSystem, renderer.mAnimVertexModelConfigHandle);
	}

	void ResizeRenderSurface(Renderer& renderer, u32 windowWidth, u32 windowHeight, u32 resolutionX, u32 resolutionY, float scaleX, float scaleY, float xOffset, float yOffset)
	{
		//no need to resize if that's the size we already are
		renderer.mRenderTargets[RTS_OUTPUT].xOffset	= round(xOffset);
		renderer.mRenderTargets[RTS_OUTPUT].yOffset	= round(yOffset);
		renderer.mRenderTargets[RTS_OUTPUT].width	= round(scaleX * resolutionX);
		renderer.mRenderTargets[RTS_OUTPUT].height	= round(scaleY * resolutionY);

		if (!util::IsSizeEqual(renderer.mResolutionSize, resolutionX, resolutionY))
		{
			DestroyRenderSurfaces(renderer);
			
			CreateZPrePassRenderSurface(renderer, resolutionX, resolutionY);
			CreateZPrePassShadowsRenderSurface(renderer, resolutionX, resolutionY);
			
			CreateAmbientOcclusionSurface(renderer, resolutionX, resolutionY);
			CreateAmbientOcclusionDenoiseSurface(renderer, resolutionX, resolutionY);
			CreateAmbientOcclusionTemporalDenoiseSurface(renderer, resolutionX, resolutionY);

			CreateShadowRenderSurface(renderer, light::SHADOW_MAP_SIZE, light::SHADOW_MAP_SIZE);
			CreatePointLightShadowRenderSurface(renderer, light::SHADOW_MAP_SIZE, light::SHADOW_MAP_SIZE);

			AssignShadowMapPagesForAllLights(renderer);

			renderer.mRenderTargets[RTS_GBUFFER] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_GBUFFER], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");
			renderer.mRenderTargets[RTS_NORMAL] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_NORMAL], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");
			renderer.mRenderTargets[RTS_SPECULAR] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_SPECULAR], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");
			
			renderer.mRenderTargets[RTS_CONVOLVED_GBUFFER] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_CONVOLVED_GBUFFER], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");


			rt::TextureAttachmentFormat gbufferFormat = {};
			gbufferFormat.type = rt::COLOR;
			gbufferFormat.filter = tex::FILTER_LINEAR;
			gbufferFormat.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
			gbufferFormat.hasAlpha = false;
			gbufferFormat.isHDR = true;
			gbufferFormat.usesLayeredRenderering = false;
			gbufferFormat.numLayers = 1;
			gbufferFormat.numMipLevels = 1;

			rt::TextureAttachmentFormat normalFormat = gbufferFormat;

			rt::TextureAttachmentFormat specularFormat = gbufferFormat;
			specularFormat.hasAlpha = true;
			specularFormat.isHDR = false;

			rt::AddTextureAttachment(renderer.mRenderTargets[RTS_GBUFFER], gbufferFormat);
			rt::AddTextureAttachment(renderer.mRenderTargets[RTS_NORMAL], normalFormat);
			rt::AddTextureAttachment(renderer.mRenderTargets[RTS_SPECULAR], specularFormat);
			
			const auto& gbufferColorAttachment = r2::sarr::At(*renderer.mRenderTargets[RTS_GBUFFER].colorAttachments, 0);
			const auto gbufferTexture = gbufferColorAttachment.texture[gbufferColorAttachment.currentTexture];

			auto ssrRoughnessMips = tex::MaxMipsForSparseTextureSize(gbufferTexture);

			//if (renderer.mSSRParams.mSSRRoughnessMips != ssrRoughnessMips)
			{
				renderer.mSSRParams.mSSRRoughnessMips = ssrRoughnessMips;
				renderer.mSSRNeedsUpdate = true;
			}

			rt::TextureAttachmentFormat convolvedGBufferFormat = {};
			convolvedGBufferFormat.type = rt::COLOR;
			convolvedGBufferFormat.swapping = true;
			convolvedGBufferFormat.uploadAllTextures = true;
			convolvedGBufferFormat.filter = tex::FILTER_LINEAR;
			convolvedGBufferFormat.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
			convolvedGBufferFormat.numLayers = 1;
			convolvedGBufferFormat.numMipLevels = ssrRoughnessMips;
			convolvedGBufferFormat.hasAlpha = false;
			convolvedGBufferFormat.isHDR = true;
			convolvedGBufferFormat.usesLayeredRenderering = false;

			rt::AddTextureAttachment(renderer.mRenderTargets[RTS_CONVOLVED_GBUFFER], convolvedGBufferFormat);

			rt::SetTextureAttachment(renderer.mRenderTargets[RTS_GBUFFER], r2::sarr::At(*renderer.mRenderTargets[RTS_ZPREPASS].depthStencilAttachments, 0));
			rt::SetTextureAttachment(renderer.mRenderTargets[RTS_GBUFFER], r2::sarr::At(*renderer.mRenderTargets[RTS_NORMAL].colorAttachments, 0));
			rt::SetTextureAttachment(renderer.mRenderTargets[RTS_GBUFFER], r2::sarr::At(*renderer.mRenderTargets[RTS_SPECULAR].colorAttachments, 0));

			CreateTransparentAccumSurface(renderer, resolutionX, resolutionY);

			CreateSSRRenderSurface(renderer, resolutionX, resolutionY);
			CreateConeTracedSSRRenderSurface(renderer, resolutionX, resolutionY);

			u32 numBloomMips = tex::MaxMipsForTextureSizeBiggerThan(gbufferTexture, util::GetWarpSize());

			CreateBloomSurface(renderer, resolutionX/2, resolutionY/2, numBloomMips); //divided by 2 because we start the downsample at half res
			CreateBloomBlurSurface(renderer, resolutionX / 2, resolutionY / 2, numBloomMips);
			CreateBloomSurfaceUpSampled(renderer, resolutionX / 2, resolutionY / 2, numBloomMips);

			CreateCompositeSurface(renderer, resolutionX, resolutionY);

			CreateSMAAEdgeDetectionSurface(renderer, resolutionX, resolutionY);
			CreateSMAABlendingWeightSurface(renderer, resolutionX, resolutionY);
			CreateSMAANeighborhoodBlendingSurface(renderer, resolutionX, resolutionY);

#ifdef R2_EDITOR
			CreateEditorPickingSurface(renderer, resolutionX, resolutionY);
#endif
			renderer.mShaderVectors.clusterTileSizes = glm::uvec4(16, 9, 24, resolutionX / 16); //@TODO(Serge): make this smarter
			
			renderer.mFlags.Set(RENDERER_FLAG_NEEDS_CLUSTER_VOLUME_TILE_UPDATE);
			renderer.mFlags.Set(RENDERER_FLAG_NEEDS_ALL_SURFACES_UPLOAD);
		}
		
		renderer.mResolutionSize.width = resolutionX;
		renderer.mResolutionSize.height = resolutionY;
		renderer.mCompositeSize.width = windowWidth;
		renderer.mCompositeSize.height = windowHeight;

		renderer.mAAParams.mFXAATexelStep = glm::vec2(1.0f / static_cast<float>(resolutionX), 1.0f / static_cast<float>(resolutionX));
		renderer.mFXAANeedsUpdate = true;


		renderer.mSMAANeedsUpdate = true;
	}
	
	void ConstrainResolution(u32& resolutionX, u32& resolutionY)
	{
		static const s32 MAX_TEXTURE_SIZE = tex::GetMaxTextureSize();

		if (resolutionX > MAX_TEXTURE_SIZE)
		{
			resolutionX = MAX_TEXTURE_SIZE;
		}

		if (resolutionY > MAX_TEXTURE_SIZE)
		{
			resolutionY = MAX_TEXTURE_SIZE;
		}
	}

	void CreateShadowRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_SHADOWS] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_SHADOWS], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");
		
		rt::TextureAttachmentFormat format;
		format.type = rt::DEPTH;
		format.filter = tex::FILTER_NEAREST;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_BORDER;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = false;
		format.isHDR = true;
		format.usesLayeredRenderering = true;
		format.mipLevelToAttach = 0;

		//@TODO(Serge): we're effectively burning the first page of this render target. May want to fix that at some point
		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_SHADOWS], format);
	}

	void CreatePointLightShadowRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_POINTLIGHT_SHADOWS] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_POINTLIGHT_SHADOWS], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

		rt::TextureAttachmentFormat format;
		format.type = rt::DEPTH_CUBEMAP;
		format.filter = tex::FILTER_NEAREST;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_BORDER;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = false;
		format.isHDR = false;
		format.usesLayeredRenderering = true;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_POINTLIGHT_SHADOWS], format);
	}

	void CreateZPrePassRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_ZPREPASS] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_ZPREPASS], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

		rt::TextureAttachmentFormat format;
		format.type = rt::DEPTH24_STENCIL8;
		format.filter = tex::FILTER_NEAREST;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = false;
		format.isHDR = false;
		format.usesLayeredRenderering = false;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_ZPREPASS], format);
	}

	void CreateZPrePassShadowsRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_ZPREPASS_SHADOWS] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_ZPREPASS_SHADOWS], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");
		
		rt::TextureAttachmentFormat format;
		format.type = rt::DEPTH;
		format.swapping = true;
		format.uploadAllTextures = true;
		format.filter = tex::FILTER_NEAREST;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_BORDER;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = false;
		format.isHDR = false;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_ZPREPASS_SHADOWS], format);
	}

	void CreateAmbientOcclusionSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_AMBIENT_OCCLUSION] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_AMBIENT_OCCLUSION], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");
		
		rt::TextureAttachmentFormat format;
		format.type = rt::RG32F;
		format.filter = tex::FILTER_NEAREST;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_BORDER;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = false;
		format.isHDR = false;
		format.usesLayeredRenderering = false;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_AMBIENT_OCCLUSION], format);
	}

	void CreateAmbientOcclusionDenoiseSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_AMBIENT_OCCLUSION_DENOISED] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_AMBIENT_OCCLUSION_DENOISED], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

		rt::TextureAttachmentFormat format;
		format.type = rt::RG32F;
		format.filter = tex::FILTER_NEAREST;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_BORDER;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = false;
		format.isHDR = false;
		format.usesLayeredRenderering = false;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_AMBIENT_OCCLUSION_DENOISED], format);
	}

	void CreateAmbientOcclusionTemporalDenoiseSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

		rt::TextureAttachmentFormat format;
		format.type = rt::RG32F;
		format.swapping = true;
		format.uploadAllTextures = true;
		format.filter = tex::FILTER_NEAREST;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_BORDER;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = false;
		format.isHDR = false;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED], format);
	}

	void CreateSSRRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_SSR] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_SSR], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");
		
		rt::TextureAttachmentFormat format;
		format.type = rt::COLOR;
		format.swapping = false;
		format.uploadAllTextures = false;
		format.filter = tex::FILTER_NEAREST;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_BORDER;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = true;
		format.isHDR = true;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_SSR], format);
	}

	void CreateConeTracedSSRRenderSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_SSR_CONE_TRACED] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_SSR_CONE_TRACED], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");
	
		rt::TextureAttachmentFormat format;
		format.type = rt::COLOR;
		format.swapping = true;
		format.uploadAllTextures = false;
		format.filter = tex::FILTER_LINEAR;
		format.wrapMode = tex::WRAP_MODE_REPEAT;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = true;
		format.isHDR = true;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_SSR_CONE_TRACED], format);
	}

	void CreateBloomSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY, u32 numMips)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_BLOOM] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_BLOOM], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

		rt::TextureAttachmentFormat format;
		format.type = rt::COLOR;
		format.swapping = false;
		format.uploadAllTextures = false;
		format.filter = tex::FILTER_LINEAR;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
		format.numLayers = 1;
		format.numMipLevels = numMips;
		format.hasAlpha = false;
		format.isHDR = true;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_BLOOM], format);
	}

	void CreateBloomBlurSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY, u32 numMips)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_BLOOM_BLUR] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_BLOOM_BLUR], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

		rt::TextureAttachmentFormat format;
		format.type = rt::COLOR;
		format.swapping = false;
		format.uploadAllTextures = false;
		format.filter = tex::FILTER_LINEAR;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
		format.numLayers = 1;
		format.numMipLevels = numMips;
		format.hasAlpha = false;
		format.isHDR = true;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_BLOOM_BLUR], format);
	}

	void CreateBloomSurfaceUpSampled(Renderer& renderer, u32 resolutionX, u32 resolutionY, u32 numMips)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_BLOOM_UPSAMPLE] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_BLOOM_UPSAMPLE], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

		rt::TextureAttachmentFormat format;
		format.type = rt::COLOR;
		format.swapping = false;
		format.uploadAllTextures = false;
		format.filter = tex::FILTER_LINEAR;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
		format.numLayers = 1;
		format.numMipLevels = numMips;
		format.hasAlpha = false;
		format.isHDR = true;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_BLOOM_UPSAMPLE], format);
	}

	void CreateCompositeSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_COMPOSITE] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_COMPOSITE], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");
	
		rt::TextureAttachmentFormat format;
		format.type = rt::COLOR;
		format.swapping = false;
		format.uploadAllTextures = false;
		format.filter = tex::FILTER_NEAREST;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = true;
		format.isHDR = false;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_COMPOSITE], format);
	}

	void CreateSMAAEdgeDetectionSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_SMAA_EDGE_DETECTION] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_SMAA_EDGE_DETECTION], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");
		
		rt::TextureAttachmentFormat format;
		format.type = rt::COLOR;
		format.swapping = false;
		format.uploadAllTextures = false;
		format.filter = tex::FILTER_LINEAR;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = true;
		format.isHDR = false;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_SMAA_EDGE_DETECTION], format);

		format.type = rt::STENCIL8;
		format.hasAlpha = false;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_SMAA_EDGE_DETECTION], format);
	}

	void CreateSMAABlendingWeightSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_SMAA_BLENDING_WEIGHT] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_SMAA_BLENDING_WEIGHT], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

		rt::TextureAttachmentFormat format;
		format.type = rt::COLOR;
		format.swapping = false;
		format.uploadAllTextures = false;
		format.filter = tex::FILTER_LINEAR;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = true;
		format.isHDR = false;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_SMAA_BLENDING_WEIGHT], format);

		rt::SetTextureAttachment(renderer.mRenderTargets[RTS_SMAA_BLENDING_WEIGHT], r2::sarr::At(*renderer.mRenderTargets[RTS_SMAA_EDGE_DETECTION].stencilAttachments, 0));
	}

	void CreateSMAANeighborhoodBlendingSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_SMAA_NEIGHBORHOOD_BLENDING] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_SMAA_NEIGHBORHOOD_BLENDING], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

		rt::TextureAttachmentFormat format;
		format.type = rt::COLOR;
		format.swapping = true;
		format.uploadAllTextures = true;
		format.filter = tex::FILTER_LINEAR;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = true;
		format.isHDR = false;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_SMAA_NEIGHBORHOOD_BLENDING], format);
	}

	void CreateTransparentAccumSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_TRANSPARENT_ACCUM] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_TRANSPARENT_ACCUM], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");
		renderer.mRenderTargets[RTS_TRANSPARENT_REVEAL] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_TRANSPARENT_REVEAL], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

		rt::TextureAttachmentFormat format;
		format.type = rt::COLOR;
		format.swapping = false;
		format.uploadAllTextures = false;
		format.filter = tex::FILTER_LINEAR;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = true;
		format.isHDR = true;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_TRANSPARENT_ACCUM], format);

		rt::TextureAttachmentFormat revealFormat;
		revealFormat.type = rt::R8;
		revealFormat.swapping = false;
		revealFormat.uploadAllTextures = false;
		revealFormat.filter = tex::FILTER_LINEAR;
		revealFormat.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
		revealFormat.numLayers = 1;
		revealFormat.numMipLevels = 1;
		revealFormat.hasAlpha = false;
		revealFormat.isHDR = false;
		revealFormat.usesLayeredRenderering = false;
		revealFormat.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_TRANSPARENT_REVEAL], revealFormat);
		rt::SetTextureAttachment(renderer.mRenderTargets[RTS_TRANSPARENT_ACCUM], r2::sarr::At(*renderer.mRenderTargets[RTS_TRANSPARENT_REVEAL].colorAttachments, 0));
		rt::SetTextureAttachment(renderer.mRenderTargets[RTS_TRANSPARENT_ACCUM], r2::sarr::At(*renderer.mRenderTargets[RTS_ZPREPASS].depthStencilAttachments, 0));
	}

#ifdef R2_EDITOR
	void CreateEditorPickingSurface(Renderer& renderer, u32 resolutionX, u32 resolutionY)
	{
		ConstrainResolution(resolutionX, resolutionY);

		renderer.mRenderTargets[RTS_EDITOR_PICKING] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargetParams[RTS_EDITOR_PICKING], 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

		rt::TextureAttachmentFormat format;
		format.type = rt::RG32UI;
		format.swapping = true;
		format.uploadAllTextures = false;
		format.filter = tex::FILTER_NEAREST;
		format.wrapMode = tex::WRAP_MODE_CLAMP_TO_EDGE;
		format.numLayers = 1;
		format.numMipLevels = 1;
		format.hasAlpha = false;
		format.isHDR = false;
		format.usesLayeredRenderering = false;
		format.mipLevelToAttach = 0;

		rt::AddTextureAttachment(renderer.mRenderTargets[RTS_EDITOR_PICKING], format);
		rt::SetTextureAttachment(renderer.mRenderTargets[RTS_EDITOR_PICKING], r2::sarr::At(*renderer.mRenderTargets[RTS_ZPREPASS].depthStencilAttachments, 0));
	}
#endif

	void DestroyRenderSurfaces(Renderer& renderer)
	{
		ClearAllShadowMapPages(renderer);
#ifdef R2_EDITOR
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_EDITOR_PICKING]);
#endif
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_SMAA_NEIGHBORHOOD_BLENDING]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_SMAA_BLENDING_WEIGHT]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_SMAA_EDGE_DETECTION]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_COMPOSITE]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_BLOOM_UPSAMPLE]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_BLOOM_BLUR]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_BLOOM]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_SSR_CONE_TRACED]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_SSR]);

		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_TRANSPARENT_REVEAL]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_TRANSPARENT_ACCUM]);

		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_CONVOLVED_GBUFFER]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_SPECULAR]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_NORMAL]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_GBUFFER]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_POINTLIGHT_SHADOWS]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_SHADOWS]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_AMBIENT_OCCLUSION_DENOISED]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_AMBIENT_OCCLUSION]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_ZPREPASS_SHADOWS]);
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_ZPREPASS]);
	}

	void SetupRenderTargetParams(rt::RenderTargetParams renderTargetParams[NUM_RENDER_TARGET_SURFACES])
	{
		u32 surfaceOffset = 0;

		constexpr auto sizeOfTextureAddress = sizeof(tex::TextureAddress);

		renderTargetParams[RTS_GBUFFER].numColorAttachments = 1;
		renderTargetParams[RTS_GBUFFER].numDepthAttachments = 0;
		renderTargetParams[RTS_GBUFFER].numStencilAttachments = 0;
		renderTargetParams[RTS_GBUFFER].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_GBUFFER].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_GBUFFER].maxPageAllocations = 0;
		renderTargetParams[RTS_GBUFFER].numAttachmentRefs = 3;
		renderTargetParams[RTS_GBUFFER].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_GBUFFER].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_GBUFFER].numSurfacesPerTarget;

		renderTargetParams[RTS_SHADOWS].numColorAttachments = 0;
		renderTargetParams[RTS_SHADOWS].numDepthAttachments = 1;
		renderTargetParams[RTS_SHADOWS].numStencilAttachments = 0;
		renderTargetParams[RTS_SHADOWS].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_SHADOWS].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_SHADOWS].maxPageAllocations = light::MAX_NUM_SHADOW_MAP_PAGES + light::MAX_NUM_SHADOW_MAP_PAGES;
		renderTargetParams[RTS_SHADOWS].numAttachmentRefs = 0;
		renderTargetParams[RTS_SHADOWS].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_SHADOWS].numSurfacesPerTarget = 1;
		
		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_SHADOWS].numSurfacesPerTarget;

		renderTargetParams[RTS_COMPOSITE].numColorAttachments = 1;
		renderTargetParams[RTS_COMPOSITE].numDepthAttachments = 0;
		renderTargetParams[RTS_COMPOSITE].numStencilAttachments = 0;
		renderTargetParams[RTS_COMPOSITE].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_COMPOSITE].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_COMPOSITE].maxPageAllocations = 0;
		renderTargetParams[RTS_COMPOSITE].numAttachmentRefs = 0;
		renderTargetParams[RTS_COMPOSITE].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_COMPOSITE].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_COMPOSITE].numSurfacesPerTarget;

		renderTargetParams[RTS_ZPREPASS].numColorAttachments = 0;
		renderTargetParams[RTS_ZPREPASS].numDepthAttachments = 0;
		renderTargetParams[RTS_ZPREPASS].numStencilAttachments = 0;
		renderTargetParams[RTS_ZPREPASS].numDepthStencilAttachments = 1; 
		renderTargetParams[RTS_ZPREPASS].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_ZPREPASS].maxPageAllocations = 0;
		renderTargetParams[RTS_ZPREPASS].numAttachmentRefs = 0;
		renderTargetParams[RTS_ZPREPASS].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_ZPREPASS].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_ZPREPASS].numSurfacesPerTarget;

		renderTargetParams[RTS_POINTLIGHT_SHADOWS].numColorAttachments = 0;
		renderTargetParams[RTS_POINTLIGHT_SHADOWS].numDepthAttachments = 1;
		renderTargetParams[RTS_POINTLIGHT_SHADOWS].numStencilAttachments = 0;
		renderTargetParams[RTS_POINTLIGHT_SHADOWS].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_POINTLIGHT_SHADOWS].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_POINTLIGHT_SHADOWS].maxPageAllocations = light::MAX_NUM_SHADOW_MAP_PAGES;
		renderTargetParams[RTS_POINTLIGHT_SHADOWS].numAttachmentRefs = 0;
		renderTargetParams[RTS_POINTLIGHT_SHADOWS].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_POINTLIGHT_SHADOWS].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_POINTLIGHT_SHADOWS].numSurfacesPerTarget;

		renderTargetParams[RTS_AMBIENT_OCCLUSION].numColorAttachments = 1;
		renderTargetParams[RTS_AMBIENT_OCCLUSION].numDepthAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION].numStencilAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION].maxPageAllocations = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION].numAttachmentRefs = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_AMBIENT_OCCLUSION].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_AMBIENT_OCCLUSION].numSurfacesPerTarget;

		renderTargetParams[RTS_AMBIENT_OCCLUSION_DENOISED].numColorAttachments = 1;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_DENOISED].numDepthAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_DENOISED].numStencilAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_DENOISED].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_DENOISED].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_DENOISED].maxPageAllocations = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_DENOISED].numAttachmentRefs = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_DENOISED].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_DENOISED].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_AMBIENT_OCCLUSION_DENOISED].numSurfacesPerTarget;

		renderTargetParams[RTS_ZPREPASS_SHADOWS].numColorAttachments = 0;
		renderTargetParams[RTS_ZPREPASS_SHADOWS].numDepthAttachments = 1;
		renderTargetParams[RTS_ZPREPASS_SHADOWS].numStencilAttachments = 0;
		renderTargetParams[RTS_ZPREPASS_SHADOWS].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_ZPREPASS_SHADOWS].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_ZPREPASS_SHADOWS].maxPageAllocations = 0;
		renderTargetParams[RTS_ZPREPASS_SHADOWS].numAttachmentRefs = 0;
		renderTargetParams[RTS_ZPREPASS_SHADOWS].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_ZPREPASS_SHADOWS].numSurfacesPerTarget = 2;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_ZPREPASS_SHADOWS].numSurfacesPerTarget;

		renderTargetParams[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED].numColorAttachments = 1;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED].numDepthAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED].numStencilAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED].maxPageAllocations = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED].numAttachmentRefs = 0;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED].numSurfacesPerTarget = 2;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED].numSurfacesPerTarget;

		renderTargetParams[RTS_NORMAL].numColorAttachments = 1;
		renderTargetParams[RTS_NORMAL].numDepthAttachments = 0;
		renderTargetParams[RTS_NORMAL].numStencilAttachments = 0;
		renderTargetParams[RTS_NORMAL].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_NORMAL].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_NORMAL].maxPageAllocations = 0;
		renderTargetParams[RTS_NORMAL].numAttachmentRefs = 0;
		renderTargetParams[RTS_NORMAL].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_NORMAL].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_NORMAL].numSurfacesPerTarget;

		renderTargetParams[RTS_SPECULAR].numColorAttachments = 1;
		renderTargetParams[RTS_SPECULAR].numDepthAttachments = 0;
		renderTargetParams[RTS_SPECULAR].numStencilAttachments = 0;
		renderTargetParams[RTS_SPECULAR].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_SPECULAR].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_SPECULAR].maxPageAllocations = 0;
		renderTargetParams[RTS_SPECULAR].numAttachmentRefs = 0;
		renderTargetParams[RTS_SPECULAR].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_SPECULAR].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_SPECULAR].numSurfacesPerTarget;

		renderTargetParams[RTS_SSR].numColorAttachments = 1;
		renderTargetParams[RTS_SSR].numDepthAttachments = 0;
		renderTargetParams[RTS_SSR].numStencilAttachments = 0;
		renderTargetParams[RTS_SSR].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_SSR].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_SSR].maxPageAllocations = 0;
		renderTargetParams[RTS_SSR].numAttachmentRefs = 0;
		renderTargetParams[RTS_SSR].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_SSR].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_SSR].numSurfacesPerTarget;

		renderTargetParams[RTS_CONVOLVED_GBUFFER].numColorAttachments = 1;
		renderTargetParams[RTS_CONVOLVED_GBUFFER].numDepthAttachments = 0;
		renderTargetParams[RTS_CONVOLVED_GBUFFER].numStencilAttachments = 0;
		renderTargetParams[RTS_CONVOLVED_GBUFFER].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_CONVOLVED_GBUFFER].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_CONVOLVED_GBUFFER].maxPageAllocations = 0;
		renderTargetParams[RTS_CONVOLVED_GBUFFER].numAttachmentRefs = 0;
		renderTargetParams[RTS_CONVOLVED_GBUFFER].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_CONVOLVED_GBUFFER].numSurfacesPerTarget = 2;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_CONVOLVED_GBUFFER].numSurfacesPerTarget;

		renderTargetParams[RTS_SSR_CONE_TRACED].numColorAttachments = 1;
		renderTargetParams[RTS_SSR_CONE_TRACED].numDepthAttachments = 0;
		renderTargetParams[RTS_SSR_CONE_TRACED].numStencilAttachments = 0;
		renderTargetParams[RTS_SSR_CONE_TRACED].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_SSR_CONE_TRACED].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_SSR_CONE_TRACED].maxPageAllocations = 0;
		renderTargetParams[RTS_SSR_CONE_TRACED].numAttachmentRefs = 0;
		renderTargetParams[RTS_SSR_CONE_TRACED].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_SSR_CONE_TRACED].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_SSR_CONE_TRACED].numSurfacesPerTarget;

		renderTargetParams[RTS_BLOOM].numColorAttachments = 1;
		renderTargetParams[RTS_BLOOM].numDepthAttachments = 0;
		renderTargetParams[RTS_BLOOM].numStencilAttachments = 0;
		renderTargetParams[RTS_BLOOM].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_BLOOM].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_BLOOM].maxPageAllocations = 0;
		renderTargetParams[RTS_BLOOM].numAttachmentRefs = 0;
		renderTargetParams[RTS_BLOOM].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_BLOOM].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_BLOOM].numSurfacesPerTarget;

		renderTargetParams[RTS_BLOOM_BLUR].numColorAttachments = 1;
		renderTargetParams[RTS_BLOOM_BLUR].numDepthAttachments = 0;
		renderTargetParams[RTS_BLOOM_BLUR].numStencilAttachments = 0;
		renderTargetParams[RTS_BLOOM_BLUR].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_BLOOM_BLUR].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_BLOOM_BLUR].maxPageAllocations = 0;
		renderTargetParams[RTS_BLOOM_BLUR].numAttachmentRefs = 0;
		renderTargetParams[RTS_BLOOM_BLUR].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_BLOOM_BLUR].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_BLOOM_BLUR].numSurfacesPerTarget;

		renderTargetParams[RTS_BLOOM_UPSAMPLE].numColorAttachments = 1;
		renderTargetParams[RTS_BLOOM_UPSAMPLE].numDepthAttachments = 0;
		renderTargetParams[RTS_BLOOM_UPSAMPLE].numStencilAttachments = 0;
		renderTargetParams[RTS_BLOOM_UPSAMPLE].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_BLOOM_UPSAMPLE].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_BLOOM_UPSAMPLE].maxPageAllocations = 0;
		renderTargetParams[RTS_BLOOM_UPSAMPLE].numAttachmentRefs = 0;
		renderTargetParams[RTS_BLOOM_UPSAMPLE].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_BLOOM_UPSAMPLE].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_BLOOM_UPSAMPLE].numSurfacesPerTarget;

		renderTargetParams[RTS_SMAA_EDGE_DETECTION].numColorAttachments = 1;
		renderTargetParams[RTS_SMAA_EDGE_DETECTION].numDepthAttachments = 0;
		renderTargetParams[RTS_SMAA_EDGE_DETECTION].numStencilAttachments = 1;
		renderTargetParams[RTS_SMAA_EDGE_DETECTION].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_SMAA_EDGE_DETECTION].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_SMAA_EDGE_DETECTION].maxPageAllocations = 0;
		renderTargetParams[RTS_SMAA_EDGE_DETECTION].numAttachmentRefs = 0;
		renderTargetParams[RTS_SMAA_EDGE_DETECTION].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_SMAA_EDGE_DETECTION].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_SMAA_EDGE_DETECTION].numSurfacesPerTarget;

		renderTargetParams[RTS_SMAA_BLENDING_WEIGHT].numColorAttachments = 1;
		renderTargetParams[RTS_SMAA_BLENDING_WEIGHT].numDepthAttachments = 0;
		renderTargetParams[RTS_SMAA_BLENDING_WEIGHT].numStencilAttachments = 0;
		renderTargetParams[RTS_SMAA_BLENDING_WEIGHT].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_SMAA_BLENDING_WEIGHT].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_SMAA_BLENDING_WEIGHT].maxPageAllocations = 0;
		renderTargetParams[RTS_SMAA_BLENDING_WEIGHT].numAttachmentRefs = 1;
		renderTargetParams[RTS_SMAA_BLENDING_WEIGHT].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_SMAA_BLENDING_WEIGHT].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_SMAA_BLENDING_WEIGHT].numSurfacesPerTarget;

		renderTargetParams[RTS_SMAA_NEIGHBORHOOD_BLENDING].numColorAttachments = 1;
		renderTargetParams[RTS_SMAA_NEIGHBORHOOD_BLENDING].numDepthAttachments = 0;
		renderTargetParams[RTS_SMAA_NEIGHBORHOOD_BLENDING].numStencilAttachments = 1;
		renderTargetParams[RTS_SMAA_NEIGHBORHOOD_BLENDING].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_SMAA_NEIGHBORHOOD_BLENDING].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_SMAA_NEIGHBORHOOD_BLENDING].maxPageAllocations = 0;
		renderTargetParams[RTS_SMAA_NEIGHBORHOOD_BLENDING].numAttachmentRefs = 0;
		renderTargetParams[RTS_SMAA_NEIGHBORHOOD_BLENDING].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_SMAA_NEIGHBORHOOD_BLENDING].numSurfacesPerTarget = 2;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_SMAA_NEIGHBORHOOD_BLENDING].numSurfacesPerTarget;

		renderTargetParams[RTS_TRANSPARENT_ACCUM].numColorAttachments = 1;
		renderTargetParams[RTS_TRANSPARENT_ACCUM].numDepthAttachments = 0;
		renderTargetParams[RTS_TRANSPARENT_ACCUM].numStencilAttachments = 0;
		renderTargetParams[RTS_TRANSPARENT_ACCUM].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_TRANSPARENT_ACCUM].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_TRANSPARENT_ACCUM].maxPageAllocations = 0;
		renderTargetParams[RTS_TRANSPARENT_ACCUM].numAttachmentRefs = 2; //RTS_REVEAL, RTS_ZPREPASS
		renderTargetParams[RTS_TRANSPARENT_ACCUM].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_TRANSPARENT_ACCUM].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_TRANSPARENT_ACCUM].numSurfacesPerTarget;

		renderTargetParams[RTS_TRANSPARENT_REVEAL].numColorAttachments = 1;
		renderTargetParams[RTS_TRANSPARENT_REVEAL].numDepthAttachments = 0;
		renderTargetParams[RTS_TRANSPARENT_REVEAL].numStencilAttachments = 0;
		renderTargetParams[RTS_TRANSPARENT_REVEAL].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_TRANSPARENT_REVEAL].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_TRANSPARENT_REVEAL].maxPageAllocations = 0;
		renderTargetParams[RTS_TRANSPARENT_REVEAL].numAttachmentRefs = 0;
		renderTargetParams[RTS_TRANSPARENT_REVEAL].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_TRANSPARENT_REVEAL].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_TRANSPARENT_REVEAL].numSurfacesPerTarget;


		//should always be last
		renderTargetParams[RTS_OUTPUT].numColorAttachments = 0;
		renderTargetParams[RTS_OUTPUT].numDepthAttachments = 0;
		renderTargetParams[RTS_OUTPUT].numStencilAttachments = 0;
		renderTargetParams[RTS_OUTPUT].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_OUTPUT].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_OUTPUT].maxPageAllocations = 0;
		renderTargetParams[RTS_OUTPUT].numAttachmentRefs = 0;
		renderTargetParams[RTS_OUTPUT].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_OUTPUT].numSurfacesPerTarget = 1;

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_OUTPUT].numSurfacesPerTarget;

#ifdef R2_EDITOR
		renderTargetParams[RTS_EDITOR_PICKING].numColorAttachments = 1;
		renderTargetParams[RTS_EDITOR_PICKING].numDepthAttachments = 0;
		renderTargetParams[RTS_EDITOR_PICKING].numStencilAttachments = 0;
		renderTargetParams[RTS_EDITOR_PICKING].numDepthStencilAttachments = 0;
		renderTargetParams[RTS_EDITOR_PICKING].numRenderBufferAttachments = 0;
		renderTargetParams[RTS_EDITOR_PICKING].maxPageAllocations = 0;
		renderTargetParams[RTS_EDITOR_PICKING].numAttachmentRefs = 1; //for depth
		renderTargetParams[RTS_EDITOR_PICKING].surfaceOffset = surfaceOffset;
		renderTargetParams[RTS_EDITOR_PICKING].numSurfacesPerTarget = 2; //@NOTE(Serge): maybe this should be 3?

		surfaceOffset += sizeOfTextureAddress * renderTargetParams[RTS_EDITOR_PICKING].numSurfacesPerTarget;
#endif
	}

	void UploadAllSurfaces(Renderer& renderer)
	{
		u32 numSurfaces = 0;

		for (u32 i = RTS_GBUFFER; i < RTS_OUTPUT; ++i)
		{
			numSurfaces += renderer.mRenderTargetParams[i].numSurfacesPerTarget;
		}

		r2::SArray<tex::TextureAddress>* textureAddresses = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, tex::TextureAddress, numSurfaces);

		for (u32 i = RTS_GBUFFER; i < RTS_OUTPUT; ++i)
		{
			RenderTarget* rt = GetRenderTarget(renderer, (RenderTargetSurface)i);
			
			tex::TextureAddress surfaceTextureAddress[2];
			u32 numSurfaceTexturesToAdd = 1;

			int currentTexture = 0;

			if (rt->colorAttachments)
			{
				const auto& attachment = r2::sarr::At(*rt->colorAttachments, 0);

				currentTexture = attachment.currentTexture;

				surfaceTextureAddress[0] = texsys::GetTextureAddress(attachment.texture[currentTexture]);

				currentTexture = (currentTexture + 1) % attachment.numTextures;

				if (attachment.numTextures > 1 && attachment.textureAttachmentFormat.uploadAllTextures)
				{
					surfaceTextureAddress[1] = texsys::GetTextureAddress(attachment.texture[currentTexture]);
					numSurfaceTexturesToAdd++;
				}
			}
			else if (rt->depthAttachments)
			{
				const auto& attachment = r2::sarr::At(*rt->depthAttachments, 0);

				currentTexture = attachment.currentTexture;

				surfaceTextureAddress[0] = texsys::GetTextureAddress(attachment.texture[currentTexture]);

				currentTexture = (currentTexture + 1) % attachment.numTextures;

				if (attachment.numTextures > 1 && attachment.textureAttachmentFormat.uploadAllTextures)
				{
					surfaceTextureAddress[1] = texsys::GetTextureAddress(attachment.texture[currentTexture]);
					numSurfaceTexturesToAdd++;
				}
			}
			else if (rt->depthStencilAttachments)
			{
				const auto& attachment = r2::sarr::At(*rt->depthStencilAttachments, 0);

				currentTexture = attachment.currentTexture;

				surfaceTextureAddress[0] = texsys::GetTextureAddress(attachment.texture[currentTexture]);

				currentTexture = (currentTexture + 1) % attachment.numTextures;

				if (attachment.numTextures > 1 && attachment.textureAttachmentFormat.uploadAllTextures)
				{
					surfaceTextureAddress[1] = texsys::GetTextureAddress(attachment.texture[currentTexture]);
					numSurfaceTexturesToAdd++;
				}
			}
			else
			{
				R2_CHECK(false, "?");
			}

			for (u32 j = 0; j < numSurfaceTexturesToAdd; ++j)
			{
				r2::sarr::Push(*textureAddresses, surfaceTextureAddress[j]);
			}
		}
		ConstantBufferHandle surfaceBufferHandle = r2::sarr::At(*renderer.mConstantBufferHandles, renderer.mSurfacesConfigHandle);

		ConstantBufferData* constBufferData = GetConstData(renderer, surfaceBufferHandle);

		key::Basic fillkey;
		fillkey.keyValue = 0;

		cmd::FillConstantBuffer* fillSurfacesCMD = AddCommand<key::Basic, cmd::FillConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, *renderer.mPreRenderBucket, fillkey, sizeof(tex::TextureAddress) * numSurfaces);

		FillConstantBufferCommand(fillSurfacesCMD, surfaceBufferHandle, constBufferData->type, constBufferData->isPersistent, textureAddresses->mData, sizeof(tex::TextureAddress) * numSurfaces, 0);

		FREE(textureAddresses, *MEM_ENG_SCRATCH_PTR);
	}

	u32 GetRenderPassTargetOffset(Renderer& renderer, RenderTargetSurface surface)
	{
		return renderer.mRenderTargetParams[surface].surfaceOffset;
	}

	//events
	void WindowResized(Renderer& renderer, u32 windowWidth, u32 windowHeight, u32 resolutionX, u32 resolutionY, float scaleX, float scaleY, float xOffset, float yOffset)
	{
		ResizeRenderSurface(renderer, windowWidth, windowHeight, resolutionX, resolutionY, scaleX, scaleY, xOffset, yOffset);
	}

	void MakeCurrent()
	{
		r2::draw::rendererimpl::MakeCurrent();
	}

	int SetFullscreen(int flags)
	{
		return r2::draw::rendererimpl::SetFullscreen(flags);
	}

	int SetVSYNC(bool vsync)
	{
		return r2::draw::rendererimpl::SetVSYNC(vsync);
	}

	void SetWindowSize(Renderer& renderer, u32 windowWidth, u32 windowHeight, u32 resolutionX, u32 resolutionY, float scaleX, float scaleY, float xOffset, float yOffset)
	{
		ResizeRenderSurface(renderer, windowWidth, windowHeight, resolutionX, resolutionY, scaleX, scaleY, xOffset, yOffset);
	}

	void SetOutputMergerType(Renderer& renderer, OutputMerger outputMerger)
	{
		renderer.mOutputMerger = outputMerger;
	}

	void SetColorCorrection(Renderer& renderer, const ColorCorrection& cc)
	{
		renderer.mColorCorrectionData = cc;
		renderer.mColorCorrectionNeedsUpdate = true;
	}

	void SetColorGradingLUT(Renderer& renderer, const tex::TextureAddress& lut, f32 halfColX, f32 halfColY, f32 numSwatches)
	{
		renderer.mColorCorrectionData.mColorGradingLUT = lut;
		renderer.mColorCorrectionData.mNumColorGradingSwatches = numSwatches;
		
		renderer.mColorCorrectionData.mColorGradingHalfColX = halfColX;
		renderer.mColorCorrectionData.mColorGradingHalfColY = halfColY;
		
		renderer.mColorCorrectionNeedsUpdate = true;
	}

	void EnableColorGrading(Renderer& renderer, bool isEnabled)
	{
		renderer.mColorGradingEnabled = isEnabled;
		renderer.mColorCorrectionNeedsUpdate = true;
	}

	bool IsColorGradingEnabled(Renderer& renderer)
	{
		return renderer.mColorGradingEnabled;
	}

	void SetColorGradingContribution(Renderer& renderer, float contribution)
	{
		renderer.mColorCorrectionData.mColorGradingContribution = glm::clamp(contribution, 0.0f, 1.0f);
		
		renderer.mColorCorrectionNeedsUpdate = true;
	}

	float GetColorGradingContribution(Renderer& renderer)
	{
		return renderer.mColorCorrectionData.mColorGradingContribution;
	}

	void UpdateColorCorrectionIfNeeded(Renderer& renderer)
	{
		if (renderer.mColorCorrectionNeedsUpdate)
		{
			const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles(renderer);
			auto colorCorrectionConstantBufferHandle = r2::sarr::At(*constantBufferHandles, renderer.mColorCorrectionConfigHandle);

			ColorCorrection colorCorrection = renderer.mColorCorrectionData;

			if (!renderer.mColorGradingEnabled)
			{
				colorCorrection.mColorGradingContribution = 0.0f;
			}

			AddFillConstantBufferCommandFull(renderer, colorCorrectionConstantBufferHandle, &colorCorrection, sizeof(renderer.mColorCorrectionData), 0);

			renderer.mColorCorrectionNeedsUpdate = false;
		}
	}

	//Camera and Lighting
	void SetRenderCamera(Renderer& renderer, Camera* cameraPtr)
	{
		R2_CHECK(cameraPtr != nullptr, "We should always pass in a valid camera");
		renderer.mnoptrRenderCam = cameraPtr;

		renderer.mShaderMatrices.prevProjection = renderer.mnoptrRenderCam->proj;
		renderer.mShaderMatrices.prevView = renderer.mnoptrRenderCam->view;
		renderer.mShaderMatrices.prevVPMatrix = renderer.mnoptrRenderCam->vp;
	}

	Camera* GetRenderCamera(Renderer& renderer)
	{
		return renderer.mnoptrRenderCam;
	}

#ifdef R2_EDITOR
	//r2::asset::FileList GetModelFiles(Renderer& renderer)
	//{
	//	return r2::draw::modlche::GetFileList(*renderer.mModelCache);
	//}

	const r2::draw::Model* GetDefaultModel(Renderer& renderer, const r2::asset::AssetName& assetName)
	{
		return r2::draw::modlche::GetModel(renderer.mModelCache, { assetName.hashID, renderer.mModelCache->mModelCache->GetSlot() });
	}

	EntityInstance ReadEntityInstanceAtMousePosition(Renderer& renderer, s32 x, s32 y)
	{
		EntityInstance entityInstance;
		
		auto* renderTarget = GetRenderTarget(renderer, RTS_EDITOR_PICKING);

		R2_CHECK(renderTarget != nullptr, "Should never happen");

		rt::ReadPixelEntity(*renderTarget, x, y, entityInstance.entityId, entityInstance.instanceId);



		return entityInstance;
	}

#endif

	DirectionLightHandle AddDirectionLight(Renderer& renderer, const DirectionLight& light)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		auto handle = lightsys::AddDirectionalLight(*renderer.mLightSystem, light);

		auto dirLight = GetDirectionLightConstPtr(renderer, handle);

		AssignShadowMapPagesForDirectionLight(renderer, *dirLight);

		return handle;
	}

	PointLightHandle AddPointLight(Renderer& renderer, const PointLight& pointLight)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		auto handle = lightsys::AddPointLight(*renderer.mLightSystem, pointLight);

		auto light = GetPointLightConstPtr(renderer, handle);

		AssignShadowMapPagesForPointLight(renderer, *light);

		return handle;
	}

	SpotLightHandle AddSpotLight(Renderer& renderer, const SpotLight& spotLight)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		auto handle = lightsys::AddSpotLight(*renderer.mLightSystem, spotLight);

		auto light = GetSpotLightConstPtr(renderer, handle);

		AssignShadowMapPagesForSpotLight(renderer, *light);

		return handle;
	}

	SkyLightHandle AddSkyLight(Renderer& renderer, const SkyLight& skylight, s32 numPrefilteredMips)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		return lightsys::AddSkyLight(*renderer.mLightSystem, skylight, numPrefilteredMips);
	}

	SkyLightHandle AddSkyLight(Renderer& renderer, const RenderMaterialParams& diffuseMaterial, const RenderMaterialParams& prefilteredMaterial, const RenderMaterialParams& lutDFG, s32 numMips)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		return lightsys::AddSkyLight(*renderer.mLightSystem, diffuseMaterial, prefilteredMaterial, lutDFG, numMips);
	}

	const DirectionLight* GetDirectionLightConstPtr(Renderer& renderer, DirectionLightHandle dirLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		return lightsys::GetDirectionLightConstPtr(*renderer.mLightSystem, dirLightHandle);
	}

	DirectionLight* GetDirectionLightPtr(Renderer& renderer, DirectionLightHandle dirLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		return lightsys::GetDirectionLightPtr(*renderer.mLightSystem, dirLightHandle);
	}

	const PointLight* GetPointLightConstPtr(Renderer& renderer, PointLightHandle pointLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		return lightsys::GetPointLightConstPtr(*renderer.mLightSystem, pointLightHandle);
	}

	PointLight* GetPointLightPtr(Renderer& renderer, PointLightHandle pointLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		return lightsys::GetPointLightPtr(*renderer.mLightSystem, pointLightHandle);
	}

	const SpotLight* GetSpotLightConstPtr(Renderer& renderer, SpotLightHandle spotLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		return lightsys::GetSpotLightConstPtr(*renderer.mLightSystem, spotLightHandle);
	}

	SpotLight* GetSpotLightPtr(Renderer& renderer, SpotLightHandle spotLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		return lightsys::GetSpotLightPtr(*renderer.mLightSystem, spotLightHandle);
	}

	const SkyLight* GetSkyLightConstPtr(Renderer& renderer, SkyLightHandle skyLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		return lightsys::GetSkyLightConstPtr(*renderer.mLightSystem, skyLightHandle);
	}

	SkyLight* GetSkyLightPtr(Renderer& renderer, SkyLightHandle skyLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		return lightsys::GetSkyLightPtr(*renderer.mLightSystem, skyLightHandle);
	}

	bool HasSkylight(Renderer& renderer)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for AddSkyLightthe renderer");
		return lightsys::HasSkylight(*renderer.mLightSystem);
	}

	SkyLightHandle GetCurrentSkylightHandle(Renderer& renderer)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		return lightsys::GetCurrentSkylightHandle(*renderer.mLightSystem);
	}

	void RemoveDirectionLight(Renderer& renderer, DirectionLightHandle dirLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");

		auto directionLight = GetDirectionLightConstPtr(renderer, dirLightHandle);

		RemoveShadowMapPagesForDirectionLight(renderer, *directionLight);

		lightsys::RemoveDirectionalLight(*renderer.mLightSystem, dirLightHandle);
	}

	void RemovePointLight(Renderer& renderer, PointLightHandle pointLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");

		auto pointLight = GetPointLightConstPtr(renderer, pointLightHandle);

		RemoveShadowMapPagesForPointLight(renderer, *pointLight);

		lightsys::RemovePointLight(*renderer.mLightSystem, pointLightHandle);
	}

	void RemoveSpotLight(Renderer& renderer, SpotLightHandle spotLightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");

		auto spotLight = GetSpotLightConstPtr(renderer, spotLightHandle);

		RemoveShadowMapPagesForSpotLight(renderer, *spotLight);

		lightsys::RemoveSpotLight(*renderer.mLightSystem, spotLightHandle);
	}

	void RemoveSkyLight(Renderer& renderer, SkyLightHandle skylightHandle)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		lightsys::RemoveSkyLight(*renderer.mLightSystem, skylightHandle);
	}

	void ClearAllLighting(Renderer& renderer)
	{
		R2_CHECK(renderer.mLightSystem != nullptr, "We should have a valid lighting system for the renderer");
		lightsys::ClearAllLighting(*renderer.mLightSystem);
	}

	void UpdateLighting(Renderer& renderer)
	{
		static u32 SHADOWMAP_SIZES[cam::NUM_FRUSTUM_SPLITS] = { light::SHADOW_MAP_SIZE, light::SHADOW_MAP_SIZE, light::SHADOW_MAP_SIZE, light::SHADOW_MAP_SIZE };
		static float Z_MULT = 10.f;

		//Do the shadow stuff here
		R2_CHECK(renderer.mnoptrRenderCam != nullptr, "We must have a camera to do this");
		R2_CHECK(renderer.mLightSystem != nullptr, "We must have a light system!");

		glm::vec3 lightProjRadius;
		bool lightingNeedsUpdate = lightsys::Update(*renderer.mLightSystem, *renderer.mnoptrRenderCam, lightProjRadius);



		renderer.mLightSystem->mSceneLighting.useSDSMShadows = USE_SDSM_SHADOWS;

		if (lightingNeedsUpdate)
		{
			UpdateSceneLighting(renderer, *renderer.mLightSystem);
		}

		if (USE_SDSM_SHADOWS)
		{

			ClearShadowData(renderer);

			//Update SDSM shadow constants
			if(renderer.mSDSMShaderParamsNeedUpdate)
			{
				renderer.mSDSMShaderParamsNeedUpdate = false;
				UpdateSDSMShaderParams(renderer);

			}

			if (renderer.mFlags.IsSet(eRendererFlags::RENDERER_FLAG_NEEDS_SHADOW_MAPS_REFRESH))
			{
				UpdateShadowMapPages(renderer);
			}


			const u32 numDirectionLights = renderer.mLightSystem->mSceneLighting.numShadowCastingDirectionLights;

			//Dispatch the compute shaders for SDSM shadows
			if (numDirectionLights > 0)
			{
				key::ShadowKey dispatchReduceZBoundsKey = key::GenerateShadowKey(key::ShadowKey::COMPUTE, 0, renderer.mSDSMReduceZBoundsComputeShader, false, light::LightType::LT_DIRECTIONAL_LIGHT, 0);
				key::ShadowKey dispatchCalculateLogPartitionsKey = key::GenerateShadowKey(key::ShadowKey::COMPUTE, 1,  renderer.mSDSMCalculateLogPartitionsComputeShader, false, light::LightType::LT_DIRECTIONAL_LIGHT, 0);

				int dispatchWidth = (renderer.mResolutionSize.width + REDUCE_TILE_DIM - 1) / REDUCE_TILE_DIM;
				int dispatchHeight = (renderer.mResolutionSize.height + REDUCE_TILE_DIM - 1) / REDUCE_TILE_DIM;

				cmd::DispatchCompute* dispatchReduceZBoundsCMD = AddCommand<key::ShadowKey, cmd::DispatchCompute, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, dispatchReduceZBoundsKey, 0);
				dispatchReduceZBoundsCMD->numGroupsX = dispatchWidth;
				dispatchReduceZBoundsCMD->numGroupsY = dispatchHeight;
				dispatchReduceZBoundsCMD->numGroupsZ = 1;

				cmd::Barrier* barrierCMD = AppendCommand<cmd::DispatchCompute, cmd::Barrier, mem::StackArena>(*renderer.mShadowArena, dispatchReduceZBoundsCMD, 0);
				barrierCMD->flags = cmd::SHADER_STORAGE_BARRIER_BIT;

				cmd::DispatchCompute* calculateLogPartitionsCMD = AddCommand<key::ShadowKey, cmd::DispatchCompute, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, dispatchCalculateLogPartitionsKey, 0);
				calculateLogPartitionsCMD->numGroupsX = 1;
				calculateLogPartitionsCMD->numGroupsY = 1;
				calculateLogPartitionsCMD->numGroupsZ = 1;

				cmd::Barrier* barrierCMD2 = AppendCommand<cmd::DispatchCompute, cmd::Barrier, mem::StackArena>(*renderer.mShadowArena, calculateLogPartitionsCMD, 0);
				barrierCMD2->flags = cmd::SHADER_STORAGE_BARRIER_BIT;
				
				key::ShadowKey dispatchShadowSDSMKey = key::GenerateShadowKey(key::ShadowKey::COMPUTE, 4, renderer.mShadowSDSMComputeShader, false, light::LightType::LT_DIRECTIONAL_LIGHT, 0);

				cmd::DispatchCompute* dispatchCMD = AddCommand<key::ShadowKey, cmd::DispatchCompute, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, dispatchShadowSDSMKey, 0);

				dispatchCMD->numGroupsX = numDirectionLights;
				dispatchCMD->numGroupsY = 1;
				dispatchCMD->numGroupsZ = 1;

				cmd::Barrier* splitShadowsBarrierCMD = AppendCommand<cmd::DispatchCompute, cmd::Barrier, mem::StackArena>(*renderer.mShadowArena, dispatchCMD, 0);
				splitShadowsBarrierCMD->flags = cmd::SHADER_STORAGE_BARRIER_BIT;
				
			}

			const u32 numSpotLights = renderer.mLightSystem->mSceneLighting.numShadowCastingSpotLights;

			if (numSpotLights > 0)
			{
				key::ShadowKey dispatchSpotLightShadowMatricesKey = key::GenerateShadowKey(key::ShadowKey::COMPUTE, 5, renderer.mSpotLightLightMatrixShader, false, light::LightType::LT_SPOT_LIGHT, 0);

				cmd::DispatchCompute* dispatchCMD = AddCommand<key::ShadowKey, cmd::DispatchCompute, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, dispatchSpotLightShadowMatricesKey, 0);
				dispatchCMD->numGroupsX = numSpotLights;
				dispatchCMD->numGroupsY = 1;
				dispatchCMD->numGroupsZ = 1;

				cmd::Barrier* spotLightBarrierCMD = AppendCommand<cmd::DispatchCompute, cmd::Barrier, mem::StackArena>(*renderer.mShadowArena, dispatchCMD, 0);
				spotLightBarrierCMD->flags = cmd::SHADER_STORAGE_BARRIER_BIT;
			}

			const u32 numPointLights = renderer.mLightSystem->mSceneLighting.numShadowCastingPointLights;

			if (numPointLights > 0)
			{
				key::ShadowKey dispatchPointLightShadowMatricesKey = key::GenerateShadowKey(key::ShadowKey::COMPUTE, 5, renderer.mPointLightLightMatrixShader, false, light::LightType::LT_POINT_LIGHT, 0);

				cmd::DispatchCompute* dispatchCMD = AddCommand<key::ShadowKey, cmd::DispatchCompute, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, dispatchPointLightShadowMatricesKey, 0);
				dispatchCMD->numGroupsX = numPointLights;
				dispatchCMD->numGroupsY = 1;
				dispatchCMD->numGroupsZ = 1;

				cmd::Barrier* pointLightBarrierCMD = AppendCommand<cmd::DispatchCompute, cmd::Barrier, mem::StackArena>(*renderer.mShadowArena, dispatchCMD, 0);
				pointLightBarrierCMD->flags = cmd::SHADER_STORAGE_BARRIER_BIT;
			}

		}
		else
		{
			//normal custom PSSM shadows

			//add the commands to split the shadow frustum in the compute shader
			key::ShadowKey dispatchShadowSplitsKey = key::GenerateShadowKey(key::ShadowKey::COMPUTE, 0, renderer.mShadowSplitComputeShader, false, light::LightType::LT_DIRECTIONAL_LIGHT, 0);

			cmd::DispatchCompute* dispatchCMD = AddCommand<key::ShadowKey, cmd::DispatchCompute, mem::StackArena>(*renderer.mShadowArena, *renderer.mShadowBucket, dispatchShadowSplitsKey, 0);

			dispatchCMD->numGroupsX = 1;
			dispatchCMD->numGroupsY = 1;
			dispatchCMD->numGroupsZ = 1;

			cmd::Barrier* barrierCMD = AppendCommand<cmd::DispatchCompute, cmd::Barrier, mem::StackArena>(*renderer.mShadowArena, dispatchCMD, 0);
			barrierCMD->flags = cmd::SHADER_STORAGE_BARRIER_BIT;
		}
	}

	glm::vec2 GetJitter(Renderer& renderer, const u64 frameCount, bool isTAA)
	{
		auto HaltonSeq = [](s32 prime, s32 idx)
		{
			f32 r = 0;
			f32 f = 1;

			while (idx > 0) {
				f /= prime;
				f += f * (idx % prime);
				idx /= prime;
			}

			return r;
		};

		if (isTAA)
		{
			f32 u = HaltonSeq(2, (frameCount % 8) + 1) - 0.5f;
			f32 v = HaltonSeq(3, (frameCount % 8) + 1) - 0.5;

			return glm::vec2(u, v) * glm::vec2(1.0f / float(renderer.mResolutionSize.width), 1.0f / float(renderer.mResolutionSize.height)) * 2.0f;
		}
		else
		{
			return glm::vec2(0);
		}
	}

	u32 GetCameraDepth(Renderer& renderer, const r2::draw::Bounds& meshBounds, const glm::mat4& modelMat)
	{
		glm::vec3 worldSpacePosition;
		glm::vec3 origin = meshBounds.origin;
		//@TODO(Serge): need a fast way of doing this properlyD
		worldSpacePosition.x = modelMat[3][0];//modelMat[0][0] * origin.x + modelMat[1][0] * origin.y + modelMat[2][0] * origin.z + modelMat[3][0];
		worldSpacePosition.y = modelMat[3][1];//modelMat[0][1] * origin.x + modelMat[1][1] * origin.y + modelMat[2][1] * origin.z + modelMat[3][1];
		worldSpacePosition.z = modelMat[3][2];//modelMat[0][2] * origin.x + modelMat[1][2] * origin.y + modelMat[2][2] * origin.z + modelMat[3][2];

		return r2::util::FloatToU24(GetSortKeyCameraDepth(renderer, worldSpacePosition));
	}

	f32 GetSortKeyCameraDepth(Renderer& renderer, glm::vec3 aabbPosition)
	{
		R2_CHECK(renderer.mnoptrRenderCam != nullptr, "We need to have the render camera!");

		glm::vec3 diff = aabbPosition - renderer.mnoptrRenderCam->position;

		return glm::dot(diff, diff);
	}
}

//The actual implementation for apps - I dunno if I like how we're doing this ATM
namespace r2::draw::renderer
{
	const Model* GetDefaultModel(r2::draw::DefaultModel defaultModel)
	{
		return GetDefaultModel(MENG.GetCurrentRendererRef(), defaultModel);
	}

	

	const r2::SArray<vb::GPUModelRefHandle>* GetDefaultModelRefs()
	{
		return GetDefaultModelRefs(MENG.GetCurrentRendererRef());
	}

	vb::GPUModelRefHandle GetDefaultModelRef(r2::draw::DefaultModel defaultModel)
	{
		return GetDefaultModelRef(MENG.GetCurrentRendererRef(), defaultModel);
	}

	vb::GPUModelRefHandle GetModelRefHandleForModelAssetName(const r2::asset::AssetName& modelAssetName)
	{
		return GetModelRefHandleForModelAssetName(MENG.GetCurrentRendererRef(), modelAssetName);
	}

	vb::GPUModelRefHandle UploadModel(const Model* model)
	{
		return UploadModel(MENG.GetCurrentRendererRef(), model);
	}

	void UploadModels(const r2::SArray<const Model*>& models, r2::SArray<vb::GPUModelRefHandle>& modelRefs)
	{
		UploadModels(MENG.GetCurrentRendererRef(), models, modelRefs);
	}

	//vb::GPUModelRefHandle UploadAnimModel(const AnimModel* model)
	//{
	//	return UploadAnimModel(MENG.GetCurrentRendererRef(), model);
	//}

	//void UploadAnimModels(const r2::SArray<const AnimModel*>& models, r2::SArray<vb::GPUModelRefHandle>& modelRefs)
	//{
	//	UploadAnimModels(MENG.GetCurrentRendererRef(), models, modelRefs);
	//}

	void UnloadModel(const vb::GPUModelRefHandle& modelRefHandle)
	{
		UnloadModel(MENG.GetCurrentRendererRef(), modelRefHandle);
	}

	void UnloadStaticModelRefHandles(const r2::SArray<vb::GPUModelRefHandle>* handles)
	{
		UnloadStaticModelRefHandles(MENG.GetCurrentRendererRef(), handles);
	}

	void UnloadAnimModelRefHandles(const r2::SArray<vb::GPUModelRefHandle>* handles)
	{
		UnloadAnimModelRefHandles(MENG.GetCurrentRendererRef(), handles);
	}

	void UnloadAllStaticModels()
	{
		UnloadAllStaticModels(MENG.GetCurrentRendererRef());
	}

	void UnloadAllAnimModels()
	{
		UnloadAllAnimModels(MENG.GetCurrentRendererRef());
	}

	bool IsModelLoaded(const vb::GPUModelRefHandle& modelRefHandle)
	{
		return IsModelLoaded(MENG.GetCurrentRendererRef(), modelRefHandle);
	}

	bool IsModelRefHandleValid(const vb::GPUModelRefHandle& modelRefHandle)
	{
		return IsModelRefHandleValid(MENG.GetCurrentRendererRef(), modelRefHandle);
	}

	r2::draw::RenderMaterialCache* GetRenderMaterialCache()
	{
		return GetRenderMaterialCache(MENG.GetCurrentRendererRef());
	}
	/*
	r2::draw::ShaderHandle GetDefaultOutlineShaderHandle(bool isStatic)
	{
		return GetDefaultOutlineShaderHandle(MENG.GetCurrentRendererRef(), isStatic);
	}

	const r2::draw::RenderMaterialParams& GetDefaultOutlineRenderMaterialParams(bool isStatic)
	{
		return GetDefaultOutlineRenderMaterialParams(MENG.GetCurrentRendererRef(), isStatic);
	}
	*/

	r2::mat::MaterialName GetDefaultOutlineMaterialName(bool alphaDiscard)
	{
		return  GetDefaultOutlineMaterialName(MENG.GetCurrentRendererRef(), alphaDiscard);
	}

	const RenderMaterialParams& GetMissingTextureRenderMaterialParam()
	{
		return GetMissingTextureRenderMaterialParam(MENG.GetCurrentRendererRef());
	}

	const tex::Texture* GetMissingTexture()
	{
		return GetMissingTexture(MENG.GetCurrentRendererPtr());
	}

	const RenderMaterialParams& GetBlueNoise64TextureMaterialParam()
	{
		return GetBlueNoise64TextureMaterialParam(MENG.GetCurrentRendererRef());
	}

	const tex::Texture* GetBlueNoise64Texture()
	{
		return GetBlueNoise64Texture(MENG.GetCurrentRendererPtr());
	}

	void DrawModel(
		const DrawParameters& drawParameters,
		const vb::GPUModelRefHandle& modelRefHandles,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<r2::mat::MaterialName>& materialsPerMesh,
		const r2::SArray<ShaderBoneTransform>* boneTransforms)
	{
		//0 is then invalid entity - if we change that - we need to change this
		//Also we should not be doing this silly emptyInstances thing!
		r2::SArray<s32>* emptyEntityInstances = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, s32, 1);
		DrawModel(MENG.GetCurrentRendererRef(), 0, *emptyEntityInstances, drawParameters, modelRefHandles, modelMatrices, materialsPerMesh, boneTransforms);
		FREE(emptyEntityInstances, *MEM_ENG_SCRATCH_PTR);
	}

	void DrawModels(
		const DrawParameters& drawParameters,
		const r2::SArray<vb::GPUModelRefHandle>& modelRefHandles,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<u32>& numInstancesPerModel,
		const r2::SArray<r2::mat::MaterialName>& materialsPerMesh,
		const r2::SArray<ShaderBoneTransform>* boneTransforms)
	{
		r2::SArray<u32>* entities = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u32, r2::sarr::Size(modelRefHandles));
		r2::SArray<u32>* numEntityInstancesPerEntity = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u32, r2::sarr::Size(modelRefHandles));
		r2::SArray<s32>* emptyEntityInstances = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, s32, 1);

		r2::sarr::Fill(*entities, 0u);
		r2::sarr::Fill(*numEntityInstancesPerEntity, 1u);

		DrawModels(MENG.GetCurrentRendererRef(), *entities, *emptyEntityInstances, *numEntityInstancesPerEntity, drawParameters, modelRefHandles, modelMatrices, numInstancesPerModel, materialsPerMesh, boneTransforms);

		FREE(emptyEntityInstances, *MEM_ENG_SCRATCH_PTR);
		FREE(numEntityInstancesPerEntity, *MEM_ENG_SCRATCH_PTR);
		FREE(entities, *MEM_ENG_SCRATCH_PTR);
	}

	void SetDefaultStencilState(DrawParameters& drawParameters)
	{
		cmd::SetDefaultStencilState(drawParameters.stencilState);
	}

	void SetDefaultBlendState(DrawParameters& drawParameters)
	{
		cmd::SetDefaultBlendState(drawParameters.blendState);
	}

	void SetDefaultCullState(DrawParameters& drawParameters)
	{
		cmd::SetDefaultCullState(drawParameters.cullState);
	}

	void SetDefaultDrawParameters(DrawParameters& drawParameters)
	{
		cmd::SetDefaultStencilState(drawParameters.stencilState);
		cmd::SetDefaultBlendState(drawParameters.blendState);
		cmd::SetDefaultCullState(drawParameters.cullState);
	}

	///More draw functions...
	ShaderHandle GetDepthShaderHandle(bool isDynamic)
	{
		return GetDepthShaderHandle(MENG.GetCurrentRendererRef(), isDynamic);
	}

	ShaderHandle GetShadowDepthShaderHandle(bool isDynamic, light::LightType lightType)
	{
		return GetShadowDepthShaderHandle(MENG.GetCurrentRendererRef(), isDynamic, lightType);
	}

	const vb::GPUModelRef* GetGPUModelRef(const vb::GPUModelRefHandle& handle)
	{
		return GetGPUModelRef(MENG.GetCurrentRendererRef(), handle);
	}

	vb::VertexBufferLayoutSize GetStaticVertexBufferRemainingSize()
	{
		return GetStaticVertexBufferRemainingSize(MENG.GetCurrentRendererRef());
	}

	vb::VertexBufferLayoutSize GetAnimVertexBufferRemainingSize()
	{
		return GetAnimVertexBufferRemainingSize(MENG.GetCurrentRendererRef());
	}

	vb::VertexBufferLayoutSize GetStaticVertexBufferSize()
	{
		return GetStaticVertexBufferSize(MENG.GetCurrentRendererRef());
	}

	vb::VertexBufferLayoutSize GetAnimVertexBufferSize()
	{
		return GetAnimVertexBufferSize(MENG.GetCurrentRendererRef());
	}

	vb::VertexBufferLayoutSize GetStaticVertexBufferCapacity()
	{
		return GetStaticVertexBufferCapacity(MENG.GetCurrentRendererRef());
	}

	vb::VertexBufferLayoutSize GetAnimVertexBufferCapacity()
	{
		return GetAnimVertexBufferCapacity(MENG.GetCurrentRendererRef());
	}

	u32 GetMaxNumModelsLoadedAtOneTimePerLayout()
	{
		return MAX_NUMBER_OF_MODELS_LOADED_AT_ONE_TIME;
	}

	u32 GetAVGMaxNumMeshesPerModel()
	{
		return AVG_NUM_OF_MESHES_PER_MODEL;
	}

	u32 GetMaxNumInstances()
	{
		return MAX_NUM_DRAWS;
	}

	u32 GetMaxNumBones()
	{
		return MAX_NUM_BONES;
	}

#ifdef R2_EDITOR

	const r2::draw::Model* GetDefaultModel(const r2::asset::AssetName& assetName)
	{
		return GetDefaultModel(MENG.GetCurrentRendererRef(), assetName);
	}

	EntityInstance ReadEntityInstanceAtMousePosition(s32 x, s32 y)
	{
		return ReadEntityInstanceAtMousePosition(MENG.GetCurrentRendererRef(), x, y);
	}

	void DrawModelEntity(
		u32 entity,
		const r2::SArray<s32>& entityInstances,
		const DrawParameters& drawParameters,
		const vb::GPUModelRefHandle& modelRefHandles,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<r2::mat::MaterialName>& materialsPerMesh,
		const r2::SArray<ShaderBoneTransform>* boneTransforms)
	{
		R2_CHECK(entityInstances.mSize == modelMatrices.mSize, "Should always be true");
		DrawModel(MENG.GetCurrentRendererRef(), entity, entityInstances, drawParameters, modelRefHandles, modelMatrices, materialsPerMesh, boneTransforms);
	}

	void DrawModelEntities(
		const r2::SArray<u32>& entities,
		const r2::SArray<s32>& entityInstances,
		const r2::SArray<u32>& numEntityInstancesPerEntity,
		const DrawParameters& drawParameters,
		const r2::SArray<vb::GPUModelRefHandle>& modelRefHandles,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<u32>& numInstancesPerModel,
		const r2::SArray<r2::mat::MaterialName>& materialNamesPerMesh,
		const r2::SArray<ShaderBoneTransform>* boneTransforms)
	{
		R2_CHECK(entityInstances.mSize == modelMatrices.mSize, "Should always be true");

		DrawModels(MENG.GetCurrentRendererRef(), entities, entityInstances, numEntityInstancesPerEntity, drawParameters, modelRefHandles, modelMatrices, numInstancesPerModel, materialNamesPerMesh, boneTransforms);
	}

	void DrawGrid()
	{
		DrawQuad(MENG.GetCurrentRendererRef(), glm::mat4(2), flat::eMeshPass_TRANSPARENT, MENG.GetCurrentRendererRef().mGridShader, true);
	}

	void UpdateShadowsForDirectionLight(DirectionLightHandle dirLightHandle)
	{
		UpdateShadowsForDirectionLight(MENG.GetCurrentRendererRef(), dirLightHandle);
	}

	void UpdateShadowsForPointLight(PointLightHandle pointLightHandle)
	{
		UpdateShadowsForPointLight(MENG.GetCurrentRendererRef(), pointLightHandle);
	}

	void UpdateShadowsForSpotLight(SpotLightHandle spotLightHandle)
	{
		UpdateShadowsForSpotLight(MENG.GetCurrentRendererRef(), spotLightHandle);
	}

#endif

	void SetRenderCamera(Camera* cameraPtr)
	{
		SetRenderCamera(MENG.GetCurrentRendererRef(), cameraPtr);
	}

	Camera* GetRenderCamera()
	{
		return GetRenderCamera(MENG.GetCurrentRendererRef());
	}

	void SetOutputMergerType(OutputMerger outputMerger)
	{
		SetOutputMergerType(MENG.GetCurrentRendererRef(), outputMerger);
	}

	DirectionLightHandle AddDirectionLight(const DirectionLight& light)
	{
		return AddDirectionLight(MENG.GetCurrentRendererRef(), light);
	}

	PointLightHandle AddPointLight(const PointLight& pointLight)
	{
		return AddPointLight(MENG.GetCurrentRendererRef(), pointLight);
	}

	SpotLightHandle AddSpotLight(const SpotLight& spotLight)
	{
		return AddSpotLight(MENG.GetCurrentRendererRef(), spotLight);
	}

	SkyLightHandle AddSkyLight(const SkyLight& skylight, s32 numPrefilteredMips)
	{
		return AddSkyLight(MENG.GetCurrentRendererRef(), skylight, numPrefilteredMips);
	}

	SkyLightHandle AddSkyLight(const RenderMaterialParams& diffuseMaterial, const RenderMaterialParams& prefilteredMaterial, const RenderMaterialParams& lutDFG, s32 numMips)
	{
		return AddSkyLight(MENG.GetCurrentRendererRef(), diffuseMaterial, prefilteredMaterial, lutDFG, numMips);
	}

	const DirectionLight* GetDirectionLightConstPtr(DirectionLightHandle dirLightHandle)
	{
		return GetDirectionLightConstPtr(MENG.GetCurrentRendererRef(), dirLightHandle);
	}

	DirectionLight* GetDirectionLightPtr(DirectionLightHandle dirLightHandle)
	{
		return GetDirectionLightPtr(MENG.GetCurrentRendererRef(), dirLightHandle);
	}

	const PointLight* GetPointLightConstPtr(PointLightHandle pointLightHandle)
	{
		return GetPointLightConstPtr(MENG.GetCurrentRendererRef(), pointLightHandle);
	}

	PointLight* GetPointLightPtr(PointLightHandle pointLightHandle)
	{
		return GetPointLightPtr(MENG.GetCurrentRendererRef(), pointLightHandle);
	}

	const SpotLight* GetSpotLightConstPtr(SpotLightHandle spotLightHandle)
	{
		return GetSpotLightConstPtr(MENG.GetCurrentRendererRef(), spotLightHandle);
	}

	SpotLight* GetSpotLightPtr(SpotLightHandle spotLightHandle)
	{
		return GetSpotLightPtr(MENG.GetCurrentRendererRef(), spotLightHandle);
	}

	const SkyLight* GetSkyLightConstPtr(SkyLightHandle skyLightHandle)
	{
		return GetSkyLightConstPtr(MENG.GetCurrentRendererRef(), skyLightHandle);
	}

	SkyLight* GetSkyLightPtr(SkyLightHandle skyLightHandle)
	{
		return GetSkyLightPtr(MENG.GetCurrentRendererRef(), skyLightHandle);
	}

	bool HasSkylight()
	{
		return HasSkylight(MENG.GetCurrentRendererRef());
	}

	SkyLightHandle GetCurrentSkylightHandle()
	{
		return GetCurrentSkylightHandle(MENG.GetCurrentRendererRef());
	}

	//@TODO(Serge): add the get light properties functions here

	void RemoveDirectionLight(DirectionLightHandle dirLightHandle)
	{
		RemoveDirectionLight(MENG.GetCurrentRendererRef(), dirLightHandle);
	}

	void RemovePointLight(PointLightHandle pointLightHandle)
	{
		RemovePointLight(MENG.GetCurrentRendererRef(), pointLightHandle);
	}

	void RemoveSpotLight(SpotLightHandle spotLightHandle)
	{
		RemoveSpotLight(MENG.GetCurrentRendererRef(), spotLightHandle);
	}

	void RemoveSkyLight(SkyLightHandle skylightHandle)
	{
		RemoveSkyLight(MENG.GetCurrentRendererRef(), skylightHandle);
	}

	void ClearAllLighting()
	{
		ClearAllLighting(MENG.GetCurrentRendererRef());
	}

	void SetColorCorrection(const ColorCorrection& cc)
	{
		SetColorCorrection(MENG.GetCurrentRendererRef(), cc);
	}

	void SetColorGradingLUT(const tex::Texture* lut, u32 numSwatches)
	{
		R2_CHECK(numSwatches > 0, "You must have one or more swatches in your texture!");
		R2_CHECK(lut != nullptr, "Your lut texture is nullptr!");

		const tex::TextureHandle* textureHandle = texsys::GetTextureHandle(*lut);

		R2_CHECK(textureHandle != nullptr, "Texture handle is nullptr!");

		float halfColX = 0.5f / static_cast<f32>(textureHandle->container->format.width);
		float halfColY = 0.5f / static_cast<f32>(textureHandle->container->format.height);

		SetColorGradingLUT(MENG.GetCurrentRendererRef(), texsys::GetTextureAddress(*lut), halfColX, halfColY, static_cast<f32>( numSwatches));
	}

	void EnableColorGrading(bool isEnabled)
	{
		EnableColorGrading(MENG.GetCurrentRendererRef(), isEnabled);
	}

	void SetColorGradingContribution(float contribution)
	{
		SetColorGradingContribution(MENG.GetCurrentRendererRef(), contribution);
	}

	bool IsColorGradingEnabled()
	{
		return IsColorGradingEnabled(MENG.GetCurrentRendererRef());
	}

	float GetColorGradingContribution()
	{
		return GetColorGradingContribution(MENG.GetCurrentRendererRef());
	}

	//------------------------------------------------------------------------------

#ifdef R2_DEBUG

	void DrawDebugBones(const r2::SArray<DebugBone>& bones, const glm::mat4& modelMatrix, const glm::vec4& color)
	{
		DrawDebugBones(MENG.GetCurrentRendererRef(), bones, modelMatrix, color);
	}

	void DrawDebugBones(
		const r2::SArray<DebugBone>& bones,
		const r2::SArray<u64>& numBonesPerModel,
		const r2::SArray<glm::mat4>& numModelMats,
		const glm::vec4& color)
	{
		DrawDebugBones(MENG.GetCurrentRendererRef(), bones, numBonesPerModel, numModelMats, color);
	}

	void DrawQuad(const glm::vec3& center, glm::vec2 scale, const glm::vec3& normal, const glm::vec4& color, bool filled, bool depthTest)
	{
		DrawQuad(MENG.GetCurrentRendererRef(), center, scale, normal, color, filled, depthTest);
	}

	void DrawSphere(const glm::vec3& center, float radius, const glm::vec4& color, bool filled, bool depthTest)
	{
		DrawSphere(MENG.GetCurrentRendererRef(), center, radius, color, filled, depthTest);
	}

	void DrawCube(const glm::vec3& center, float scale, const glm::vec4& color, bool filled, bool depthTest)
	{
		DrawCube(MENG.GetCurrentRendererRef(), center, scale, color, filled, depthTest);
	}

	void DrawCylinder(const glm::vec3& basePosition, const glm::vec3& dir, float radius, float height, const glm::vec4& color, bool filled, bool depthTest)
	{
		DrawCylinder(MENG.GetCurrentRendererRef(), basePosition, dir, radius, height, color, filled, depthTest);
	}

	void DrawCone(const glm::vec3& basePosition, const glm::vec3& dir, float radius, float height, const glm::vec4& color, bool filled, bool depthTest)
	{
		DrawCone(MENG.GetCurrentRendererRef(), basePosition, dir, radius, height, color, filled, depthTest);
	}

	void DrawArrow(const glm::vec3& basePosition, const glm::vec3& dir, float length, float headBaseRadius, const glm::vec4& color, bool filled, bool depthTest)
	{
		DrawArrow(MENG.GetCurrentRendererRef(), basePosition, dir, length, headBaseRadius, color, filled, depthTest);
	}

	void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, bool disableDepth)
	{
		DrawLine(MENG.GetCurrentRendererRef(), p0, p1, color, disableDepth);
	}

	void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::mat4& modelMat, const glm::vec4& color, bool depthTest)
	{
		DrawLine(MENG.GetCurrentRendererRef(), p0, p1, modelMat, color, depthTest);
	}

	void DrawTangentVectors(DefaultModel model, const glm::mat4& transform)
	{
		DrawTangentVectors(MENG.GetCurrentRendererRef(), model, transform);
	}

	//instanced methods

	void DrawQuadInstanced(
		const r2::SArray<glm::vec3>& centers,
		const r2::SArray<glm::vec2>& scales,
		const r2::SArray<glm::vec3>& normals,
		const r2::SArray<glm::vec4>& colors, 
		bool filled,
		bool depthTest)
	{
		DrawQuadInstanced(MENG.GetCurrentRendererRef(), centers, scales, normals, colors, filled, depthTest);
	}

	void DrawSphereInstanced(
		const r2::SArray<glm::vec3>& centers,
		const r2::SArray<float>& radii,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest)
	{
		DrawSphereInstanced(MENG.GetCurrentRendererRef(), centers, radii, colors, filled, depthTest);
	}

	void DrawCubeInstanced(
		const r2::SArray<glm::vec3>& centers,
		const r2::SArray<float>& scales,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest)
	{
		DrawCubeInstanced(MENG.GetCurrentRendererRef(), centers, scales, colors, filled, depthTest);
	}

	void DrawCylinderInstanced(
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& radii,
		const r2::SArray<float>& heights,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest)
	{
		DrawCylinderInstanced(MENG.GetCurrentRendererRef(), basePositions, directions, radii, heights, colors, filled, depthTest);
	}

	void DrawConeInstanced(
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& radii,
		const r2::SArray<float>& heights,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest)
	{
		DrawConeInstanced(MENG.GetCurrentRendererRef(), basePositions, directions, radii, heights, colors, filled, depthTest);
	}

	void DrawArrowInstanced(
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& lengths,
		const r2::SArray<float>& headBaseRadii,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest)
	{
		DrawArrowInstanced(MENG.GetCurrentRendererRef(), basePositions, directions, lengths, headBaseRadii, colors, filled, depthTest);
	}

#endif
}