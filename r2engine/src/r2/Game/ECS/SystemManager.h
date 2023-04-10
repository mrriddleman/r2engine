#ifndef __SYSTEM_MANAGER_H__
#define __SYSTEM_MANAGER_H__

#include "r2/Core/Containers/SHashMap.h"
#include "r2/Game/ECS/Entity.h"
#include "r2/Game/ECS/Component.h"
#include <typeindex>

namespace r2::ecs
{
	struct System;

	class SystemManager
	{
	public:
		SystemManager();
		~SystemManager();

		template<class ARENA>
		bool Init(ARENA& arena, u32 maxNumSystems)
		{
			mSystems = MAKE_SHASHMAP(arena, System*, maxNumSystems * r2::SHashMap<System*>::LoadFactorMultiplier());

			if (!mSystems)
			{
				R2_CHECK(false, "Failed to create mSystems");
				return false;
			}

			mSignatures = MAKE_SHASHMAP(arena, Signature, maxNumSystems * r2::SHashMap<System*>::LoadFactorMultiplier());

			if (!mSignatures)
			{
				R2_CHECK(false, "Failed to create mSignatures");
				return false;
			}

			return true;
		}

		template<class ARENA>
		void Shutdown(ARENA& arena)
		{
			R2_CHECK(r2::sarr::IsEmpty(*mSystems->mData), "This should be empty by the time we get here: did you forget to unregister all of the systems?");

			r2::shashmap::Clear(*mSystems);
			r2::shashmap::Clear(*mSignatures);

			FREE(mSignatures, arena);
			FREE(mSystems, arena);

			mSignatures = nullptr;
			mSystems = nullptr;
		}

		template<class ARENA, typename SystemType>
		SystemType* RegisterSystem(ARENA& arena)
		{
			R2_CHECK(mSystems != nullptr, "We haven't intialized the SystemManager yet!");
			R2_CHECK(mSignatures != nullptr, "We haven't initialized the SystemManager yet!");
			auto systemTypeHash = std::type_index(typeid(SystemType)).hash_code();

			System* nullSystem = nullptr;
			System* aSystem = r2::shashmap::Get(*mSystems, systemTypeHash, nullSystem);
			
			if (aSystem != nullSystem)
			{
				//we already have it so just return it
				return static_cast<SystemType*>(aSystem);
			}

			SystemType* system = ALLOC(SystemType, arena);

			system->mEntities = MAKE_SARRAY(arena, Entity, MAX_NUM_ENTITIES);

			r2::shashmap::Set(*mSystems, systemTypeHash, (System*)system);

			r2::shashmap::Set(*mSignatures, systemTypeHash, Signature{});

			return system;
		}

		template<class ARENA, typename SystemType>
		void UnRegisterSystem(ARENA& arena)
		{
			auto systemTypeHash = std::type_index(typeid(SystemType)).hash_code();

			System* nullSystem = nullptr;

			System* system = r2::shashmap::Get(*mSystems, systemTypeHash, nullSystem);

			if (system == nullSystem)
			{
				return;
			}

			r2::shashmap::Remove(*mSystems, systemTypeHash);
			r2::shashmap::Remove(*mSignatures, systemTypeHash);

			FREE(system->mEntities, arena);

			FREE(system, arena);
		}

		template<typename SystemType>
		void SetSignature(Signature signature)
		{
			auto systemTypeHash = std::type_index(typeid(SystemType)).hash_code();
			R2_CHECK(r2::shashmap::Has(*mSystems, systemTypeHash), "We've already registered that system!");
			R2_CHECK(r2::shashmap::Has(*mSignatures, systemTypeHash), "We should have a signature already - even if empty");

			r2::shashmap::Set(*mSignatures, systemTypeHash, signature);
		}

		template<typename SystemType>
		SystemType* GetSystem()
		{
			auto systemTypeHash = std::type_index(typeid(SystemType)).hash_code();

			System* nullSystem = nullptr;

			return r2::shashmap::Get(*mSystems, systemTypeHash, nullSystem);
		}

		void EntityDestroyed(Entity entity);
		void EntitySignatureChanged(Entity entity, Signature entitySignature);

		template<typename SystemType>
		void MoveEntity(u64 fromIndex, u64 toIndex)
		{
			auto systemTypeHash = std::type_index(typeid(SystemType)).hash_code();
			R2_CHECK(r2::shashmap::Has(*mSystems, systemTypeHash), "We've already registered that system!");

			System* nullSystem = nullptr;
			System* system = r2::shashmap::Get(*mSystems, systemTypeHash, nullSystem);
			R2_CHECK(system != nullSystem, "We don't have that system?");

			const auto numEntities = r2::sarr::Size(*system->mEntities);
			R2_CHECK(fromIndex < numEntities, "fromIndex is invalid");
			R2_CHECK(toIndex < numEntities, "toIndex is invalid");
			if (fromIndex == toIndex)
			{
				return; //nothing to do
			}

			Entity e = r2::sarr::At(*system->mEntities, fromIndex);

			const int direction = fromIndex < toIndex ? 1 : -1;

			for (u64 i = fromIndex; i != toIndex; i += direction)
			{
				const auto next = i + direction;
				r2::sarr::At(*system->mEntities, i) = r2::sarr::At(*system->mEntities, next);
			}

			r2::sarr::At(*system->mEntities, toIndex) = e;
		}

		void DestoryAllEntities();

		static u64 MemorySize(u32 maxNumSystems, u32 maxNumEntities, u32 alignment, u32 headerSize, u32 boundsChecking);

	private:

		void RemoveEntityKeepSorted(const System* system, Entity entity, s64 index);

		r2::SHashMap<System*>* mSystems;
		r2::SHashMap<Signature>* mSignatures;
	};
}

#endif // __SYSTEM_MANAGER_H__
