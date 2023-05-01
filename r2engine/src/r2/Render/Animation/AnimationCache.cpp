#include "r2pch.h"

#include "r2/Render/Animation/AnimationCache.h"
#include "r2/Render/Animation/Animation.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetLoaders/AssimpAnimationAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/RAnimationAssetLoader.h"

namespace r2::draw
{
	namespace animcache
	{
		const u64 ALIGNMENT = 16;

		AnimationCache* Create(r2::mem::MemoryArea::Handle memoryAreaHandle, u64 modelCacheSize, r2::asset::FileList files, const char* areaName)
		{
			if (!files)
			{
				R2_CHECK(false, "Trying to Init() a model system with no files!");
				return nullptr;
			}

			r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

			R2_CHECK(memoryArea != nullptr, "Memory area is null?");

			u64 subAreaSize = MemorySize(r2::sarr::Size(*files), modelCacheSize);
			if (memoryArea->UnAllocatedSpace() < subAreaSize)
			{
				R2_CHECK(false, "We don't have enought space to allocate the model system! We have: %llu bytes left but trying to allocate: %llu bytes",
					memoryArea->UnAllocatedSpace(), subAreaSize);
				return false;
			}

			r2::mem::MemoryArea::SubArea::Handle subAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

			if ((subAreaHandle = memoryArea->AddSubArea(subAreaSize, areaName)) == r2::mem::MemoryArea::SubArea::Invalid)
			{
				R2_CHECK(false, "We couldn't create a sub area for %s", areaName);
				return false;
			}

			//emplace the linear arena
			r2::mem::LinearArena* animationCacheArena = EMPLACE_LINEAR_ARENA(*memoryArea->GetSubArea(subAreaHandle));

			R2_CHECK(animationCacheArena != nullptr, "We couldn't emplace the linear arena - no way to recover!");

			AnimationCache* newAnimationCache = ALLOC(AnimationCache, *animationCacheArena);

			newAnimationCache->mMemoryAreaHandle = memoryAreaHandle;
			newAnimationCache->mSubAreaHandle = subAreaHandle;
			newAnimationCache->mSubAreaArena = animationCacheArena;
			newAnimationCache->mAnimationRecords = MAKE_SHASHMAP(*animationCacheArena, r2::asset::AssetCacheRecord, r2::sarr::Capacity(*files) * r2::SHashMap<r2::asset::AssetCacheRecord>::LoadFactorMultiplier());

			newAnimationCache->mAssetBoundary = MAKE_BOUNDARY(*animationCacheArena, modelCacheSize, ALIGNMENT);

			newAnimationCache->mAnimationCache = r2::asset::lib::CreateAssetCache(newAnimationCache->mAssetBoundary, files);
		
			r2::asset::AssimpAnimationLoader* animationLoader = (r2::asset::AssimpAnimationLoader*)newAnimationCache->mAnimationCache->MakeAssetLoader<r2::asset::AssimpAnimationLoader>();
			newAnimationCache->mAnimationCache->RegisterAssetLoader(animationLoader);

			r2::asset::RAnimationAssetLoader* ranimationLoader = (r2::asset::RAnimationAssetLoader*)newAnimationCache->mAnimationCache->MakeAssetLoader<r2::asset::RAnimationAssetLoader>();
			newAnimationCache->mAnimationCache->RegisterAssetLoader(ranimationLoader);

			return newAnimationCache;
		}

		void Shutdown(AnimationCache& system)
		{
			r2::mem::LinearArena* arena = system.mSubAreaArena;

			//return all of the models back to the cache

			r2::draw::animcache::FlushAll(system);

			r2::asset::lib::DestroyCache(system.mAnimationCache);
			FREE(system.mAssetBoundary.location, *arena);

			FREE(system.mAnimationRecords, *arena);

			FREE(&system, *arena);

			FREE_EMPLACED_ARENA(arena);
		}

		u64 MemorySize(u64 numAssets, u64 assetCapacityInBytes)
		{
			u32 boundsChecking = 0;
#ifdef R2_DEBUG
			boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
			u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

			return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(AnimationCache), ALIGNMENT, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), ALIGNMENT, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::asset::AssetCacheRecord>::MemorySize(numAssets * r2::SHashMap<r2::asset::AssetCacheRecord>::LoadFactorMultiplier()), ALIGNMENT, headerSize, boundsChecking) +
				r2::asset::AssetCache::TotalMemoryNeeded(headerSize, boundsChecking, numAssets, assetCapacityInBytes, ALIGNMENT);
		}

		AnimationHandle LoadAnimation(AnimationCache& system, const r2::asset::Asset& model)
		{
			return system.mAnimationCache->LoadAsset(model);
		}

		const Animation* GetAnimation(AnimationCache& system, const AnimationHandle& handle)
		{
			if (handle.assetCache != system.mAnimationCache->GetSlot())
			{
				//R2_CHECK(false, "Trying to get a model that doesn't exist in this model system");
				return nullptr;
			}

			r2::asset::AssetCacheRecord record;

			if (!r2::shashmap::Has(*system.mAnimationRecords, handle.handle))
			{
				record = system.mAnimationCache->GetAssetBuffer(handle);
				r2::shashmap::Set(*system.mAnimationRecords, handle.handle, record);
			}
			else
			{
				r2::asset::AssetCacheRecord defaultRecord;
				record = r2::shashmap::Get(*system.mAnimationRecords, handle.handle, defaultRecord);

				R2_CHECK(record.buffer != defaultRecord.buffer, "We couldn't get the record!");
				
			}

			r2::draw::Animation* animation = (r2::draw::Animation*)record.buffer->MutableData();

			animation->hashName = handle.handle;

			return animation;
		}

		void ReturnAnimation(AnimationCache& system, const Animation* animation)
		{
			if (!animation)
			{
				R2_CHECK(false, "Passed in a null animation");
				return;
			}

			r2::asset::AssetCacheRecord defaultRecord;

			r2::asset::AssetCacheRecord theRecord = r2::shashmap::Get(*system.mAnimationRecords, animation->hashName, defaultRecord);

			if (theRecord.buffer == defaultRecord.buffer)
			{
				R2_CHECK(false, "Failed to get the asset cache record!");
				return;
			}

			system.mAnimationCache->ReturnAssetBuffer(theRecord);
		}

		void LoadAnimations(AnimationCache& system, const r2::SArray<r2::asset::Asset>& assets, r2::SArray<AnimationHandle>& handles)
		{
			const u64 numAssetsToLoad = r2::sarr::Size(assets);

			for (u64 i = 0; i < numAssetsToLoad; ++i)
			{
				auto animationHandle = system.mAnimationCache->LoadAsset(r2::sarr::At(assets, i));
				r2::sarr::Push(handles, animationHandle);
			}
		}

		void GetAnimations(AnimationCache& system, const SArray<AnimationHandle>& handles, r2::SArray<const Animation*>& outAnimations)
		{
			const u64 numAssetHandles = r2::sarr::Size(handles);

			for (u64 i = 0; i < numAssetHandles; ++i)
			{
				r2::sarr::Push(outAnimations, GetAnimation(system, r2::sarr::At(handles, i)));
			}
		}

		void ReturnAnimations(AnimationCache& system, r2::SArray<const Animation*>& animations)
		{
			const u64 numAnimations = r2::sarr::Size(animations);

			for (u64 i = 0; i < numAnimations; ++i)
			{
				ReturnAnimation(system, r2::sarr::At(animations, i));
			}

			r2::sarr::Clear(animations);
		}

		void FlushAll(AnimationCache& system)
		{
			auto beginHash = r2::shashmap::Begin(*system.mAnimationRecords);

			auto iter = beginHash;
			for (; iter != r2::shashmap::End(*system.mAnimationRecords); ++iter)
			{
				system.mAnimationCache->ReturnAssetBuffer(iter->value);
			}
			
			r2::shashmap::Clear(*system.mAnimationRecords);

			system.mAnimationCache->FlushAll();
		}

		const r2::asset::FileList GetFileList(const AnimationCache& animationCache)
		{
			return animationCache.mAnimationCache->GetFileList();
		}

		void ClearFileList(AnimationCache& animationCache)
		{
			animationCache.mAnimationCache->ClearFileList();
		}
	}
}