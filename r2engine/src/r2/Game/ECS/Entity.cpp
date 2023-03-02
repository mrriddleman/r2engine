#include "r2pch.h"

#include "r2/Game/ECS/Entity.h"

namespace r2::ecs
{
	EntityManager::EntityManager()
		: mAvailbleEntities(nullptr)
		, mEntitySignatures(nullptr)
	{
	}

	EntityManager::~EntityManager()
	{
		R2_CHECK(mAvailbleEntities == nullptr, "This should have been freed already!");
	}

	Entity EntityManager::CreateEntity()
	{
		if (!mAvailbleEntities)
		{
			R2_CHECK(false, "We haven't initialized the EntityManager!");
			return INVALID_ENTITY;
		}

		if (r2::squeue::Size(*mAvailbleEntities) == 0)
		{
			R2_CHECK(false, "We ran out of entities!");
			return INVALID_ENTITY;
		}

		Entity e = r2::squeue::First(*mAvailbleEntities);

		r2::squeue::PopFront(*mAvailbleEntities);

		return e;

	}

	void EntityManager::DestroyEntity(Entity entity)
	{
		if (!mAvailbleEntities)
		{
			R2_CHECK(false, "We haven't initialized the EntityManager!");
			return;
		}

		if (r2::squeue::Space(*mAvailbleEntities) == 0)
		{
			R2_CHECK(false, "We're full?");
			return;
		}

		r2::squeue::PushFront(*mAvailbleEntities, entity);

	}



	u32 EntityManager::NumLivingEntities() const
	{
		if (!mAvailbleEntities)
		{
			R2_CHECK(false, "We haven't initialized the EntityManager!");
			return 0;
		}

		return static_cast<u32>(r2::squeue::Space(*mAvailbleEntities));
	}

	void EntityManager::SetSignature(Entity entity, Signature signature)
	{
		r2::sarr::At(*mEntitySignatures, entity) = signature;
	}

	Signature& EntityManager::GetSignature(Entity entity)
	{
		return r2::sarr::At(*mEntitySignatures, entity);
	}

	u64 EntityManager::MemorySize(u32 maxEntities, u32 alignment, u32 headerSize, u32 boundsChecking)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(EntityManager), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SQueue<Entity>::MemorySize(maxEntities), alignment, headerSize, boundsChecking);
	}

	bool EntityManager::IsValidEntity(const Entity& entity)
	{
		return entity != INVALID_ENTITY;
	}
}