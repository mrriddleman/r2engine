#include "r2pch.h"

#include "r2/Game/ECS/ECSCoordinator.h"

namespace r2::ecs
{

	ECSCoordinator::ECSCoordinator()
		:mEntityManager(nullptr)
		,mComponentManager(nullptr)
		,mSystemManager(nullptr)
	{
	}

	ECSCoordinator::~ECSCoordinator()
	{
		R2_CHECK(mEntityManager == nullptr, "Should be nullptr!");
		R2_CHECK(mComponentManager == nullptr, "Should be nullptr!");
		R2_CHECK(mSystemManager == nullptr, "Should be nullptr!");
	}

	Entity ECSCoordinator::CreateEntity()
	{
		return mEntityManager->CreateEntity();
	}

	void ECSCoordinator::DestroyEntity(Entity entity)
	{
		mEntityManager->DestroyEntity(entity);
		mComponentManager->EntityDestroyed(entity);
		mSystemManager->EntityDestroyed(entity);
	}

	void ECSCoordinator::DestoryAllEntities()
	{
		mEntityManager->DestoryAllEntities();
		mComponentManager->DestroyAllEntities();
		mSystemManager->DestoryAllEntities();
	}

	u32 ECSCoordinator::NumLivingEntities() const
	{
		return mEntityManager->NumLivingEntities();
	}

	void ECSCoordinator::LoadAllECSDataFromLevel(Level& level)
	{
		//@TODO(Serge):
		R2_CHECK(false, "Not implemented yet!");
	}

	void ECSCoordinator::UnloadAllECSDataFromLevel(Level& level)
	{
		//@TODO(Serge):
		R2_CHECK(false, "Not implemented yet!");
	}

	void ECSCoordinator::SerializeECS (
		flatbuffers::FlatBufferBuilder& builder,
		std::vector<flatbuffers::Offset<flat::EntityData>>& entityVec,
		std::vector<flatbuffers::Offset<flat::ComponentArrayData>>& componentData) const
	{
		mEntityManager->Serialize(builder, entityVec);
		mComponentManager->Serialize(builder, componentData);
	}

	u64 ECSCoordinator::MemorySize(u32 maxNumComponents, u32 maxNumEntities, u32 maxNumSystems, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		u64 memorySize = 0;

		memorySize +=
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(ECSCoordinator), alignment, headerSize, boundsChecking) +
			EntityManager::MemorySize(maxNumEntities, alignment, headerSize, boundsChecking) +
			ComponentManager::MemorySize(maxNumComponents, alignment, headerSize, boundsChecking) +
			SystemManager::MemorySize(maxNumSystems, maxNumEntities, alignment, headerSize, boundsChecking);

		return memorySize;
	}
}