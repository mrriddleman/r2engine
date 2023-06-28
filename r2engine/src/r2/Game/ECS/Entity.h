#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SQueue.h"
#include "r2/Game/ECS/Component.h"
#include "r2/Game/ECS/EntityData_generated.h"

namespace r2::ecs
{
	using Entity = u64;
	
	static const Entity INVALID_ENTITY = 0;
	static const u32 MAX_NUM_ENTITIES = 5000; //I dunno yet

	class EntityManager
	{
	public:
		EntityManager();
		~EntityManager();

		template<class ARENA>
		bool Init(ARENA& arena, u32 maxNumEntities, Entity startingEntity)
		{
			R2_CHECK(startingEntity > 0, "We can't use 0 as our starting entity since 0 means INVALID_ENTITY currently");

			mAvailbleEntities = MAKE_SQUEUE(arena, Entity, maxNumEntities);

			if (!mAvailbleEntities)
			{
				R2_CHECK(false, "We couldn't allocate the mAvailableEntities!");
				return false;
			}

			for (Entity i = startingEntity; i < (maxNumEntities + 1); ++i) //@NOTE(Serge): adding one here to offset the fact that 0 isn't possible
			{
				r2::squeue::PushBack(*mAvailbleEntities, i);
			}

			mCreatedEntities = MAKE_SARRAY(arena, Entity, maxNumEntities);

			if (!mCreatedEntities)
			{
				FREE(mAvailbleEntities, arena);
				R2_CHECK(false, "We couldn't allocate the created entities");
				return false;
			}

			mEntitySignatures = MAKE_SARRAY(arena, Signature, maxNumEntities);

			if ( !mEntitySignatures)
			{
				FREE(mAvailbleEntities, arena);
				FREE(mAvailbleEntities, arena);
				R2_CHECK(false, "We couldn't allocate the mEntitySignatures!");
				return false;
			}

			r2::sarr::Fill(*mEntitySignatures, Signature{});

			return true;
		}

		template<typename ARENA>
		void Shutdown(ARENA& arena)
		{
			FREE(mEntitySignatures, arena);
			mEntitySignatures = nullptr;

			FREE(mCreatedEntities, arena);
			mCreatedEntities = nullptr;

			FREE(mAvailbleEntities, arena);
			mAvailbleEntities = nullptr;
		}

		Entity CreateEntity();
		void DestroyEntity(Entity entity);
		void DestoryAllEntities();

		u32 NumLivingEntities() const;
		
		void SetSignature(Entity entity, Signature signature);
		Signature& GetSignature(Entity entity);

		const r2::SArray<Entity>& GetCreatedEntities() const;

		void Serialize(flatbuffers::FlatBufferBuilder& builder, std::vector<flatbuffers::Offset<flat::EntityData>>& entityVec) const;

		static u64 MemorySize(u32 maxEntities, u32 alignment, u32 headerSize, u32 boundsChecking);
		static bool IsValidEntity(const Entity& entity);


	private:
		r2::SQueue<Entity>* mAvailbleEntities;
		r2::SArray<Entity>* mCreatedEntities;
		r2::SArray<Signature>* mEntitySignatures;
	};
}

#endif