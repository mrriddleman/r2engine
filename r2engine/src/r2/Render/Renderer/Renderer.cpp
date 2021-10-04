#include "r2pch.h"

#include "r2/Core/File/FileSystem.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/ModelAssetLoader.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Render/Renderer/RendererImpl.h"
#include "r2/Render/Renderer/CommandPacket.h"
#include "r2/Render/Renderer/CommandBucket.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Renderer/Commands.h"
#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Model/Material.h"
#include "r2/Render/Renderer/ShaderSystem.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Model/ModelSystem.h"
#include "r2/Render/Model/Light.h"
#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Utils/Hash.h"
#include <filesystem>
//#include "r2/Render/Renderer/CommandPacket.h"
#include "r2/Core/Math/MathUtils.h"

#include "r2/Render/Model/Material_generated.h"
#include "r2/Render/Model/MaterialPack_generated.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "glm/gtc/type_ptr.hpp"


namespace r2::draw::cmd
{
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
}

namespace r2::draw
{
	struct MaterialBatch
	{
		struct Info
		{
			s32 start = -1;
			s32 numMaterials = 0;
		};

		r2::SArray<Info>* infos = nullptr;
		r2::SArray<MaterialHandle>* materialHandles = nullptr;
	};

	struct ConstantBufferData
	{
		r2::draw::ConstantBufferHandle handle = EMPTY_BUFFER;
		r2::draw::ConstantBufferLayout::Type type = r2::draw::ConstantBufferLayout::Type::Small;
		b32 isPersistent = false;
		u64 bufferSize = 0;
		u64 currentOffset = 0;

		void AddDataSize(u64 size);
	};

	void ConstantBufferData::AddDataSize(u64 size)
	{
		R2_CHECK(size <= bufferSize, "We're adding too much to this buffer. We're trying to add: %llu bytes to a %llu sized buffer", size, bufferSize);

		if (currentOffset + size > bufferSize)
		{
			currentOffset = (currentOffset + size) % bufferSize;
		}

		currentOffset += size;
	}

	struct VertexLayoutConfigHandle
	{
		BufferLayoutHandle mBufferLayoutHandle;
		VertexBufferHandle mVertexBufferHandles[BufferLayoutConfiguration::MAX_VERTEX_BUFFER_CONFIGS];
		IndexBufferHandle mIndexBufferHandle;
		u32 mNumVertexBufferHandles;
	};

	struct VertexLayoutVertexOffset
	{
		u64 baseVertex = 0;
		u64 numVertices = 0;
	};

	struct VertexLayoutIndexOffset
	{
		u64 baseIndex = 0;
		u64 numIndices = 0;
	};

	struct VertexLayoutUploadOffset
	{
		VertexLayoutVertexOffset mVertexBufferOffset;
		VertexLayoutIndexOffset mIndexBufferOffset;
	};

	struct ClearSurfaceOptions
	{
		b32 shouldClear = false;
		u32 flags = 0;
	};

	struct RenderBatch
	{
		VertexConfigHandle vertexLayoutConfigHandle = InvalidVertexConfigHandle;

		ConstantBufferHandle subCommandsHandle;
		ConstantBufferHandle modelsHandle;
		ConstantBufferHandle materialsHandle;
		ConstantBufferHandle boneTransformOffsetsHandle;
		ConstantBufferHandle boneTransformsHandle;

		r2::SArray<ModelRef>* modelRefs = nullptr;

		MaterialBatch materialBatch;

		r2::SArray<glm::mat4>* models = nullptr;
		r2::SArray<r2::draw::ShaderBoneTransform>* boneTransforms = nullptr;
		r2::SArray<cmd::DrawState>* drawState = nullptr; //stuff to help generate the keys
		
		PrimitiveType primitiveType = PrimitiveType::TRIANGLES;

		static u64 MemorySize(u64 numModels, u64 numModelRefs, u64 numBoneTransforms, u64 alignment, u32 headerSize, u32 boundsChecking);
	};

#ifdef R2_DEBUG

	struct DebugRenderConstants
	{
		glm::vec4 color;
		glm::mat4 modelMatrix;
	};

	struct DebugRenderBatch
	{
		DebugDrawType debugDrawType;

		VertexConfigHandle vertexConfigHandle = InvalidVertexConfigHandle;
		r2::draw::MaterialHandle materialHandle = mat::InvalidMaterial;

		ConstantConfigHandle subCommandsConstantConfigHandle = InvalidConstantConfigHandle;
		ConstantBufferHandle renderDebugConstantsConfigHandle = InvalidConstantConfigHandle;

		r2::SArray<DebugModelType>* debugModelTypesToDraw = nullptr;
		r2::SArray<math::Transform>* transforms = nullptr; //this is for the models - @TODO(Serge): maybe should just convert the transforms in the draw func, but might be more efficient to just do it a big loop so that's what we're doing for now
		r2::SArray<glm::mat4>* matTransforms = nullptr; //this is for the lines
		r2::SArray<glm::vec4>* colors = nullptr;
		r2::SArray<DebugVertex>* vertices = nullptr;
		r2::SArray<DrawFlags>* drawFlags = nullptr;

		static u64 MemorySize(u32 maxDraws, bool hasDebugLines, u64 alignment, u32 headerSize, u32 boundsChecking);
	};

#endif

	struct Renderer
	{
		//memory
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;

		//@TODO(Serge): don't expose this to the outside (or figure out how to remove this)
		//				we should only be exposing/using mVertexLayoutConfigHandles
		r2::draw::BufferHandles mBufferHandles; 
		r2::SArray<r2::draw::ConstantBufferHandle>* mConstantBufferHandles = nullptr;
		r2::SHashMap<ConstantBufferData>* mConstantBufferData = nullptr;


		r2::SArray<VertexLayoutConfigHandle>* mVertexLayoutConfigHandles = nullptr;
		r2::SArray<r2::draw::BufferLayoutConfiguration>* mVertexLayouts = nullptr;
		r2::SArray<r2::draw::ConstantBufferLayoutConfiguration>* mConstantLayouts = nullptr;
		r2::SArray<VertexLayoutUploadOffset>* mVertexLayoutUploadOffsets = nullptr;

		//@TODO(Serge): Maybe should move these to somewhere else?
		r2::SArray<r2::draw::ModelRef>* mEngineModelRefs = nullptr;
		ModelSystem* mModelSystem = nullptr;
		r2::SArray<ModelHandle>* mDefaultModelHandles = nullptr;
		MaterialSystem* mMaterialSystem = nullptr;

		
		
		r2::draw::MaterialHandle mFinalCompositeMaterialHandle;

		VertexConfigHandle mStaticVertexModelConfigHandle = InvalidVertexConfigHandle;
		VertexConfigHandle mAnimVertexModelConfigHandle = InvalidVertexConfigHandle;
		VertexConfigHandle mFinalBatchVertexLayoutConfigHandle = InvalidVertexConfigHandle;

		ConstantConfigHandle mSurfacesConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mModelConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mMaterialConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mSubcommandsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mLightingConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mResolutionConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mBoneTransformOffsetsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mBoneTransformsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mVPMatricesConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mVectorsConfigHandle = InvalidConstantConfigHandle;

		r2::mem::StackArena* mRenderTargetsArena = nullptr;

		util::Size mResolutionSize;
		util::Size mCompositeSize;

		RenderTarget mRenderTargets[NUM_RENDER_TARGET_SURFACES];
		RenderPass* mRenderPasses[NUM_RENDER_PASSES];

	//	RenderTarget mOffscreenRenderTarget;
	//	RenderTarget mScreenRenderTarget;
		

		//@TODO(Serge): Each bucket needs the bucket and an arena for that bucket. We should partition the AUX memory properly
		r2::draw::CommandBucket<r2::draw::key::Basic>* mPreRenderBucket = nullptr;
		r2::draw::CommandBucket<r2::draw::key::Basic>* mPostRenderBucket = nullptr;
		r2::mem::StackArena* mPrePostRenderCommandArena = nullptr;

		r2::draw::CommandBucket<r2::draw::key::Basic>* mCommandBucket = nullptr;
		r2::draw::CommandBucket<r2::draw::key::Basic>* mFinalBucket = nullptr;
		r2::mem::StackArena* mCommandArena = nullptr;

		r2::SArray<RenderBatch>* mRenderBatches = nullptr; //should be size of NUM_DRAW_TYPES

#ifdef R2_DEBUG
		r2::draw::MaterialHandle mDebugLinesMaterialHandle;
		r2::draw::MaterialHandle mDebugModelMaterialHandle;

		VertexConfigHandle mDebugLinesVertexConfigHandle = InvalidVertexConfigHandle;
		VertexConfigHandle mDebugModelVertexConfigHandle = InvalidVertexConfigHandle;

		ConstantConfigHandle mDebugLinesSubCommandsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mDebugModelSubCommandsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mDebugRenderConstantsConfigHandle = InvalidConstantConfigHandle;

		r2::draw::CommandBucket<key::DebugKey>* mDebugCommandBucket = nullptr;
		r2::draw::CommandBucket<key::DebugKey>* mPreDebugCommandBucket = nullptr;
		r2::draw::CommandBucket<key::DebugKey>* mPostDebugCommandBucket = nullptr;
		r2::mem::StackArena* mDebugCommandArena = nullptr;

		r2::SArray<DebugRenderBatch>* mDebugRenderBatches = nullptr;
#endif

	};
}

namespace r2::draw
{

	u64 RenderBatch::MemorySize(u64 numModels, u64 numModelRefs, u64 numBoneTransforms, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		u64 totalBytes =
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<cmd::DrawState>::MemorySize(numModelRefs), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ModelRef>::MemorySize(numModelRefs), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<MaterialBatch::Info>::MemorySize(numModelRefs), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<MaterialHandle>::MemorySize(numModelRefs * MAX_NUM_MESHES), alignment, headerSize, boundsChecking);

		if (numModels > 0)
		{
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<glm::mat4>::MemorySize(numModels), alignment, headerSize, boundsChecking);
		}

		if (numBoneTransforms > 0)
		{
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ShaderBoneTransform>::MemorySize(numBoneTransforms), alignment, headerSize, boundsChecking);
		}

		return totalBytes;
	}

#ifdef R2_DEBUG
	u64 DebugRenderBatch::MemorySize(u32 maxDraws, bool isDebugLines, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		u64 totalBytes =
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<glm::vec4>::MemorySize(maxDraws), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<DrawFlags>::MemorySize(maxDraws), alignment, headerSize, boundsChecking);

		if (isDebugLines)
		{
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<glm::mat4>::MemorySize(maxDraws), alignment, headerSize, boundsChecking);
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<DebugVertex>::MemorySize(maxDraws) * 2, alignment, headerSize, boundsChecking); //*2 because we need double the memory for debug vertices
		}
		else
		{
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<DebugModelType>::MemorySize(maxDraws), alignment, headerSize, boundsChecking);
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<math::Transform>::MemorySize(maxDraws), alignment, headerSize, boundsChecking);
		}

		return totalBytes;
	}
#endif
}

namespace
{
	r2::draw::Renderer* s_optrRenderer = nullptr;

	const u64 COMMAND_CAPACITY = 2048;
	const u64 COMMAND_AUX_MEMORY = Megabytes(16); //I dunno lol
	const u64 ALIGNMENT = 16;
	const u32 MAX_BUFFER_LAYOUTS = 32;
	const u64 MAX_NUM_MATERIALS = 2048;
	const u64 MAX_NUM_SHADERS = 1000;
	const u64 MAX_DEFAULT_MODELS = 16;
	const u64 MAX_NUM_TEXTURES = 2048;
	const u64 MAX_NUM_MATERIAL_SYSTEMS = 16;
	const u64 MAX_NUM_MATERIALS_PER_MATERIAL_SYSTEM = 32;
	
	const u32 MAX_NUM_DRAWS = 2 << 13;

	const u64 MAX_NUM_CONSTANT_BUFFERS = 16; //?
	const u64 MAX_NUM_CONSTANT_BUFFER_LOCKS = MAX_NUM_DRAWS; 

	const u64 MAX_NUM_BONES = MAX_NUM_DRAWS;

#ifdef R2_DEBUG
	const u32 MAX_NUM_DEBUG_DRAW_COMMANDS = MAX_NUM_DRAWS;//Megabytes(4) / sizeof(InternalDebugRenderCommand);
	const u32 MAX_NUM_DEBUG_LINES = MAX_NUM_DRAWS;// Megabytes(8) / (2 * sizeof(DebugVertex));
	const u32 MAX_NUM_DEBUG_MODELS = 5;
	const u64 DEBUG_COMMAND_AUX_MEMORY = Megabytes(4);
#endif

	const std::string MODL_EXT = ".modl";
	const std::string MESH_EXT = ".mesh";

	u64 DefaultModelsMemorySize()
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u64 quadMeshSize = r2::draw::Mesh::MemorySize(4, 6, ALIGNMENT, headerSize, boundsChecking);
		u64 cubeMeshSize = r2::draw::Mesh::MemorySize(24, 36, ALIGNMENT, headerSize, boundsChecking);
		u64 sphereMeshSize = r2::draw::Mesh::MemorySize(1089, 5952, ALIGNMENT, headerSize, boundsChecking);
		u64 coneMeshSize = r2::draw::Mesh::MemorySize(148, 144 * 3, ALIGNMENT, headerSize, boundsChecking);
		u64 cylinderMeshSize = r2::draw::Mesh::MemorySize(148, 144 * 3, ALIGNMENT, headerSize, boundsChecking);
		u64 modelSize = r2::draw::Model::ModelMemorySize(1, ALIGNMENT, headerSize, boundsChecking) * r2::draw::NUM_DEFAULT_MODELS;

		return quadMeshSize + cubeMeshSize + sphereMeshSize + coneMeshSize + cylinderMeshSize + modelSize;
	}

	bool LoadEngineModels()
	{
		if (s_optrRenderer == nullptr || s_optrRenderer->mModelSystem == nullptr)
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
		SKYBOX
		*/

		r2::SArray<r2::asset::Asset>* defaultModels = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::asset::Asset, MAX_DEFAULT_MODELS); 

		r2::sarr::Push(*defaultModels, r2::asset::Asset("Quad.modl", r2::asset::MODEL));
		r2::sarr::Push(*defaultModels, r2::asset::Asset("Cube.modl", r2::asset::MODEL));
		r2::sarr::Push(*defaultModels, r2::asset::Asset("Sphere.modl", r2::asset::MODEL));
		r2::sarr::Push(*defaultModels, r2::asset::Asset("Cone.modl", r2::asset::MODEL));
		r2::sarr::Push(*defaultModels, r2::asset::Asset("Cylinder.modl", r2::asset::MODEL));
		r2::sarr::Push(*defaultModels, r2::asset::Asset("Skybox.modl", r2::asset::MODEL));

		r2::draw::modlsys::LoadMeshes(s_optrRenderer->mModelSystem, *defaultModels, *s_optrRenderer->mDefaultModelHandles);

		FREE(defaultModels, *MEM_ENG_SCRATCH_PTR);

		return true;
	}
}

namespace r2::draw::renderer
{
	ConstantBufferData* GetConstData(ConstantBufferHandle handle);
	ConstantBufferData* GetConstDataByConfigHandle(ConstantConfigHandle handle);

	BufferHandles& GetVertexBufferHandles();
	const r2::SArray<r2::draw::ConstantBufferHandle>* GetConstantBufferHandles();

	void UploadEngineModels(VertexConfigHandle vertexConfigHandle);

	bool GenerateLayouts();

	VertexConfigHandle AddStaticModelLayout(const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize);
	VertexConfigHandle AddAnimatedModelLayout(const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize);

	//@TODO(Serge): hide maybe? We need to re-thing how we expose layouts to the game
	ConstantConfigHandle AddConstantBufferLayout(ConstantBufferLayout::Type type, const std::initializer_list<ConstantBufferElement>& elements);
	ConstantConfigHandle AddModelsLayout(ConstantBufferLayout::Type type);
	ConstantConfigHandle AddMaterialLayout();
	ConstantConfigHandle AddSubCommandsLayout(); //we can use this for the debug 
	ConstantConfigHandle AddBoneTransformsLayout();
	ConstantConfigHandle AddBoneTransformOffsetsLayout();
	ConstantConfigHandle AddLightingLayout();
	ConstantConfigHandle AddSurfacesLayout();

	void InitializeVertexLayouts(Renderer& renderer, u32 staticVertexLayoutSizeInBytes, u32 animVertexLayoutSizeInBytes);

	u64 MaterialSystemMemorySize(u64 numMaterials, u64 textureCacheInBytes, u64 totalNumberOfTextures, u64 numPacks, u64 maxTexturesInAPack);
	bool GenerateBufferLayouts(const r2::SArray<BufferLayoutConfiguration>* layouts);
	bool GenerateConstantBuffers(const r2::SArray<ConstantBufferLayoutConfiguration>* constantBufferConfigs);
	r2::draw::cmd::Clear* AddClearCommand(CommandBucket<key::Basic>& bucket, r2::draw::key::Basic key);
	template<class ARENA>
	r2::draw::cmd::FillConstantBuffer* AddFillConstantBufferCommand(ARENA& arena, CommandBucket<key::Basic>& bucket, r2::draw::key::Basic key, u64 auxMemory);
	ModelRef UploadModelInternal(const Model* model, r2::SArray<BoneData>* boneData, r2::SArray<BoneInfo>* boneInfo, VertexConfigHandle vHandle);
	u64 AddFillConstantBufferCommandForData(ConstantBufferHandle handle, u64 elementIndex, const void* data);


	RenderTarget* GetRenderTarget(Renderer& renderer, RenderTargetSurface surface);
	RenderPass* GetRenderPass(Renderer& renderer, RenderPassType renderPassType);

	void CreateRenderPasses(Renderer& renderer);
	void DestroyRenderPasses(Renderer& renderer);

	void BeginRenderPass(Renderer& renderer, RenderPassType renderPass, const ClearSurfaceOptions& clearOptions, const ShaderHandle& shaderHandle, r2::draw::CommandBucket<key::Basic>& commandBucket, mem::StackArena& arena);
	void EndRenderPass(Renderer& renderer, RenderPassType renderPass, r2::draw::CommandBucket<key::Basic>& commandBucket);


	void ResizeRenderSurface(Renderer& renderer, u32 windowWidth, u32 windowHeight, u32 resolutionX, u32 resolutionY, float scaleX, float scaleY, float xOffset, float yOffset);
	void DestroyRenderSurfaces(Renderer& renderer);

	void PreRender(Renderer& renderer);

	void ClearRenderBatches(Renderer& renderer);

#ifdef R2_DEBUG
	void DebugPreRender(Renderer& renderer);
	void ClearDebugRenderData();

	VertexConfigHandle AddDebugDrawLayout();
	ConstantConfigHandle AddDebugLineSubCommandsLayout();
	ConstantConfigHandle AddDebugModelSubCommandsLayout();
	ConstantConfigHandle AddDebugColorsLayout();
#endif


	//basic stuff
	bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle, const char* shaderManifestPath, const char* internalShaderManifestPath)
	{
		R2_CHECK(s_optrRenderer == nullptr, "We've already create the s_optrRenderer - are you trying to initialize more than once?");
		R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "The memoryAreaHandle passed in is invalid!");

		if (memoryAreaHandle == r2::mem::MemoryArea::Invalid ||
			s_optrRenderer != nullptr)
		{
			return false;
		}

		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		//@Temporary
		char materialsPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(R2_ENGINE_INTERNAL_MATERIALS_MANIFESTS_BIN, materialsPath, "engine_material_pack.mpak");

		char texturePackPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(R2_ENGINE_INTERNAL_TEXTURES_MANIFESTS_BIN, texturePackPath, "engine_texture_pack.tman");

		void* materialPackData = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, materialsPath);
		if (!materialPackData)
		{
			R2_CHECK(false, "Failed to read the material pack file: %s", materialsPath);
			return false;
		}

		const flat::MaterialPack* materialPack = flat::GetMaterialPack(materialPackData);

		R2_CHECK(materialPack != nullptr, "Failed to get the material pack from the data!");

		void* texturePacksData = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, texturePackPath);
		if (!texturePacksData)
		{
			R2_CHECK(false, "Failed to read the texture packs file: %s", texturePackPath);
			return false;
		}

		const flat::TexturePacksManifest* texturePacksManifest = flat::GetTexturePacksManifest(texturePacksData);

		R2_CHECK(texturePacksManifest != nullptr, "Failed to get the material pack from the data!");

		u64 materialMemorySystemSize = MaterialSystemMemorySize(materialPack->materials()->size(),
			texturePacksManifest->totalTextureSize(),
			texturePacksManifest->totalNumberOfTextures(),
			texturePacksManifest->texturePacks()->size(),
			texturePacksManifest->maxTexturesInAPack());

		u64 subAreaSize = MemorySize(materialMemorySystemSize);

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

		s_optrRenderer = ALLOC(r2::draw::Renderer, *rendererArena);

		R2_CHECK(s_optrRenderer != nullptr, "We couldn't allocate s_optrRenderer!");

		s_optrRenderer->mMemoryAreaHandle = memoryAreaHandle;
		s_optrRenderer->mSubAreaHandle = subAreaHandle;
		s_optrRenderer->mSubAreaArena = rendererArena;

		

		s_optrRenderer->mBufferHandles.bufferLayoutHandles = MAKE_SARRAY(*rendererArena, r2::draw::BufferLayoutHandle, MAX_BUFFER_LAYOUTS);
		
		R2_CHECK(s_optrRenderer->mBufferHandles.bufferLayoutHandles != nullptr, "We couldn't create the buffer layout handles!");
		
		s_optrRenderer->mBufferHandles.vertexBufferHandles = MAKE_SARRAY(*rendererArena, r2::draw::VertexBufferHandle, MAX_BUFFER_LAYOUTS * BufferLayoutConfiguration::MAX_VERTEX_BUFFER_CONFIGS);
		
		R2_CHECK(s_optrRenderer->mBufferHandles.vertexBufferHandles != nullptr, "We couldn't create the vertex buffer layout handles!");
		
		s_optrRenderer->mBufferHandles.indexBufferHandles = MAKE_SARRAY(*rendererArena, r2::draw::IndexBufferHandle, MAX_BUFFER_LAYOUTS);

		R2_CHECK(s_optrRenderer->mBufferHandles.indexBufferHandles != nullptr, "We couldn't create the index buffer layout handles!");

		s_optrRenderer->mBufferHandles.drawIDHandles = MAKE_SARRAY(*rendererArena, r2::draw::DrawIDHandle, MAX_BUFFER_LAYOUTS);

		R2_CHECK(s_optrRenderer->mBufferHandles.drawIDHandles != nullptr, "We couldn't create the draw id handles");
		
		s_optrRenderer->mConstantBufferHandles = MAKE_SARRAY(*rendererArena, r2::draw::ConstantBufferHandle, MAX_BUFFER_LAYOUTS);
		
		R2_CHECK(s_optrRenderer->mConstantBufferHandles != nullptr, "We couldn't create the constant buffer handles");

		s_optrRenderer->mConstantBufferData = MAKE_SHASHMAP(*rendererArena, ConstantBufferData, MAX_BUFFER_LAYOUTS* r2::SHashMap<ConstantBufferData>::LoadFactorMultiplier());

		R2_CHECK(s_optrRenderer->mConstantBufferData != nullptr, "We couldn't create the constant buffer data!");

		s_optrRenderer->mDefaultModelHandles = MAKE_SARRAY(*rendererArena, ModelHandle, MAX_DEFAULT_MODELS);

		R2_CHECK(s_optrRenderer->mDefaultModelHandles != nullptr, "We couldn't create the default model handles");

		s_optrRenderer->mVertexLayoutConfigHandles = MAKE_SARRAY(*rendererArena, VertexLayoutConfigHandle, MAX_BUFFER_LAYOUTS);

		R2_CHECK(s_optrRenderer->mVertexLayoutConfigHandles != nullptr, "We couldn't create the mVertexLayoutConfigHandles");

		s_optrRenderer->mVertexLayoutUploadOffsets = MAKE_SARRAY(*rendererArena, VertexLayoutUploadOffset, MAX_BUFFER_LAYOUTS);

		R2_CHECK(s_optrRenderer->mVertexLayoutUploadOffsets != nullptr, "We couldn't create the mVertexLayoutUploadOffsets");

		s_optrRenderer->mVertexLayouts = MAKE_SARRAY(*rendererArena, r2::draw::BufferLayoutConfiguration, MAX_BUFFER_LAYOUTS);

		R2_CHECK(s_optrRenderer->mVertexLayouts != nullptr, "We couldn't create the vertex layouts!");

		s_optrRenderer->mConstantLayouts = MAKE_SARRAY(*rendererArena, r2::draw::ConstantBufferLayoutConfiguration, MAX_BUFFER_LAYOUTS);

		R2_CHECK(s_optrRenderer->mConstantLayouts != nullptr, "We couldn't create the constant layouts!");

		s_optrRenderer->mEngineModelRefs = MAKE_SARRAY(*rendererArena, r2::draw::ModelRef, NUM_DEFAULT_MODELS);

		R2_CHECK(s_optrRenderer->mEngineModelRefs != nullptr, "We couldn't create the engine model refs");

#ifdef R2_DEBUG

		s_optrRenderer->mPreDebugCommandBucket = MAKE_CMD_BUCKET(*rendererArena, key::DebugKey, key::DecodeDebugKey, COMMAND_CAPACITY);

		s_optrRenderer->mPostDebugCommandBucket = MAKE_CMD_BUCKET(*rendererArena, key::DebugKey, key::DecodeDebugKey, COMMAND_CAPACITY);

		s_optrRenderer->mDebugCommandBucket = MAKE_CMD_BUCKET(*rendererArena, key::DebugKey, key::DecodeDebugKey, COMMAND_CAPACITY);

		s_optrRenderer->mDebugCommandArena = MAKE_STACK_ARENA(*rendererArena, COMMAND_CAPACITY * cmd::LargestCommand() + DEBUG_COMMAND_AUX_MEMORY);
		s_optrRenderer->mDebugRenderBatches = MAKE_SARRAY(*rendererArena, DebugRenderBatch, NUM_DEBUG_DRAW_TYPES);

		for (s32 i = 0; i < NUM_DEBUG_DRAW_TYPES; ++i)
		{
			DebugRenderBatch debugRenderBatch;

			debugRenderBatch.debugDrawType = (DebugDrawType)i;
			debugRenderBatch.vertexConfigHandle = InvalidVertexConfigHandle;
			debugRenderBatch.materialHandle = mat::InvalidMaterial;
			
			
			debugRenderBatch.colors = MAKE_SARRAY(*rendererArena, glm::vec4, MAX_NUM_DRAWS);
			debugRenderBatch.drawFlags = MAKE_SARRAY(*rendererArena, DrawFlags, MAX_NUM_DRAWS);

			if (debugRenderBatch.debugDrawType == DDT_LINES)
			{
				debugRenderBatch.matTransforms = MAKE_SARRAY(*rendererArena, glm::mat4, MAX_NUM_DRAWS);
				debugRenderBatch.vertices = MAKE_SARRAY(*rendererArena, DebugVertex, MAX_NUM_DRAWS * 2);
			}
			else if (debugRenderBatch.debugDrawType == DDT_MODELS)
			{
				debugRenderBatch.transforms = MAKE_SARRAY(*rendererArena, math::Transform, MAX_NUM_DRAWS);
				debugRenderBatch.debugModelTypesToDraw = MAKE_SARRAY(*rendererArena, DebugModelType, MAX_NUM_DRAWS);
			}
			else
			{
				R2_CHECK(false, "Currently unsupported!");
			}

			r2::sarr::Push(*s_optrRenderer->mDebugRenderBatches, debugRenderBatch);
		}
#endif

		bool rendererImpl = r2::draw::rendererimpl::RendererImplInit(memoryAreaHandle, MAX_NUM_CONSTANT_BUFFERS, MAX_NUM_DRAWS, "RendererImpl");
		if (!rendererImpl)
		{
			R2_CHECK(false, "We couldn't initialize the renderer implementation");
			return false;
		}

		bool shaderSystemIntialized = r2::draw::shadersystem::Init(memoryAreaHandle, MAX_NUM_SHADERS, shaderManifestPath, internalShaderManifestPath);
		if (!shaderSystemIntialized)
		{
			R2_CHECK(false, "We couldn't initialize the shader system");
			return false;
		}

		bool textureSystemInitialized = r2::draw::texsys::Init(memoryAreaHandle, MAX_NUM_TEXTURES, "Texture System");
		if (!textureSystemInitialized)
		{
			R2_CHECK(false, "We couldn't initialize the texture system");
			return false;
		}


		bool materialSystemInitialized = r2::draw::matsys::Init(memoryAreaHandle, MAX_NUM_MATERIAL_SYSTEMS, MAX_NUM_MATERIALS_PER_MATERIAL_SYSTEM, "Material Systems Area");
		if (!materialSystemInitialized)
		{
			R2_CHECK(false, "We couldn't initialize the material systems");
			return false;
		}

		
		r2::mem::utils::MemBoundary boundary = MAKE_BOUNDARY(*s_optrRenderer->mSubAreaArena, materialMemorySystemSize, ALIGNMENT);
		
		s_optrRenderer->mMaterialSystem = r2::draw::matsys::CreateMaterialSystem(boundary, materialPack, texturePacksManifest);

		if (!s_optrRenderer->mMaterialSystem)
		{
			R2_CHECK(false, "We couldn't initialize the material system");
			return false;
		}

		FREE(texturePacksData, *MEM_ENG_SCRATCH_PTR);
		FREE(materialPackData, *MEM_ENG_SCRATCH_PTR);


#ifdef R2_DEBUG
		s_optrRenderer->mDebugLinesMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*s_optrRenderer->mMaterialSystem, STRING_ID("DebugLines"));
		s_optrRenderer->mDebugModelMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*s_optrRenderer->mMaterialSystem, STRING_ID("DebugModels"));
#endif
		s_optrRenderer->mFinalCompositeMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*s_optrRenderer->mMaterialSystem, STRING_ID("FinalComposite"));


		auto size = CENG.DisplaySize();
		

		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();

		
		s_optrRenderer->mPreRenderBucket = MAKE_CMD_BUCKET(*rendererArena, key::Basic, key::DecodeBasicKey, COMMAND_CAPACITY);
		s_optrRenderer->mPostRenderBucket = MAKE_CMD_BUCKET(*rendererArena, key::Basic, key::DecodeBasicKey, COMMAND_CAPACITY);

		s_optrRenderer->mPrePostRenderCommandArena = MAKE_STACK_ARENA(*rendererArena, 2 * COMMAND_CAPACITY * cmd::LargestCommand() + COMMAND_AUX_MEMORY / 2);

		
		s_optrRenderer->mRenderTargetsArena = MAKE_STACK_ARENA(*rendererArena,
			RenderTarget::MemorySize(1, 1, ALIGNMENT, stackHeaderSize, boundsChecking));

		//@TODO(Serge): we need to get the scale, x and y offsets
		ResizeRenderSurface(*s_optrRenderer, size.width, size.height, size.width, size.height, 1.0f, 1.0f, 0.0f, 0.0f); //@TODO(Serge): we need to get the scale, x and y offsets

		s_optrRenderer->mCommandBucket = MAKE_CMD_BUCKET(*rendererArena, r2::draw::key::Basic, r2::draw::key::DecodeBasicKey, COMMAND_CAPACITY);
		R2_CHECK(s_optrRenderer->mCommandBucket != nullptr, "We couldn't create the command bucket!");

		s_optrRenderer->mFinalBucket = MAKE_CMD_BUCKET(*rendererArena, r2::draw::key::Basic, r2::draw::key::DecodeBasicKey, COMMAND_CAPACITY);
		R2_CHECK(s_optrRenderer->mFinalBucket != nullptr, "We couldn't create the final command bucket!");

		s_optrRenderer->mCommandArena = MAKE_STACK_ARENA(*rendererArena, 2 * COMMAND_CAPACITY * cmd::LargestCommand() + COMMAND_AUX_MEMORY/2);

		R2_CHECK(s_optrRenderer->mCommandArena != nullptr, "We couldn't create the stack arena for commands");

		


		//Make the render batches
		{
			s_optrRenderer->mRenderBatches = MAKE_SARRAY(*rendererArena, RenderBatch, NUM_DRAW_TYPES);

			for (s32 i = 0; i < NUM_DRAW_TYPES; ++i)
			{
				RenderBatch nextBatch;

				nextBatch.vertexLayoutConfigHandle = InvalidVertexConfigHandle;
				nextBatch.subCommandsHandle = InvalidConstantConfigHandle;
				nextBatch.modelsHandle = InvalidConstantConfigHandle;
				nextBatch.materialsHandle = InvalidConstantConfigHandle;
				nextBatch.boneTransformOffsetsHandle = InvalidConstantConfigHandle;
				nextBatch.boneTransformsHandle = InvalidConstantConfigHandle;

				nextBatch.modelRefs = MAKE_SARRAY(*rendererArena, ModelRef, MAX_NUM_DRAWS);

				nextBatch.materialBatch.infos = MAKE_SARRAY(*rendererArena, MaterialBatch::Info, MAX_NUM_DRAWS);
				nextBatch.materialBatch.materialHandles = MAKE_SARRAY(*rendererArena, MaterialHandle, MAX_NUM_DRAWS * MAX_NUM_MESHES);
				nextBatch.models = MAKE_SARRAY(*rendererArena, glm::mat4, MAX_NUM_DRAWS);
				nextBatch.drawState = MAKE_SARRAY(*rendererArena, cmd::DrawState, MAX_NUM_DRAWS);

				if (i == DrawType::DYNAMIC)
				{
					nextBatch.boneTransforms = MAKE_SARRAY(*rendererArena, ShaderBoneTransform, MAX_NUM_BONES);
				}
				else
				{
					nextBatch.boneTransforms = nullptr;
				}

				r2::sarr::Push(*s_optrRenderer->mRenderBatches, nextBatch);
			}
		}

		CreateRenderPasses(*s_optrRenderer);

		r2::asset::FileList files = r2::asset::lib::MakeFileList(MAX_DEFAULT_MODELS);

		for (const auto& file : std::filesystem::recursive_directory_iterator(R2_ENGINE_INTERNAL_MODELS_BIN))
		{
			if (std::filesystem::file_size(file.path()) <= 0 || 
				((file.path().extension().string() != MODL_EXT) && (file.path().extension().string() != MESH_EXT)))
			{
				continue;
			}

			char filePath[r2::fs::FILE_PATH_LENGTH];

			r2::fs::utils::SanitizeSubPath(file.path().string().c_str(), filePath);

			r2::sarr::Push(*files, (r2::asset::AssetFile*)r2::asset::lib::MakeRawAssetFile(filePath));
		}

		s_optrRenderer->mModelSystem = modlsys::Init(memoryAreaHandle, DefaultModelsMemorySize(), true, files, "Rendering Engine Default Models");
		if (!s_optrRenderer->mModelSystem)
		{
			R2_CHECK(false, "We couldn't init the default engine models");
			return false;
		}

		bool loadedModels = LoadEngineModels();

		R2_CHECK(loadedModels, "We didn't load the models for the engine!");


		InitializeVertexLayouts(*s_optrRenderer, Megabytes(8), Megabytes(8));


		return loadedModels;
	}

	void Update()
	{
		r2::draw::shadersystem::Update();
		r2::draw::matsys::Update();
	}

	void Render(float alpha)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		

#ifdef R2_DEBUG
		DebugPreRender(*s_optrRenderer);
#endif

		PreRender(*s_optrRenderer); 

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
		cmdbkt::Sort(*s_optrRenderer->mPreRenderBucket, r2::draw::key::CompareBasicKey);
		cmdbkt::Sort(*s_optrRenderer->mCommandBucket, r2::draw::key::CompareBasicKey);
		cmdbkt::Sort(*s_optrRenderer->mFinalBucket, r2::draw::key::CompareBasicKey);
		cmdbkt::Sort(*s_optrRenderer->mPostRenderBucket, r2::draw::key::CompareBasicKey);
#ifdef R2_DEBUG
		cmdbkt::Sort(*s_optrRenderer->mPreDebugCommandBucket, r2::draw::key::CompareDebugKey);
		cmdbkt::Sort(*s_optrRenderer->mDebugCommandBucket, r2::draw::key::CompareDebugKey);
		cmdbkt::Sort(*s_optrRenderer->mPostDebugCommandBucket, r2::draw::key::CompareDebugKey);
#endif
		//for (u64 i = 0; i < numEntries; ++i)
		//{
		//	const r2::draw::CommandBucket<r2::draw::key::Basic>::Entry* entry = r2::sarr::At(*s_optrRenderer->mCommandBucket->sortedEntries, i);
		//	printf("sorted - key: %llu, data: %p, func: %p\n", entry->aKey.keyValue, entry->data, entry->func);
		//}

		cmdbkt::Submit(*s_optrRenderer->mPreRenderBucket);
		cmdbkt::Submit(*s_optrRenderer->mCommandBucket);

#ifdef R2_DEBUG
		cmdbkt::Submit(*s_optrRenderer->mPreDebugCommandBucket);
		cmdbkt::Submit(*s_optrRenderer->mDebugCommandBucket);
		
#endif

		cmdbkt::Submit(*s_optrRenderer->mFinalBucket);
		cmdbkt::Submit(*s_optrRenderer->mPostRenderBucket);

#ifdef R2_DEBUG
		cmdbkt::Submit(*s_optrRenderer->mPostDebugCommandBucket);

		ClearDebugRenderData();
#endif

		cmdbkt::ClearAll(*s_optrRenderer->mPreRenderBucket);
		cmdbkt::ClearAll(*s_optrRenderer->mCommandBucket);
		cmdbkt::ClearAll(*s_optrRenderer->mFinalBucket);
		cmdbkt::ClearAll(*s_optrRenderer->mPostRenderBucket);
		
		ClearRenderBatches(*s_optrRenderer);

		//This is kinda bad but... 
		RESET_ARENA(*s_optrRenderer->mCommandArena);
		RESET_ARENA(*s_optrRenderer->mPrePostRenderCommandArena);
	}

	void Shutdown()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}



		r2::mem::LinearArena* arena = s_optrRenderer->mSubAreaArena;


#ifdef R2_DEBUG

		for (s32 i = NUM_DEBUG_DRAW_TYPES - 1; i >= 0; --i)
		{
			DebugRenderBatch& debugRenderBatch = r2::sarr::At(*s_optrRenderer->mDebugRenderBatches, i);

			if ((DebugDrawType)i == DDT_LINES)
			{
				FREE(debugRenderBatch.vertices, *arena);
				FREE(debugRenderBatch.matTransforms, *arena);
			}
			else if ((DebugDrawType)i == DDT_MODELS)
			{
				FREE(debugRenderBatch.debugModelTypesToDraw, *arena);
				FREE(debugRenderBatch.transforms, *arena);
			}
			else
			{
				R2_CHECK(false, "Unsupported");
			}

			FREE(debugRenderBatch.drawFlags, *arena);
			FREE(debugRenderBatch.colors, *arena);
			
		}

		FREE(s_optrRenderer->mDebugRenderBatches, *arena);
		FREE(s_optrRenderer->mDebugCommandArena, *arena);
		FREE_CMD_BUCKET(*arena, key::DebugKey, s_optrRenderer->mDebugCommandBucket);
		FREE_CMD_BUCKET(*arena, key::DebugKey, s_optrRenderer->mPostDebugCommandBucket);
		FREE_CMD_BUCKET(*arena, key::DebugKey, s_optrRenderer->mPreDebugCommandBucket);

#endif
		for (int i = NUM_DRAW_TYPES - 1; i >= 0; --i)
		{
			RenderBatch& nextBatch = r2::sarr::At(*s_optrRenderer->mRenderBatches, i);
			
			if (i == DrawType::DYNAMIC)
			{
				FREE(nextBatch.boneTransforms, *arena);
			}

			FREE(nextBatch.drawState, *arena);
			FREE(nextBatch.models, *arena);

			FREE(nextBatch.materialBatch.materialHandles, *arena);
			FREE(nextBatch.materialBatch.infos, *arena);
			FREE(nextBatch.modelRefs, *arena);

		}

		DestroyRenderPasses(*s_optrRenderer);

		FREE(s_optrRenderer->mRenderBatches, *arena);



		DestroyRenderSurfaces(*s_optrRenderer);
		
		FREE(s_optrRenderer->mCommandArena, *arena);

		FREE(s_optrRenderer->mRenderTargetsArena, *arena);

		FREE(s_optrRenderer->mPrePostRenderCommandArena, *arena);

		FREE_CMD_BUCKET(*arena, r2::draw::key::Basic, s_optrRenderer->mFinalBucket);
		FREE_CMD_BUCKET(*arena, r2::draw::key::Basic, s_optrRenderer->mCommandBucket);
		FREE_CMD_BUCKET(*arena, key::Basic, s_optrRenderer->mPostRenderBucket);
		FREE_CMD_BUCKET(*arena, key::Basic, s_optrRenderer->mPreRenderBucket);


		modlsys::Shutdown(s_optrRenderer->mModelSystem);
		FREE(s_optrRenderer->mDefaultModelHandles, *arena);

		r2::mem::utils::MemBoundary materialSystemBoundary = s_optrRenderer->mMaterialSystem->mMaterialMemBoundary;
		
		r2::draw::matsys::FreeMaterialSystem(s_optrRenderer->mMaterialSystem);
		r2::draw::matsys::ShutdownMaterialSystems();
		r2::draw::texsys::Shutdown();
		r2::draw::shadersystem::Shutdown();

		s_optrRenderer->mSubAreaArena = nullptr;

		FREE(materialSystemBoundary.location, *arena);

		//delete the buffer handles
		r2::draw::rendererimpl::DeleteBuffers(
			r2::sarr::Size(*s_optrRenderer->mBufferHandles.bufferLayoutHandles),
			s_optrRenderer->mBufferHandles.bufferLayoutHandles->mData);

		r2::draw::rendererimpl::DeleteBuffers(
			r2::sarr::Size(*s_optrRenderer->mBufferHandles.vertexBufferHandles),
			s_optrRenderer->mBufferHandles.vertexBufferHandles->mData);

		r2::draw::rendererimpl::DeleteBuffers(
			r2::sarr::Size(*s_optrRenderer->mBufferHandles.indexBufferHandles),
			s_optrRenderer->mBufferHandles.indexBufferHandles->mData);

		r2::draw::rendererimpl::DeleteBuffers(
			r2::sarr::Size(*s_optrRenderer->mBufferHandles.drawIDHandles),
			s_optrRenderer->mBufferHandles.drawIDHandles->mData);

		r2::draw::rendererimpl::DeleteBuffers(
			r2::sarr::Size(*s_optrRenderer->mConstantBufferHandles),
			s_optrRenderer->mConstantBufferHandles->mData);
		
		FREE(s_optrRenderer->mBufferHandles.bufferLayoutHandles, *arena);
		FREE(s_optrRenderer->mBufferHandles.vertexBufferHandles, *arena);
		FREE(s_optrRenderer->mBufferHandles.indexBufferHandles, *arena);
		FREE(s_optrRenderer->mBufferHandles.drawIDHandles, *arena);
		FREE(s_optrRenderer->mConstantBufferHandles, *arena);
		FREE(s_optrRenderer->mConstantBufferData, *arena);
		FREE(s_optrRenderer->mVertexLayoutUploadOffsets, *arena);
		FREE(s_optrRenderer->mVertexLayoutConfigHandles, *arena);
		FREE(s_optrRenderer->mVertexLayouts, *arena);
		FREE(s_optrRenderer->mConstantLayouts, *arena);
		FREE(s_optrRenderer->mEngineModelRefs, *arena);

		FREE(s_optrRenderer, *arena);

		
		s_optrRenderer = nullptr;

		FREE_EMPLACED_ARENA(arena);
	}

	//Setup code
	void SetClearColor(const glm::vec4& color)
	{
		r2::draw::rendererimpl::SetClearColor(color);
	}

	void InitializeVertexLayouts(Renderer& renderer, u32 staticModelLayoutSize, u32 animatedModelLayoutSize)
	{
		AddStaticModelLayout({ staticModelLayoutSize }, staticModelLayoutSize);
		AddAnimatedModelLayout({ animatedModelLayoutSize, animatedModelLayoutSize }, animatedModelLayoutSize);

		renderer.mVPMatricesConfigHandle = AddConstantBufferLayout(r2::draw::ConstantBufferLayout::Type::Small, {
			{r2::draw::ShaderDataType::Mat4, "projection"},
			{r2::draw::ShaderDataType::Mat4, "view"},
			{r2::draw::ShaderDataType::Mat4, "skyboxView"}
		});

		renderer.mVectorsConfigHandle = AddConstantBufferLayout(r2::draw::ConstantBufferLayout::Type::Small, {
			{r2::draw::ShaderDataType::Float4, "CameraPosTimeW"},
			{r2::draw::ShaderDataType::Float4, "Exposure"}
		});

		AddModelsLayout(r2::draw::ConstantBufferLayout::Type::Big);

		AddSubCommandsLayout();
		AddMaterialLayout();

		//Maybe these should automatically be added by the animated models layout
		AddBoneTransformsLayout();

		AddBoneTransformOffsetsLayout();

		AddLightingLayout();

		bool success = GenerateLayouts();
		R2_CHECK(success, "We couldn't create the buffer layouts!");

		UploadEngineModels(renderer.mStaticVertexModelConfigHandle);//r2::sarr::At(*mVertexConfigHandles, STATIC_MODELS_CONFIG));
	}

	bool GenerateLayouts()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return false;
		}

#ifdef R2_DEBUG
		//add the debug stuff here
		AddDebugDrawLayout();
		AddDebugColorsLayout();
		AddDebugModelSubCommandsLayout();
		AddDebugLineSubCommandsLayout();
#endif
		AddSurfacesLayout();

		bool success = GenerateBufferLayouts(s_optrRenderer->mVertexLayouts) &&
		GenerateConstantBuffers(s_optrRenderer->mConstantLayouts);


		R2_CHECK(success, "We didn't properly generate the layouts!");

		if (success)
		{
			//setup the layouts for the render batches
			

#ifdef R2_DEBUG
			DebugRenderBatch& debugLinesRenderBatch = r2::sarr::At(*s_optrRenderer->mDebugRenderBatches, DDT_LINES);
			debugLinesRenderBatch.vertexConfigHandle = s_optrRenderer->mDebugLinesVertexConfigHandle;
			debugLinesRenderBatch.materialHandle = s_optrRenderer->mDebugLinesMaterialHandle;
			debugLinesRenderBatch.renderDebugConstantsConfigHandle = s_optrRenderer->mDebugRenderConstantsConfigHandle;
			debugLinesRenderBatch.subCommandsConstantConfigHandle = s_optrRenderer->mDebugLinesSubCommandsConfigHandle;

			DebugRenderBatch& debugModelRenderBatch = r2::sarr::At(*s_optrRenderer->mDebugRenderBatches, DDT_MODELS);
			debugModelRenderBatch.vertexConfigHandle = s_optrRenderer->mDebugModelVertexConfigHandle;
			debugModelRenderBatch.materialHandle = s_optrRenderer->mDebugModelMaterialHandle;
			debugModelRenderBatch.renderDebugConstantsConfigHandle = s_optrRenderer->mDebugRenderConstantsConfigHandle;
			debugModelRenderBatch.subCommandsConstantConfigHandle = s_optrRenderer->mDebugModelSubCommandsConfigHandle;
#endif

			for (s32 i = 0; i < DrawType::NUM_DRAW_TYPES; ++i)
			{
				RenderBatch& batch = r2::sarr::At(*s_optrRenderer->mRenderBatches, i);

				batch.materialsHandle = r2::sarr::At(*s_optrRenderer->mConstantBufferHandles, s_optrRenderer->mMaterialConfigHandle);
				batch.modelsHandle = r2::sarr::At(*s_optrRenderer->mConstantBufferHandles, s_optrRenderer->mModelConfigHandle);
				batch.subCommandsHandle = r2::sarr::At(*s_optrRenderer->mConstantBufferHandles, s_optrRenderer->mSubcommandsConfigHandle);
			
				if (i == DrawType::STATIC)
				{
					batch.vertexLayoutConfigHandle = s_optrRenderer->mStaticVertexModelConfigHandle;
				}
				else if (i == DrawType::DYNAMIC)
				{
					batch.vertexLayoutConfigHandle = s_optrRenderer->mAnimVertexModelConfigHandle;
					batch.boneTransformsHandle = s_optrRenderer->mBoneTransformsConfigHandle;
					batch.boneTransformOffsetsHandle = s_optrRenderer->mBoneTransformOffsetsConfigHandle;
				}
			}
		}

		return success;
	}

	bool GenerateBufferLayouts(const r2::SArray<BufferLayoutConfiguration>* layouts)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return false;
		}

		if (r2::sarr::Size(*s_optrRenderer->mBufferHandles.bufferLayoutHandles) > 0)
		{
			R2_CHECK(false, "We have already generated the buffer layouts!");
			return false;
		}

		R2_CHECK(layouts != nullptr, "layouts cannot be null");

		if (layouts == nullptr)
		{
			return false;
		}

		if (r2::sarr::Size(*layouts) > MAX_BUFFER_LAYOUTS)
		{
			R2_CHECK(false, "Trying to configure more buffer layouts than we have allocated");
			return false;
		}

		const auto numLayouts = r2::sarr::Size(*layouts);

		//VAOs
		rendererimpl::GenerateBufferLayouts((u32)numLayouts, s_optrRenderer->mBufferHandles.bufferLayoutHandles->mData);
		s_optrRenderer->mBufferHandles.bufferLayoutHandles->mSize = numLayouts;

		//VBOs
		u64 numVertexLayouts = 0;
		for (u64 i = 0; i < numLayouts; ++i)
		{
			const auto& layout = r2::sarr::At(*layouts, i);
			numVertexLayouts += layout.numVertexConfigs;
		}

		rendererimpl::GenerateVertexBuffers((u32)numVertexLayouts, s_optrRenderer->mBufferHandles.vertexBufferHandles->mData);
		s_optrRenderer->mBufferHandles.vertexBufferHandles->mSize = numVertexLayouts;

		//IBOs
		u32 numIBOs = 0;
		u32 numDrawIDs = 0;
		for (u64 i = 0; i < numLayouts; ++i)
		{
			if (r2::sarr::At(*layouts, i).indexBufferConfig.bufferSize != EMPTY_BUFFER)
			{
				++numIBOs;
			}
			if (r2::sarr::At(*layouts, i).useDrawIDs)
			{
				++numDrawIDs;
			}
		}

		r2::SArray<IndexBufferHandle>* tempIBOs = nullptr;
		if (numIBOs > 0)
		{
			tempIBOs = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, IndexBufferHandle, numIBOs);
			rendererimpl::GenerateIndexBuffers(numIBOs, tempIBOs->mData);
			tempIBOs->mSize = numIBOs;

			R2_CHECK(tempIBOs != nullptr, "We should have memory for tempIBOs");
		}

		r2::SArray<DrawIDHandle>* tempDrawIDs = nullptr;
		if (numDrawIDs > 0)
		{
			tempDrawIDs = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, DrawIDHandle, numDrawIDs);
			rendererimpl::GenerateVertexBuffers(numDrawIDs, tempDrawIDs->mData);
			tempDrawIDs->mSize = numDrawIDs;

			R2_CHECK(tempDrawIDs != nullptr, "We should have memory for tempIBOs");
		}

		u32 nextIndexBuffer = 0;
		u32 nextDrawIDBuffer = 0;
		u32 nextVertexBufferID = 0;
		//do the actual setup
		for (size_t i = 0; i < numLayouts; ++i)
		{
			const BufferLayoutConfiguration& config = r2::sarr::At(*layouts, i);
			
			if (config.indexBufferConfig.bufferSize != EMPTY_BUFFER && tempIBOs)
			{
				r2::sarr::Push(*s_optrRenderer->mBufferHandles.indexBufferHandles, r2::sarr::At(*tempIBOs, nextIndexBuffer++));
			}
			else
			{
				r2::sarr::Push(*s_optrRenderer->mBufferHandles.indexBufferHandles, EMPTY_BUFFER);
			}

			if (config.useDrawIDs)
			{
				r2::sarr::Push(*s_optrRenderer->mBufferHandles.drawIDHandles, r2::sarr::At(*tempDrawIDs, nextDrawIDBuffer++));
			}
			else
			{
				r2::sarr::Push(*s_optrRenderer->mBufferHandles.drawIDHandles, EMPTY_BUFFER);
			}

			VertexLayoutConfigHandle nextHandle;
			nextHandle.mBufferLayoutHandle = r2::sarr::At(*s_optrRenderer->mBufferHandles.bufferLayoutHandles, i);
			nextHandle.mIndexBufferHandle = r2::sarr::At(*s_optrRenderer->mBufferHandles.indexBufferHandles, i);
			
			u32 vertexBufferHandles[BufferLayoutConfiguration::MAX_VERTEX_BUFFER_CONFIGS];
			nextHandle.mNumVertexBufferHandles = config.numVertexConfigs;

			VertexLayoutUploadOffset nextOffset;

			for (size_t k = 0; k < config.numVertexConfigs; ++k)
			{
				vertexBufferHandles[k] = r2::sarr::At(*s_optrRenderer->mBufferHandles.vertexBufferHandles, nextVertexBufferID);
				nextHandle.mVertexBufferHandles[k] = vertexBufferHandles[k];
				
				++nextVertexBufferID;
			}

			rendererimpl::SetupBufferLayoutConfiguration(config,
				r2::sarr::At(*s_optrRenderer->mBufferHandles.bufferLayoutHandles, i),
				vertexBufferHandles, config.numVertexConfigs,
				r2::sarr::At(*s_optrRenderer->mBufferHandles.indexBufferHandles, i),
				r2::sarr::At(*s_optrRenderer->mBufferHandles.drawIDHandles, i));

			
			r2::sarr::Push(*s_optrRenderer->mVertexLayoutConfigHandles, nextHandle);
			r2::sarr::Push(*s_optrRenderer->mVertexLayoutUploadOffsets, nextOffset);
		}

		FREE(tempDrawIDs, *MEM_ENG_SCRATCH_PTR);
		FREE(tempIBOs, *MEM_ENG_SCRATCH_PTR);

		

		return true;
	}

	bool GenerateConstantBuffers(const r2::SArray<r2::draw::ConstantBufferLayoutConfiguration>* constantBufferConfigs)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return false;
		}

		if (r2::sarr::Size(*s_optrRenderer->mConstantBufferHandles) > 0)
		{
			R2_CHECK(false, "We have already generated the constant buffer handles!");
			return false;
		}

		if (!s_optrRenderer->mConstantBufferData)
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

		r2::draw::rendererimpl::GenerateContantBuffers(static_cast<u32>(numConstantBuffers), s_optrRenderer->mConstantBufferHandles->mData);
		s_optrRenderer->mConstantBufferHandles->mSize = numConstantBuffers;

		for (u64 i = 0; i < numConstantBuffers; ++i)
		{
			ConstantBufferData constData;
			const ConstantBufferLayoutConfiguration& config = r2::sarr::At(*constantBufferConfigs, i);
			auto handle = r2::sarr::At(*s_optrRenderer->mConstantBufferHandles, i);

			constData.handle = handle;
			constData.type = config.layout.GetType();
			constData.isPersistent = config.layout.GetFlags().IsSet(CB_FLAG_MAP_PERSISTENT);
			constData.bufferSize = config.layout.GetSize();

			r2::shashmap::Set(*s_optrRenderer->mConstantBufferData, handle, constData);
		}

		r2::draw::rendererimpl::SetupConstantBufferConfigs(constantBufferConfigs, s_optrRenderer->mConstantBufferHandles->mData);

		return true;
	}

	VertexConfigHandle AddStaticModelLayout(const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidVertexConfigHandle;
		}

		if (s_optrRenderer->mVertexLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidVertexConfigHandle;
		}

		auto numVertexLayouts = vertexLayoutSizes.size();
		r2::draw::BufferLayoutConfiguration layoutConfig;

		layoutConfig.layout = BufferLayout(
			{
				{
				{r2::draw::ShaderDataType::Float3, "aPos"},
				{r2::draw::ShaderDataType::Float3, "aNormal"},
				{r2::draw::ShaderDataType::Float3, "aTexCoord"},
				{r2::draw::ShaderDataType::Float3, "aTangent"},
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

		r2::sarr::Push(*s_optrRenderer->mVertexLayouts, layoutConfig);

		s_optrRenderer->mStaticVertexModelConfigHandle = r2::sarr::Size(*s_optrRenderer->mVertexLayouts) - 1;
		s_optrRenderer->mFinalBatchVertexLayoutConfigHandle = s_optrRenderer->mStaticVertexModelConfigHandle;


		s_optrRenderer->mDebugModelVertexConfigHandle = s_optrRenderer->mStaticVertexModelConfigHandle;

		return s_optrRenderer->mStaticVertexModelConfigHandle;
	}

	VertexConfigHandle AddAnimatedModelLayout(const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidVertexConfigHandle;
		}

		if (s_optrRenderer->mVertexLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidVertexConfigHandle;
		}

		R2_CHECK(vertexLayoutSizes.size() == 2, "Only support 2 vertex layouts for Animated Models right now");

		auto numVertexLayouts = vertexLayoutSizes.size();
		r2::draw::BufferLayoutConfiguration layoutConfig;

		layoutConfig.layout = BufferLayout(
			{
				{r2::draw::ShaderDataType::Float3, "aPos", 0},
				{r2::draw::ShaderDataType::Float3, "aNormal", 0},
				{r2::draw::ShaderDataType::Float3, "aTexCoord", 0},
				{r2::draw::ShaderDataType::Float3, "aTangent", 0},
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

		r2::sarr::Push(*s_optrRenderer->mVertexLayouts, layoutConfig);

		s_optrRenderer->mAnimVertexModelConfigHandle = r2::sarr::Size(*s_optrRenderer->mVertexLayouts) - 1;

		return s_optrRenderer->mAnimVertexModelConfigHandle;
	}
#ifdef R2_DEBUG
	VertexConfigHandle AddDebugDrawLayout()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidVertexConfigHandle;
		}

		if (s_optrRenderer->mVertexLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidVertexConfigHandle;
		}

		if (s_optrRenderer->mDebugLinesVertexConfigHandle != InvalidVertexConfigHandle)
		{
			R2_CHECK(false, "We have already added the debug vertex layout!");
			return s_optrRenderer->mDebugLinesVertexConfigHandle;
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

		r2::sarr::Push(*s_optrRenderer->mVertexLayouts, layoutConfig);

		s_optrRenderer->mDebugLinesVertexConfigHandle = r2::sarr::Size(*s_optrRenderer->mVertexLayouts) - 1;

		return s_optrRenderer->mDebugLinesVertexConfigHandle;
	}
#endif

	ConstantConfigHandle AddConstantBufferLayout(ConstantBufferLayout::Type type, const std::initializer_list<ConstantBufferElement>& elements)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		if (s_optrRenderer->mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		ConstantBufferFlags flags = 0;
		CreateConstantBufferFlags createFlags = 0;
		if (type > ConstantBufferLayout::Type::Small)
		{
			flags = r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT;
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
		
		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, constConfig);

		return r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;
	}

	ConstantConfigHandle AddMaterialLayout()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		if (s_optrRenderer->mConstantLayouts == nullptr)
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

		materials.layout.InitForMaterials(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_DRAWS);

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, materials);

		s_optrRenderer->mMaterialConfigHandle = r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;

		return s_optrRenderer->mMaterialConfigHandle;
	}

	ConstantConfigHandle AddModelsLayout(ConstantBufferLayout::Type type)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		if (s_optrRenderer->mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		s_optrRenderer->mModelConfigHandle = AddConstantBufferLayout(type, {
			{
				r2::draw::ShaderDataType::Mat4, "models", MAX_NUM_DRAWS}
			});

		return s_optrRenderer->mModelConfigHandle;
	}

	ConstantConfigHandle AddSubCommandsLayout()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		if (s_optrRenderer->mConstantLayouts == nullptr)
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

		subCommands.layout.InitForSubCommands(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_DRAWS);

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, subCommands);

		s_optrRenderer->mSubcommandsConfigHandle = r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;

		return s_optrRenderer->mSubcommandsConfigHandle;
	}

#ifdef R2_DEBUG
	ConstantConfigHandle AddDebugLineSubCommandsLayout()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		if (s_optrRenderer->mConstantLayouts == nullptr)
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

		subCommands.layout.InitForDebugSubCommands(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_DRAWS);

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, subCommands);

		s_optrRenderer->mDebugLinesSubCommandsConfigHandle = r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;

		return s_optrRenderer->mDebugLinesSubCommandsConfigHandle;
	}

	ConstantConfigHandle AddDebugModelSubCommandsLayout()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		if (s_optrRenderer->mConstantLayouts == nullptr)
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

		subCommands.layout.InitForSubCommands(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_DRAWS);

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, subCommands);

		s_optrRenderer->mDebugModelSubCommandsConfigHandle = r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;

		return s_optrRenderer->mDebugModelSubCommandsConfigHandle;

	}

	ConstantConfigHandle AddDebugColorsLayout()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		if (s_optrRenderer->mConstantLayouts == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		ConstantBufferFlags flags = r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT;
		CreateConstantBufferFlags createFlags = r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE;
		auto type = ConstantBufferLayout::Type::Big;

		r2::draw::ConstantBufferLayoutConfiguration debugRenderConstantsLayout
		{
			//layout
			{
			},
			r2::draw::VertexDrawTypeDynamic
		};

		debugRenderConstantsLayout.layout.InitForDebugRenderConstants(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_DRAWS * 2);

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, debugRenderConstantsLayout);

		s_optrRenderer->mDebugRenderConstantsConfigHandle = r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;

		return s_optrRenderer->mDebugRenderConstantsConfigHandle;
	}

#endif

	ConstantConfigHandle AddBoneTransformsLayout()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		if (s_optrRenderer->mConstantLayouts == nullptr)
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

		boneTransforms.layout.InitForBoneTransforms(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_BONES);

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, boneTransforms);

		s_optrRenderer->mBoneTransformsConfigHandle = r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;
		return s_optrRenderer->mBoneTransformsConfigHandle;
	}

	ConstantConfigHandle AddBoneTransformOffsetsLayout()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		if (s_optrRenderer->mConstantLayouts == nullptr)
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

		boneTransformOffsets.layout.InitForBoneTransformOffsets(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, MAX_NUM_BONES);

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, boneTransformOffsets);

		s_optrRenderer->mBoneTransformOffsetsConfigHandle = r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;

		return s_optrRenderer->mBoneTransformOffsetsConfigHandle;
	}

	ConstantConfigHandle AddLightingLayout()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		if (s_optrRenderer->mConstantLayouts == nullptr)
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

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, lighting);

		s_optrRenderer->mLightingConfigHandle = r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;

		return s_optrRenderer->mLightingConfigHandle;
	}

	ConstantConfigHandle AddSurfacesLayout()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return InvalidConstantConfigHandle;
		}

		if (s_optrRenderer->mConstantLayouts == nullptr)
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

		surfaces.layout.InitForSurfaces();

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, surfaces);

		s_optrRenderer->mSurfacesConfigHandle = r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;

		return s_optrRenderer->mSurfacesConfigHandle;

	}

	u64 MaterialSystemMemorySize(u64 numMaterials, u64 textureCacheInBytes, u64 totalNumberOfTextures, u64 numPacks, u64 maxTexturesInAPack)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();

		return r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::mat::MemorySize(ALIGNMENT, numMaterials, textureCacheInBytes, totalNumberOfTextures, numPacks, maxTexturesInAPack), ALIGNMENT, headerSize, boundsChecking);
	}

	u64 MemorySize(u64 materialSystemMemorySize)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();

		u64 memorySize =
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::draw::Renderer), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::CommandBucket<r2::draw::key::Basic>::MemorySize(COMMAND_CAPACITY), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::CommandBucket<r2::draw::key::Basic>::MemorySize(COMMAND_CAPACITY), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::CommandBucket<r2::draw::key::Basic>::MemorySize(COMMAND_CAPACITY), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::CommandBucket<r2::draw::key::Basic>::MemorySize(COMMAND_CAPACITY), ALIGNMENT, headerSize, boundsChecking) +
			r2::draw::RenderTarget::MemorySize(1, 1, ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::BufferLayoutHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::VertexBufferHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::IndexBufferHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::DrawIDHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ConstantBufferHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::BufferLayoutConfiguration>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ConstantBufferLayoutConfiguration>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<ConstantBufferData>::MemorySize(MAX_BUFFER_LAYOUTS * r2::SHashMap<ConstantBufferData>::LoadFactorMultiplier()), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(RenderPass), ALIGNMENT, headerSize, boundsChecking) * NUM_RENDER_PASSES +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking) +

			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(cmd::LargestCommand(), ALIGNMENT, stackHeaderSize, boundsChecking) * COMMAND_CAPACITY * 2 +
			r2::mem::utils::GetMaxMemoryForAllocation(COMMAND_AUX_MEMORY/2, ALIGNMENT, headerSize, boundsChecking) +

			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(cmd::LargestCommand(), ALIGNMENT, stackHeaderSize, boundsChecking) * COMMAND_CAPACITY * 2 +
			r2::mem::utils::GetMaxMemoryForAllocation(COMMAND_AUX_MEMORY/2, ALIGNMENT, headerSize, boundsChecking) +

			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ModelHandle>::MemorySize(MAX_DEFAULT_MODELS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<VertexLayoutConfigHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<VertexLayoutUploadOffset>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ModelRef>::MemorySize(NUM_DEFAULT_MODELS), ALIGNMENT, headerSize, boundsChecking) +
			materialSystemMemorySize +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<RenderBatch>::MemorySize(DrawType::NUM_DRAW_TYPES), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(RenderBatch::MemorySize(MAX_NUM_DRAWS, MAX_NUM_DRAWS, MAX_NUM_BONES, ALIGNMENT, headerSize, boundsChecking), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(RenderBatch::MemorySize(MAX_NUM_DRAWS, MAX_NUM_DRAWS, 0, ALIGNMENT, headerSize, boundsChecking), ALIGNMENT, headerSize, boundsChecking) * (NUM_DRAW_TYPES - 1)


#ifdef R2_DEBUG
			+ r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<DebugRenderBatch>::MemorySize(DebugDrawType::NUM_DEBUG_DRAW_TYPES), ALIGNMENT, headerSize, boundsChecking) * 2
			+ r2::mem::utils::GetMaxMemoryForAllocation(DebugRenderBatch::MemorySize(MAX_NUM_DRAWS, false, ALIGNMENT, headerSize, boundsChecking), ALIGNMENT, headerSize, boundsChecking)
			+ r2::mem::utils::GetMaxMemoryForAllocation(DebugRenderBatch::MemorySize(MAX_NUM_DRAWS, true, ALIGNMENT, headerSize, boundsChecking), ALIGNMENT, headerSize, boundsChecking)
			+ r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::CommandBucket<r2::draw::key::DebugKey>::MemorySize(COMMAND_CAPACITY), ALIGNMENT, headerSize, boundsChecking) * 3
			+ r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking)
			+ r2::mem::utils::GetMaxMemoryForAllocation(cmd::LargestCommand(), ALIGNMENT, stackHeaderSize, boundsChecking) * COMMAND_CAPACITY 
			+ r2::mem::utils::GetMaxMemoryForAllocation(DEBUG_COMMAND_AUX_MEMORY, ALIGNMENT, headerSize, boundsChecking)
			//+ r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ModelRef>::MemorySize(MAX_NUM_DEBUG_MODELS), ALIGNMENT, headerSize, boundsChecking)
			//+ r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<cmd::DrawDebugBatchSubCommand>::MemorySize(MAX_NUM_DEBUG_DRAW_COMMANDS), ALIGNMENT, headerSize, boundsChecking)
			//+ r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<cmd::DrawBatchSubCommand>::MemorySize(MAX_NUM_DEBUG_DRAW_COMMANDS), ALIGNMENT, headerSize, boundsChecking)
			//+ r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<DebugVertex>::MemorySize(MAX_NUM_DEBUG_LINES*2), ALIGNMENT, headerSize, boundsChecking) * 2 +
			//r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<InternalDebugRenderCommand>::MemorySize(MAX_NUM_DEBUG_DRAW_COMMANDS), ALIGNMENT, headerSize, boundsChecking) * 3
#endif
			; //end of sizes

		return r2::mem::utils::GetMaxMemoryForAllocation(memorySize, ALIGNMENT);
	}

	BufferHandles& GetVertexBufferHandles()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
		}

		return s_optrRenderer->mBufferHandles;
	}

	const r2::SArray<ConstantBufferHandle>* GetConstantBufferHandles()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		return s_optrRenderer->mConstantBufferHandles;
	}

	template<class CMD, class ARENA>
	CMD* AddCommand(ARENA& arena, r2::draw::CommandBucket<r2::draw::key::Basic>& bucket, r2::draw::key::Basic key, u64 auxMemory)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		return r2::draw::cmdbkt::AddCommand<r2::draw::key::Basic, r2::mem::StackArena, CMD>(arena, bucket, key, auxMemory);
	}

	template<class CMDTOAPPENDTO, class CMD, class ARENA>
	CMD* AppendCommand(ARENA& arena, CMDTOAPPENDTO* cmdToAppendTo, u64 auxMemory)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		return r2::draw::cmdbkt::AppendCommand<CMDTOAPPENDTO, CMD, r2::mem::StackArena>(arena, cmdToAppendTo, auxMemory);
	}

	const r2::draw::Model* GetDefaultModel(r2::draw::DefaultModel defaultModel)
	{
		if (s_optrRenderer == nullptr ||
			s_optrRenderer->mModelSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		auto modelHandle = r2::sarr::At(*s_optrRenderer->mDefaultModelHandles, defaultModel);
		return modlsys::GetModel(s_optrRenderer->mModelSystem, modelHandle);
	}

	const r2::SArray<r2::draw::ModelRef>* GetDefaultModelRefs()
	{
		if (s_optrRenderer == nullptr ||
			s_optrRenderer->mEngineModelRefs == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		return s_optrRenderer->mEngineModelRefs;
	}

	r2::draw::ModelRef GetDefaultModelRef(r2::draw::DefaultModel defaultModel)
	{
		if (s_optrRenderer == nullptr ||
			s_optrRenderer->mEngineModelRefs == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return r2::draw::ModelRef{};
		}

		return r2::sarr::At(*s_optrRenderer->mEngineModelRefs, defaultModel);
	}

	void GetDefaultModelMaterials(r2::SArray<r2::draw::MaterialHandle>& defaultModelMaterials)
	{
		const r2::draw::Model* quadModel = GetDefaultModel(r2::draw::QUAD);
		const r2::draw::Model* sphereModel = GetDefaultModel(r2::draw::SPHERE);
		const r2::draw::Model* cubeModel = GetDefaultModel(r2::draw::CUBE);
		const r2::draw::Model* cylinderModel = GetDefaultModel(r2::draw::CYLINDER);
		const r2::draw::Model* coneModel = GetDefaultModel(r2::draw::CONE);
		const r2::draw::Model* skyboxModel = GetDefaultModel(r2::draw::SKYBOX);

		r2::SArray<const r2::draw::Model*>* defaultModels = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const r2::draw::Model*, NUM_DEFAULT_MODELS);
		r2::sarr::Push(*defaultModels, quadModel);
		r2::sarr::Push(*defaultModels, cubeModel);
		r2::sarr::Push(*defaultModels, sphereModel);
		r2::sarr::Push(*defaultModels, coneModel);
		r2::sarr::Push(*defaultModels, cylinderModel);
		r2::sarr::Push(*defaultModels, skyboxModel);

		u64 numModels = r2::sarr::Size(*defaultModels);
		for (u64 i = 0; i < numModels; ++i)
		{
			const r2::draw::Model* model = r2::sarr::At(*defaultModels, i);
			r2::draw::MaterialHandle materialHandle = r2::sarr::At(*model->optrMaterialHandles, 0);
			r2::sarr::Push(defaultModelMaterials, materialHandle);
		}

		FREE(defaultModels, *MEM_ENG_SCRATCH_PTR);
	}

	r2::draw::MaterialHandle GetMaterialHandleForDefaultModel(r2::draw::DefaultModel defaultModel)
	{
		if (s_optrRenderer == nullptr ||
			s_optrRenderer->mModelSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return r2::draw::MaterialHandle{};
		}

		const r2::draw::Model* model = GetDefaultModel(defaultModel);

		r2::draw::MaterialHandle materialHandle = r2::sarr::At(*model->optrMaterialHandles, 0);

		return materialHandle;
	}

	void UploadEngineModels(VertexConfigHandle vertexLayoutConfig)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}
		
		const r2::draw::Model* quadModel = GetDefaultModel(r2::draw::QUAD);
		const r2::draw::Model* sphereModel = GetDefaultModel(r2::draw::SPHERE);
		const r2::draw::Model* cubeModel = GetDefaultModel(r2::draw::CUBE);
		const r2::draw::Model* cylinderModel = GetDefaultModel(r2::draw::CYLINDER);
		const r2::draw::Model* coneModel = GetDefaultModel(r2::draw::CONE);

		r2::SArray<const r2::draw::Model*>* modelsToUpload = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const r2::draw::Model*, NUM_DEFAULT_MODELS-1);
		r2::sarr::Push(*modelsToUpload, quadModel);
		r2::sarr::Push(*modelsToUpload, cubeModel);
		r2::sarr::Push(*modelsToUpload, sphereModel);
		r2::sarr::Push(*modelsToUpload, coneModel);
		r2::sarr::Push(*modelsToUpload, cylinderModel);

		UploadModels(*modelsToUpload, *s_optrRenderer->mEngineModelRefs);

		FREE(modelsToUpload, *MEM_ENG_SCRATCH_PTR);

		//@NOTE: because we can now re-use meshes for other models, we can re-use the CUBE mesh for the SKYBOX model
		r2::sarr::Push(*s_optrRenderer->mEngineModelRefs, GetDefaultModelRef(CUBE));
	}

	void LoadEngineTexturesFromDisk()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		r2::draw::mat::LoadAllMaterialTexturesFromDisk(*s_optrRenderer->mMaterialSystem);
	}

	void UploadEngineMaterialTexturesToGPUFromMaterialName(u64 materialName)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		r2::draw::mat::UploadMaterialTexturesToGPUFromMaterialName(*s_optrRenderer->mMaterialSystem, materialName);
	}

	void UploadEngineMaterialTexturesToGPU()
	{
		r2::draw::mat::UploadAllMaterialTexturesToGPU(*s_optrRenderer->mMaterialSystem);
	}

	ModelRef UploadModel(const Model* model)
	{
		return UploadModelInternal(model, nullptr, nullptr, s_optrRenderer->mStaticVertexModelConfigHandle);
	}

	void UploadModels(const r2::SArray<const Model*>& models, r2::SArray<ModelRef>& modelRefs)
	{
		if (r2::sarr::Size(models) + r2::sarr::Size(modelRefs) > r2::sarr::Capacity(modelRefs))
		{
			R2_CHECK(false, "We don't have enough space to put the model refs");
			return;
		}

		const u64 numModels = r2::sarr::Size(models);

		for (u64 i = 0; i < numModels; ++i)
		{
			r2::sarr::Push(modelRefs, UploadModel(r2::sarr::At(models, i)));
		}
	}

	ModelRef UploadAnimModel(const AnimModel* model)
	{
		return UploadModelInternal(&model->model, model->boneData, model->boneInfo, s_optrRenderer->mAnimVertexModelConfigHandle);
	}

	void UploadAnimModels(const r2::SArray<const AnimModel*>& models, r2::SArray<ModelRef>& modelRefs)
	{
		if (r2::sarr::Size(models) + r2::sarr::Size(modelRefs) > r2::sarr::Capacity(modelRefs))
		{
			R2_CHECK(false, "We don't have enough space to put the model refs");
			return;
		}

		const u64 numModels = r2::sarr::Size(models);

		for (u64 i = 0; i < numModels; ++i)
		{
			r2::sarr::Push(modelRefs, UploadAnimModel(r2::sarr::At(models, i)));
		}
	}


	ModelRef UploadModelInternal(const Model* model, r2::SArray<BoneData>* boneData, r2::SArray<BoneInfo>* boneInfo, VertexConfigHandle vertexConfigHandle)
	{
		ModelRef modelRef;

		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return modelRef;
		}

		if (model == nullptr)
		{
			R2_CHECK(false, "We don't have a proper model!");
			return modelRef;
		}

		if (vertexConfigHandle == InvalidVertexConfigHandle)
		{
			R2_CHECK(false, "We only should have 2 vHandles, 0 - mesh data, 1 - bone data");
			return modelRef;
		}

		u64 numMeshes = r2::sarr::Size(*model->optrMeshes);
		r2::draw::key::Basic fillKey;
		//@TODO(Serge): fix this or pass it in
		fillKey.keyValue = 0;

		if (numMeshes == 0)
		{
			R2_CHECK(false, "We don't have any meshes in the model!");
			return modelRef;
		}

		u64 numMaterals = r2::sarr::Size(*model->optrMaterialHandles);

		const VertexLayoutConfigHandle& vHandle = r2::sarr::At(*s_optrRenderer->mVertexLayoutConfigHandles, vertexConfigHandle);
		VertexLayoutUploadOffset& vOffsets = r2::sarr::At(*s_optrRenderer->mVertexLayoutUploadOffsets, vertexConfigHandle);
		const BufferLayoutConfiguration& layoutConfig = r2::sarr::At(*s_optrRenderer->mVertexLayouts, vertexConfigHandle);

		modelRef.hash = model->hash;
		modelRef.indexBufferHandle = vHandle.mIndexBufferHandle;
		modelRef.vertexBufferHandle = vHandle.mVertexBufferHandles[0];
		modelRef.mMeshRefs[0].baseIndex = vOffsets.mIndexBufferOffset.baseIndex + vOffsets.mIndexBufferOffset.numIndices;
		modelRef.mMeshRefs[0].baseVertex = vOffsets.mVertexBufferOffset.baseVertex + vOffsets.mVertexBufferOffset.numVertices;

		modelRef.mNumMeshRefs = numMeshes;
		modelRef.mNumMaterialHandles = numMaterals;

		for (u64 i = 0; i < numMaterals; ++i)
		{
			modelRef.mMaterialHandles[i] = r2::sarr::At(*model->optrMaterialHandles, i);
		}

		u64 vOffset = sizeof(r2::draw::Vertex) * (modelRef.mMeshRefs[0].baseVertex);
		u64 iOffset = sizeof(u32) * (modelRef.mMeshRefs[0].baseIndex);

		u64 totalNumVertices = 0;
		u64 totalNumIndices = 0;

		modelRef.mMeshRefs[0].numVertices = r2::sarr::Size(*model->optrMeshes->mData[0]->optrVertices);
		totalNumVertices = modelRef.mMeshRefs[0].numVertices;

		u64 resultingMemorySize = (vOffset + sizeof(r2::draw::Vertex) * modelRef.mMeshRefs[0].numVertices);

		if (resultingMemorySize > layoutConfig.vertexBufferConfigs[0].bufferSize)
		{
			R2_CHECK(false, "We don't have enough room in the buffer for all of these vertices! We're over by %llu", resultingMemorySize - layoutConfig.vertexBufferConfigs[0].bufferSize);
			return {};
		}

		//@TODO(Serge): maybe change to the upload command bucket?
		r2::draw::cmd::FillVertexBuffer* fillVertexCommand = r2::draw::renderer::AddCommand<r2::draw::cmd::FillVertexBuffer, mem::StackArena>(*s_optrRenderer->mCommandArena, *s_optrRenderer->mCommandBucket, fillKey, 0);
		vOffset = r2::draw::cmd::FillVertexBufferCommand(fillVertexCommand, *r2::sarr::At(*model->optrMeshes, 0), vHandle.mVertexBufferHandles[0], vOffset);

		cmd::FillVertexBuffer* nextVertexCmd = fillVertexCommand;

		for (u64 i = 1; i < numMeshes; ++i)
		{
			u64 numMeshVertices = r2::sarr::Size(*model->optrMeshes->mData[i]->optrVertices);
			modelRef.mMeshRefs[i].numVertices = numMeshVertices;
			modelRef.mMeshRefs[i].baseVertex = modelRef.mMeshRefs[i - 1].numVertices + modelRef.mMeshRefs[i - 1].baseVertex;

			totalNumVertices += numMeshVertices;
			resultingMemorySize = (vOffset + sizeof(r2::draw::Vertex) * numMeshVertices);

			if (resultingMemorySize > layoutConfig.vertexBufferConfigs[0].bufferSize)
			{
				R2_CHECK(false, "We don't have enough room in the buffer for all of these vertices! We're over by %llu", resultingMemorySize - layoutConfig.vertexBufferConfigs[0].bufferSize);
				return {};
			}

			//@TODO(Serge): maybe change to the upload command bucket?
			nextVertexCmd = AppendCommand<r2::draw::cmd::FillVertexBuffer, cmd::FillVertexBuffer, mem::StackArena>(*s_optrRenderer->mCommandArena, nextVertexCmd, 0);
			vOffset = r2::draw::cmd::FillVertexBufferCommand(nextVertexCmd, *r2::sarr::At(*model->optrMeshes, i), vHandle.mVertexBufferHandles[0], vOffset);
		}

		modelRef.mAnimated = boneData && boneInfo;

		if (boneInfo)
		{
			modelRef.mNumBones = boneInfo->mSize;
		}

		if (boneData)
		{
			
			u64 bOffset = sizeof(r2::draw::BoneData) * (modelRef.mMeshRefs[0].baseVertex);

			resultingMemorySize = (bOffset + sizeof(r2::draw::BoneData) * r2::sarr::Size(*boneData));

			if (resultingMemorySize > layoutConfig.vertexBufferConfigs[1].bufferSize)
			{
				R2_CHECK(false, "We don't have enough room in the buffer for all of these vertices! We're over by %llu", resultingMemorySize - layoutConfig.vertexBufferConfigs[1].bufferSize);
				return {};
			}

			r2::draw::cmd::FillVertexBuffer* fillBoneDataCommand = AppendCommand<r2::draw::cmd::FillVertexBuffer, cmd::FillVertexBuffer, mem::StackArena>(*s_optrRenderer->mCommandArena, nextVertexCmd, 0);
			r2::draw::cmd::FillBonesBufferCommand(fillBoneDataCommand, *boneData, vHandle.mVertexBufferHandles[1], bOffset);

			nextVertexCmd = fillBoneDataCommand;
		}
		
		modelRef.mMeshRefs[0].numIndices = r2::sarr::Size(*model->optrMeshes->mData[0]->optrIndices);
		totalNumIndices = modelRef.mMeshRefs[0].numIndices;

		resultingMemorySize = (iOffset + sizeof(u32) * modelRef.mMeshRefs[0].numIndices);
		if (resultingMemorySize > layoutConfig.indexBufferConfig.bufferSize)
		{
			R2_CHECK(false, "We don't have enough room in the buffer for all of these vertices! We're over by %llu", resultingMemorySize - layoutConfig.indexBufferConfig.bufferSize);
			return {};
		}

		cmd::FillIndexBuffer* fillIndexCommand = AppendCommand<cmd::FillVertexBuffer, cmd::FillIndexBuffer, mem::StackArena>(*s_optrRenderer->mCommandArena, nextVertexCmd, 0);
		iOffset = r2::draw::cmd::FillIndexBufferCommand(fillIndexCommand, *r2::sarr::At(*model->optrMeshes, 0), vHandle.mIndexBufferHandle, iOffset);

		cmd::FillIndexBuffer* nextIndexCmd = fillIndexCommand;

		for (u64 i = 1; i < numMeshes; ++i)
		{
			u64 numMeshIndices = r2::sarr::Size(*model->optrMeshes->mData[i]->optrIndices);
			modelRef.mMeshRefs[i].numIndices = numMeshIndices;
			modelRef.mMeshRefs[i].baseIndex = modelRef.mMeshRefs[i - 1].baseIndex + modelRef.mMeshRefs[i - 1].numIndices;

			totalNumIndices += numMeshIndices;

			resultingMemorySize = (iOffset + sizeof(u32) * numMeshIndices);
			if (resultingMemorySize > layoutConfig.indexBufferConfig.bufferSize)
			{
				R2_CHECK(false, "We don't have enough room in the buffer for all of these vertices! We're over by %llu", resultingMemorySize - layoutConfig.indexBufferConfig.bufferSize);
				return {};
			}

			nextIndexCmd = AppendCommand<cmd::FillIndexBuffer, cmd::FillIndexBuffer, mem::StackArena>(*s_optrRenderer->mCommandArena, nextIndexCmd, 0);
			iOffset = r2::draw::cmd::FillIndexBufferCommand(nextIndexCmd, *r2::sarr::At(*model->optrMeshes, i), vHandle.mIndexBufferHandle, iOffset);
		}

		vOffsets.mVertexBufferOffset.baseVertex = modelRef.mMeshRefs[0].baseVertex;
		vOffsets.mVertexBufferOffset.numVertices = totalNumVertices;

		vOffsets.mIndexBufferOffset.baseIndex = modelRef.mMeshRefs[0].baseIndex;
		vOffsets.mIndexBufferOffset.numIndices = totalNumIndices;

		return modelRef;
	}

	void ClearVertexLayoutOffsets(VertexConfigHandle vHandle)
	{
		if (s_optrRenderer == nullptr ||
			s_optrRenderer->mVertexLayoutUploadOffsets == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		VertexLayoutUploadOffset& offset = r2::sarr::At(*s_optrRenderer->mVertexLayoutUploadOffsets, vHandle);

		offset.mIndexBufferOffset.baseIndex = 0;
		offset.mIndexBufferOffset.numIndices = 0;

		offset.mVertexBufferOffset.baseVertex = 0;
		offset.mVertexBufferOffset.numVertices = 0;
	}

	void ClearAllVertexLayoutOffsets()
	{
		if (s_optrRenderer == nullptr || 
			s_optrRenderer->mVertexLayoutUploadOffsets == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		u64 size = r2::sarr::Size(*s_optrRenderer->mVertexLayoutUploadOffsets);

		for (u64 i = 0; i < size; ++i)
		{
			VertexLayoutUploadOffset& offset = r2::sarr::At(*s_optrRenderer->mVertexLayoutUploadOffsets, i);

			offset.mIndexBufferOffset.baseIndex = 0;
			offset.mIndexBufferOffset.numIndices = 0;
			
			offset.mVertexBufferOffset.baseVertex = 0;
			offset.mVertexBufferOffset.numVertices = 0;
		}
	}


	u64 AddFillConstantBufferCommandForData(ConstantBufferHandle handle, u64 elementIndex, const void* data)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return 0;
		}

		r2::draw::key::Basic fillKey;
		//@TODO(Serge): fix this or pass it in
		fillKey.keyValue = 0;

		ConstantBufferData* constBufferData = GetConstData(handle);

		R2_CHECK(constBufferData != nullptr, "We couldn't find the constant buffer handle!");

		u64 numConstantBufferHandles = r2::sarr::Size(*s_optrRenderer->mConstantBufferHandles);
		u64 constBufferIndex = 0;
		bool found = false;
		for (; constBufferIndex < numConstantBufferHandles; ++constBufferIndex)
		{
			if (handle == r2::sarr::At(*s_optrRenderer->mConstantBufferHandles, constBufferIndex))
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
		
		const ConstantBufferLayoutConfiguration& config = r2::sarr::At(*s_optrRenderer->mConstantLayouts, constBufferIndex);

		r2::draw::cmd::FillConstantBuffer* fillConstantBufferCommand = r2::draw::renderer::AddFillConstantBufferCommand<mem::StackArena>(*s_optrRenderer->mCommandArena, *s_optrRenderer->mCommandBucket, fillKey, config.layout.GetSize());
		return r2::draw::cmd::FillConstantBufferCommand(fillConstantBufferCommand, handle, constBufferData->type, constBufferData->isPersistent, data, config.layout.GetElements().at(elementIndex).size, config.layout.GetElements().at(elementIndex).offset);
	}

	void UpdateSceneLighting(const r2::draw::LightSystem& lightSystem)
	{
		key::Basic lightKey;
		lightKey.keyValue = 0;

		cmd::FillConstantBuffer* fillLightsCMD = AddFillConstantBufferCommand(*s_optrRenderer->mCommandArena, *s_optrRenderer->mCommandBucket, lightKey, sizeof(r2::draw::SceneLighting));

		ConstantBufferHandle lightBufferHandle = r2::sarr::At(*s_optrRenderer->mConstantBufferHandles, s_optrRenderer->mLightingConfigHandle);

		ConstantBufferData* constBufferData = GetConstData(lightBufferHandle);

		r2::draw::cmd::FillConstantBufferCommand(fillLightsCMD, lightBufferHandle, constBufferData->type, constBufferData->isPersistent, &lightSystem.mSceneLighting, sizeof(r2::draw::SceneLighting), 0);
	}

	r2::draw::cmd::Clear* AddClearCommand(CommandBucket<key::Basic>& bucket, r2::draw::key::Basic key)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		R2_CHECK(s_optrRenderer != nullptr, "We haven't initialized the renderer yet!");

		return r2::draw::cmdbkt::AddCommand<r2::draw::key::Basic, r2::mem::StackArena, r2::draw::cmd::Clear>(*s_optrRenderer->mCommandArena, bucket, key, 0);
	}

	template<class ARENA>
	r2::draw::cmd::FillConstantBuffer* AddFillConstantBufferCommand(ARENA& arena, CommandBucket<key::Basic>& bucket, r2::draw::key::Basic key, u64 auxMemory)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		return r2::draw::renderer::AddCommand<r2::draw::cmd::FillConstantBuffer, ARENA>(arena, bucket, key, auxMemory);
	}

	ConstantBufferData* GetConstData(ConstantBufferHandle handle)
	{
		ConstantBufferData defaultConstBufferData;
		ConstantBufferData& constData = r2::shashmap::Get(*s_optrRenderer->mConstantBufferData, handle, defaultConstBufferData);

		if (constData.handle == EMPTY_BUFFER)
		{
			R2_CHECK(false, "We didn't find the constant buffer data associated with this handle: %u", handle);
			
			return nullptr;
		}

		return &constData;
	}

	ConstantBufferData* GetConstDataByConfigHandle(ConstantConfigHandle handle)
	{
		auto constantBufferHandles = GetConstantBufferHandles();

		return GetConstData(r2::sarr::At(*constantBufferHandles, handle));
	}

	struct DrawCommandData
	{
		ShaderHandle shaderId = InvalidShader;
		DrawLayer layer = DL_WORLD;
		b32 isDynamic = false;
		r2::SArray<cmd::DrawBatchSubCommand>* subCommands = nullptr;
	};

	struct BatchRenderOffsets
	{
		ShaderHandle shaderId = InvalidShader;
		PrimitiveType primitiveType;
		DrawLayer layer = DL_WORLD;
		u32 subCommandsOffset = 0;
		u32 numSubCommands = 0;
		b32 depthEnabled = false;
	};

	void FillRenderMaterial(const Material& material, RenderMaterial& renderMaterial)
	{
		renderMaterial.baseColor = material.baseColor;
		renderMaterial.metallic = material.metallic;
		renderMaterial.roughness = material.roughness;
		renderMaterial.specular = material.specular;
		renderMaterial.reflectance = material.reflectance;
		renderMaterial.ambientOcclusion = material.ambientOcclusion;

		renderMaterial.diffuseTexture = texsys::GetTextureAddress(material.diffuseTexture);
		renderMaterial.specularTexture = texsys::GetTextureAddress(material.specularTexture);
		renderMaterial.normalMapTexture = texsys::GetTextureAddress(material.normalMapTexture);
		renderMaterial.emissionTexture = texsys::GetTextureAddress(material.emissionTexture);
		renderMaterial.metallicTexture = texsys::GetTextureAddress(material.metallicTexture);
		renderMaterial.roughnessTexture = texsys::GetTextureAddress(material.roughnessTexture);
		renderMaterial.aoTexture = texsys::GetTextureAddress(material.aoTexture);
		renderMaterial.heightTexture = texsys::GetTextureAddress(material.heightTexture);
	}

	void PopulateRenderDataFromRenderBatch(r2::SArray<void*>* tempAllocations, const RenderBatch& renderBatch, r2::SHashMap<DrawCommandData*>* shaderDrawCommandData, r2::SArray<RenderMaterial>* renderMaterials, u32 baseInstanceOffset, u32 drawCommandBatchSize)
	{
		const u64 numStaticModels = r2::sarr::Size(*renderBatch.modelRefs);

		for (u64 modelIndex = 0; modelIndex < numStaticModels; ++modelIndex)
		{
			const ModelRef& modelRef = r2::sarr::At(*renderBatch.modelRefs, modelIndex);
			const cmd::DrawState& drawState = r2::sarr::At(*renderBatch.drawState, modelIndex);

			const u32 numMeshRefs = modelRef.mNumMeshRefs;

			const MaterialBatch::Info& materialBatchInfo = r2::sarr::At(*renderBatch.materialBatch.infos, modelIndex);

			ShaderHandle shaders[MAX_NUM_MESHES];

			for (u32 materialIndex = 0; materialIndex < materialBatchInfo.numMaterials; ++materialIndex)
			{
				const MaterialHandle materialHandle = r2::sarr::At(*renderBatch.materialBatch.materialHandles, materialBatchInfo.start + materialIndex);

				R2_CHECK(!mat::IsInvalidHandle(materialHandle), "This can't be invalid!");

				r2::draw::MaterialSystem* matSystem = r2::draw::matsys::GetMaterialSystem(materialHandle.slot);

				R2_CHECK(matSystem != nullptr, "Failed to get the material system!");

				const Material* material = mat::GetMaterial(*matSystem, materialHandle);

				R2_CHECK(material != nullptr, "Invalid material?");

				RenderMaterial nextRenderMaterial;

				FillRenderMaterial(*material, nextRenderMaterial);

				r2::sarr::Push(*renderMaterials, nextRenderMaterial);

				shaders[materialIndex] = material->shaderId;
			}

			for (u32 renderMaterialIndex = materialBatchInfo.numMaterials; renderMaterialIndex < MAX_NUM_MATERIAL_TEXTURES_PER_OBJECT; ++renderMaterialIndex)
			{
				RenderMaterial emptyRenderMaterial = {};
				r2::sarr::Push(*renderMaterials, emptyRenderMaterial);
			}

			R2_CHECK(numMeshRefs >= materialBatchInfo.numMaterials, "We should always have greater than or equal the amount of meshes to materials for a model");


			if (numMeshRefs != materialBatchInfo.numMaterials)
			{
				R2_CHECK(materialBatchInfo.numMaterials == 1, "We should probably only have 1 material in this case");
			}
			
			for (u32 shaderIndex = materialBatchInfo.numMaterials; shaderIndex < numMeshRefs; ++shaderIndex)
			{
				shaders[shaderIndex] = shaders[0]; //This is kind of assuming that there was only 1 shader and all meshes use the same one
			}

			for (u32 meshRefIndex = 0; meshRefIndex < numMeshRefs; ++meshRefIndex)
			{
				const MeshRef& meshRef = modelRef.mMeshRefs[meshRefIndex];

				ShaderHandle shaderId = shaders[meshRefIndex];

				R2_CHECK(shaderId != r2::draw::InvalidShader, "We don't have a proper shader?");


				key::Basic commandKey = key::GenerateBasicKey(0, 0, drawState.layer, 0, 0, shaderId);

				DrawCommandData* defaultDrawCommandData = nullptr;

				DrawCommandData* drawCommandData = r2::shashmap::Get(*shaderDrawCommandData, commandKey.keyValue, defaultDrawCommandData);

				if (drawCommandData == defaultDrawCommandData)
				{
					drawCommandData = ALLOC(DrawCommandData, *MEM_ENG_SCRATCH_PTR);

					r2::sarr::Push(*tempAllocations, (void*)drawCommandData);

					R2_CHECK(drawCommandData != nullptr, "We couldn't allocate a drawCommandData!");

					drawCommandData->shaderId = shaderId;
					drawCommandData->isDynamic = modelRef.mAnimated;
					drawCommandData->layer = drawState.layer;
					drawCommandData->subCommands = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, cmd::DrawBatchSubCommand, drawCommandBatchSize);
					r2::sarr::Push(*tempAllocations, (void*)drawCommandData->subCommands);

					r2::shashmap::Set(*shaderDrawCommandData, commandKey.keyValue, drawCommandData);
				}

				R2_CHECK(drawCommandData != nullptr, "This shouldn't be nullptr!");

				r2::draw::cmd::DrawBatchSubCommand subCommand;
				subCommand.baseInstance = baseInstanceOffset + modelIndex;
				subCommand.baseVertex = meshRef.baseVertex;
				subCommand.firstIndex = meshRef.baseIndex;
				subCommand.instanceCount = 1;
				subCommand.count = meshRef.numIndices;

				r2::sarr::Push(*drawCommandData->subCommands, subCommand);
				
			}
		}
	}

	void PreRender(Renderer& renderer)
	{
		//PreRender should be setting up the batches to render

		r2::SArray<void*>* tempAllocations = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, void*, 100); //@TODO(Serge): measure how many allocations

		const r2::SArray<r2::draw::ConstantBufferHandle>* constHandles = r2::draw::renderer::GetConstantBufferHandles();
		const VertexLayoutConfigHandle& animVertexLayoutHandles = r2::sarr::At(*renderer.mVertexLayoutConfigHandles, renderer.mAnimVertexModelConfigHandle);
		const VertexLayoutConfigHandle& staticVertexLayoutHandles = r2::sarr::At(*renderer.mVertexLayoutConfigHandles, renderer.mStaticVertexModelConfigHandle);
		const VertexLayoutConfigHandle& finalBatchVertexLayoutConfigHandle = r2::sarr::At(*renderer.mVertexLayoutConfigHandles, renderer.mFinalBatchVertexLayoutConfigHandle);

		const RenderBatch& staticRenderBatch = r2::sarr::At(*renderer.mRenderBatches, DrawType::STATIC);
		const RenderBatch& dynamicRenderBatch = r2::sarr::At(*renderer.mRenderBatches, DrawType::DYNAMIC);
		const u64 numStaticModels = r2::sarr::Size(*staticRenderBatch.modelRefs);
		const u64 numDynamicModels = r2::sarr::Size(*dynamicRenderBatch.modelRefs);

		//We can calculate exactly how many materials there are
		u64 totalSubCommands = 0;
		u64 staticDrawCommandBatchSize = 0;
		u64 dynamicDrawCommandBatchSize = 0;

		for (u64 i = 0; i < numStaticModels; ++i)
		{
			const ModelRef& modelRef = r2::sarr::At(*staticRenderBatch.modelRefs, i);

			totalSubCommands += modelRef.mNumMeshRefs;
		}

		staticDrawCommandBatchSize = totalSubCommands;

		r2::SArray<glm::ivec4>* boneOffsets = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::ivec4, numDynamicModels);

		r2::sarr::Push(*tempAllocations, (void*)boneOffsets);
		u32 boneOffset = 0;

		for (u64 i = 0; i < numDynamicModels; ++i)
		{
			const ModelRef& modelRef = r2::sarr::At(*dynamicRenderBatch.modelRefs, i);

			R2_CHECK(modelRef.mAnimated, "This should be animated if it's dynamic");

			r2::sarr::Push(*boneOffsets, glm::ivec4(boneOffset, 0, 0, 0));

			boneOffset += modelRef.mNumBones;

			totalSubCommands += modelRef.mNumMeshRefs;
		}

		dynamicDrawCommandBatchSize = totalSubCommands - staticDrawCommandBatchSize;

		//@Threading: If we want to thread this in the future, we can call PopulateRenderDataFromRenderBatch from different threads provided they have their own temp allocators
		//			  You will need to add jobs(or whatever) to dealloc the memory after we merge and create the prerenderbucket which might be tricky since we'll have to make sure the proper threads free the memory

		r2::SArray<RenderMaterial>* renderMaterials = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, RenderMaterial, (numStaticModels + numDynamicModels) * MAX_NUM_MATERIAL_TEXTURES_PER_OBJECT);

		r2::sarr::Push(*tempAllocations, (void*)renderMaterials);

		r2::SHashMap<DrawCommandData*>* shaderDrawCommandData = MAKE_SHASHMAP(*MEM_ENG_SCRATCH_PTR, DrawCommandData*, totalSubCommands * r2::SHashMap<DrawCommandData>::LoadFactorMultiplier());

		r2::sarr::Push(*tempAllocations, (void*)shaderDrawCommandData);

		PopulateRenderDataFromRenderBatch(tempAllocations, dynamicRenderBatch, shaderDrawCommandData, renderMaterials, 0, dynamicDrawCommandBatchSize);
		PopulateRenderDataFromRenderBatch(tempAllocations, staticRenderBatch, shaderDrawCommandData, renderMaterials, numDynamicModels, staticDrawCommandBatchSize);


		key::Basic basicKey;
		basicKey.keyValue = 0;

		const u64 numModels =  numDynamicModels + numStaticModels + 1; //+1 for the final batch model
		const u64 finalBatchModelOffset = numDynamicModels + numStaticModels;
		const u64 modelsMemorySize = numModels * sizeof(glm::mat4);

		glm::mat4 finalBatchModelMat = glm::mat4(1.0f);
		//Add final batch here
		{
			//We need to scale because our quad is -0.5 to 0.5 and it needs to be -1.0 to 1.0
			finalBatchModelMat = glm::scale(finalBatchModelMat, glm::vec3(2.0f));
		}

		cmd::FillConstantBuffer* modelsCmd = AddFillConstantBufferCommand(*renderer.mPrePostRenderCommandArena, *renderer.mPreRenderBucket, basicKey, modelsMemorySize);

		char* modelsAuxMemory = cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(modelsCmd);
		 
		const u64 dynamicModelsMemorySize = numDynamicModels * sizeof(glm::mat4);
		const u64 staticModelsMemorySize = numStaticModels * sizeof(glm::mat4);

		memcpy(modelsAuxMemory, dynamicRenderBatch.models->mData, dynamicModelsMemorySize);
		memcpy(mem::utils::PointerAdd(modelsAuxMemory, dynamicModelsMemorySize), staticRenderBatch.models->mData, staticModelsMemorySize);
		memcpy(mem::utils::PointerAdd(modelsAuxMemory, dynamicModelsMemorySize + staticModelsMemorySize), glm::value_ptr(finalBatchModelMat), sizeof(glm::mat4));

		auto modelsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mModelConfigHandle);

		//fill out constCMD
		{
			ConstantBufferData* modelConstData = GetConstData(modelsConstantBufferHandle);

			modelsCmd->data = modelsAuxMemory;
			modelsCmd->dataSize = modelsMemorySize;
			modelsCmd->offset = modelConstData->currentOffset;
			modelsCmd->constantBufferHandle = modelsConstantBufferHandle;

			modelsCmd->isPersistent = modelConstData->isPersistent;
			modelsCmd->type = modelConstData->type;

			modelConstData->AddDataSize(modelsMemorySize);
		}

		const u64 numRenderMaterials = r2::sarr::Size(*renderMaterials);
		const u64 materialsDataSize = sizeof(r2::draw::RenderMaterial) * numRenderMaterials;

		cmd::FillConstantBuffer* materialsCMD = nullptr;

		materialsCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, modelsCmd, materialsDataSize);

		auto materialsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mMaterialConfigHandle);

		ConstantBufferData* materialsConstData = GetConstData(materialsConstantBufferHandle);

		FillConstantBufferCommand(
			materialsCMD,
			materialsConstantBufferHandle,
			materialsConstData->type,
			materialsConstData->isPersistent,
			renderMaterials->mData,
			materialsDataSize,
			materialsConstData->currentOffset);

		materialsConstData->AddDataSize(materialsDataSize);

		cmd::FillConstantBuffer* prevFillCMD = materialsCMD;

		if (dynamicRenderBatch.boneTransforms && r2::sarr::Size(*dynamicRenderBatch.boneTransforms) > 0)
		{
			const u64 boneTransformMemorySize = dynamicRenderBatch.boneTransforms->mSize * sizeof(ShaderBoneTransform);
			r2::draw::cmd::FillConstantBuffer* boneTransformsCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, materialsCMD, boneTransformMemorySize);

			auto boneTransformsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mBoneTransformsConfigHandle);
			ConstantBufferData* boneXFormConstData = GetConstData(boneTransformsConstantBufferHandle);

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

			ConstantBufferData* boneXFormOffsetsConstData = GetConstData(boneTransformOffsetsConstantBufferHandle);
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


		r2::SArray<BatchRenderOffsets>* staticRenderBatchesOffsets = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, BatchRenderOffsets, staticDrawCommandBatchSize);//@NOTE: pretty sure this is an overestimate - could reduce to save mem
		r2::SArray<BatchRenderOffsets>* dynamicRenderBatchesOffsets = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, BatchRenderOffsets, dynamicDrawCommandBatchSize); 

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
				//drawCommandData
				const u32 numSubCommandsInBatch = static_cast<u32>(r2::sarr::Size(*drawCommandData->subCommands));
				const u64 batchSubCommandsMemorySize = numSubCommandsInBatch * sizeof(cmd::DrawBatchSubCommand);
				memcpy(mem::utils::PointerAdd(subCommandsAuxMemory, subCommandsMemoryOffset), drawCommandData->subCommands->mData, batchSubCommandsMemorySize);

				BatchRenderOffsets batchOffsets;
				batchOffsets.shaderId = drawCommandData->shaderId;
				batchOffsets.subCommandsOffset = subCommandsOffset;

				batchOffsets.numSubCommands = numSubCommandsInBatch;
				R2_CHECK(batchOffsets.numSubCommands > 0, "We should have a count!");
				batchOffsets.layer = drawCommandData->layer;

				if (drawCommandData->isDynamic)
				{
					r2::sarr::Push(*dynamicRenderBatchesOffsets, batchOffsets);
				}
				else
				{
					r2::sarr::Push(*staticRenderBatchesOffsets, batchOffsets);
				}

				subCommandsMemoryOffset += batchSubCommandsMemorySize;
				subCommandsOffset += numSubCommandsInBatch;
			}
		}


		//Make our final batch data here
		BatchRenderOffsets finalBatchOffsets;
		{
			const ModelRef& quadModelRef = GetDefaultModelRef(QUAD);

			r2::draw::MaterialSystem* matSystem = r2::draw::matsys::GetMaterialSystem(renderer.mFinalCompositeMaterialHandle.slot);
			R2_CHECK(matSystem != nullptr, "Failed to get the material system!");

			const Material* material = mat::GetMaterial(*matSystem, renderer.mFinalCompositeMaterialHandle);

			cmd::DrawBatchSubCommand finalBatchSubcommand;
			finalBatchSubcommand.baseInstance = finalBatchModelOffset;
			finalBatchSubcommand.baseVertex = quadModelRef.mMeshRefs[0].baseVertex;
			finalBatchSubcommand.firstIndex = quadModelRef.mMeshRefs[0].baseIndex;
			finalBatchSubcommand.instanceCount = 1;
			finalBatchSubcommand.count = quadModelRef.mMeshRefs[0].numIndices;
			memcpy(mem::utils::PointerAdd(subCommandsAuxMemory, subCommandsMemoryOffset), &finalBatchSubcommand, sizeof(cmd::DrawBatchSubCommand));

			finalBatchOffsets.layer = DL_SCREEN;
			finalBatchOffsets.numSubCommands = 1;
			finalBatchOffsets.subCommandsOffset = subCommandsOffset;
			finalBatchOffsets.shaderId = material->shaderId;

			subCommandsMemoryOffset += sizeof(cmd::DrawBatchSubCommand);
			subCommandsOffset += 1;
		}


		auto subCommandsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mSubcommandsConfigHandle);

		ConstantBufferData* subCommandsConstData = GetConstData(subCommandsConstantBufferHandle);

		subCommandsCMD->constantBufferHandle = subCommandsConstantBufferHandle;
		subCommandsCMD->data = subCommandsAuxMemory;
		subCommandsCMD->dataSize = subCommandsMemorySize;
		subCommandsCMD->offset = subCommandsConstData->currentOffset;
		subCommandsCMD->type = subCommandsConstData->type;
		subCommandsCMD->isPersistent = subCommandsConstData->isPersistent;

		subCommandsConstData->AddDataSize(subCommandsMemorySize);


		//PostRenderBucket commands
		{
			cmd::CompleteConstantBuffer* completeModelsCMD = AddCommand<cmd::CompleteConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, *renderer.mPostRenderBucket, basicKey, 0);
			completeModelsCMD->constantBufferHandle = modelsConstantBufferHandle;
			completeModelsCMD->count = numModels;

			cmd::CompleteConstantBuffer* completeMaterialsCMD = AppendCommand <cmd::CompleteConstantBuffer, cmd::CompleteConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, completeModelsCMD, 0);

			completeMaterialsCMD->constantBufferHandle = materialsConstantBufferHandle;
			completeMaterialsCMD->count = numRenderMaterials;

			cmd::CompleteConstantBuffer* prevCompleteCMD = completeMaterialsCMD;

			if (dynamicRenderBatch.boneTransforms && r2::sarr::Size(*dynamicRenderBatch.boneTransforms) > 0)
			{
				auto boneTransformsConstantBufferHandle = r2::sarr::At(*constHandles, renderer.mBoneTransformsConfigHandle);

				cmd::CompleteConstantBuffer* completeBoneTransformsCMD = AppendCommand <cmd::CompleteConstantBuffer, cmd::CompleteConstantBuffer, mem::StackArena>(*renderer.mPrePostRenderCommandArena, completeMaterialsCMD, 0);

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
		clearGBufferOptions.flags = cmd::CLEAR_COLOR_BUFFER | cmd::CLEAR_DEPTH_BUFFER;

		
		
		const u64 numStaticDrawBatches = r2::sarr::Size(*staticRenderBatchesOffsets);

		ShaderHandle clearShaderHandle = InvalidShader;

		if (numStaticDrawBatches > 0)
		{
			const auto& batchOffset = r2::sarr::At(*staticRenderBatchesOffsets, 0);
			clearShaderHandle = batchOffset.shaderId;
		}

		BeginRenderPass(renderer, RPT_GBUFFER, clearGBufferOptions, clearShaderHandle, *renderer.mCommandBucket, *renderer.mCommandArena);

		//@TODO(Serge): figure out how these two for loops will work with different render passes
		for (u64 i = 0; i < numStaticDrawBatches; ++i)
		{
			const auto& batchOffset = r2::sarr::At(*staticRenderBatchesOffsets, i);

			key::Basic key = key::GenerateBasicKey(0, 0, batchOffset.layer, 0, 0, batchOffset.shaderId);

			

			cmd::DrawBatch* drawBatch = AddCommand<cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mCommandBucket, key, 0);
			drawBatch->batchHandle = subCommandsConstantBufferHandle;
			drawBatch->bufferLayoutHandle = staticVertexLayoutHandles.mBufferLayoutHandle;
			drawBatch->numSubCommands = batchOffset.numSubCommands;
			R2_CHECK(drawBatch->numSubCommands > 0, "We should have a count!");
			drawBatch->startCommandIndex = batchOffset.subCommandsOffset;
			drawBatch->primitiveType = PrimitiveType::TRIANGLES;
			drawBatch->subCommands = nullptr;
			drawBatch->state.depthEnabled = true;//TODO(Serge): fix with proper draw state

			//@TODO(Serge): add commands to different buckets 
			
		}

		const u64 numDynamicDrawBatches = r2::sarr::Size(*dynamicRenderBatchesOffsets);
		for (u64 i = 0; i < numDynamicDrawBatches; ++i)
		{
			const auto& batchOffset = r2::sarr::At(*dynamicRenderBatchesOffsets, i);

			key::Basic key = key::GenerateBasicKey(0, 0, batchOffset.layer, 0, 0, batchOffset.shaderId);

			cmd::DrawBatch* drawBatch = AddCommand<cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mCommandBucket, key, 0);
			drawBatch->batchHandle = subCommandsConstantBufferHandle;
			drawBatch->bufferLayoutHandle = animVertexLayoutHandles.mBufferLayoutHandle;
			drawBatch->numSubCommands = batchOffset.numSubCommands;
			R2_CHECK(drawBatch->numSubCommands > 0, "We should have a count!");
			drawBatch->startCommandIndex = batchOffset.subCommandsOffset;
			drawBatch->primitiveType = PrimitiveType::TRIANGLES;
			drawBatch->subCommands = nullptr;
			drawBatch->state.depthEnabled = true;//TODO(Serge): fix with proper draw state

			//@TODO(Serge): add commands to different buckets

		}

		EndRenderPass(renderer, RPT_GBUFFER, *renderer.mCommandBucket);


		ClearSurfaceOptions clearCompositeOptions;
		clearCompositeOptions.shouldClear = true;
		clearCompositeOptions.flags = cmd::CLEAR_COLOR_BUFFER;


		BeginRenderPass(renderer, RPT_FINAL_COMPOSITE, clearCompositeOptions, finalBatchOffsets.shaderId, *renderer.mFinalBucket, *renderer.mCommandArena);

		key::Basic finalBatchKey = key::GenerateBasicKey(0, 0, finalBatchOffsets.layer, 0, 0, finalBatchOffsets.shaderId);

		cmd::DrawBatch* finalDrawBatch = AddCommand<cmd::DrawBatch, mem::StackArena>(*renderer.mCommandArena, *renderer.mFinalBucket, finalBatchKey, 0); //@TODO(Serge): we should have mFinalBucket have it's own arena instead of renderer.mCommandArena
		finalDrawBatch->batchHandle = subCommandsConstantBufferHandle;
		finalDrawBatch->bufferLayoutHandle = finalBatchVertexLayoutConfigHandle.mBufferLayoutHandle;
		finalDrawBatch->numSubCommands = finalBatchOffsets.numSubCommands;
		R2_CHECK(finalDrawBatch->numSubCommands > 0, "We should have a count!");
		finalDrawBatch->startCommandIndex = finalBatchOffsets.subCommandsOffset;
		finalDrawBatch->primitiveType = PrimitiveType::TRIANGLES;
		finalDrawBatch->subCommands = nullptr;
		finalDrawBatch->state.depthEnabled = false;

		EndRenderPass(renderer, RPT_FINAL_COMPOSITE, *renderer.mFinalBucket);

		const s64 numAllocations = r2::sarr::Size(*tempAllocations);

		for (s64 i = numAllocations - 1; i >= 0; --i)
		{
			FREE(r2::sarr::At(*tempAllocations, i), *MEM_ENG_SCRATCH_PTR );
		}

		r2::sarr::Clear(*tempAllocations);

		FREE(tempAllocations, *MEM_ENG_SCRATCH_PTR);
	}

	void ClearRenderBatches(Renderer& renderer)
	{
		u32 numRenderBatches = r2::sarr::Size(*renderer.mRenderBatches);

		for (u32 i = 0; i < numRenderBatches; ++i)
		{
			RenderBatch& batch = r2::sarr::At(*renderer.mRenderBatches, i);

			r2::sarr::Clear(*batch.modelRefs);
			r2::sarr::Clear(*batch.materialBatch.infos);
			r2::sarr::Clear(*batch.materialBatch.materialHandles);
			r2::sarr::Clear(*batch.models);
			r2::sarr::Clear(*batch.drawState);
			if (batch.boneTransforms)
			{
				r2::sarr::Clear(*batch.boneTransforms);
			}
		}
	}

	RenderTarget* GetRenderTarget(Renderer& renderer, RenderTargetSurface surface)
	{
		if (surface == RTS_EMPTY || surface == NUM_RENDER_PASSES)
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
		renderer.mRenderPasses[RPT_GBUFFER] = rp::CreateRenderPass(*renderer.mSubAreaArena, RPT_GBUFFER, passConfig, {}, RTS_GBUFFER, __FILE__, __LINE__, "");

		renderer.mRenderPasses[RPT_FINAL_COMPOSITE] = rp::CreateRenderPass(*renderer.mSubAreaArena, RPT_FINAL_COMPOSITE, passConfig, { RTS_GBUFFER }, RTS_COMPOSITE, __FILE__, __LINE__, "");
	}

	void DestroyRenderPasses(Renderer& renderer)
	{
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_FINAL_COMPOSITE]);
		rp::DestroyRenderPass(*renderer.mSubAreaArena, renderer.mRenderPasses[RPT_GBUFFER]);
	}

	void BeginRenderPass(Renderer& renderer, RenderPassType renderPassType, const ClearSurfaceOptions& clearOptions, const ShaderHandle& shaderHandle, CommandBucket<key::Basic>& commandBucket, mem::StackArena& arena)
	{
		RenderPass* renderPass = GetRenderPass(renderer, renderPassType);

		R2_CHECK(renderPass != nullptr, "This should never be null");

		RenderTarget* renderTarget = GetRenderTarget(renderer, renderPass->renderOutputTargetHandle );

		R2_CHECK(renderTarget != nullptr, "We have an null render target!");

		key::Basic renderKey = key::GenerateBasicKey(0, 0, DL_CLEAR, 0, 0, shaderHandle);

		cmd::SetRenderTarget* setRenderTargetCMD = AddCommand<cmd::SetRenderTarget, mem::StackArena>(arena, commandBucket, renderKey, 0);
			
		setRenderTargetCMD->framebufferID = renderTarget->frameBufferID;

		setRenderTargetCMD->numAttachments = 0;
		
		if (renderTarget->colorAttachments)
		{
			setRenderTargetCMD->numAttachments = static_cast<u32>(r2::sarr::Size(*renderTarget->colorAttachments));
		}
		 
		setRenderTargetCMD->xOffset = renderTarget->xOffset;
		setRenderTargetCMD->yOffset = renderTarget->yOffset;
		setRenderTargetCMD->width = renderTarget->width;
		setRenderTargetCMD->height = renderTarget->height;

		cmd::Clear* clearCMD = nullptr;
		if (clearOptions.shouldClear)
		{
			clearCMD = AppendCommand<cmd::SetRenderTarget, cmd::Clear, mem::StackArena>(arena, setRenderTargetCMD, 0);
			clearCMD->flags = clearOptions.flags;
		}

		const auto numInputTextures = renderPass->numRenderInputTargets;

		ConstantBufferHandle surfaceBufferHandle = r2::sarr::At(*renderer.mConstantBufferHandles, renderer.mSurfacesConfigHandle);

		ConstantBufferData* constBufferData = GetConstData(surfaceBufferHandle);

		cmd::FillConstantBuffer* prevCommand = nullptr;

		for (u32 i = 0; i < numInputTextures; ++i)
		{
			RenderTarget* inputRenderTarget = GetRenderTarget(renderer, renderPass->renderInputTargetHandles[i]);

			R2_CHECK(inputRenderTarget != nullptr, "We should have a render target here!");

			cmd::FillConstantBuffer* fillSurfaceCMD = nullptr;

			if (i == 0)
			{
				if (clearCMD)
				{
					fillSurfaceCMD = AppendCommand<cmd::Clear, cmd::FillConstantBuffer, mem::StackArena>(arena, clearCMD, sizeof(tex::TextureAddress));
				}
				else
				{
					fillSurfaceCMD = AppendCommand<cmd::SetRenderTarget, cmd::FillConstantBuffer, mem::StackArena>(arena, setRenderTargetCMD, sizeof(tex::TextureAddress));
				}
			}
			else
			{
				R2_CHECK(prevCommand != nullptr, "Should never be null here");
				fillSurfaceCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer, mem::StackArena>(arena, prevCommand, sizeof(tex::TextureAddress));
			}

			auto surfaceTextureAddress = texsys::GetTextureAddress(r2::sarr::At(*inputRenderTarget->colorAttachments, 0).texture);

			FillConstantBufferCommand(fillSurfaceCMD, surfaceBufferHandle, constBufferData->type, constBufferData->isPersistent, &surfaceTextureAddress, sizeof(tex::TextureAddress), renderPass->renderInputTargetHandles[i] * sizeof(tex::TextureAddress));

			prevCommand = fillSurfaceCMD;
		}
	}

	void EndRenderPass(Renderer& renderer, RenderPassType renderPass, r2::draw::CommandBucket<key::Basic>& commandBucket)
	{
		cmdbkt::Close(commandBucket);
	}


	void UpdatePerspectiveMatrix(const glm::mat4& perspectiveMatrix)
	{
		const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles();

		AddFillConstantBufferCommandForData(
			r2::sarr::At(*constantBufferHandles, s_optrRenderer->mVPMatricesConfigHandle),
			0,
			glm::value_ptr(perspectiveMatrix));

	}

	void UpdateViewMatrix(const glm::mat4& viewMatrix)
	{
		const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles();

		AddFillConstantBufferCommandForData(
			r2::sarr::At(*constantBufferHandles, s_optrRenderer->mVPMatricesConfigHandle),
			1,
			glm::value_ptr(viewMatrix));

		
		AddFillConstantBufferCommandForData(
			r2::sarr::At(*constantBufferHandles, s_optrRenderer->mVPMatricesConfigHandle),
			2,
			glm::value_ptr(glm::mat4(glm::mat3(viewMatrix))));
	}

	void UpdateCameraPosition(const glm::vec3& camPosition)
	{
		const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles();

		glm::vec4 cameraPosTimeW = glm::vec4(camPosition, CENG.GetTicks() / 1000.0f);

		AddFillConstantBufferCommandForData(r2::sarr::At(*constantBufferHandles, s_optrRenderer->mVectorsConfigHandle),
			0, glm::value_ptr(cameraPosTimeW));
	}

	void UpdateExposure(float exposure)
	{
		const r2::SArray<ConstantBufferHandle>* constantBufferHandles = GetConstantBufferHandles();

		glm::vec4 exposureVec = glm::vec4(exposure, 0, 0, 0);
		r2::draw::renderer::AddFillConstantBufferCommandForData(r2::sarr::At(*constantBufferHandles, s_optrRenderer->mVectorsConfigHandle),
			1, glm::value_ptr(exposureVec));
	}

	void DrawModels(const r2::SArray<ModelRef>& modelRefs, const r2::SArray<glm::mat4>& modelMatrices, const r2::SArray<DrawFlags>& flags, const r2::SArray<ShaderBoneTransform>* boneTransforms)
	{
		R2_CHECK(!r2::sarr::IsEmpty(modelRefs), "This should not be empty!");

		const ModelRef& firstModelRef = r2::sarr::At(modelRefs, 0);

		DrawModelsOnLayer(firstModelRef.mAnimated? DL_CHARACTER : DL_WORLD, modelRefs, nullptr, modelMatrices, flags, boneTransforms);
	}

	void DrawModel(const ModelRef& modelRef, const glm::mat4& modelMatrix, const DrawFlags& flags, const r2::SArray<ShaderBoneTransform>* boneTransforms)
	{
		DrawModelOnLayer(modelRef.mAnimated ? DL_CHARACTER : DL_WORLD, modelRef, nullptr, modelMatrix, flags, boneTransforms);
	}

	void DrawModelOnLayer(DrawLayer layer, const ModelRef& modelRef, const r2::SArray<MaterialHandle>* materials, const glm::mat4& modelMatrix, const DrawFlags& flags, const r2::SArray<ShaderBoneTransform>* boneTransforms)
	{
		if (s_optrRenderer == nullptr || s_optrRenderer->mRenderBatches == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		if (s_optrRenderer->mStaticVertexModelConfigHandle == InvalidVertexConfigHandle ||
			s_optrRenderer->mAnimVertexModelConfigHandle == InvalidVertexConfigHandle)
		{
			R2_CHECK(false, "We haven't generated the layouts yet!");
			return;
		}

		DrawType drawType = STATIC;

		if (modelRef.mAnimated && !boneTransforms)
		{
			R2_CHECK(false, "Submitted an animated model with no bone transforms!");
		}

		if (modelRef.mAnimated && layer != DL_CHARACTER)
		{
			R2_CHECK(false, "Submitted an animated model not on the character layer!");
		}

		if (modelRef.mAnimated)
		{
			drawType = DYNAMIC;
		}

		RenderBatch& batch = r2::sarr::At(*s_optrRenderer->mRenderBatches, drawType);

		r2::sarr::Push(*batch.modelRefs, modelRef);

		r2::sarr::Push(*batch.models, modelMatrix);

		cmd::DrawState state;
		state.depthEnabled = flags.IsSet(DEPTH_TEST);
		state.layer = layer;

		r2::sarr::Push(*batch.drawState, state);

		if (!materials)
		{
			MaterialBatch::Info materialBatchInfo;

			materialBatchInfo.start = r2::sarr::Size(*batch.materialBatch.materialHandles);
			materialBatchInfo.numMaterials = modelRef.mNumMaterialHandles;

			r2::sarr::Push(*batch.materialBatch.infos, materialBatchInfo);
			
			for (u32 i = 0; i < modelRef.mNumMaterialHandles; ++i)
			{
				r2::sarr::Push(*batch.materialBatch.materialHandles, modelRef.mMaterialHandles[i]);
			}
		}
		else
		{
			u64 numMaterials = r2::sarr::Size(*materials);
			R2_CHECK(numMaterials == modelRef.mNumMaterialHandles, "This should be the same in this case");


			MaterialBatch::Info materialBatchInfo;

			materialBatchInfo.start = r2::sarr::Size(*batch.materialBatch.materialHandles);
			materialBatchInfo.numMaterials = numMaterials;

			r2::sarr::Push(*batch.materialBatch.infos, materialBatchInfo);

			r2::sarr::Append(*batch.materialBatch.materialHandles, *materials);
		}

		if (drawType == DYNAMIC)
		{
			R2_CHECK(boneTransforms != nullptr && r2::sarr::Size(*boneTransforms) > 0, "bone transforms should be valid");
			r2::sarr::Append(*batch.boneTransforms, *boneTransforms);
		}
	}

	void DrawModelsOnLayer(DrawLayer layer, const r2::SArray<ModelRef>& modelRefs, const r2::SArray<MaterialHandle>* materialHandles, const r2::SArray<glm::mat4>& modelMatrices, const r2::SArray<DrawFlags>& flags, const r2::SArray<ShaderBoneTransform>* boneTransforms)
	{
		if (s_optrRenderer == nullptr || s_optrRenderer->mRenderBatches == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		R2_CHECK(r2::sarr::Size(modelRefs) == r2::sarr::Size(modelMatrices), "These must be the same!");

		auto numModelRefs = r2::sarr::Size(modelRefs);

		R2_CHECK(numModelRefs > 0, "We should have model refs here!");

#if defined(R2_DEBUG)
		//check to see if what we're being given is all one type
		if (layer == DrawLayer::DL_WORLD)
		{
			for (u64 i = 0; i < numModelRefs; ++i)
			{
				R2_CHECK(r2::sarr::At(modelRefs, i).mAnimated == false, "modelRef at index: %llu should not be animated!", i);
			}
		}
		else if (layer == DrawLayer::DL_CHARACTER)
		{
			for (u64 i = 0; i < numModelRefs; ++i)
			{
				R2_CHECK(r2::sarr::At(modelRefs, i).mAnimated == true, "modelRef at index: %llu should be animated!", i);
			}
		}
#endif
		R2_CHECK(layer == DrawLayer::DL_WORLD || layer == DrawLayer::DL_SKYBOX || layer == DrawLayer::DL_CHARACTER, "These are the only supported draw layers");

		DrawType drawType = (layer == DrawLayer::DL_WORLD || layer == DrawLayer::DL_SKYBOX) ? DrawType::STATIC : DrawType::DYNAMIC;

		RenderBatch& batch = r2::sarr::At(*s_optrRenderer->mRenderBatches, drawType);

		r2::sarr::Append(*batch.modelRefs, modelRefs);

		r2::sarr::Append(*batch.models, modelMatrices);

		const u64 numFlags = r2::sarr::Size(flags);

		r2::SArray<cmd::DrawState>* tempDrawState = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, cmd::DrawState, numFlags);

		for (u64 i = 0; i < numFlags; ++i)
		{
			cmd::DrawState state;
			state.depthEnabled = r2::sarr::At(flags, i).IsSet(eDrawFlags::DEPTH_TEST);
			state.layer = drawType == STATIC ? DrawLayer::DL_WORLD : DrawLayer::DL_CHARACTER;

			r2::sarr::Push(*tempDrawState, state);
		}

		r2::sarr::Append(*batch.drawState, *tempDrawState);

		FREE(tempDrawState, *MEM_ENG_SCRATCH_PTR);

		if (!materialHandles)
		{
			for (u32 i = 0; i < numModelRefs; ++i)
			{
				const ModelRef& modelRef = r2::sarr::At(modelRefs, i);

				MaterialBatch::Info info;
				info.start = r2::sarr::Size(*batch.materialBatch.materialHandles);
				info.numMaterials = modelRef.mNumMaterialHandles;

				r2::sarr::Push(*batch.materialBatch.infos, info);

				for (u32 j = 0; j < modelRef.mNumMaterialHandles; ++j)
				{
					r2::sarr::Push(*batch.materialBatch.materialHandles, modelRef.mMaterialHandles[j]);
				}
			}
		}
		else
		{

			u32 materialOffset = 0;

			for (u32 i = 0; i < numModelRefs; ++i)
			{
				const ModelRef& modelRef = r2::sarr::At(modelRefs, i);


				MaterialBatch::Info materialBatchInfo;
				materialBatchInfo.start = r2::sarr::Size(*batch.materialBatch.materialHandles);
				materialBatchInfo.numMaterials = modelRef.mNumMaterialHandles;

				r2::sarr::Push(*batch.materialBatch.infos, materialBatchInfo);

				for (u32 j = 0; j < modelRef.mNumMaterialHandles; ++j)
				{
					r2::sarr::Push(*batch.materialBatch.materialHandles, r2::sarr::At(*materialHandles,j + materialOffset));
				}

				materialOffset += modelRef.mNumMaterialHandles;

			}
		}

		if (drawType == DYNAMIC)
		{
			R2_CHECK(boneTransforms != nullptr && r2::sarr::Size(*boneTransforms) > 0, "bone transforms should be valid");
			r2::sarr::Append(*batch.boneTransforms, *boneTransforms);
		}
	}

#ifdef R2_DEBUG


	struct DebugDrawCommandData
	{
		ShaderHandle shaderID = InvalidShader;
		DebugModelType modelType = DEBUG_LINE;
		PrimitiveType primitiveType = PrimitiveType::LINES;
		b32 depthEnabled = false;

		r2::SArray<cmd::DrawBatchSubCommand>* debugModelDrawBatchCommands = nullptr;
		r2::SArray<cmd::DrawDebugBatchSubCommand>* debugLineDrawBatchCommands = nullptr;
	};

	void CreateDebugSubCommands(Renderer& renderer, const DebugRenderBatch& debugRenderBatch, u32 numDebugObjectsToDraw, u32 instanceOffset, r2::SArray<void*>* tempAllocations, r2::SArray<DebugRenderConstants>* debugRenderConstants, r2::SHashMap<DebugDrawCommandData*>* debugModelDrawCommandData)
	{
		//@NOTE: this isn't at all thread safe! 
		if (renderer.mDebugCommandBucket == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		R2_CHECK(debugRenderBatch.transforms != nullptr || debugRenderBatch.matTransforms != nullptr, "Transforms haven't been created!");

		R2_CHECK(debugRenderBatch.drawFlags != nullptr, "We don't have any draw flags!");

		R2_CHECK(debugRenderBatch.colors != nullptr, "We don't have any colors!");

		if (debugRenderBatch.transforms)
		{
			R2_CHECK(r2::sarr::Size(*debugRenderBatch.transforms) == r2::sarr::Size(*debugRenderBatch.drawFlags) &&
				r2::sarr::Size(*debugRenderBatch.drawFlags) == r2::sarr::Size(*debugRenderBatch.colors) &&
				r2::sarr::Size(*debugRenderBatch.debugModelTypesToDraw) == r2::sarr::Size(*debugRenderBatch.drawFlags),
				"These should all be equal");
		}
		else
		{
			R2_CHECK(r2::sarr::Size(*debugRenderBatch.matTransforms) == r2::sarr::Size(*debugRenderBatch.drawFlags) &&
				r2::sarr::Size(*debugRenderBatch.drawFlags) == r2::sarr::Size(*debugRenderBatch.colors) &&
				r2::sarr::Size(*debugRenderBatch.vertices) / 2 == r2::sarr::Size(*debugRenderBatch.drawFlags),
				"These should all be equal");
		}

		R2_CHECK(!mat::IsInvalidHandle(debugRenderBatch.materialHandle), "This can't be invalid!");

		r2::draw::MaterialSystem* matSystem = r2::draw::matsys::GetMaterialSystem(debugRenderBatch.materialHandle.slot);

		R2_CHECK(matSystem != nullptr, "Failed to get the material system!");

		const Material* material = mat::GetMaterial(*matSystem, debugRenderBatch.materialHandle);

		R2_CHECK(material != nullptr, "Invalid material?");

		ShaderHandle shaderID = material->shaderId;

		for (u64 i = 0; i < numDebugObjectsToDraw; ++i)
		{
			const glm::vec4& color = r2::sarr::At(*debugRenderBatch.colors, i);
			const DrawFlags& flags = r2::sarr::At(*debugRenderBatch.drawFlags, i);
			glm::mat4 transform;

			DebugModelType modelType = DEBUG_LINE;

			if (debugRenderBatch.matTransforms)
			{
				transform = r2::sarr::At(*debugRenderBatch.matTransforms, i);
			}
			else
			{
				R2_CHECK(debugRenderBatch.transforms != nullptr, "We should have this in this case");
				transform = math::ToMatrix(r2::sarr::At(*debugRenderBatch.transforms, i));
				modelType = r2::sarr::At(*debugRenderBatch.debugModelTypesToDraw, i);
			}

			key::DebugKey debugKey = key::GenerateDebugKey(shaderID, flags.IsSet(eDrawFlags::FILL_MODEL) ? PrimitiveType::TRIANGLES : PrimitiveType::LINES, flags.IsSet(eDrawFlags::DEPTH_TEST), 0, 0);//@TODO(Serge): last two params unused - needed for transparency


			DebugDrawCommandData* defaultDebugDrawCommandData = nullptr;

			DebugDrawCommandData* debugDrawCommandData = r2::shashmap::Get(*debugModelDrawCommandData, debugKey.keyValue, defaultDebugDrawCommandData);

			if (debugDrawCommandData == defaultDebugDrawCommandData)
			{
				debugDrawCommandData = ALLOC(DebugDrawCommandData, *MEM_ENG_SCRATCH_PTR);
				r2::sarr::Push(*tempAllocations, (void*)debugDrawCommandData);

				if (modelType != DEBUG_LINE)
				{
					debugDrawCommandData->debugModelDrawBatchCommands = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, cmd::DrawBatchSubCommand, numDebugObjectsToDraw); //@NOTE(Serge): overestimate
					r2::sarr::Push(*tempAllocations, (void*)debugDrawCommandData->debugModelDrawBatchCommands);
				}
				else
				{
					debugDrawCommandData->debugLineDrawBatchCommands = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, cmd::DrawDebugBatchSubCommand, numDebugObjectsToDraw);
					r2::sarr::Push(*tempAllocations, (void*)debugDrawCommandData->debugLineDrawBatchCommands);
				}

				debugDrawCommandData->shaderID = shaderID;
				debugDrawCommandData->modelType = modelType;
				
				debugDrawCommandData->depthEnabled = flags.IsSet(eDrawFlags::DEPTH_TEST);
				debugDrawCommandData->primitiveType = flags.IsSet(eDrawFlags::FILL_MODEL) ? PrimitiveType::TRIANGLES : PrimitiveType::LINES;

				r2::shashmap::Set(*debugModelDrawCommandData, debugKey.keyValue, debugDrawCommandData);
			}

			DebugRenderConstants constants;
			constants.color = color;
			constants.modelMatrix = transform;

			r2::sarr::Push(*debugRenderConstants, constants);

			if (modelType != DEBUG_LINE)
			{
				const ModelRef& modelRef = GetDefaultModelRef(static_cast<DefaultModel>(modelType));

				for (u64 j = 0; j < modelRef.mNumMeshRefs; ++j)
				{
					r2::draw::cmd::DrawBatchSubCommand subCommand;
					subCommand.baseInstance = i + instanceOffset;
					subCommand.baseVertex = modelRef.mMeshRefs[j].baseVertex;
					subCommand.firstIndex = modelRef.mMeshRefs[j].baseIndex;
					subCommand.instanceCount = 1;
					subCommand.count = modelRef.mMeshRefs[j].numIndices;

					r2::sarr::Push(*debugDrawCommandData->debugModelDrawBatchCommands, subCommand);
				}
			}
			else
			{
				//it is a line
				r2::draw::cmd::DrawDebugBatchSubCommand subCommand;
				subCommand.baseInstance = i + instanceOffset;
				subCommand.instanceCount = 1;
				subCommand.count = 2;

				subCommand.firstVertex = i * 2;

				r2::sarr::Push(*debugDrawCommandData->debugLineDrawBatchCommands, subCommand);
			}
		}
	}

	void AddDebugSubCommandPreAndPostCMDs(Renderer& renderer, const DebugRenderBatch& debugRenderBatch, r2::SHashMap<DebugDrawCommandData*>* debugDrawCommandData, r2::SArray<BatchRenderOffsets>* debugRenderBatchesOffsets, u32 numObjectsToDraw, u64 subCommandMemorySize)
	{

		R2_CHECK(!mat::IsInvalidHandle(debugRenderBatch.materialHandle), "This can't be invalid!");

		r2::draw::MaterialSystem* matSystem = r2::draw::matsys::GetMaterialSystem(debugRenderBatch.materialHandle.slot);

		R2_CHECK(matSystem != nullptr, "Failed to get the material system!");

		const Material* material = mat::GetMaterial(*matSystem, debugRenderBatch.materialHandle);

		R2_CHECK(material != nullptr, "Invalid material?");

		ShaderHandle shaderID = material->shaderId;

		key::DebugKey preDrawKey;
		preDrawKey.keyValue = 0;

		cmd::FillVertexBuffer* fillVertexBufferCMD = nullptr;

		if (debugRenderBatch.vertices)
		{
			const VertexLayoutConfigHandle& debugLinesVertexLayoutConfigHandle = r2::sarr::At(*renderer.mVertexLayoutConfigHandles, debugRenderBatch.vertexConfigHandle);
			u64 vOffset = 0;

			fillVertexBufferCMD = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::FillVertexBuffer>(*s_optrRenderer->mDebugCommandArena, *s_optrRenderer->mPreDebugCommandBucket, preDrawKey, 0);

			cmd::FillVertexBufferCommand(fillVertexBufferCMD, *debugRenderBatch.vertices, debugLinesVertexLayoutConfigHandle.mVertexBufferHandles[0], vOffset);
		}

		const r2::SArray<r2::draw::ConstantBufferHandle>* constHandles = r2::draw::renderer::GetConstantBufferHandles();

		const u64 subCommandsMemorySize = subCommandMemorySize * numObjectsToDraw;

		
		cmd::FillConstantBuffer* subCommandsCMD = nullptr;
		if (fillVertexBufferCMD)
		{
			subCommandsCMD = cmdbkt::AppendCommand<cmd::FillVertexBuffer, cmd::FillConstantBuffer, mem::StackArena>(*renderer.mDebugCommandArena, fillVertexBufferCMD, subCommandsMemorySize);
		}
		else
		{
			subCommandsCMD = cmdbkt::AddCommand<r2::draw::key::DebugKey, r2::mem::StackArena, cmd::FillConstantBuffer>(*renderer.mDebugCommandArena, *renderer.mPreDebugCommandBucket, preDrawKey, subCommandsMemorySize);
		}
		 

		char* subCommandsAuxMemory = cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(subCommandsCMD);

		u64 subCommandsMemoryOffset = 0;
		u32 subCommandsOffset = 0;


		auto hashIter = r2::shashmap::Begin(*debugDrawCommandData);

		for (; hashIter != r2::shashmap::End(*debugDrawCommandData); ++hashIter)
		{
			DebugDrawCommandData* debugDrawCommandData = hashIter->value;
			if (debugDrawCommandData != nullptr)
			{

				u32 numSubCommandsInBatch = 0;
				u64 batchSubCommandsMemorySize = 0;
				void* batchCommandData = nullptr;

				if (debugDrawCommandData->debugModelDrawBatchCommands)
				{
					numSubCommandsInBatch = static_cast<u32>(r2::sarr::Size(*debugDrawCommandData->debugModelDrawBatchCommands));
					batchCommandData = (void*)debugDrawCommandData->debugModelDrawBatchCommands->mData;
				}
				else
				{
					numSubCommandsInBatch = static_cast<u32>(r2::sarr::Size(*debugDrawCommandData->debugLineDrawBatchCommands));
					batchCommandData = (void*)debugDrawCommandData->debugLineDrawBatchCommands->mData;
				}


				batchSubCommandsMemorySize = numSubCommandsInBatch * subCommandMemorySize;
				memcpy(mem::utils::PointerAdd(subCommandsAuxMemory, subCommandsMemoryOffset), batchCommandData, batchSubCommandsMemorySize);

				BatchRenderOffsets offsets;
				offsets.layer = DL_DEBUG;
				offsets.primitiveType = debugDrawCommandData->primitiveType;
				offsets.depthEnabled = debugDrawCommandData->depthEnabled;
				offsets.shaderId = shaderID;
				offsets.numSubCommands = numSubCommandsInBatch;
				offsets.subCommandsOffset = subCommandsOffset;

				subCommandsOffset += offsets.numSubCommands;
				subCommandsMemoryOffset += batchSubCommandsMemorySize;

				r2::sarr::Push(*debugRenderBatchesOffsets, offsets);
			}
		}

		auto subCommandsConstantBufferHandle = r2::sarr::At(*constHandles, debugRenderBatch.subCommandsConstantConfigHandle);

		ConstantBufferData* subCommandsConstData = GetConstData(subCommandsConstantBufferHandle);

		subCommandsCMD->constantBufferHandle = subCommandsConstantBufferHandle;
		subCommandsCMD->data = subCommandsAuxMemory;
		subCommandsCMD->dataSize = subCommandsMemorySize;
		subCommandsCMD->offset = subCommandsConstData->currentOffset;
		subCommandsCMD->type = subCommandsConstData->type;
		subCommandsCMD->isPersistent = subCommandsConstData->isPersistent;

		subCommandsConstData->AddDataSize(subCommandsMemorySize);

		key::DebugKey postKey;
		postKey.keyValue = 0;
		cmd::CompleteConstantBuffer* completeSubCommandsCMD = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::CompleteConstantBuffer>(*renderer.mDebugCommandArena, *renderer.mPostDebugCommandBucket, postKey, 0);
		completeSubCommandsCMD->constantBufferHandle = subCommandsConstantBufferHandle;
		completeSubCommandsCMD->count = subCommandsOffset;
	}


	void FillDebugDrawCommands(Renderer& renderer, r2::SArray<BatchRenderOffsets>* debugBatchOffsets, const VertexLayoutConfigHandle& vertexLayoutConfigHandle, ConstantBufferHandle subCommandsBufferHandle, bool debugLines)
	{
		
		const u64 numDebugModelBatchOffsets = r2::sarr::Size(*debugBatchOffsets);

		for (u64 i = 0; i < numDebugModelBatchOffsets; ++i)
		{
			const auto& batchOffset = r2::sarr::At(*debugBatchOffsets, i);

			key::DebugKey key = key::GenerateDebugKey(batchOffset.shaderId, batchOffset.primitiveType, batchOffset.depthEnabled, 0, 0);//key::GenerateBasicKey(0, 0, batchOffset.layer, 0, 0, batchOffset.shaderId);

			
			if (debugLines)
			{
				cmd::DrawDebugBatch* drawBatch = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::DrawDebugBatch>(*renderer.mDebugCommandArena, *renderer.mDebugCommandBucket, key, 0);
				drawBatch->batchHandle = subCommandsBufferHandle;
				drawBatch->bufferLayoutHandle = vertexLayoutConfigHandle.mBufferLayoutHandle;
				drawBatch->numSubCommands = batchOffset.numSubCommands;
				R2_CHECK(drawBatch->numSubCommands > 0, "We should have a count!");
				drawBatch->startCommandIndex = batchOffset.subCommandsOffset;
				drawBatch->primitiveType = batchOffset.primitiveType;
				drawBatch->subCommands = nullptr;
				drawBatch->state.depthEnabled = batchOffset.depthEnabled;

			}
			else
			{
				cmd::DrawBatch* drawBatch = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::DrawBatch>(*renderer.mDebugCommandArena, *renderer.mDebugCommandBucket, key, 0);
				drawBatch->batchHandle = subCommandsBufferHandle;
				drawBatch->bufferLayoutHandle = vertexLayoutConfigHandle.mBufferLayoutHandle;
				drawBatch->numSubCommands = batchOffset.numSubCommands;
				R2_CHECK(drawBatch->numSubCommands > 0, "We should have a count!");
				drawBatch->startCommandIndex = batchOffset.subCommandsOffset;
				drawBatch->primitiveType = batchOffset.primitiveType;
				drawBatch->subCommands = nullptr;
				drawBatch->state.depthEnabled = batchOffset.depthEnabled;
			}
		}
	}

	void DebugPreRender(Renderer& renderer)
	{
		const r2::SArray<r2::draw::ConstantBufferHandle>* constHandles = r2::draw::renderer::GetConstantBufferHandles();

		const DebugRenderBatch& debugModelsRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_MODELS);
		const DebugRenderBatch& debugLinesRenderBatch = r2::sarr::At(*renderer.mDebugRenderBatches, DDT_LINES);

		ConstantBufferHandle debugModelsSubCommandsBufferHandle = r2::sarr::At(*constHandles, debugModelsRenderBatch.subCommandsConstantConfigHandle);
		ConstantBufferHandle debugLinesSubCommandsBufferHandle = r2::sarr::At(*constHandles, debugLinesRenderBatch.subCommandsConstantConfigHandle);

		const VertexLayoutConfigHandle& debugModelsVertexLayoutConfigHandle = r2::sarr::At(*renderer.mVertexLayoutConfigHandles, debugModelsRenderBatch.vertexConfigHandle);
		const VertexLayoutConfigHandle& debugLinesVertexLayoutConfigHandle = r2::sarr::At(*renderer.mVertexLayoutConfigHandles, debugLinesRenderBatch.vertexConfigHandle);

		u64 numModelsToDraw = r2::sarr::Size(*debugModelsRenderBatch.debugModelTypesToDraw);
		u64 numLinesToDraw = r2::sarr::Size(*debugLinesRenderBatch.vertices) / 2;
		
		const u64 totalObjectsToDraw = numModelsToDraw + numLinesToDraw;

		r2::SArray<void*>* tempAllocations = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, void*, 1000);

		r2::SArray<DebugRenderConstants>* debugRenderConstants = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, DebugRenderConstants, totalObjectsToDraw);
		r2::sarr::Push(*tempAllocations, (void*)debugRenderConstants);

		r2::SHashMap<DebugDrawCommandData*>* debugDrawCommandData = MAKE_SHASHMAP(*MEM_ENG_SCRATCH_PTR, DebugDrawCommandData*, (totalObjectsToDraw) * r2::SHashMap<DebugDrawCommandData*>::LoadFactorMultiplier());
		r2::sarr::Push(*tempAllocations, (void*)debugDrawCommandData);


		r2::SArray<BatchRenderOffsets>* modelBatchRenderOffsets = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, BatchRenderOffsets, numModelsToDraw );
		r2::sarr::Push(*tempAllocations, (void*)modelBatchRenderOffsets);

		CreateDebugSubCommands(renderer, debugModelsRenderBatch, numModelsToDraw, 0, tempAllocations, debugRenderConstants, debugDrawCommandData);
		AddDebugSubCommandPreAndPostCMDs(renderer, debugModelsRenderBatch, debugDrawCommandData, modelBatchRenderOffsets, numModelsToDraw, sizeof(cmd::DrawBatchSubCommand));

		r2::shashmap::Clear(*debugDrawCommandData);

		r2::SArray<BatchRenderOffsets>* linesBatchRenderOffsets = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, BatchRenderOffsets, numLinesToDraw);
		r2::sarr::Push(*tempAllocations, (void*)linesBatchRenderOffsets);

		CreateDebugSubCommands(renderer, debugLinesRenderBatch, numLinesToDraw, numModelsToDraw, tempAllocations, debugRenderConstants, debugDrawCommandData);
		AddDebugSubCommandPreAndPostCMDs(renderer, debugLinesRenderBatch, debugDrawCommandData, linesBatchRenderOffsets, numLinesToDraw, sizeof(cmd::DrawDebugBatchSubCommand));

		key::DebugKey emptyDebugKey;
		emptyDebugKey.keyValue = 0;


		//Fill the pre and post buckets with the DebugRenderConstants data needed
		{
			const u64 renderDebugConstantsMemorySize = (totalObjectsToDraw) * sizeof(DebugRenderConstants);

			cmd::FillConstantBuffer* renderDebugConstantsCMD = cmdbkt::AddCommand<key::DebugKey, mem::StackArena, cmd::FillConstantBuffer>(*renderer.mDebugCommandArena, *renderer.mPreDebugCommandBucket, emptyDebugKey, renderDebugConstantsMemorySize);

			char* renderDebugConstantsAuxMemory = cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(renderDebugConstantsCMD);

			memcpy(renderDebugConstantsAuxMemory, debugRenderConstants->mData, renderDebugConstantsMemorySize);

			auto renderDebugConstantsBufferHandle = r2::sarr::At(*constHandles, renderer.mDebugRenderConstantsConfigHandle);

			ConstantBufferData* renderDebugConstantsConstData = GetConstData(renderDebugConstantsBufferHandle);

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
		}

		FillDebugDrawCommands(renderer, modelBatchRenderOffsets, debugModelsVertexLayoutConfigHandle, debugModelsSubCommandsBufferHandle, false);
		FillDebugDrawCommands(renderer, linesBatchRenderOffsets, debugLinesVertexLayoutConfigHandle, debugLinesSubCommandsBufferHandle, true);

		const s64 numAllocations = r2::sarr::Size(*tempAllocations);

		for (s64 i = numAllocations - 1; i >= 0; --i)
		{
			FREE(r2::sarr::At(*tempAllocations, i), *MEM_ENG_SCRATCH_PTR);
		}

		r2::sarr::Clear(*tempAllocations);

		FREE(tempAllocations, *MEM_ENG_SCRATCH_PTR);
	}

	void ClearDebugRenderData()
	{
		if (s_optrRenderer == nullptr )
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		for (u32 i = 0; i < NUM_DEBUG_DRAW_TYPES; ++i)
		{
			DebugRenderBatch& batch = r2::sarr::At(*s_optrRenderer->mDebugRenderBatches, i);

			
			r2::sarr::Clear(*batch.colors);
			r2::sarr::Clear(*batch.drawFlags);

			if (batch.debugDrawType == DDT_LINES)
			{ 
				r2::sarr::Clear(*batch.matTransforms);
				r2::sarr::Clear(*batch.vertices);
			}
			else if (batch.debugDrawType == DDT_MODELS)
			{
				r2::sarr::Clear(*batch.transforms);
				r2::sarr::Clear(*batch.debugModelTypesToDraw);
			}
			else
			{
				R2_CHECK(false, "Unsupported");
			}

		}

		cmdbkt::ClearAll(*s_optrRenderer->mPreDebugCommandBucket);
		cmdbkt::ClearAll(*s_optrRenderer->mDebugCommandBucket);
		cmdbkt::ClearAll(*s_optrRenderer->mPostDebugCommandBucket);

		RESET_ARENA(*s_optrRenderer->mDebugCommandArena);
	}

	void DrawDebugBones(const r2::SArray<DebugBone>& bones, const glm::mat4& modelMatrix, const glm::vec4& color)
	{
		//@TODO(Serge): this is kind of dumb at the moment - we should find a better way to batch things
		r2::SArray<u64>* numBonesPerModel = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, 1);
		r2::SArray<glm::mat4>* modelMats = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, 1);

		r2::sarr::Push(*numBonesPerModel, r2::sarr::Size(bones));
		r2::sarr::Push(*modelMats, modelMatrix);

		DrawDebugBones(bones, *numBonesPerModel, *modelMats, color);

		FREE(modelMats, *MEM_ENG_SCRATCH_PTR);
		FREE(numBonesPerModel, *MEM_ENG_SCRATCH_PTR);
	}
	
	void DrawDebugBones(
		const r2::SArray<DebugBone>& bones,
		const r2::SArray<u64>& numBonesPerModel,
		const r2::SArray<glm::mat4>& modelMats,
		const glm::vec4& color)
	{
		if (s_optrRenderer == nullptr || s_optrRenderer->mVertexLayoutConfigHandles == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		if (!s_optrRenderer->mConstantBufferData)
		{
			R2_CHECK(false, "We haven't generated any constant buffers!");
			return;
		}

		if (s_optrRenderer->mDebugLinesVertexConfigHandle == InvalidVertexConfigHandle)
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
				DrawLine(bone.p0, bone.p1, modelMat, color, false);
			}

			boneOffset += numBonesForModel;
		}
	}

	void DrawSphere(const glm::vec3& center, float radius, const glm::vec4& color, bool filled, bool depthTest)
	{
		if (s_optrRenderer == nullptr || s_optrRenderer->mVertexLayoutConfigHandles == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}
		
		DebugRenderBatch& debugModelRenderBatch = r2::sarr::At(*s_optrRenderer->mDebugRenderBatches, DDT_MODELS);

		R2_CHECK(debugModelRenderBatch.debugModelTypesToDraw != nullptr, "We haven't properly initialized the debug render batches!");

		math::Transform t;
		t.position = center;
		t.scale = glm::vec3(radius);

		r2::sarr::Push(*debugModelRenderBatch.transforms, t);
		r2::sarr::Push(*debugModelRenderBatch.colors, color);
		
		DrawFlags flags;
		filled ? flags.Set(eDrawFlags::FILL_MODEL) : flags.Remove(eDrawFlags::FILL_MODEL);
		depthTest ? flags.Set(eDrawFlags::DEPTH_TEST) : flags.Remove(eDrawFlags::DEPTH_TEST);

		r2::sarr::Push(*debugModelRenderBatch.drawFlags, flags);

		r2::sarr::Push(*debugModelRenderBatch.debugModelTypesToDraw, DEBUG_SPHERE);
	}

	void DrawCube(const glm::vec3& center, float scale, const glm::vec4& color, bool filled, bool depthTest)
	{
		if (s_optrRenderer == nullptr || s_optrRenderer->mVertexLayoutConfigHandles == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}


		DebugRenderBatch& debugModelRenderBatch = r2::sarr::At(*s_optrRenderer->mDebugRenderBatches, DDT_MODELS);

		R2_CHECK(debugModelRenderBatch.debugModelTypesToDraw != nullptr, "We haven't properly initialized the debug render batches!");

		math::Transform t;
		t.position = center;
		t.scale = glm::vec3(scale);

		r2::sarr::Push(*debugModelRenderBatch.transforms, t);
		r2::sarr::Push(*debugModelRenderBatch.colors, color);

		DrawFlags flags;
		filled ? flags.Set(eDrawFlags::FILL_MODEL) : flags.Remove(eDrawFlags::FILL_MODEL);
		depthTest ? flags.Set(eDrawFlags::DEPTH_TEST) : flags.Remove(eDrawFlags::DEPTH_TEST);

		r2::sarr::Push(*debugModelRenderBatch.drawFlags, flags);

		r2::sarr::Push(*debugModelRenderBatch.debugModelTypesToDraw, DEBUG_CUBE);
	}

	void DrawCylinder(const glm::vec3& basePosition, const glm::vec3& dir, float radius, float height, const glm::vec4& color, bool filled, bool depthTest)
	{
		if (s_optrRenderer == nullptr || s_optrRenderer->mVertexLayoutConfigHandles == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		DebugRenderBatch& debugModelRenderBatch = r2::sarr::At(*s_optrRenderer->mDebugRenderBatches, DDT_MODELS);

		R2_CHECK(debugModelRenderBatch.debugModelTypesToDraw != nullptr, "We haven't properly initialized the debug render batches!");
	
		DrawFlags flags;
		filled ? flags.Set(eDrawFlags::FILL_MODEL) : flags.Remove(eDrawFlags::FILL_MODEL);
		depthTest ? flags.Set(eDrawFlags::DEPTH_TEST) : flags.Remove(eDrawFlags::DEPTH_TEST);

		math::Transform t;

		glm::vec3 initialFacing = glm::vec3(0, 0, 1);

		t.position = initialFacing * 0.5f * height;

		glm::vec3 ndir = glm::normalize(dir);

		float angle = glm::acos(glm::dot(ndir, initialFacing));

		glm::vec3 axis = glm::normalize(glm::cross(ndir, initialFacing));

		math::Transform r;
		r.rotation = glm::normalize(glm::rotate(r.rotation, angle, axis));

		math::Transform s;

		s.scale = glm::vec3(radius, radius, height);

		math::Transform t2;
		t2.position = -t.position;

		math::Transform transformToDraw = math::Combine(math::Combine(r, t), s);
		transformToDraw.position += basePosition;

		r2::sarr::Push(*debugModelRenderBatch.transforms, transformToDraw);
		r2::sarr::Push(*debugModelRenderBatch.colors, color);
		r2::sarr::Push(*debugModelRenderBatch.drawFlags, flags);
		r2::sarr::Push(*debugModelRenderBatch.debugModelTypesToDraw, DEBUG_CYLINDER);
	}

	void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, bool depthTest)
	{
		DrawLine(p0, p1, glm::mat4(1.0f), color, depthTest);
	}

	void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::mat4& modelMat, const glm::vec4& color, bool depthTest)
	{
		if (s_optrRenderer == nullptr || s_optrRenderer->mVertexLayoutConfigHandles == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		DebugRenderBatch& debugLinesRenderBatch = r2::sarr::At(*s_optrRenderer->mDebugRenderBatches, DDT_LINES);

		R2_CHECK(debugLinesRenderBatch.vertices != nullptr, "We haven't properly initialized the debug render batches!");

		r2::draw::DebugVertex v1{ p0 };
		r2::draw::DebugVertex v2{ p1 };

		DrawFlags flags;
		depthTest ? flags.Set(eDrawFlags::DEPTH_TEST) : flags.Remove(eDrawFlags::DEPTH_TEST);

		r2::sarr::Push(*debugLinesRenderBatch.colors, color);
		r2::sarr::Push(*debugLinesRenderBatch.drawFlags, flags);
		r2::sarr::Push(*debugLinesRenderBatch.vertices, v1);
		r2::sarr::Push(*debugLinesRenderBatch.vertices, v2);
		r2::sarr::Push(*debugLinesRenderBatch.matTransforms, modelMat);
	}

	void DrawTangentVectors(DefaultModel defaultModel, const glm::mat4& transform)
	{
		const Model* model = GetDefaultModel(defaultModel);

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

				DrawLine(initialPosition, initialPosition + normal * 0.1f, glm::vec4(0, 0, 1, 1), true);
				DrawLine(initialPosition, initialPosition + tangent * 0.1f, glm::vec4(1, 0, 0, 1), true);
				DrawLine(initialPosition, initialPosition + bitangent * 0.1f, glm::vec4(0, 1, 0, 1), true);
			}

		}
	}

#endif //  R2_DEBUG

	void ResizeRenderSurface(Renderer& renderer, u32 windowWidth, u32 windowHeight, u32 resolutionX, u32 resolutionY, float scaleX, float scaleY, float xOffset, float yOffset)
	{
		//no need to resize if that's the size we already are
		renderer.mRenderTargets[RTS_COMPOSITE].xOffset	= round(xOffset);
		renderer.mRenderTargets[RTS_COMPOSITE].yOffset	= round(yOffset);
		renderer.mRenderTargets[RTS_COMPOSITE].width	= round(scaleX * resolutionX);
		renderer.mRenderTargets[RTS_COMPOSITE].height	= round(scaleY * resolutionY);

		if (!util::IsSizeEqual(renderer.mResolutionSize, resolutionX, resolutionY))
		{
			DestroyRenderSurfaces(renderer);

			renderer.mRenderTargets[RTS_GBUFFER] = rt::CreateRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, 1, 1, 0, 0, resolutionX, resolutionY, __FILE__, __LINE__, "");

			rt::AddTextureAttachment(renderer.mRenderTargets[RTS_GBUFFER], tex::FILTER_NEAREST, tex::WRAP_MODE_REPEAT, 1, false, true);
			rt::AddDepthAndStencilAttachment(renderer.mRenderTargets[RTS_GBUFFER]);
		}
		
		renderer.mResolutionSize.width = resolutionX;
		renderer.mResolutionSize.height = resolutionY;
		renderer.mCompositeSize.width = windowWidth;
		renderer.mCompositeSize.height = windowHeight;

	}

	void DestroyRenderSurfaces(Renderer& renderer)
	{
		rt::DestroyRenderTarget<r2::mem::StackArena>(*renderer.mRenderTargetsArena, renderer.mRenderTargets[RTS_GBUFFER]);
	}

	//events
	void WindowResized(u32 windowWidth, u32 windowHeight, u32 resolutionX, u32 resolutionY, float scaleX, float scaleY, float xOffset, float yOffset)
	{
		R2_CHECK(s_optrRenderer != nullptr, "We should have created the renderer already!");
		ResizeRenderSurface(*s_optrRenderer, windowWidth, windowHeight, resolutionX, resolutionY, scaleX, scaleY, xOffset, yOffset);
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

	void SetWindowSize(u32 windowWidth, u32 windowHeight, u32 resolutionX, u32 resolutionY, float scaleX, float scaleY, float xOffset, float yOffset)
	{
		R2_CHECK(s_optrRenderer != nullptr, "We should have created the renderer already!");
		ResizeRenderSurface(*s_optrRenderer, windowWidth, windowHeight, resolutionX, resolutionY, scaleX, scaleY, xOffset, yOffset);
	}
}
