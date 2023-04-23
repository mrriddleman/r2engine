#include "r2pch.h"

#include "r2/Game/ECS/Entity.h"


namespace r2::ecs
{
	EntityManager::EntityManager()
		: mAvailbleEntities(nullptr)
		, mCreatedEntities(nullptr)
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

		r2::sarr::Push(*mCreatedEntities, e);

		return e;
	}

	void EntityManager::DestroyEntity(Entity entity)
	{
		if (!mAvailbleEntities || !mCreatedEntities)
		{
			R2_CHECK(false, "We haven't initialized the EntityManager!");
			return;
		}

		if (r2::squeue::Space(*mAvailbleEntities) == 0)
		{
			R2_CHECK(false, "We're full?");
			return;
		}


		s64 index = r2::sarr::IndexOf(*mCreatedEntities, entity);

		R2_CHECK(index != -1, "Shouldn't ever happen");

		r2::sarr::At(*mEntitySignatures, entity) = {};

		r2::sarr::RemoveAndSwapWithLastElement(*mCreatedEntities, index);

		r2::squeue::PushFront(*mAvailbleEntities, entity);
	}

	void EntityManager::DestoryAllEntities()
	{
		r2::sarr::Clear(*mEntitySignatures);
		r2::sarr::Fill(*mEntitySignatures, Signature{});

		for (u32 i = 0; i < r2::sarr::Size(*mCreatedEntities); ++i)
		{
			r2::squeue::PushFront(*mAvailbleEntities, r2::sarr::At(*mCreatedEntities, i));
		}

		r2::sarr::Clear(*mCreatedEntities);
	}

	u32 EntityManager::NumLivingEntities() const
	{
		if (!mCreatedEntities)
		{
			R2_CHECK(false, "We haven't initialized the EntityManager!");
			return 0;
		}

		return r2::sarr::Size(*mCreatedEntities);
	}

	void EntityManager::SetSignature(Entity entity, Signature signature)
	{
		r2::sarr::At(*mEntitySignatures, entity) = signature;
	}

	Signature& EntityManager::GetSignature(Entity entity)
	{
		return r2::sarr::At(*mEntitySignatures, entity);
	}

	const r2::SArray<Entity>& EntityManager::GetCreatedEntities() const
	{
		return *mCreatedEntities;
	}

	void EntityManager::Serialize(flatbuffers::FlatBufferBuilder& builder, std::vector<flatbuffers::Offset<flat::EntityData>>& entityVec) const
	{
		const auto numCreatedEntities = r2::sarr::Size(*mCreatedEntities);

		for (u32 i = 0; i <numCreatedEntities; ++i)
		{
			Entity e = r2::sarr::At(*mCreatedEntities, i);
			auto entityData = flat::CreateEntityData(builder, e);
			entityVec.push_back(entityData);
		}
	}

	u64 EntityManager::MemorySize(u32 maxEntities, u32 alignment, u32 headerSize, u32 boundsChecking)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(EntityManager), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Entity>::MemorySize(maxEntities), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Signature>::MemorySize(maxEntities), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SQueue<Entity>::MemorySize(maxEntities), alignment, headerSize, boundsChecking);
	}

	bool EntityManager::IsValidEntity(const Entity& entity)
	{
		return entity != INVALID_ENTITY;
	}
}