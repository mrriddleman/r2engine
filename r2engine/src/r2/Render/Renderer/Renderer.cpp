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

#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Utils/Hash.h"
#include <filesystem>
//#include "r2/Render/Renderer/CommandPacket.h"

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
		cmd->dataSize = sizeof(r2::sarr::At(*mesh.optrVertices, 0)) * numVertices;
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

	u64 FillConstantBufferCommand(FillConstantBuffer* cmd, ConstantBufferHandle handle, r2::draw::ConstantBufferLayout::Type type, b32 isPersistent, void* data, u64 size, u64 offset)
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
	struct ConstantBufferData
	{
		r2::draw::ConstantBufferHandle handle = EMPTY_BUFFER;
		r2::draw::ConstantBufferLayout::Type type = r2::draw::ConstantBufferLayout::Type::Small;
		b32 isPersistent = false;
	};

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

	struct Renderer
	{
		//memory
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;

		//@TODO(Serge): don't expose this to the outside (or figure out how to remove this)
		//				we should only be exposing/using mVertexLayoutConfigHandles
		r2::draw::BufferHandles mBufferHandles; 
		r2::SArray<r2::draw::ConstantBufferHandle>* mContantBufferHandles = nullptr;
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
		r2::draw::MaterialHandle mDebugMaterialHandle;
		r2::draw::MaterialHandle mFinalCompositeMaterialHandle;

		BatchConfig finalBatch;
		ConstantConfigHandle mModelConfigHandle;
		ConstantConfigHandle mMaterialConfigHandle;
		ConstantConfigHandle mSubcommandsConfigHandle;

		//Each bucket needs the bucket and an arena for that bucket
		r2::draw::CommandBucket<r2::draw::key::Basic>* mCommandBucket = nullptr;
		r2::draw::CommandBucket<r2::draw::key::Basic>* mFinalBucket = nullptr;
		r2::mem::StackArena* mCommandArena = nullptr;

		VertexConfigHandle mDebugVertexConfigHandle = InvalidVertexConfigHandle;
	};
}

namespace r2::draw
{
	u64 BatchConfig::MemorySize(u64 numModels, u64 numSubcommands, u64 numMaterials, u64 numBoneTransforms, u64 numBoneOffsets, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		u64 totalBytes =
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<cmd::DrawBatchSubCommand>::MemorySize(numSubcommands), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<MaterialHandle>::MemorySize(numMaterials), alignment, headerSize, boundsChecking);

		if (numModels > 0)
		{
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<glm::mat4>::MemorySize(numModels), alignment, headerSize, boundsChecking);
		}

		if (numBoneTransforms > 0)
		{
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ShaderBoneTransform>::MemorySize(numBoneTransforms), alignment, headerSize, boundsChecking);
		}
	
		if (numBoneOffsets > 0)
		{
			totalBytes += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<glm::ivec4>::MemorySize(numBoneOffsets), alignment, headerSize, boundsChecking);
		}

		return totalBytes;
	}
}

namespace
{
	r2::draw::Renderer* s_optrRenderer = nullptr;

	const u64 COMMAND_CAPACITY = 2048;
	const u64 COMMAND_AUX_MEMORY = Megabytes(4); //I dunno lol
	const u64 ALIGNMENT = 16;
	const u32 MAX_BUFFER_LAYOUTS = 32;
	const u64 MAX_NUM_MATERIALS = 2048;
	const u64 MAX_NUM_SHADERS = 1000;
	const u64 MAX_DEFAULT_MODELS = 16;
	const u64 MAX_NUM_TEXTURES = 2048;
	const u64 MAX_NUM_MATERIAL_SYSTEMS = 16;
	const u64 MAX_NUM_MATERIALS_PER_MATERIAL_SYSTEM = 32;
	const u64 MAX_NUM_MATERIAL_TEXTURES_PER_OBJECT = 8;

	const u64 MAX_NUM_CONSTANT_BUFFERS = 16; //?
	const u64 MAX_NUM_CONSTANT_BUFFER_LOCKS = 2048; //?

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
	const ConstantBufferData GetConstData(ConstantBufferHandle handle);
	u64 MaterialSystemMemorySize(u64 numMaterials, u64 textureCacheInBytes, u64 totalNumberOfTextures, u64 numPacks, u64 maxTexturesInAPack);
	bool GenerateBufferLayouts(const r2::SArray<BufferLayoutConfiguration>* layouts);
	bool GenerateConstantBuffers(const r2::SArray<ConstantBufferLayoutConfiguration>* constantBufferConfigs);
	r2::draw::cmd::Clear* AddClearCommand(CommandBucket<key::Basic>& bucket, r2::draw::key::Basic key);
	r2::draw::cmd::FillConstantBuffer* AddFillConstantBufferCommand(CommandBucket<key::Basic>& bucket, r2::draw::key::Basic key, u64 auxMemory);
	ModelRef UploadModelInternal(const Model* model, r2::SArray<BoneData>* boneData, VertexConfigHandle vHandle);
	void AddDrawBatchInternal(CommandBucket<key::Basic>& bucket, const BatchConfig& batch, tex::TextureAddress* addr);
	void AddFinalBatchInternal();
	void SetupFinalBatchInternal();

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
			R2_CHECK(false, "We don't have enought space to allocate the renderer!");
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




		s_optrRenderer->finalBatch.subcommands = MAKE_SARRAY(*rendererArena, cmd::DrawBatchSubCommand, 1);
		R2_CHECK(s_optrRenderer->finalBatch.subcommands != nullptr, "We couldn't create the subcommands for the final batch!");

		s_optrRenderer->finalBatch.materials = MAKE_SARRAY(*rendererArena, MaterialHandle, 1);
		R2_CHECK(s_optrRenderer->finalBatch.materials != nullptr, "We couldn't create the materials for the final batch!");

		s_optrRenderer->finalBatch.models = MAKE_SARRAY(*rendererArena, glm::mat4, 1);
		R2_CHECK(s_optrRenderer->finalBatch.models != nullptr, "We couldn't create the models for the final batch!");


		

		s_optrRenderer->mBufferHandles.bufferLayoutHandles = MAKE_SARRAY(*rendererArena, r2::draw::BufferLayoutHandle, MAX_BUFFER_LAYOUTS);
		
		R2_CHECK(s_optrRenderer->mBufferHandles.bufferLayoutHandles != nullptr, "We couldn't create the buffer layout handles!");
		
		s_optrRenderer->mBufferHandles.vertexBufferHandles = MAKE_SARRAY(*rendererArena, r2::draw::VertexBufferHandle, MAX_BUFFER_LAYOUTS * BufferLayoutConfiguration::MAX_VERTEX_BUFFER_CONFIGS);
		
		R2_CHECK(s_optrRenderer->mBufferHandles.vertexBufferHandles != nullptr, "We couldn't create the vertex buffer layout handles!");
		
		s_optrRenderer->mBufferHandles.indexBufferHandles = MAKE_SARRAY(*rendererArena, r2::draw::IndexBufferHandle, MAX_BUFFER_LAYOUTS);

		R2_CHECK(s_optrRenderer->mBufferHandles.indexBufferHandles != nullptr, "We couldn't create the index buffer layout handles!");

		s_optrRenderer->mBufferHandles.drawIDHandles = MAKE_SARRAY(*rendererArena, r2::draw::DrawIDHandle, MAX_BUFFER_LAYOUTS);

		R2_CHECK(s_optrRenderer->mBufferHandles.drawIDHandles != nullptr, "We couldn't create the draw id handles");
		
		s_optrRenderer->mContantBufferHandles = MAKE_SARRAY(*rendererArena, r2::draw::ConstantBufferHandle, MAX_BUFFER_LAYOUTS);
		
		R2_CHECK(s_optrRenderer->mContantBufferHandles != nullptr, "We couldn't create the constant buffer handles");

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



		

		bool rendererImpl = r2::draw::rendererimpl::RendererImplInit(memoryAreaHandle, MAX_NUM_CONSTANT_BUFFERS, MAX_NUM_CONSTANT_BUFFER_LOCKS, "RendererImpl");
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


		s_optrRenderer->mDebugMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*s_optrRenderer->mMaterialSystem, STRING_ID("Debug"));
		s_optrRenderer->mFinalCompositeMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*s_optrRenderer->mMaterialSystem, STRING_ID("FinalComposite"));


		//TODO(Serge): resize when window changes
		auto size = CENG.DisplaySize();

		
		r2::draw::RenderTarget offscreenRenderTarget = rt::CreateRenderTarget<r2::mem::LinearArena>(*rendererArena, 1, 1, size.width, size.height, __FILE__, __LINE__, "");

		rt::AddTextureAttachment(offscreenRenderTarget, tex::FILTER_NEAREST, tex::WRAP_MODE_REPEAT, 1, false);
		rt::AddDepthAndStencilAttachment(offscreenRenderTarget);

		s_optrRenderer->mCommandBucket = MAKE_CMD_BUCKET(*rendererArena, r2::draw::key::Basic, r2::draw::key::DecodeBasicKey, COMMAND_CAPACITY, offscreenRenderTarget);
		R2_CHECK(s_optrRenderer->mCommandBucket != nullptr, "We couldn't create the command bucket!");


		r2::draw::RenderTarget screenRenderTarget;

		s_optrRenderer->mFinalBucket = MAKE_CMD_BUCKET(*rendererArena, r2::draw::key::Basic, r2::draw::key::DecodeBasicKey, COMMAND_CAPACITY, screenRenderTarget);
		R2_CHECK(s_optrRenderer->mFinalBucket != nullptr, "We couldn't create the final command bucket!");


		s_optrRenderer->mCommandArena = MAKE_STACK_ARENA(*rendererArena, COMMAND_CAPACITY * cmd::LargestCommand() + COMMAND_AUX_MEMORY);

		R2_CHECK(s_optrRenderer->mCommandArena != nullptr, "We couldn't create the stack arena for commands");


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

		if (r2::sarr::Size(*s_optrRenderer->finalBatch.subcommands) == 0)
		{
			//@NOTE: this is here because we need to wait till the user of the code makes all of the handles
			SetupFinalBatchInternal();
		}

		AddFinalBatchInternal();
		//const u64 numEntries = r2::sarr::Size(*s_optrRenderer->mCommandBucket->entries);

		//for (u64 i = 0; i < numEntries; ++i)
		//{
		//	const r2::draw::CommandBucket<r2::draw::key::Basic>::Entry& entry = r2::sarr::At(*s_optrRenderer->mCommandBucket->entries, i);
		//	printf("entry - key: %llu, data: %p, func: %p\n", entry.aKey.keyValue, entry.data, entry.func);
		//}

		//printf("================================================\n");
		cmdbkt::Sort(*s_optrRenderer->mCommandBucket, r2::draw::key::CompareKey);
		cmdbkt::Sort(*s_optrRenderer->mFinalBucket, r2::draw::key::CompareKey);

		//for (u64 i = 0; i < numEntries; ++i)
		//{
		//	const r2::draw::CommandBucket<r2::draw::key::Basic>::Entry* entry = r2::sarr::At(*s_optrRenderer->mCommandBucket->sortedEntries, i);
		//	printf("sorted - key: %llu, data: %p, func: %p\n", entry->aKey.keyValue, entry->data, entry->func);
		//}

		

		cmdbkt::Submit(*s_optrRenderer->mCommandBucket);
		cmdbkt::Submit(*s_optrRenderer->mFinalBucket);

		cmdbkt::ClearAll(*s_optrRenderer->mCommandBucket);
		cmdbkt::ClearAll(*s_optrRenderer->mFinalBucket);
		//This is kinda bad but... 
		RESET_ARENA(*s_optrRenderer->mCommandArena);
	}

	void Shutdown()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}



		r2::mem::LinearArena* arena = s_optrRenderer->mSubAreaArena;


		FREE(s_optrRenderer->mCommandArena, *arena);
		FREE_CMD_BUCKET(*arena, r2::draw::key::Basic, s_optrRenderer->mCommandBucket);
		FREE_CMD_BUCKET(*arena, r2::draw::key::Basic, s_optrRenderer->mFinalBucket);
		FREE(s_optrRenderer->finalBatch.models, *arena);
		FREE(s_optrRenderer->finalBatch.materials, *arena);
		FREE(s_optrRenderer->finalBatch.subcommands, *arena);


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
			r2::sarr::Size(*s_optrRenderer->mContantBufferHandles),
			s_optrRenderer->mContantBufferHandles->mData);
		
		FREE(s_optrRenderer->mBufferHandles.bufferLayoutHandles, *arena);
		FREE(s_optrRenderer->mBufferHandles.vertexBufferHandles, *arena);
		FREE(s_optrRenderer->mBufferHandles.indexBufferHandles, *arena);
		FREE(s_optrRenderer->mBufferHandles.drawIDHandles, *arena);
		FREE(s_optrRenderer->mContantBufferHandles, *arena);
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

	void SetDepthTest(bool shouldDepthTest)
	{
		r2::draw::rendererimpl::SetDepthTest(shouldDepthTest);
	}

	bool GenerateLayouts()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return false;
		}

		return GenerateBufferLayouts(s_optrRenderer->mVertexLayouts) &&
		GenerateConstantBuffers(s_optrRenderer->mConstantLayouts);
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

		if (r2::sarr::Size(*s_optrRenderer->mContantBufferHandles) > 0)
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

		const u64 numContantBuffers = r2::sarr::Size(*constantBufferConfigs);

		r2::draw::rendererimpl::GenerateContantBuffers(static_cast<u32>(numContantBuffers), s_optrRenderer->mContantBufferHandles->mData);
		s_optrRenderer->mContantBufferHandles->mSize = numContantBuffers;

		for (u64 i = 0; i < numContantBuffers; ++i)
		{
			ConstantBufferData constData;
			const ConstantBufferLayoutConfiguration& config = r2::sarr::At(*constantBufferConfigs, i);
			auto handle = r2::sarr::At(*s_optrRenderer->mContantBufferHandles, i);

			constData.handle = handle;
			constData.type = config.layout.GetType();
			constData.isPersistent = config.layout.GetFlags().IsSet(CB_FLAG_MAP_PERSISTENT);

			r2::shashmap::Set(*s_optrRenderer->mConstantBufferData, handle, constData);
		}

		r2::draw::rendererimpl::SetupConstantBufferConfigs(constantBufferConfigs, s_optrRenderer->mContantBufferHandles->mData);

		return true;
	}

	VertexConfigHandle AddStaticModelLayout(const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize, u64 numDraws, bool generateDrawIDs)
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
				{{r2::draw::ShaderDataType::Float3, "aPos"},
				{r2::draw::ShaderDataType::Float3, "aNormal"},
				{r2::draw::ShaderDataType::Float3, "aTexCoord"}}
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

		layoutConfig.useDrawIDs = generateDrawIDs;
		layoutConfig.maxDrawCount = numDraws;
		layoutConfig.numVertexConfigs = numVertexLayouts;

		r2::sarr::Push(*s_optrRenderer->mVertexLayouts, layoutConfig);

		s_optrRenderer->finalBatch.vertexLayoutConfigHandle = r2::sarr::Size(*s_optrRenderer->mVertexLayouts) - 1;

		return s_optrRenderer->finalBatch.vertexLayoutConfigHandle;
	}

	VertexConfigHandle AddAnimatedModelLayout(const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize, u64 numDraws, bool generateDrawIDs)
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

		layoutConfig.useDrawIDs = generateDrawIDs;
		layoutConfig.maxDrawCount = numDraws;
		layoutConfig.numVertexConfigs = numVertexLayouts;

		r2::sarr::Push(*s_optrRenderer->mVertexLayouts, layoutConfig);

		return r2::sarr::Size(*s_optrRenderer->mVertexLayouts) - 1;
	}

	VertexConfigHandle AddDebugDrawLayout(u64 maxDraws)
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

		if (s_optrRenderer->mDebugVertexConfigHandle != InvalidVertexConfigHandle)
		{
			R2_CHECK(false, "We have already added the debug vertex layout!");
			return s_optrRenderer->mDebugVertexConfigHandle;
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
		layoutConfig.maxDrawCount = maxDraws;
		layoutConfig.numVertexConfigs = 1;

		r2::sarr::Push(*s_optrRenderer->mVertexLayouts, layoutConfig);

		s_optrRenderer->mDebugVertexConfigHandle = r2::sarr::Size(*s_optrRenderer->mVertexLayouts) - 1;

		return s_optrRenderer->mDebugVertexConfigHandle;
	}

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

	ConstantConfigHandle AddMaterialLayout(u64 maxDraws)
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

		materials.layout.InitForMaterials(0, 0, maxDraws);

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, materials);

		s_optrRenderer->mMaterialConfigHandle = r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;

		return s_optrRenderer->mMaterialConfigHandle;
	}

	ConstantConfigHandle AddModelsLayout(ConstantBufferLayout::Type type, u64 maxDraws)
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
				r2::draw::ShaderDataType::Mat4, "models", maxDraws}
			});

		return s_optrRenderer->mModelConfigHandle;
	}

	ConstantConfigHandle AddSubCommandsLayout(u64 maxDraws)
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

		subCommands.layout.InitForSubCommands(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, maxDraws);

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, subCommands);

		s_optrRenderer->mSubcommandsConfigHandle = r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;

		return s_optrRenderer->mSubcommandsConfigHandle;
	}

	ConstantConfigHandle AddBoneTransformsLayout(u64 maxDraws)
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

		boneTransforms.layout.InitForBoneTransforms(0, 0, maxDraws);

		r2::sarr::Push(*s_optrRenderer->mConstantLayouts, boneTransforms);

		return r2::sarr::Size(*s_optrRenderer->mConstantLayouts) - 1;
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
			r2::draw::RenderTarget::MemorySize(1, 1, ALIGNMENT, headerSize, boundsChecking ) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::BufferLayoutHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::VertexBufferHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::IndexBufferHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::DrawIDHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ConstantBufferHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::BufferLayoutConfiguration>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ConstantBufferLayoutConfiguration>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<ConstantBufferData>::MemorySize(MAX_BUFFER_LAYOUTS * r2::SHashMap<ConstantBufferData>::LoadFactorMultiplier()), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena) + r2::mem::utils::GetMaxMemoryForAllocation(cmd::LargestCommand(), ALIGNMENT, stackHeaderSize, boundsChecking) * COMMAND_CAPACITY + COMMAND_AUX_MEMORY, ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ModelHandle>::MemorySize(MAX_DEFAULT_MODELS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<VertexLayoutConfigHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<VertexLayoutUploadOffset>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ModelRef>::MemorySize(NUM_DEFAULT_MODELS), ALIGNMENT, headerSize, boundsChecking) +
			BatchConfig::MemorySize(1, 1, 1, 0, 0, ALIGNMENT, headerSize, boundsChecking) + //for the finalBatch
			materialSystemMemorySize;


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

		return s_optrRenderer->mContantBufferHandles;
	}

	template<class CMD>
	CMD* AddCommand(r2::draw::CommandBucket<r2::draw::key::Basic>& bucket, r2::draw::key::Basic key, u64 auxMemory)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		R2_CHECK(s_optrRenderer->mCommandBucket != nullptr, "Command Bucket is nullptr!");

		return r2::draw::cmdbkt::AddCommand<r2::draw::key::Basic, r2::mem::StackArena, CMD>(*s_optrRenderer->mCommandArena, bucket, key, auxMemory);
	}

	template<class CMDTOAPPENDTO, class CMD>
	CMD* AppendCommand(CMDTOAPPENDTO* cmdToAppendTo, u64 auxMemory)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		R2_CHECK(s_optrRenderer->mCommandBucket != nullptr, "Command Bucket is nullptr!");

		return r2::draw::cmdbkt::AppendCommand<CMDTOAPPENDTO, CMD, r2::mem::StackArena>(*s_optrRenderer->mCommandArena, cmdToAppendTo, auxMemory);
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

	void GetMaterialsAndBoneOffsetsForAnimModels(const r2::SArray<const r2::draw::AnimModel*>& models, r2::SArray<r2::draw::MaterialHandle>& materialHandles, r2::SArray<glm::ivec4>& boneOffsets)
	{
		auto numModels = r2::sarr::Size(models);
		u32 boneOffset = 0;
		for (u64 i = 0; i < numModels; ++i)
		{
			const r2::draw::AnimModel* model = r2::sarr::At(models, i);

			bool found = false;

			const u64 numMaterials = r2::sarr::Size(*model->model.optrMaterialHandles);

			for (u64 k = 0; k < numMaterials && !found; ++k)
			{
				r2::draw::MaterialHandle materialHandle = r2::sarr::At(*model->model.optrMaterialHandles, k);

				if (!r2::draw::mat::IsInvalidHandle(materialHandle))
				{
					r2::sarr::Push(materialHandles, materialHandle);
					found = true;
				}
			}

			R2_CHECK(found, "We couldn't find a valid material handle!");

			r2::sarr::Push(boneOffsets, glm::ivec4(boneOffset, 0, 0, 0));

			boneOffset += model->boneInfo->mSize;
		}
	}

	void UploadEngineModels(VertexConfigHandle vertexLayoutConfig)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		const VertexLayoutConfigHandle& handles = r2::sarr::At(*s_optrRenderer->mVertexLayoutConfigHandles, vertexLayoutConfig);

		
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

		UploadModels(*modelsToUpload, vertexLayoutConfig, *s_optrRenderer->mEngineModelRefs);

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

	ModelRef UploadModel(const Model* model, VertexConfigHandle vertexConfigHandle)
	{
		return UploadModelInternal(model, nullptr, vertexConfigHandle);
	}

	void UploadModels(const r2::SArray<const Model*>& models, VertexConfigHandle vertexConfigHandle, r2::SArray<ModelRef>& modelRefs)
	{
		if (r2::sarr::Size(models) + r2::sarr::Size(modelRefs) > r2::sarr::Capacity(modelRefs))
		{
			R2_CHECK(false, "We don't have enough space to put the model refs");
			return;
		}

		const u64 numModels = r2::sarr::Size(models);

		for (u64 i = 0; i < numModels; ++i)
		{
			r2::sarr::Push(modelRefs, UploadModel(r2::sarr::At(models, i), vertexConfigHandle));
		}
	}

	ModelRef UploadAnimModel(const AnimModel* model, VertexConfigHandle vertexConfigHandle)
	{
		return UploadModelInternal(&model->model, model->boneData, vertexConfigHandle);
	}

	void UploadAnimModels(const r2::SArray<const AnimModel*>& models, VertexConfigHandle vHandle, r2::SArray<ModelRef>& modelRefs)
	{
		if (r2::sarr::Size(models) + r2::sarr::Size(modelRefs) > r2::sarr::Capacity(modelRefs))
		{
			R2_CHECK(false, "We don't have enough space to put the model refs");
			return;
		}

		const u64 numModels = r2::sarr::Size(models);

		for (u64 i = 0; i < numModels; ++i)
		{
			r2::sarr::Push(modelRefs, UploadAnimModel(r2::sarr::At(models, i), vHandle));
		}
	}


	ModelRef UploadModelInternal(const Model* model, r2::SArray<BoneData>* boneData, VertexConfigHandle vertexConfigHandle)
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

		const VertexLayoutConfigHandle& vHandle = r2::sarr::At(*s_optrRenderer->mVertexLayoutConfigHandles, vertexConfigHandle);
		VertexLayoutUploadOffset& vOffsets = r2::sarr::At(*s_optrRenderer->mVertexLayoutUploadOffsets, vertexConfigHandle);
		const BufferLayoutConfiguration& layoutConfig = r2::sarr::At(*s_optrRenderer->mVertexLayouts, vertexConfigHandle);

		modelRef.hash = model->hash;
		modelRef.indexBufferHandle = vHandle.mIndexBufferHandle;
		modelRef.vertexBufferHandle = vHandle.mVertexBufferHandles[0];
		modelRef.mMeshRefs[0].baseIndex = vOffsets.mIndexBufferOffset.baseIndex + vOffsets.mIndexBufferOffset.numIndices;
		modelRef.mMeshRefs[0].baseVertex = vOffsets.mVertexBufferOffset.baseVertex + vOffsets.mVertexBufferOffset.numVertices;
		modelRef.mNumMeshRefs = numMeshes;

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

		r2::draw::cmd::FillVertexBuffer* fillVertexCommand = r2::draw::renderer::AddCommand<r2::draw::cmd::FillVertexBuffer>(*s_optrRenderer->mCommandBucket, fillKey, 0);
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

			nextVertexCmd = AppendCommand<r2::draw::cmd::FillVertexBuffer, cmd::FillVertexBuffer>(nextVertexCmd, 0);
			vOffset = r2::draw::cmd::FillVertexBufferCommand(nextVertexCmd, *r2::sarr::At(*model->optrMeshes, i), vHandle.mVertexBufferHandles[0], vOffset);
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

			r2::draw::cmd::FillVertexBuffer* fillBoneDataCommand = AppendCommand<r2::draw::cmd::FillVertexBuffer, cmd::FillVertexBuffer>(nextVertexCmd, 0);
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

		cmd::FillIndexBuffer* fillIndexCommand = AppendCommand<cmd::FillVertexBuffer, cmd::FillIndexBuffer>(nextVertexCmd, 0);
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

			nextIndexCmd = AppendCommand<cmd::FillIndexBuffer, cmd::FillIndexBuffer>(nextIndexCmd, 0);
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


	u64 AddFillConstantBufferCommandForData(ConstantBufferHandle handle, u64 elementIndex, void* data)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return 0;
		}

		r2::draw::key::Basic fillKey;
		//@TODO(Serge): fix this or pass it in
		fillKey.keyValue = 0;

		const ConstantBufferData constBufferData = GetConstData(handle);

		u64 numConstantBufferHandles = r2::sarr::Size(*s_optrRenderer->mContantBufferHandles);
		u64 constBufferIndex = 0;
		bool found = false;
		for (; constBufferIndex < numConstantBufferHandles; ++constBufferIndex)
		{
			if (handle == r2::sarr::At(*s_optrRenderer->mContantBufferHandles, constBufferIndex))
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

		r2::draw::cmd::FillConstantBuffer* fillConstantBufferCommand = r2::draw::renderer::AddFillConstantBufferCommand(*s_optrRenderer->mCommandBucket, fillKey, config.layout.GetSize());
		return r2::draw::cmd::FillConstantBufferCommand(fillConstantBufferCommand, handle, constBufferData.type, constBufferData.isPersistent, data, config.layout.GetElements().at(elementIndex).size, config.layout.GetElements().at(elementIndex).offset);
	}

	void FillSubCommandsFromModelRefs(r2::SArray<r2::draw::cmd::DrawBatchSubCommand>& subCommands, const r2::SArray<ModelRef>& modelRefs)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		const u64 numModels = r2::sarr::Size(modelRefs);

		u64 numVertices = 0;
		u64 numIndices = 0;

		for (u64 i = 0; i < numModels; ++i)
		{
			const ModelRef& modelRef = r2::sarr::At(modelRefs, i);

			for (u64 j = 0; j < modelRef.mNumMeshRefs; ++j)
			{
				r2::draw::cmd::DrawBatchSubCommand subCommand;
				subCommand.baseInstance = i;
				subCommand.baseVertex = modelRef.mMeshRefs[j].baseVertex;
				subCommand.firstIndex = modelRef.mMeshRefs[j].baseIndex;
				subCommand.instanceCount = 1;
				subCommand.count = modelRef.mMeshRefs[j].numIndices;

				r2::sarr::Push(subCommands, subCommand);
			}
		}
	}

	void FillSubCommandsForDebugBones(r2::SArray<r2::draw::cmd::DrawDebugBatchSubCommand>& subCommands, const r2::SArray<u64>& numBonesPerModel)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		const u64 numModels = r2::sarr::Size(numBonesPerModel);

		u64 numVertices = 0;

		for (u64 i = 0; i < numModels; ++i)
		{
			u64 numBonesForModeli = r2::sarr::At(numBonesPerModel, i);
			r2::draw::cmd::DrawDebugBatchSubCommand subCommand;

			subCommand.baseInstance = i;
			subCommand.firstVertex = numVertices;
			subCommand.instanceCount = 1;
			
			u64 verts = 2 * numBonesForModeli;
			numVertices += verts; 
			subCommand.count = verts;

			r2::sarr::Push(subCommands, subCommand);
		}
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

	r2::draw::cmd::FillConstantBuffer* AddFillConstantBufferCommand(CommandBucket<key::Basic>& bucket, r2::draw::key::Basic key, u64 auxMemory)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		return r2::draw::renderer::AddCommand<r2::draw::cmd::FillConstantBuffer>(bucket, key, auxMemory);
	}

	const ConstantBufferData GetConstData(ConstantBufferHandle handle)
	{
		ConstantBufferData defaultConstBufferData;
		const ConstantBufferData& constData = r2::shashmap::Get(*s_optrRenderer->mConstantBufferData, handle, defaultConstBufferData);

		if (constData.handle == EMPTY_BUFFER)
		{
			R2_CHECK(false, "We didn't find the constant buffer data associated with this handle: %u", handle);
			
			return defaultConstBufferData;
		}

		return constData;
	}

	void AddDrawBatch(const BatchConfig& batch)
	{
		AddDrawBatchInternal(*s_optrRenderer->mCommandBucket, batch, nullptr);
	}

	void AddDrawBatchInternal(CommandBucket<key::Basic>& bucket, const BatchConfig& batch, tex::TextureAddress* addr)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;                                  
		}

		if (!s_optrRenderer->mConstantBufferData)
		{
			R2_CHECK(false, "We haven't generated any constant buffers!");
			return;
		}

		if (batch.numDraws == 0)
		{
			R2_CHECK(false, "Need to know the number of draws for now!");
			return;
		}

		if (batch.subcommands && r2::sarr::Size(*batch.subcommands) == 0)
		{
			R2_CHECK(false, "We need subcommands to draw!");
			return;
		}

		if (batch.models && (r2::sarr::Size(*batch.materials) != r2::sarr::Size(*batch.models)))
		{
			R2_CHECK(false, "Mismatched number of elements in batch arrays");
			return;
		}

		if (batch.boneTransformOffsets && r2::sarr::Size(*batch.boneTransformOffsets) == 0)
		{
			R2_CHECK(false, "Mismatched number of elements in the boneTranformOffsets!");
			return;
		}


		const VertexLayoutConfigHandle& vertexLayoutHandles = r2::sarr::At(*s_optrRenderer->mVertexLayoutConfigHandles, batch.vertexLayoutConfigHandle);

		// r2::draw::key::Basic clearKey;

	   // r2::draw::cmd::Clear* clearCMD = r2::draw::renderer::AddClearCommand(clearKey);
	   // clearCMD->flags = r2::draw::cmd::CLEAR_COLOR_BUFFER | r2::draw::cmd::CLEAR_DEPTH_BUFFER;

		r2::draw::cmd::Clear* clearCMD = nullptr;

		if (batch.clear)
		{
			clearCMD = r2::draw::renderer::AddClearCommand(bucket, batch.key);

			s32 flags = r2::draw::cmd::CLEAR_COLOR_BUFFER;
			if (batch.clearDepth)
			{
				flags = r2::draw::cmd::CLEAR_COLOR_BUFFER | r2::draw::cmd::CLEAR_DEPTH_BUFFER;
			}
			clearCMD->flags =flags;
		}

		r2::draw::cmd::FillConstantBuffer* constCMD = nullptr;
		
		if (batch.models)
		{
			u64 modelsSize = batch.models->mSize * sizeof(glm::mat4);
			

			if (clearCMD)
			{
				constCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::Clear, r2::draw::cmd::FillConstantBuffer>(clearCMD, modelsSize);
			}
			else
			{
				constCMD = r2::draw::renderer::AddFillConstantBufferCommand(bucket, batch.key, modelsSize);
			}

			char* auxMemory = r2::draw::cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(constCMD);
			memcpy(auxMemory, batch.models->mData, modelsSize);

			//fill out constCMD
			{
				constCMD->data = auxMemory;
				constCMD->dataSize = modelsSize;
				constCMD->offset = 0;
				constCMD->constantBufferHandle = batch.modelsHandle;

				const ConstantBufferData modelConstData = GetConstData(batch.modelsHandle);

				constCMD->isPersistent = modelConstData.isPersistent;
				constCMD->type = modelConstData.type;
			}
		}
		
		

		//Set the texture addresses for all of the materials used in this batch

		const u64 numMaterialsInBatch = r2::sarr::Size(*batch.materials);

		r2::SArray<r2::draw::tex::TextureAddress>* modelMaterials = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::TextureAddress, MAX_NUM_MATERIAL_TEXTURES_PER_OBJECT * batch.numDraws);

		for (u64 i = 0; i < numMaterialsInBatch; ++i)
		{
			const r2::draw::MaterialHandle& matHandle = r2::sarr::At(*batch.materials, i);

			r2::draw::MaterialSystem* matSystem = r2::draw::matsys::GetMaterialSystem(matHandle.slot);
			R2_CHECK(matSystem != nullptr, "Failed to get the material system!");

			u64 totalNumTextures = 0;

			if (!addr)
			{
				const r2::SArray<r2::draw::tex::Texture>* textures = r2::draw::mat::GetTexturesForMaterial(*matSystem, matHandle);
				if (textures)
				{
					const auto numTextures = r2::sarr::Size(*textures);
					totalNumTextures += numTextures;

					for (u64 t = 0; t < numTextures; ++t)
					{
						const r2::draw::tex::Texture& texture = r2::sarr::At(*textures, t);
						const r2::draw::tex::TextureAddress& addr = r2::draw::texsys::GetTextureAddress(texture);
						r2::sarr::Push(*modelMaterials, addr);
					}
				}

				//do this for the cubemaps
				const r2::SArray<r2::draw::tex::CubemapTexture>* cubemapTextures = r2::draw::mat::GetCubemapTextures(*matSystem);

				if (cubemapTextures)
				{
					const auto numCubemaps = r2::sarr::Size(*cubemapTextures);

					totalNumTextures += numCubemaps;

					for (u64 t = 0; t < numCubemaps; ++t)
					{
						const r2::draw::tex::CubemapTexture& cubemap = r2::sarr::At(*cubemapTextures, t);
						r2::sarr::Push(*modelMaterials, r2::draw::texsys::GetTextureAddress(cubemap));
					}
				}
			}
			else
			{

				r2::sarr::Push(*modelMaterials, *addr);
				totalNumTextures = 1;

			}

			for (u64 i = totalNumTextures; i < MAX_NUM_MATERIAL_TEXTURES_PER_OBJECT; ++i)
			{
				r2::sarr::Push(*modelMaterials, { 1, 1.0 });
			}
		}


		//fill out material data
		u64 materialDataSize = sizeof(r2::draw::tex::TextureAddress) * MAX_NUM_MATERIAL_TEXTURES_PER_OBJECT * batch.numDraws;
		
		r2::draw::cmd::FillConstantBuffer* materialsCMD = nullptr;
		if (constCMD)
		{
			materialsCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::FillConstantBuffer, r2::draw::cmd::FillConstantBuffer>(constCMD, materialDataSize);
		}
		else if (clearCMD)
		{
			materialsCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::Clear, r2::draw::cmd::FillConstantBuffer>(clearCMD, materialDataSize);
		}
		else
		{
			materialsCMD = r2::draw::renderer::AddFillConstantBufferCommand(bucket, batch.key, materialDataSize);
		}
		
		char* materialsAuxMemory = r2::draw::cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(materialsCMD);
		memcpy(materialsAuxMemory, modelMaterials->mData, materialDataSize);

		materialsCMD->data = materialsAuxMemory;
		materialsCMD->dataSize = materialDataSize;
		materialsCMD->offset = 0;
		materialsCMD->constantBufferHandle = batch.materialsHandle;
		{
			const ConstantBufferData materialConstData = GetConstData(batch.materialsHandle);

			materialsCMD->isPersistent = materialConstData.isPersistent;
			materialsCMD->type = materialConstData.type;

		}

		FREE(modelMaterials, *MEM_ENG_SCRATCH_PTR);

		cmd::FillConstantBuffer* prevFillCMD = materialsCMD;

		//fill out the bone data if we have it
		if (batch.boneTransforms && batch.boneTransformOffsets)
		{
			u64 boneTransformSize = batch.boneTransforms->mSize * sizeof(ShaderBoneTransform);
			r2::draw::cmd::FillConstantBuffer* boneTransformsCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer>(materialsCMD, boneTransformSize);
			
			char* boneTransformsAuxMem = r2::draw::cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(boneTransformsCMD);
			memcpy(boneTransformsAuxMem, batch.boneTransforms->mData, boneTransformSize);

			boneTransformsCMD->data = boneTransformsAuxMem;
			boneTransformsCMD->dataSize = boneTransformSize;
			boneTransformsCMD->offset = 0;
			boneTransformsCMD->constantBufferHandle = batch.boneTransformsHandle;

			const ConstantBufferData boneXFormConstData = GetConstData(batch.boneTransformsHandle);

			boneTransformsCMD->isPersistent = boneXFormConstData.isPersistent;
			boneTransformsCMD->type = boneXFormConstData.type;


			u64 boneTransformOffsetsDataSize = batch.boneTransformOffsets->mSize * sizeof(glm::ivec4);
			r2::draw::cmd::FillConstantBuffer* boneTransformOffsetsCMD = AppendCommand<cmd::FillConstantBuffer, cmd::FillConstantBuffer>(boneTransformsCMD, boneTransformOffsetsDataSize);

			char* boneTransformsOffsetsAuxMem = r2::draw::cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(boneTransformOffsetsCMD);
			memcpy(boneTransformsOffsetsAuxMem, batch.boneTransformOffsets->mData, boneTransformOffsetsDataSize);

			boneTransformOffsetsCMD->data = boneTransformsOffsetsAuxMem;
			boneTransformOffsetsCMD->dataSize = boneTransformOffsetsDataSize;
			boneTransformOffsetsCMD->offset = 0;
			boneTransformOffsetsCMD->constantBufferHandle = batch.boneTransformOffsetsHandle;

			const ConstantBufferData boneXFormOffsetsConstData = GetConstData(batch.boneTransformOffsetsHandle);

			boneTransformOffsetsCMD->isPersistent = boneXFormOffsetsConstData.isPersistent;
			boneTransformOffsetsCMD->type = boneXFormOffsetsConstData.type;

			prevFillCMD = boneTransformOffsetsCMD;
		}

		u64 subCommandsSize = batch.subcommands->mSize * sizeof(cmd::DrawBatchSubCommand);
		r2::draw::cmd::DrawBatch* batchCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::FillConstantBuffer, r2::draw::cmd::DrawBatch>(prevFillCMD, subCommandsSize);

		cmd::DrawBatchSubCommand* subCommandsMem = (cmd::DrawBatchSubCommand*)r2::draw::cmdpkt::GetAuxiliaryMemory<cmd::DrawBatch>(batchCMD);
		memcpy(subCommandsMem, batch.subcommands->mData, subCommandsSize);

		batchCMD->bufferLayoutHandle = vertexLayoutHandles.mBufferLayoutHandle;
		batchCMD->batchHandle = batch.subCommandsHandle;
		batchCMD->numSubCommands = batch.subcommands->mSize;
		batchCMD->subCommands = subCommandsMem;




		r2::draw::cmd::CompleteConstantBuffer* completeMaterialsCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::DrawBatch, r2::draw::cmd::CompleteConstantBuffer>(batchCMD, 0);

		completeMaterialsCMD->constantBufferHandle = batch.materialsHandle;
		completeMaterialsCMD->count = batch.materials->mSize;

		r2::draw::cmd::CompleteConstantBuffer* completeConstCMD = nullptr;
		if (batch.models)
		{
			completeConstCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::CompleteConstantBuffer, r2::draw::cmd::CompleteConstantBuffer>(completeMaterialsCMD, 0);

			completeConstCMD->constantBufferHandle = batch.modelsHandle;
			completeConstCMD->count = batch.models->mSize;
		}
		

		if (batch.boneTransforms && batch.boneTransformOffsets)
		{
			r2::draw::cmd::CompleteConstantBuffer* completeBoneTransformCMD = nullptr;
			if (completeConstCMD)
			{
				completeBoneTransformCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::CompleteConstantBuffer, r2::draw::cmd::CompleteConstantBuffer>(completeConstCMD, 0);
			}
			else
			{
				completeBoneTransformCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::CompleteConstantBuffer, r2::draw::cmd::CompleteConstantBuffer>(completeMaterialsCMD, 0);
			}

			completeBoneTransformCMD->constantBufferHandle = batch.boneTransformsHandle;
			completeBoneTransformCMD->count = batch.boneTransforms->mSize;

			r2::draw::cmd::CompleteConstantBuffer* completeBoneTransformOffsetsCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::CompleteConstantBuffer, r2::draw::cmd::CompleteConstantBuffer>(completeBoneTransformCMD, 0);

			completeBoneTransformOffsetsCMD->constantBufferHandle = batch.boneTransformOffsetsHandle;
			completeBoneTransformOffsetsCMD->count = batch.boneTransformOffsets->mSize;
		}
	}

	void AddDebugBatch(
		const r2::SArray<DebugBone>& bones,
		const r2::SArray<u64>& numBonesPerModel,
		const r2::SArray<glm::mat4>& modelMats, 
		r2::draw::ConstantBufferHandle modelMatsHandle,
		r2::draw::ConstantBufferHandle subCommandsHandle)
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

		if (s_optrRenderer->mDebugVertexConfigHandle == InvalidVertexConfigHandle)
		{
			R2_CHECK(false, "We haven't setup a debug vertex configuration!");
			return;
		}


		const VertexLayoutConfigHandle& vertexLayoutHandles = r2::sarr::At(*s_optrRenderer->mVertexLayoutConfigHandles, s_optrRenderer->mDebugVertexConfigHandle);


		r2::SArray<cmd::DrawDebugBatchSubCommand>* subCommands = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, cmd::DrawDebugBatchSubCommand, r2::sarr::Size(numBonesPerModel));
		FillSubCommandsForDebugBones(*subCommands, numBonesPerModel);

		u64 vOffset = 0;

		r2::draw::key::Basic fillKey = r2::draw::key::GenerateKey(0, 0, key::Basic::VPL_DEBUG, 0, 0, s_optrRenderer->mDebugMaterialHandle);

		r2::draw::cmd::FillVertexBuffer* fillVertexCommand = r2::draw::renderer::AddCommand<r2::draw::cmd::FillVertexBuffer>(*s_optrRenderer->mCommandBucket, fillKey, 0);
		vOffset = r2::draw::cmd::FillVertexBufferCommand(fillVertexCommand, bones, vertexLayoutHandles.mVertexBufferHandles[0], vOffset);

		u64 modelsSize = modelMats.mSize * sizeof(glm::mat4);

		cmd::FillConstantBuffer* fillModelMatsCommand = r2::draw::renderer::AppendCommand<cmd::FillVertexBuffer, cmd::FillConstantBuffer>(fillVertexCommand, modelsSize);

		char* auxMemory = r2::draw::cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(fillModelMatsCommand);
		memcpy(auxMemory, modelMats.mData, modelsSize);

		//fill out constCMD
		{
			fillModelMatsCommand->data = auxMemory;
			fillModelMatsCommand->dataSize = modelsSize;
			fillModelMatsCommand->offset = 0;
			fillModelMatsCommand->constantBufferHandle = modelMatsHandle;

			const ConstantBufferData modelConstData = GetConstData(modelMatsHandle);

			fillModelMatsCommand->isPersistent = modelConstData.isPersistent;
			fillModelMatsCommand->type = modelConstData.type;
		}


		u64 numSubCommands = r2::sarr::Size(*subCommands);

		u64 subCommandsSize = numSubCommands * sizeof(cmd::DrawDebugBatchSubCommand);
		r2::draw::cmd::DrawDebugBatch* batchCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::FillConstantBuffer, r2::draw::cmd::DrawDebugBatch>(fillModelMatsCommand, subCommandsSize);

		cmd::DrawDebugBatchSubCommand* subCommandsMem = (cmd::DrawDebugBatchSubCommand*)r2::draw::cmdpkt::GetAuxiliaryMemory<cmd::DrawDebugBatch>(batchCMD);
		memcpy(subCommandsMem, subCommands->mData, subCommandsSize);

		FREE(subCommands, *MEM_ENG_SCRATCH_PTR);

		batchCMD->bufferLayoutHandle = vertexLayoutHandles.mBufferLayoutHandle;
		batchCMD->batchHandle = subCommandsHandle;
		batchCMD->numSubCommands = numSubCommands;
		batchCMD->subCommands = subCommandsMem;
		

		r2::draw::cmd::CompleteConstantBuffer* completeConstCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::DrawDebugBatch, r2::draw::cmd::CompleteConstantBuffer>(batchCMD, 0);

		completeConstCMD->constantBufferHandle = modelMatsHandle;
		completeConstCMD->count = modelMats.mSize;
	}


	void AddFinalBatchInternal()
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

		if (s_optrRenderer->finalBatch.vertexLayoutConfigHandle == InvalidVertexConfigHandle)
		{
			R2_CHECK(false, "We haven't setup a debug vertex configuration!");
			return;
		}

		R2_CHECK(r2::sarr::Size(*s_optrRenderer->mCommandBucket->renderTarget.colorAttachments) == 1, "Currently we only support 1 texture");

		auto addr = texsys::GetTextureAddress(r2::sarr::At(*s_optrRenderer->mCommandBucket->renderTarget.colorAttachments, 0).texture);
		AddDrawBatchInternal(*s_optrRenderer->mFinalBucket, s_optrRenderer->finalBatch, &addr);
	}

	void SetupFinalBatchInternal()
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

		if (s_optrRenderer->finalBatch.vertexLayoutConfigHandle == InvalidVertexConfigHandle)
		{
			R2_CHECK(false, "We haven't setup a debug vertex configuration!");
			return;
		}

		const r2::SArray<r2::draw::ConstantBufferHandle>* constantBufferHandles = r2::draw::renderer::GetConstantBufferHandles();

		s_optrRenderer->finalBatch.clear = true;
		s_optrRenderer->finalBatch.clearDepth = false;
		s_optrRenderer->finalBatch.materialsHandle = r2::sarr::At(*constantBufferHandles, s_optrRenderer->mMaterialConfigHandle);
		s_optrRenderer->finalBatch.modelsHandle = r2::sarr::At(*constantBufferHandles, s_optrRenderer->mModelConfigHandle);
		s_optrRenderer->finalBatch.subCommandsHandle = r2::sarr::At(*constantBufferHandles, s_optrRenderer->mSubcommandsConfigHandle);
		s_optrRenderer->finalBatch.numDraws = 1;
		s_optrRenderer->finalBatch.key = r2::draw::key::GenerateKey(0, 0, key::Basic::VPL_SCREEN, 0, 0, s_optrRenderer->mFinalCompositeMaterialHandle);

		//We need to scale because our quad is -0.5 to 0.5 and it needs to be -1.0 to 1.0
		glm::mat4 scale = glm::mat4(1.0f);
		scale = glm::scale(scale, glm::vec3(2.0f));
		r2::sarr::Push(*s_optrRenderer->finalBatch.models, scale);

		
		r2::SArray<draw::ModelRef>* quadModelRef = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::ModelRef, 1);
		r2::sarr::Push(*quadModelRef, GetDefaultModelRef(QUAD));
		FillSubCommandsFromModelRefs(*s_optrRenderer->finalBatch.subcommands, *quadModelRef);
		FREE(quadModelRef, *MEM_ENG_SCRATCH_PTR);

		r2::sarr::Push(*s_optrRenderer->finalBatch.materials, s_optrRenderer->mFinalCompositeMaterialHandle);
	}

	//events
	void WindowResized(u32 width, u32 height)
	{
		r2::draw::rendererimpl::WindowResized(width, height);
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

	void SetWindowSize(u32 width, u32 height)
	{
		r2::draw::rendererimpl::SetWindowSize(width, height);
	}
}
