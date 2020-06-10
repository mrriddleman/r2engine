#include "r2pch.h"
#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Containers/SHashMap.h"

#include "r2/Core/Assets/AssetTypes.h"

namespace r2::draw
{
	struct TextureSystem
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;
		r2::SHashMap<r2::draw::tex::GPUHandle>* mTextureMap = nullptr;

	};
}

namespace
{
	static r2::draw::TextureSystem* s_optrTextureSystem = nullptr;
	const u64 ALIGNMENT = 32;
	const f64 LOAD_FACTOR = 1.5;
}

namespace r2::draw::texsys
{
	bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 maxNumTextures, const char* systemName)
	{
		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = MemorySize(maxNumTextures);
		if (memoryArea->UnAllocatedSpace() < subAreaSize)
		{
			R2_CHECK(false, "We don't have enought space to allocate the renderer!");
			return false;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		if ((subAreaHandle = memoryArea->AddSubArea(subAreaSize, systemName)) == r2::mem::MemoryArea::SubArea::Invalid)
		{
			R2_CHECK(false, "We couldn't create a sub area for the engine model area");
			return false;
		}

		//emplace the linear arena
		r2::mem::LinearArena* textureLinearArena = EMPLACE_LINEAR_ARENA(*memoryArea->GetSubArea(subAreaHandle));

		R2_CHECK(textureLinearArena != nullptr, "We couldn't emplace the linear arena - no way to recover!");

		s_optrTextureSystem = ALLOC(r2::draw::TextureSystem, *textureLinearArena);


		R2_CHECK(s_optrTextureSystem != nullptr, "Failed to allocate the texture system!");

		s_optrTextureSystem->mMemoryAreaHandle = memoryAreaHandle;
		s_optrTextureSystem->mSubAreaHandle = subAreaHandle;
		s_optrTextureSystem->mSubAreaArena = textureLinearArena;

		s_optrTextureSystem->mTextureMap = MAKE_SHASHMAP(*textureLinearArena, r2::draw::tex::GPUHandle, static_cast<u64>(maxNumTextures * LOAD_FACTOR));
		R2_CHECK(s_optrTextureSystem->mTextureMap != nullptr, "We couldn't allocate the texture map!");

		return textureLinearArena != nullptr && s_optrTextureSystem->mTextureMap != nullptr;
	}

	void Shutdown()
	{
		if (s_optrTextureSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the texture system yet!");
			return;
		}

		r2::mem::LinearArena* arena = s_optrTextureSystem->mSubAreaArena;
		
		FREE(s_optrTextureSystem->mTextureMap, *arena);

		FREE(s_optrTextureSystem, *arena);

		FREE_EMPLACED_ARENA(arena);
	}

	void UploadToGPU(const r2::asset::AssetHandle& texture)
	{
		if (s_optrTextureSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the texture system yet!");
			return;
		}

		r2::draw::tex::GPUHandle theDefault = 0;

		r2::draw::tex::GPUHandle gpuHandle = r2::shashmap::Get(*s_optrTextureSystem->mTextureMap, texture.handle, theDefault);

		if (gpuHandle != theDefault)
			return;
		
		gpuHandle = r2::draw::tex::UploadToGPU(texture, false);

		r2::shashmap::Set(*s_optrTextureSystem->mTextureMap, texture.handle, gpuHandle);

	}

	void UnloadFromGPU(const r2::asset::AssetHandle& texture)
	{
		if (s_optrTextureSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the texture system yet!");
			return;
		}

		r2::draw::tex::GPUHandle defaultGPUHandle;
		r2::draw::tex::GPUHandle gpuHandle = r2::shashmap::Get(*s_optrTextureSystem->mTextureMap, texture.handle, defaultGPUHandle);

		if (gpuHandle != defaultGPUHandle)
		{
			r2::draw::tex::UnloadFromGPU(gpuHandle);
			r2::shashmap::Remove(*s_optrTextureSystem->mTextureMap, texture.handle);
		}
	}

	r2::draw::tex::GPUHandle GetGPUHandle(const r2::asset::AssetHandle& texture)
	{
		if (s_optrTextureSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the texture system yet!");
			return 0;
		}

		r2::draw::tex::GPUHandle defaultGPUHandle;
		r2::draw::tex::GPUHandle gpuHandle = r2::shashmap::Get(*s_optrTextureSystem->mTextureMap, texture.handle, defaultGPUHandle);

		if (gpuHandle == defaultGPUHandle)
		{
			R2_CHECK(false, "Couldn't find the GPUHandle for texture: %llu", texture.handle);
			return 0;
		}

		return gpuHandle;
	}

	u64 MemorySize(u64 maxNumTextures)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(TextureSystem), ALIGNMENT, headerSize, boundsChecking) + 
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), ALIGNMENT, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::draw::tex::GPUHandle>::MemorySize(static_cast<u64>(maxNumTextures * LOAD_FACTOR)), ALIGNMENT, headerSize, boundsChecking);
	}

}