#ifndef __SCENE_GRAPH_H__
#define __SCENE_GRAPH_H__

#include "r2/Game/ECS/Components/HeirarchyComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Systems/SceneGraphSystem.h"
#include "r2/Game/ECS/Systems/SceneGraphTransformUpdateSystem.h"


namespace r2
{
	class Level;

	class SceneGraph
	{
	public:
		SceneGraph();
		~SceneGraph();

		template <class ARENA>
		bool Init(ARENA& arena, ecs::ECSCoordinator* coordinator)
		{
			if (coordinator == nullptr)
			{
				R2_CHECK(false, "Passed in a null coordinator");
				return false;
			}

			mnoptrECSCoordinator = coordinator;

			mnoptrSceneGraphSystem = (ecs::SceneGraphSystem*)coordinator->RegisterSystem<ARENA, ecs::SceneGraphSystem>(arena);
			
			if (mnoptrSceneGraphSystem == nullptr)
			{
				R2_CHECK(false, "Couldn't register the SceneGraphSystem");
				return false;
			}

			mnoptrSceneGraphSystem->SetSceneGraph(this);

			ecs::Signature systemSignature;

			const auto heirarchyComponentType = coordinator->GetComponentType<ecs::HeirarchyComponent>();
			const auto transformComponentType = coordinator->GetComponentType<ecs::TransformComponent>();
			systemSignature.set(heirarchyComponentType);
			systemSignature.set(transformComponentType);

			coordinator->SetSystemSignature<ecs::SceneGraphSystem>(systemSignature);

			mnoptrSceneGraphTransformUpdateSystem = (ecs::SceneGraphTransformUpdateSystem*)coordinator->RegisterSystem<ARENA, ecs::SceneGraphTransformUpdateSystem>(arena);

			if (mnoptrSceneGraphTransformUpdateSystem == nullptr)
			{
				R2_CHECK(false, "Couldn't register the SceneGraphTransformUpdateSystem");
				return false;
			}

			ecs::Signature systemUpdateSignature;
			systemUpdateSignature.set(heirarchyComponentType);
			systemUpdateSignature.set(transformComponentType);
			systemUpdateSignature.set(coordinator->GetComponentType<ecs::TransformDirtyComponent>());

			coordinator->SetSystemSignature<ecs::SceneGraphTransformUpdateSystem>(systemUpdateSignature);

			return true;
		}

		template <class ARENA>
		void Shutdown(ARENA& arena)
		{
			mnoptrECSCoordinator->UnRegisterSystem<ARENA, ecs::SceneGraphTransformUpdateSystem>(arena);
			mnoptrECSCoordinator->UnRegisterSystem<ARENA, ecs::SceneGraphSystem>(arena);
			mnoptrSceneGraphSystem = nullptr;
			mnoptrECSCoordinator = nullptr;
		}

		void Update();

		ecs::Entity CreateEntity();
		ecs::Entity CreateEntity(ecs::Entity parent);
		void DestroyEntity(ecs::Entity entity);

		void LoadedNewLevel(const Level& level);

		void Attach(ecs::Entity entity, ecs::Entity parent);
		void Detach(ecs::Entity entity);
		void DetachChildren(ecs::Entity parent);

		void GetAllChildrenForEntity(ecs::Entity parent, r2::SArray<ecs::Entity>& children);
		void GetAllEntitiesInSubTree(ecs::Entity parent, u32 parentIndex, r2::SArray<ecs::Entity>& entities);
		void GetAllTopLevelEntities(r2::SArray<ecs::Entity>& entities, r2::SArray<u32>& indices);
		void GetAllEntitiesInScene(r2::SArray<ecs::Entity>& entities);
		s32 GetEntityIndex(ecs::Entity entity);
		ecs::Entity GetParent(ecs::Entity entity);

		ecs::ECSCoordinator* GetECSCoordinator() const;

	private:

		struct UpdatedEntity
		{
			ecs::Entity e = ecs::INVALID_ENTITY;
			s32 index = -1;
		};

		void SetDirtyFlagOnHeirarchy(ecs::Entity entity, u32 startingIndex);

		ecs::ECSCoordinator* mnoptrECSCoordinator;
		ecs::SceneGraphSystem* mnoptrSceneGraphSystem;
		ecs::SceneGraphTransformUpdateSystem* mnoptrSceneGraphTransformUpdateSystem;
	};
}

#endif // __SCENE_GRAPH_H__
