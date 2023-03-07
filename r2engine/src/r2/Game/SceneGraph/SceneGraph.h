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

			coordinator->RegisterComponent<ARENA, ecs::HeirarchyComponent>(arena);
			coordinator->RegisterComponent<ARENA, ecs::TransformComponent>(arena);
			coordinator->RegisterComponent<ARENA, ecs::TransformDirtyComponent>(arena);

			mnoptrSceneGraphSystem = coordinator->RegisterSystem<ARENA, ecs::SceneGraphSystem>(arena);

			if (mnoptrSceneGraphSystem == nullptr)
			{
				R2_CHECK(false, "Couldn't register the SceneGraphSystem");
				return false;
			}

			ecs::Signature systemSignature;

			const auto heirarchyComponentType = coordinator->GetComponentType<ecs::HeirarchyComponent>();
			const auto transformComponentType = coordinator->GetComponentType<ecs::TransformComponent>();
			systemSignature.set(heirarchyComponentType);
			systemSignature.set(transformComponentType);

			coordinator->SetSystemSignature<ecs::SceneGraphSystem>(systemSignature);



			mnoptrSceneGraphTransformUpdateSystem = coordinator->RegisterSystem<ARENA, ecs::SceneGraphTransformUpdateSystem>(arena);

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
			mnoptrECSCoordinator->UnRegisterSystem<ecs::SceneGraphSystem>(arena);
			mnoptrECSCoordinator->UnRegisterComponent<ARENA, ecs::TransformDirtyComponent>(arena);
			mnoptrECSCoordinator->UnRegisterComponent<ARENA, ecs::TransformComponent>(arena);
			mnoptrECSCoordinator->UnRegisterComponent<ARENA, ecs::HeirarchyComponent>(arena);
			mnoptrSceneGraphSystem = nullptr;
			mnoptrECSCoordinator = nullptr;
		}

		void Attach(ecs::Entity entity, ecs::Entity parent);
		void Detach(ecs::Entity entity);
		void DetachChildren(ecs::Entity parent);

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
