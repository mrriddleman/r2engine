#ifndef __SCENE_GRAPH_H__
#define __SCENE_GRAPH_H__

#include "r2/Game/ECS/Components/HeirarchyComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Systems/SceneGraphSystem.h"


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

			mnoptrSceneGraphSystem = coordinator->RegisterSystem<ARENA, ecs::SceneGraphSystem>(arena);

			if (mnoptrSceneGraphSystem == nullptr)
			{
				R2_CHECK(false, "Couldn't register the SceneGraphSystem");
				return false;
			}

			ecs::Signature systemSignature;

			systemSignature.set(coordinator->GetComponentType<ecs::HeirarchyComponent>());
			systemSignature.set(coordinator->GetComponentType<ecs::TransformComponent>());

			coordinator->SetSystemSignature<ecs::SceneGraphSystem>(systemSignature);

			return true;
		}

		template <class ARENA>
		void Shutdown(ARENA& arena)
		{
			mnoptrECSCoordinator->UnRegisterSystem<ecs::SceneGraphSystem>(arena);

			mnoptrSceneGraphSystem = nullptr;
			mnoptrECSCoordinator = nullptr;
		}
	private:
		ecs::ECSCoordinator* mnoptrECSCoordinator;
		ecs::SceneGraphSystem* mnoptrSceneGraphSystem;
	};
}

#endif // __SCENE_GRAPH_H__
