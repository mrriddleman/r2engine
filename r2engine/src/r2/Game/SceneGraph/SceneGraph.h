#ifndef __SCENE_GRAPH_H__
#define __SCENE_GRAPH_H__

#include "r2/Game/ECS/Entity.h"

namespace flat
{
	struct LevelData;
}

namespace r2
{
	class Level;
}

namespace r2::ecs
{
	class SceneGraphSystem;
	class SceneGraphTransformUpdateSystem;
	class ECSCoordinator;
	class ECSWorld;

	class SceneGraph
	{
	public:
		SceneGraph();
		~SceneGraph();

		
		bool Init(ecs::SceneGraphSystem* sceneGraphSystem, ecs::SceneGraphTransformUpdateSystem* sceneGraphtransformUpdateSystem, ecs::ECSCoordinator* coordinator);
		void Shutdown();

		void Update();

		ecs::Entity CreateEntity();
		ecs::Entity CreateEntity(ecs::Entity parent);
		void DestroyEntity(ecs::Entity entity);

		void LoadLevel(ECSWorld& ecsWorld, const Level& level, const flat::LevelData* levelData);
		void UnloadLevel(const Level& level);

		void Attach(ecs::Entity entity, ecs::Entity parent);
		void Detach(ecs::Entity entity);
		void DetachChildren(ecs::Entity parent);

		void GetAllChildrenForEntity(ecs::Entity parent, r2::SArray<ecs::Entity>& children);
		void GetAllEntitiesInSubTree(ecs::Entity parent, u32 parentIndex, r2::SArray<ecs::Entity>& entities);
		void GetAllTopLevelEntities(r2::SArray<ecs::Entity>& entities, r2::SArray<u32>& indices);
		void GetAllEntitiesInScene(r2::SArray<ecs::Entity>& entities);
		s32 GetEntityIndex(ecs::Entity entity);
		ecs::Entity GetParent(ecs::Entity entity);

		void UpdateTransformForEntity(ecs::Entity entity, ecs::eTransformDirtyFlags dirtyFlags);

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
