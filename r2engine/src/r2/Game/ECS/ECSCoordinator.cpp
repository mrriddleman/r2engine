#include "r2pch.h"

#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/Level/Level.h"
#include "r2/Game/Level/LevelData_generated.h"

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

	const r2::SArray<Entity>& ECSCoordinator::GetAllLivingEntities()
	{
		return mEntityManager->GetCreatedEntities();
	}

	u32 ECSCoordinator::NumLivingEntities() const
	{
		return mEntityManager->NumLivingEntities();
	}

	void ECSCoordinator::LoadAllECSDataFromLevel(const Level& level)
	{
		const flat::LevelData* flatLevelData = level.GetLevelData();

		//@TODO(Serge): Not sure if we want to destroy all entities - maybe not, for now we are
		DestoryAllEntities();

		u32 numLiveEntities = flatLevelData->numEntities();

		const auto& levelEntities = flatLevelData->entities();

		R2_CHECK(numLiveEntities == levelEntities->size(), "These should be the same");

		r2::SArray<Entity>* newlyCreatedEntities = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, Entity, numLiveEntities);
		r2::SArray<const flat::EntityData*>* flatEntities = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::EntityData*, numLiveEntities);
		r2::SArray<Signature>* entitySignatures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, Signature, numLiveEntities);

		for (flatbuffers::uoffset_t i = 0; i < numLiveEntities; ++i)
		{
			//not sure how this should work - currently the entities might not match up perfectly (in terms of their values)
			//but they should match in the sense that the entities will effectively be the same
			Entity nextECSEntity = CreateEntity();
			const flat::EntityData* nextFlatEntity = levelEntities->Get(i);

			r2::sarr::Push(*newlyCreatedEntities, nextECSEntity);
			r2::sarr::Push(*flatEntities, nextFlatEntity);	
			r2::sarr::Push(*entitySignatures, {});
		}

		mComponentManager->DeSerialize(newlyCreatedEntities, flatEntities, entitySignatures, flatLevelData->componentArrays());

		//now set the entity signatures
		for (u32 i = 0; i < numLiveEntities; ++i)
		{
			mEntityManager->SetSignature(r2::sarr::At(*newlyCreatedEntities, i), r2::sarr::At(*entitySignatures, i));
		}

		mSystemManager->DeSerializeEntitySignatures(newlyCreatedEntities, entitySignatures);

		FREE(entitySignatures, *MEM_ENG_SCRATCH_PTR);
		FREE(flatEntities, *MEM_ENG_SCRATCH_PTR);
		FREE(newlyCreatedEntities, *MEM_ENG_SCRATCH_PTR);
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