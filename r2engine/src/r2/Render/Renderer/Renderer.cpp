#include "r2pch.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Render/Renderer/RendererImpl.h"
#include "r2/Render/Renderer/CommandPacket.h"
#include "r2/Render/Renderer/CommandBucket.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Renderer/Commands.h"

#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Model/Material.h"
#include "r2/Render/Renderer/ShaderSystem.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Model/ModelLoader.h"
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
	struct ModelSystem
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;
		r2::SArray<Model*>* mDefaultModels = nullptr;
	};

	struct ConstantBufferData
	{
		r2::draw::ConstantBufferHandle handle;
		r2::draw::ConstantBufferLayout::Type type;
		b32 isPersistent;
	};

	struct Renderer
	{
		//memory
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;

		r2::draw::BufferHandles mBufferHandles;
		r2::SArray<r2::draw::ConstantBufferHandle>* mContantBufferHandles;
		r2::SArray<ConstantBufferData>* mConstantBufferData;

		ModelSystem mModelSystem;
		MaterialSystem* mMaterialSystem = nullptr;

		//Each bucket needs the bucket and an arena for that bucket
		r2::draw::CommandBucket<r2::draw::key::Basic>* mCommandBucket = nullptr;
		r2::mem::StackArena* mCommandArena = nullptr;

		//@TODO(Serge): hold pointers to the material system, and model system
	};

	namespace modelsystem
	{
		bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle);
		void Shutdown();
		u64 MemorySize();
		bool LoadEngineModels(const char* modelDirectory);
	}
}

namespace
{
	r2::draw::Renderer* s_optrRenderer = nullptr;

	const u64 COMMAND_CAPACITY = 2048;
	const u64 COMMAND_AUX_MEMORY = Megabytes(1); //I dunno lol
	const u64 ALIGNMENT = 16;
	const u32 MAX_BUFFER_LAYOUTS = 32;
	const u64 MAX_NUM_MATERIALS = 2048;
	const u64 MAX_NUM_SHADERS = 1000;
	const u64 MAX_DEFAULT_MODELS = 16;
	const u64 MAX_NUM_TEXTURES = 2048;
	const u64 MAX_NUM_MATERIAL_SYSTEMS = 16;


	const u64 MAX_NUM_CONSTANT_BUFFERS = 16; //?
	const u64 MAX_NUM_CONSTANT_BUFFER_LOCKS = 3; //?

	const std::string MODL_EXT = ".modl";
}

namespace r2::draw::renderer
{
	u64 MaterialSystemMemorySize(u64 numMaterials, u64 textureCacheInBytes, u64 totalNumberOfTextures, u64 numPacks, u64 maxTexturesInAPack);

	//basic stuff
	bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle, const char* shaderManifestPath)
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

		s_optrRenderer->mCommandBucket = MAKE_CMD_BUCKET(*rendererArena, r2::draw::key::Basic, r2::draw::key::DecodeBasicKey, COMMAND_CAPACITY);
		R2_CHECK(s_optrRenderer->mCommandBucket != nullptr, "We couldn't create the command bucket!");

		s_optrRenderer->mBufferHandles.bufferLayoutHandles = MAKE_SARRAY(*rendererArena, r2::draw::BufferLayoutHandle, MAX_BUFFER_LAYOUTS);
		
		R2_CHECK(s_optrRenderer->mBufferHandles.bufferLayoutHandles != nullptr, "We couldn't create the buffer layout handles!");
		
		s_optrRenderer->mBufferHandles.vertexBufferHandles = MAKE_SARRAY(*rendererArena, r2::draw::VertexBufferHandle, MAX_BUFFER_LAYOUTS);
		
		R2_CHECK(s_optrRenderer->mBufferHandles.vertexBufferHandles != nullptr, "We couldn't create the vertex buffer layout handles!");
		
		s_optrRenderer->mBufferHandles.indexBufferHandles = MAKE_SARRAY(*rendererArena, r2::draw::IndexBufferHandle, MAX_BUFFER_LAYOUTS);

		R2_CHECK(s_optrRenderer->mBufferHandles.indexBufferHandles != nullptr, "We couldn't create the index buffer layout handles!");

		s_optrRenderer->mBufferHandles.drawIDHandles = MAKE_SARRAY(*rendererArena, r2::draw::DrawIDHandle, MAX_BUFFER_LAYOUTS);

		R2_CHECK(s_optrRenderer->mBufferHandles.drawIDHandles != nullptr, "We couldn't create the draw id handles");
		
		s_optrRenderer->mContantBufferHandles = MAKE_SARRAY(*rendererArena, r2::draw::ConstantBufferHandle, MAX_BUFFER_LAYOUTS);
		
		R2_CHECK(s_optrRenderer->mContantBufferHandles != nullptr, "We couldn't create the constant buffer handles");

		s_optrRenderer->mConstantBufferData = MAKE_SARRAY(*rendererArena, ConstantBufferData, MAX_BUFFER_LAYOUTS);

		R2_CHECK(s_optrRenderer->mConstantBufferData != nullptr, "We couldn't create the constant buffer data!");

		s_optrRenderer->mCommandArena = MAKE_STACK_ARENA(*rendererArena, COMMAND_CAPACITY * cmd::LargestCommand() + COMMAND_AUX_MEMORY);

		R2_CHECK(s_optrRenderer->mCommandArena != nullptr, "We couldn't create the stack arena for commands");

		bool rendererImpl = r2::draw::rendererimpl::RendererImplInit(memoryAreaHandle, MAX_NUM_CONSTANT_BUFFERS, MAX_NUM_CONSTANT_BUFFER_LOCKS, "RendererImpl");
		if (!rendererImpl)
		{
			R2_CHECK(false, "We couldn't initialize the renderer implementation");
			return false;
		}

		bool shaderSystemIntialized = r2::draw::shadersystem::Init(memoryAreaHandle, MAX_NUM_SHADERS, shaderManifestPath);
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


		bool materialSystemInitialized = r2::draw::matsys::Init(memoryAreaHandle, MAX_NUM_MATERIAL_SYSTEMS, "Material Systems Area");
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

		if (!modelsystem::Init(memoryAreaHandle))
		{
			R2_CHECK(false, "We couldn't init the default engine models");
			return false;
		}

		bool loadedModels = modelsystem::LoadEngineModels(R2_ENGINE_INTERNAL_MODELS_BIN);

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

		//const u64 numEntries = r2::sarr::Size(*s_optrRenderer->mCommandBucket->entries);

		//for (u64 i = 0; i < numEntries; ++i)
		//{
		//	const r2::draw::CommandBucket<r2::draw::key::Basic>::Entry& entry = r2::sarr::At(*s_optrRenderer->mCommandBucket->entries, i);
		//	printf("entry - key: %llu, data: %p, func: %p\n", entry.aKey.keyValue, entry.data, entry.func);
		//}

		//printf("================================================\n");
		cmdbkt::Sort(*s_optrRenderer->mCommandBucket, r2::draw::key::CompareKey);

		//for (u64 i = 0; i < numEntries; ++i)
		//{
		//	const r2::draw::CommandBucket<r2::draw::key::Basic>::Entry* entry = r2::sarr::At(*s_optrRenderer->mCommandBucket->sortedEntries, i);
		//	printf("sorted - key: %llu, data: %p, func: %p\n", entry->aKey.keyValue, entry->data, entry->func);
		//}


		cmdbkt::Submit(*s_optrRenderer->mCommandBucket);

		cmdbkt::ClearAll(*s_optrRenderer->mCommandBucket);

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

		modelsystem::Shutdown();

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

		FREE(s_optrRenderer->mCommandBucket, *arena);
		FREE(s_optrRenderer->mCommandArena, *arena);

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
		rendererimpl::GenerateVertexBuffers((u32)numLayouts, s_optrRenderer->mBufferHandles.vertexBufferHandles->mData);
		s_optrRenderer->mBufferHandles.vertexBufferHandles->mSize = numLayouts;

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

			rendererimpl::SetupBufferLayoutConfiguration(config,
				r2::sarr::At(*s_optrRenderer->mBufferHandles.bufferLayoutHandles, i),
				r2::sarr::At(*s_optrRenderer->mBufferHandles.vertexBufferHandles, i),
				r2::sarr::At(*s_optrRenderer->mBufferHandles.indexBufferHandles, i),
				r2::sarr::At(*s_optrRenderer->mBufferHandles.drawIDHandles, i));
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

		if (r2::sarr::Size(*s_optrRenderer->mConstantBufferData) > 0)
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

			r2::sarr::Push(*s_optrRenderer->mConstantBufferData, constData);
		}

		r2::draw::rendererimpl::SetupConstantBufferConfigs(constantBufferConfigs, s_optrRenderer->mContantBufferHandles->mData);

		return true;
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
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::BufferLayoutHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::VertexBufferHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::IndexBufferHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::DrawIDHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ConstantBufferHandle>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ConstantBufferData>::MemorySize(MAX_BUFFER_LAYOUTS), ALIGNMENT, headerSize, boundsChecking) + 
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena) + r2::mem::utils::GetMaxMemoryForAllocation(cmd::LargestCommand(), ALIGNMENT, stackHeaderSize, boundsChecking ) * COMMAND_CAPACITY + COMMAND_AUX_MEMORY, ALIGNMENT, headerSize, boundsChecking) +
			materialSystemMemorySize;

		return r2::mem::utils::GetMaxMemoryForAllocation(memorySize, ALIGNMENT);
	}

	BufferHandles& GetBufferHandles()
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

	r2::draw::CommandBucket<r2::draw::key::Basic>& GetCommandBucket()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
		}

		R2_CHECK(s_optrRenderer != nullptr, "We haven't initialized the renderer yet!");
		return *s_optrRenderer->mCommandBucket;
	}

	template<class CMD>
	CMD* AddCommand(r2::draw::key::Basic key, u64 auxMemory)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		R2_CHECK(s_optrRenderer->mCommandBucket != nullptr, "Command Bucket is nullptr!");

		return r2::draw::cmdbkt::AddCommand<r2::draw::key::Basic, r2::mem::StackArena, CMD>(*s_optrRenderer->mCommandArena, *s_optrRenderer->mCommandBucket, key, auxMemory);
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

	r2::draw::Model* GetDefaultModel(r2::draw::DefaultModel defaultModel)
	{
		if (s_optrRenderer == nullptr ||
			s_optrRenderer->mModelSystem.mDefaultModels == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		return r2::sarr::At(*s_optrRenderer->mModelSystem.mDefaultModels, defaultModel);
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

	void UploadMaterialTexturesToGPUFromMaterialName(u64 materialName)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		r2::draw::mat::UploadMaterialTexturesToGPUFromMaterialName(*s_optrRenderer->mMaterialSystem, materialName);
	}

	u64 AddFillVertexCommandsForModel(Model* model, VertexBufferHandle handle, u64 offset)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return 0;
		}

		if (model == nullptr)
		{
			R2_CHECK(false, "We don't have a proper model!");
			return 0;
		}

		const u64 numMeshes = r2::sarr::Size(*model->optrMeshes);

		u64 currentOffset = offset;
		for (u64 i = 0; i < numMeshes; ++i)
		{
			r2::draw::key::Basic fillKey;
			//@TODO(Serge): fix this or pass it in
			fillKey.keyValue = currentOffset;

			r2::draw::cmd::FillVertexBuffer* fillVertexCommand = r2::draw::renderer::AddCommand<r2::draw::cmd::FillVertexBuffer>(fillKey, 0);
			currentOffset = r2::draw::cmd::FillVertexBufferCommand(fillVertexCommand, r2::sarr::At(*model->optrMeshes, i), handle, currentOffset);
		}

		return currentOffset;
	}

	u64 AddFillIndexCommandsForModel(Model* model, IndexBufferHandle handle, u64 offset)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return 0;
		}

		if (model == nullptr)
		{
			R2_CHECK(false, "We don't have a proper model!");
			return 0;
		}

		const u64 numMeshes = r2::sarr::Size(*model->optrMeshes);

		u64 currentOffset = offset;
		for (u64 i = 0; i < numMeshes; ++i)
		{
			r2::draw::key::Basic fillKey;
			//@TODO(Serge): fix this or pass it in
			fillKey.keyValue = currentOffset;

			r2::draw::cmd::FillIndexBuffer* fillIndexCommand = r2::draw::renderer::AddCommand<r2::draw::cmd::FillIndexBuffer>(fillKey, 0);
			currentOffset = r2::draw::cmd::FillIndexBufferCommand(fillIndexCommand, r2::sarr::At(*model->optrMeshes, i), handle, currentOffset);
		}

		return currentOffset;
	}

	u64 AddFillConstantBufferCommandForData(ConstantBufferHandle handle, r2::draw::ConstantBufferLayout::Type type, b32 isPersistent, void* data, u64 size, u64 offset)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return 0;
		}

		r2::draw::key::Basic fillKey;
		//@TODO(Serge): fix this or pass it in
		fillKey.keyValue = 0;

		r2::draw::cmd::FillConstantBuffer* fillConstantBufferCommand = r2::draw::renderer::AddFillConstantBufferCommand(fillKey, size);
		return r2::draw::cmd::FillConstantBufferCommand(fillConstantBufferCommand, handle, type, isPersistent, data, size, offset);
	}

	void FillSubCommandsFromModels(r2::SArray<r2::draw::cmd::DrawBatchSubCommand>& subCommands, const r2::SArray<Model*>& models)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		const u64 numModels = r2::sarr::Size(models);

		u64 numVertices = 0;
		u64 numIndices = 0;

		for (u64 i = 0; i < numModels; ++i)
		{
			r2::draw::cmd::DrawBatchSubCommand subCommand;

			//@NOTE: this is assuming that each model's index count starts at 0 (and not each mesh starts its index at 0)

			subCommand.baseInstance = i;
			subCommand.baseVertex = numVertices;
			subCommand.firstIndex = numIndices;
			subCommand.instanceCount = 1;

			Model* model = r2::sarr::At(models, i);

			const u64 numMeshes = r2::sarr::Size(*model->optrMeshes);

			u64 count = 0;
			for (u64 m = 0; m < numMeshes; ++m)
			{
				const Mesh& mesh = r2::sarr::At(*model->optrMeshes, m);

				count += r2::sarr::Size(*mesh.optrIndices);
				numVertices += r2::sarr::Size(*mesh.optrVertices);
			}
			numIndices += count;

			subCommand.count = count;

			r2::sarr::Push(subCommands, subCommand);
		}

	}

	r2::draw::cmd::Clear* AddClearCommand(r2::draw::key::Basic key)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		R2_CHECK(s_optrRenderer != nullptr, "We haven't initialized the renderer yet!");

		return r2::draw::cmdbkt::AddCommand<r2::draw::key::Basic, r2::mem::StackArena, r2::draw::cmd::Clear>(*s_optrRenderer->mCommandArena, *s_optrRenderer->mCommandBucket, key, 0);
	}

	r2::draw::cmd::DrawIndexed* AddDrawIndexedCommand(r2::draw::key::Basic key)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		R2_CHECK(s_optrRenderer != nullptr, "We haven't initialized the renderer yet!");

		return r2::draw::renderer::AddCommand<r2::draw::cmd::DrawIndexed>(key, 0);
	}

	r2::draw::cmd::FillIndexBuffer* AddFillIndexBufferCommand(r2::draw::key::Basic key)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		R2_CHECK(s_optrRenderer != nullptr, "We haven't initialized the renderer yet!");

		return r2::draw::renderer::AddCommand<r2::draw::cmd::FillIndexBuffer>(key, 0);
	}

	r2::draw::cmd::FillVertexBuffer* AddFillVertexBufferCommand(r2::draw::key::Basic key)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		R2_CHECK(s_optrRenderer != nullptr, "We haven't initialized the renderer yet!");

		return r2::draw::renderer::AddCommand<r2::draw::cmd::FillVertexBuffer>(key, 0);
	}

	r2::draw::cmd::FillConstantBuffer* AddFillConstantBufferCommand(r2::draw::key::Basic key, u64 auxMemory)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return nullptr;
		}

		return r2::draw::renderer::AddCommand<r2::draw::cmd::FillConstantBuffer>(key, auxMemory);
	}

	void AddDrawBatch(const BatchConfig& batch)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;                                  
		}

		if (r2::sarr::Size(*s_optrRenderer->mConstantBufferData) == 0)
		{
			R2_CHECK(false, "We haven't generated any constant buffers!");
			return;
		}

		u64 modelsSize = batch.models->mSize * sizeof(glm::mat4);
		r2::draw::cmd::FillConstantBuffer* constCMD = r2::draw::renderer::AddCommand<r2::draw::cmd::FillConstantBuffer>(batch.key, modelsSize);
		
		char* auxMemory = r2::draw::cmdpkt::GetAuxiliaryMemory<cmd::FillConstantBuffer>(constCMD);
		memcpy(auxMemory, glm::value_ptr(*batch.models->mData), modelsSize);
		
		//fill out constCMD
		{
			constCMD->data = auxMemory;
			constCMD->dataSize = modelsSize;
			constCMD->offset = 0;
			constCMD->constantBufferHandle = batch.modelsHandle;

			//find the constant buffer data
			bool found = false;
			const u64 numConstantBuffers = r2::sarr::Size(*s_optrRenderer->mConstantBufferData);
			for (u64 i = 0; i < numConstantBuffers; ++i)
			{
				const ConstantBufferData& constBufData = r2::sarr::At(*s_optrRenderer->mConstantBufferData, i);
				if (constBufData.handle == batch.modelsHandle)
				{
					constCMD->isPersistent = constBufData.isPersistent;
					constCMD->type = constBufData.type;
					found = true;
					break;
				}
			}

			if (!found)
			{
				R2_CHECK(false, "We didn't find the constant buffer data associated with this handle: %u", batch.modelsHandle);
			}
		}

		u64 subCommandsSize = batch.subcommands->mSize * sizeof(cmd::DrawBatchSubCommand);
		r2::draw::cmd::DrawBatch* batchCMD = r2::draw::renderer::AppendCommand<r2::draw::cmd::FillConstantBuffer, r2::draw::cmd::DrawBatch>(constCMD, subCommandsSize);

		cmd::DrawBatchSubCommand* subCommandsMem = (cmd::DrawBatchSubCommand*)r2::draw::cmdpkt::GetAuxiliaryMemory<cmd::DrawBatch>(batchCMD);
		memcpy(subCommandsMem, batch.subcommands->mData, subCommandsSize);

		batchCMD->bufferLayoutHandle = batch.layoutHandle;
		batchCMD->batchHandle = batch.subCommandsHandle;
		batchCMD->numSubCommands = batch.subcommands->mSize;
		batchCMD->subCommands = subCommandsMem;
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

namespace r2::draw::modelsystem
{
	bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return false;
		}

		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = modelsystem::MemorySize();
		if (memoryArea->UnAllocatedSpace() < subAreaSize)
		{
			R2_CHECK(false, "We don't have enought space to allocate the renderer!");
			return false;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		if ((subAreaHandle = memoryArea->AddSubArea(subAreaSize, "Engine Model Area")) == r2::mem::MemoryArea::SubArea::Invalid)
		{
			R2_CHECK(false, "We couldn't create a sub area for the engine model area");
			return false;
		}

		//emplace the linear arena
		r2::mem::LinearArena* modelArena = EMPLACE_LINEAR_ARENA(*memoryArea->GetSubArea(subAreaHandle));

		R2_CHECK(modelArena != nullptr, "We couldn't emplace the linear arena - no way to recover!");

		s_optrRenderer->mModelSystem.mMemoryAreaHandle = memoryAreaHandle;
		s_optrRenderer->mModelSystem.mSubAreaHandle = subAreaHandle;
		s_optrRenderer->mModelSystem.mSubAreaArena = modelArena;
		s_optrRenderer->mModelSystem.mDefaultModels = MAKE_SARRAY(*modelArena, Model*, MAX_DEFAULT_MODELS);

		return modelArena != nullptr && s_optrRenderer->mModelSystem.mDefaultModels != nullptr;
	}

	void Shutdown()
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return;
		}

		r2::mem::LinearArena* arena = s_optrRenderer->mModelSystem.mSubAreaArena;

		u64 numDefaultModels = r2::sarr::Size(*s_optrRenderer->mModelSystem.mDefaultModels);
		for (u64 i = 0; i < numDefaultModels; ++i)
		{
			FREE_MODEL(*arena, r2::sarr::At(*s_optrRenderer->mModelSystem.mDefaultModels, i));
		}

		FREE(s_optrRenderer->mModelSystem.mDefaultModels, *arena);

		FREE_EMPLACED_ARENA(arena);
	}

	u64 MemorySize()
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u64 quadModelSize = Model::MemorySize(1, 4, 6, 1, headerSize, boundsChecking, ALIGNMENT);
		u64 cubeModelSize = Model::MemorySize(1, 24, 36, 1, headerSize, boundsChecking, ALIGNMENT);
		u64 sphereModelSize = Model::MemorySize(1, 1089, 5952, 1, headerSize, boundsChecking, ALIGNMENT);
		u64 coneModelSize = Model::MemorySize(1, 148, 144*3, 1, headerSize, boundsChecking, ALIGNMENT);
		u64 cylinderModelSize = Model::MemorySize(1, 148, 144*3, 1, headerSize, boundsChecking, ALIGNMENT);

		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Model*>::MemorySize(MAX_DEFAULT_MODELS), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(quadModelSize, ALIGNMENT, headerSize, boundsChecking) + //For quad
			r2::mem::utils::GetMaxMemoryForAllocation(cubeModelSize, ALIGNMENT, headerSize, boundsChecking) + //For Cube
			r2::mem::utils::GetMaxMemoryForAllocation(sphereModelSize, ALIGNMENT, headerSize, boundsChecking) + //For Sphere
			r2::mem::utils::GetMaxMemoryForAllocation(coneModelSize, ALIGNMENT, headerSize, boundsChecking) + // For Cone
			r2::mem::utils::GetMaxMemoryForAllocation(cylinderModelSize, ALIGNMENT, headerSize, boundsChecking);
	}

	bool LoadEngineModels(const char* modelDirectory)
	{
		if (s_optrRenderer == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the renderer yet!");
			return false;
		}

		for (const auto& file : std::filesystem::recursive_directory_iterator(modelDirectory))
		{
			if (std::filesystem::file_size(file.path()) <= 0 || (file.path().extension().string() != MODL_EXT))
			{
				continue;
			}
			char filePath[r2::fs::FILE_PATH_LENGTH];

			r2::fs::utils::SanitizeSubPath(file.path().string().c_str(), filePath);

			Model* nextModel = LoadModel<r2::mem::LinearArena>(*s_optrRenderer->mModelSystem.mSubAreaArena, *s_optrRenderer->mMaterialSystem, filePath);
			if (!nextModel)
			{
				R2_CHECK(false, "Failed to load the model: %s", file.path().string().c_str());
				return false;
			}

			if (nextModel->hash == STRING_ID("Quad"))
			{
				(*s_optrRenderer->mModelSystem.mDefaultModels)[r2::draw::QUAD] = nextModel;
				s_optrRenderer->mModelSystem.mDefaultModels->mSize++;
			}
			else if (nextModel->hash == STRING_ID("Cube"))
			{
				(*s_optrRenderer->mModelSystem.mDefaultModels)[r2::draw::CUBE] = nextModel;
				s_optrRenderer->mModelSystem.mDefaultModels->mSize++;
			}
			else if (nextModel->hash == STRING_ID("Sphere"))
			{
				(*s_optrRenderer->mModelSystem.mDefaultModels)[r2::draw::SPHERE] = nextModel;
				s_optrRenderer->mModelSystem.mDefaultModels->mSize++;
			}
			else if (nextModel->hash == STRING_ID("Cone"))
			{
				(*s_optrRenderer->mModelSystem.mDefaultModels)[r2::draw::CONE] = nextModel;
				s_optrRenderer->mModelSystem.mDefaultModels->mSize++;
			}
			else if (nextModel->hash == STRING_ID("Cylinder"))
			{
				(*s_optrRenderer->mModelSystem.mDefaultModels)[r2::draw::CYLINDER] = nextModel;
				s_optrRenderer->mModelSystem.mDefaultModels->mSize++;
			}
		}

		return true;
	}
}