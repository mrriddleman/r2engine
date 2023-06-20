#include "r2pch.h"
#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Math/MathUtils.h"
#include "r2/Core/Assets/AssetFiles/MemoryAssetFile.h"
#include "assetlib/TextureAsset.h"
#include "r2/Render/Renderer/RendererTypes.h"

namespace r2::draw
{
	struct TextureGPUHandle
	{
		r2::draw::tex::GPUHandle gpuHandle;
		r2::draw::tex::TextureType type = tex::Diffuse;
		float anisotropy;
		s32 wrapMode;
		s32 minFilter;
		s32 magFilter;
	};


	struct TextureSystem
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;
		r2::SHashMap<TextureGPUHandle>* mTextureMap = nullptr;
		r2::mem::utils::MemBoundary implBoundary;
	};
}

namespace
{
	static r2::draw::TextureSystem* s_optrTextureSystem = nullptr;
	const u64 ALIGNMENT = 16;
	const f64 LOAD_FACTOR = 1.5;
	const u32 MAX_TEXTURE_CONTAINERS = 32;
	const u32 MAX_TEXTURE_CONATINERS_PER_FROMAT = 16;
}

namespace r2::draw::texsys
{
	r2::draw::tex::TextureAddress GetTextureAddressInternal(const r2::asset::AssetHandle& texture);

	bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 maxNumTextures, const r2::SArray<InitialTextureFormat>* formatsToMake, const char* systemName)
	{
		//const u64 numInitialFormats = r2::sarr::Size(*formatsToMake);

		//for (u64 i = 0; i < numInitialFormats; ++i)
		//{
		//	const InitialTextureFormat& format = r2::sarr::At(*formatsToMake, i);
		//	
		//	printf("InitialTextureFormat: %s, width: %lu, height: %lu, num mips: %lu, is cubemap: %lu, anisotropic: %lu\n", flat::EnumNameTextureFormat(format.textureFormat), format.width, format.height, format.numMips, format.isCubemap, format.isAnisotropic);
		//}

		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		const u64 maxTextureLayers = r2::draw::tex::impl::GetMaxTextureLayers(true);

		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		const u64 texImplMemorySize = r2::draw::tex::impl::MemorySize(
			MAX_TEXTURE_CONTAINERS,
			MAX_TEXTURE_CONATINERS_PER_FROMAT,
			(u32)maxTextureLayers, ALIGNMENT, headerSize, boundsChecking);

		u64 subAreaSize = MemorySize(maxNumTextures) + texImplMemorySize;

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

		s_optrTextureSystem->mTextureMap = MAKE_SHASHMAP(*textureLinearArena, TextureGPUHandle, static_cast<u64>(maxNumTextures * LOAD_FACTOR));
		R2_CHECK(s_optrTextureSystem->mTextureMap != nullptr, "We couldn't allocate the texture map!");


		s_optrTextureSystem->implBoundary = MAKE_BOUNDARY(*textureLinearArena, texImplMemorySize, ALIGNMENT);

		bool success = r2::draw::tex::impl::Init(s_optrTextureSystem->implBoundary, MAX_TEXTURE_CONTAINERS, MAX_TEXTURE_CONATINERS_PER_FROMAT, true);
		R2_CHECK(success, "Failed to initialize the texture implementation!");

		return textureLinearArena != nullptr && s_optrTextureSystem->mTextureMap != nullptr && success;
	}

	void Shutdown()
	{
		if (s_optrTextureSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the texture system yet!");
			return;
		}

		r2::mem::LinearArena* arena = s_optrTextureSystem->mSubAreaArena;
		

		r2::draw::tex::impl::Shutdown();

		FREE(s_optrTextureSystem->implBoundary.location, *arena);

		FREE(s_optrTextureSystem->mTextureMap, *arena);

		FREE(s_optrTextureSystem, *arena);

		FREE_EMPLACED_ARENA(arena);
	}

	void UploadToGPU(const r2::draw::tex::Texture& texture, float anisotropy, s32 wrapMode, s32 minFilter, s32 magFilter)
	{
		if (s_optrTextureSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the texture system yet!");
			return;
		}

		//@TODO(Serge): one thing we should do here is check to see if we've already uploaded this texture with the exact parameters
		//				if we have, then nothing to do - don't waste pages if we don't have to
		TextureGPUHandle theDefault;

		TextureGPUHandle texGPUHandle = r2::shashmap::Get(*s_optrTextureSystem->mTextureMap, texture.textureAssetHandle.handle, theDefault);

		if (!TextureHandlesEqual(texGPUHandle.gpuHandle, theDefault.gpuHandle))
			return;

		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		const void* imageData = gameAssetManager.GetAssetDataConst<void>(texture.textureAssetHandle);
		u64 dataSize = gameAssetManager.GetAssetDataSize(texture.textureAssetHandle);

		texGPUHandle.gpuHandle = r2::draw::tex::UploadToGPU(imageData, dataSize, anisotropy, wrapMode, minFilter, magFilter);
		texGPUHandle.type = texture.type;
		texGPUHandle.anisotropy = anisotropy;
		texGPUHandle.minFilter = minFilter;
		texGPUHandle.magFilter = magFilter;
		texGPUHandle.wrapMode = wrapMode;

		r2::shashmap::Set(*s_optrTextureSystem->mTextureMap, texture.textureAssetHandle.handle, texGPUHandle);
	}

	void UploadToGPU(const r2::draw::tex::CubemapTexture& cubemap, float anisotropy, s32 wrapMode, s32 minFilter, s32 magFilter)
	{
		if (s_optrTextureSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the texture system yet!");
			return;
		}

		TextureGPUHandle theDefault;

		auto cubemapAssetHandle = GetCubemapAssetHandle(cubemap);

		//I guess we'll just take the first asset handle for our mapping? Dunno if that will be a problem down the road?
		TextureGPUHandle texGPUHandle = r2::shashmap::Get(*s_optrTextureSystem->mTextureMap, cubemapAssetHandle.handle, theDefault);

		if (!TextureHandlesEqual(texGPUHandle.gpuHandle, theDefault.gpuHandle))
			return;

		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		r2::draw::tex::TextureFormat textureFormat;
		{
			const void* imageData = gameAssetManager.GetAssetDataConst<void>(cubemap.mips[0].sides[0].textureAssetHandle);
			u64 dataSize = gameAssetManager.GetAssetDataSize(cubemap.mips[0].sides[0].textureAssetHandle);

			r2::asset::MemoryAssetFile memoryAssetFile{ imageData, dataSize };

			r2::assets::assetlib::load_binaryfile("", memoryAssetFile);

			const auto textureInfo = r2::assets::assetlib::read_texture_meta_data(memoryAssetFile);

			u32 format;
			u32 internalFormat;
			u32 imageFormatSize = r2::draw::UNSIGNED_BYTE; 
			tex::GetInternalTextureFormatDataForTextureFormat(textureInfo->textureFormat(), format, internalFormat, imageFormatSize);
			
			textureFormat.internalformat = internalFormat;
			textureFormat.width = textureInfo->mips()->Get(0)->width();
			textureFormat.height = textureInfo->mips()->Get(0)->height();
			textureFormat.mipLevels = cubemap.numMipLevels;
			textureFormat.isCubemap = true;
			textureFormat.wrapMode = wrapMode;
			textureFormat.magFilter = magFilter;
			textureFormat.minFilter = minFilter;
			textureFormat.isAnisotropic = !r2::math::NearZero(anisotropy);
			textureFormat.anisotropy = anisotropy;
		}
		
		texGPUHandle.gpuHandle = tex::CreateTexture(textureFormat, 1, true);
		texGPUHandle.type = tex::Cubemap;
		texGPUHandle.anisotropy = anisotropy;
		texGPUHandle.minFilter = minFilter;
		texGPUHandle.magFilter = magFilter;
		texGPUHandle.wrapMode = wrapMode;

		for (u32 mipLevel = 0; mipLevel < cubemap.numMipLevels; ++mipLevel)
		{
			for (u32 i = 0; i < r2::draw::tex::NUM_SIDES; ++i)
			{
				const void* imageData = gameAssetManager.GetAssetDataConst<void>(cubemap.mips[mipLevel].sides[i].textureAssetHandle);
				u64 dataSize = gameAssetManager.GetAssetDataSize(cubemap.mips[mipLevel].sides[i].textureAssetHandle);
				tex::UploadCubemapPageToGPU(texGPUHandle.gpuHandle, imageData, dataSize, mipLevel, i, anisotropy, wrapMode, minFilter, magFilter);
			}
		}

		r2::shashmap::Set(*s_optrTextureSystem->mTextureMap, cubemapAssetHandle.handle, texGPUHandle);
	}

	bool IsUploaded(const r2::asset::AssetHandle& texture)
	{
		if (s_optrTextureSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the texture system yet!");
			return false;
		}

		TextureGPUHandle theDefault;

		TextureGPUHandle texGPUHandle = r2::shashmap::Get(*s_optrTextureSystem->mTextureMap, texture.handle, theDefault);

		return !TextureHandlesEqual(texGPUHandle.gpuHandle, theDefault.gpuHandle);
	}

	void ReloadTexture(const r2::asset::AssetHandle& texture)
	{
		TextureGPUHandle theDefault;

		TextureGPUHandle texGPUHandle = r2::shashmap::Get(*s_optrTextureSystem->mTextureMap, texture.handle, theDefault);

		if (!TextureHandlesEqual(texGPUHandle.gpuHandle, theDefault.gpuHandle))
		{
			UnloadFromGPU(texture);
		}

		texsys::UploadToGPU({ texture, texGPUHandle.type }, texGPUHandle.anisotropy, texGPUHandle.wrapMode, texGPUHandle.minFilter, texGPUHandle.magFilter);
	}

	void UnloadFromGPU(const r2::asset::AssetHandle& texture)
	{
		if (s_optrTextureSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the texture system yet!");
			return;
		}

		TextureGPUHandle defaultGPUHandle;
		TextureGPUHandle texGPUHandle = r2::shashmap::Get(*s_optrTextureSystem->mTextureMap, texture.handle, defaultGPUHandle);

		if (!TextureHandlesEqual(texGPUHandle.gpuHandle, defaultGPUHandle.gpuHandle))
		{
			r2::draw::tex::UnloadFromGPU(texGPUHandle.gpuHandle);
			r2::shashmap::Remove(*s_optrTextureSystem->mTextureMap, texture.handle);

			R2_CHECK(!r2::shashmap::Has(*s_optrTextureSystem->mTextureMap, texture.handle), "remove don't work!");
		}
	}

	r2::draw::tex::TextureAddress GetTextureAddress(const r2::draw::tex::Texture& texture)
	{
		return GetTextureAddressInternal(texture.textureAssetHandle);
	}

	r2::draw::tex::TextureAddress GetTextureAddress(const r2::draw::tex::TextureHandle& textureHandle)
	{
		return r2::draw::tex::GetTextureAddress(textureHandle);
	}

	r2::draw::tex::TextureAddress GetTextureAddress(const r2::draw::tex::CubemapTexture& cubemap)
	{
		return GetTextureAddressInternal(GetCubemapAssetHandle(cubemap));
	}

	const r2::draw::tex::TextureHandle* GetTextureHandle(const r2::draw::tex::Texture& texture)
	{
		TextureGPUHandle defaultGPUHandle;
		const TextureGPUHandle& texGPUHandle = r2::shashmap::Get(*s_optrTextureSystem->mTextureMap, texture.textureAssetHandle.handle, defaultGPUHandle);

		if (!TextureHandlesEqual(texGPUHandle.gpuHandle, defaultGPUHandle.gpuHandle))
		{
			return &texGPUHandle.gpuHandle;
		}

		return nullptr;
	}

	r2::draw::tex::TextureAddress GetTextureAddressInternal(const r2::asset::AssetHandle& texture)
	{
		if (s_optrTextureSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the texture system yet!");
			return tex::TextureAddress{};
		}

		TextureGPUHandle defaultGPUHandle;
		TextureGPUHandle gpuHandle = r2::shashmap::Get(*s_optrTextureSystem->mTextureMap, texture.handle, defaultGPUHandle);

		if (TextureHandlesEqual(gpuHandle.gpuHandle, defaultGPUHandle.gpuHandle))
		{
		//	R2_CHECK(false, "Couldn't find the TextureAddress for texture: %llu", texture.handle);
			return tex::TextureAddress{};
		}

		return r2::draw::tex::GetTextureAddress(gpuHandle.gpuHandle);
	}

	u32 GetNumberOfMipMaps(const r2::draw::tex::CubemapTexture& cubemap)
	{
		TextureGPUHandle defaultGPUHandle;
		TextureGPUHandle texGPUHandle = r2::shashmap::Get(*s_optrTextureSystem->mTextureMap, GetCubemapAssetHandle(cubemap).handle, defaultGPUHandle);

		if (TextureHandlesEqual(texGPUHandle.gpuHandle, defaultGPUHandle.gpuHandle))
		{
			return 1;
		}

		return r2::draw::tex::impl::GetNumberOfMipMaps(texGPUHandle.gpuHandle);
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
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<TextureGPUHandle>::MemorySize(static_cast<u64>(maxNumTextures * LOAD_FACTOR)), ALIGNMENT, headerSize, boundsChecking);
	}

}