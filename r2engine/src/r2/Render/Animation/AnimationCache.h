#ifndef __ANIMATION_SYSTEM_H__
#define __ANIMATION_SYSTEM_H__

#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"

namespace r2::draw
{
	struct Animation;
	struct AnimationCache
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;
		r2::asset::AssetCache* mAnimationCache = nullptr;
		r2::SHashMap<r2::asset::AssetCacheRecord>* mAnimationRecords = nullptr;
		r2::mem::utils::MemBoundary mAssetBoundary;
	};

	using AnimationHandle = r2::asset::AssetHandle;

	namespace animcache
	{
		AnimationCache* Init(r2::mem::MemoryArea::Handle memoryAreaHandle, u64 modelCacheSize, r2::asset::FileList files, const char* areaName);
		void Shutdown(AnimationCache& system);
		u64 MemorySize(u64 numAssets, u64 assetCapacityInBytes);

		AnimationHandle LoadAnimation(AnimationCache& system, const r2::asset::Asset& model);
		const Animation* GetAnimation(AnimationCache& system, const AnimationHandle& handle);
		void ReturnAnimation(AnimationCache& system, const Animation* animation);
		void FreeAnimation(AnimationCache& system, const AnimationHandle& handle);

		void LoadAnimations(AnimationCache& system, const r2::SArray<r2::asset::Asset>& assets, r2::SArray<AnimationHandle>& handles);
		void GetAnimations(AnimationCache& system, const SArray<AnimationHandle>& handles, r2::SArray<const Animation*>& outAnimations);
		void ReturnAnimations(AnimationCache& system, r2::SArray<const Animation*>& animations);
		void FreeAnimations(AnimationCache& system, const r2::SArray<AnimationHandle>& handles);
		
		void FlushAll(AnimationCache& system);
	}
}

#endif // __ANIMATION_SYSTEM_H__
