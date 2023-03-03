#include "r2pch.h"

#include "r2/Game/ECS/SystemManager.h"
#include "r2/Game/ECS/System.h"

namespace r2::ecs
{
	SystemManager::SystemManager()
		: mSystems(nullptr)
		, mSignatures(nullptr)
	{
	}
	SystemManager::~SystemManager()
	{
		R2_CHECK(mSystems == nullptr, "Should be freed already");
		R2_CHECK(mSignatures == nullptr, "Should be freed already");
	}

	void SystemManager::EntityDestroyed(Entity entity)
	{
		auto iter = r2::shashmap::Begin(*mSystems);

		for (; iter != r2::shashmap::End(*mSystems); ++iter)
		{
			const auto& system = iter->value;

			r2::SArray<Entity>* entities = iter->value->mEntities;

			s64 index = r2::sarr::IndexOf(*entities, entity);
			
			R2_CHECK(index != -1, "Index is -1");
			
			if (!system->mKeepSorted)
			{
				r2::sarr::RemoveAndSwapWithLastElement(*entities, index);
			}
			else
			{
				RemoveEntityKeepSorted(system, entity, index);
			}
		}
	}

	void SystemManager::EntitySignatureChanged(Entity entity, Signature entitySignature)
	{
		auto iter = r2::shashmap::Begin(*mSystems);

		for (; iter != r2::shashmap::End(*mSystems); ++iter)
		{
			const auto& type = iter->key;
			const auto& system = iter->value;

			Signature emptySignature = {};
			const Signature & systemSignature = r2::shashmap::Get(*mSignatures, type, emptySignature);

			if ((entitySignature & systemSignature) == systemSignature)
			{
				r2::sarr::Push(*system->mEntities, entity);
			}
			else
			{
				s64 index = r2::sarr::IndexOf(*system->mEntities, entity);
				
				R2_CHECK(index != -1, "Index is -1");
				
				if (!system->mKeepSorted)
				{
					r2::sarr::RemoveAndSwapWithLastElement(*system->mEntities, index);
				}
				else
				{
					RemoveEntityKeepSorted(system, entity, index);
				}
			}
		}
	}

	u64 SystemManager::MemorySize(u32 maxNumSystems, u32 maxNumEntities, u32 alignment, u32 headerSize, u32 boundsChecking)
	{
		u64 memorySize = 0;

		memorySize +=
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(SystemManager), alignment, headerSize, boundsChecking) + //maybe not needed?
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<System*>::MemorySize(maxNumSystems), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<Signature>::MemorySize(maxNumSystems), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Entity>::MemorySize(maxNumEntities), alignment, headerSize, boundsChecking) * maxNumSystems
			;

		return memorySize;
	}

	void SystemManager::RemoveEntityKeepSorted(const System* system, Entity entity, s64 index)
	{
		r2::sarr::RemoveElementAtIndexShiftLeft(*system->mEntities, index);
	}
}