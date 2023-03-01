#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SQueue.h"

namespace r2::ecs
{
	using Entity = u32;
	static const Entity INVALID_ENTITY = 0;
	static const u32 MAX_NUM_ENTITIES = 10000; //I dunno yet

	class EntityManager
	{
	public:
		EntityManager();
		~EntityManager();

		template<class ARENA>
		bool Init(ARENA& arena, u32 maxNumEntities, Entity startingEntity)
		{
			mAvailbleEntities = MAKE_SQUEUE(arena, Entity, maxNumEntities);

			if (!mAvailbleEntities)
			{
				R2_CHECK(false, "We couldn't allocate the mAvailableEntities!");
				return false;
			}

			for (Entity i = startingEntity; i < maxNumEntities; ++i)
			{
				r2::squeue::PushBack(*mAvailbleEntities, i);
			}

			return true;
		}

		template<typename ARENA>
		void Shutdown(ARENA& arena)
		{
			FREE(mAvailbleEntities, arena);
			mAvailbleEntities = nullptr;
		}

		Entity CreateEntity();
		void DestroyEntity(Entity entity);

		u32 NumLivingEntities() const;

		static u64 MemorySize(u32 maxEntities, u32 alignment, u32 headerSize, u32 boundsChecking);
		static bool IsValidEntity(const Entity& entity);
	private:
		r2::SQueue<Entity>* mAvailbleEntities;
	};
}

#endif