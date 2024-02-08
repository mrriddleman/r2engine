#ifndef __SCENE_GRAPH_TRANSFORM_UPDATE_SYSTEM_H__
#define __SCENE_GRAPH_TRANSFORM_UPDATE_SYSTEM_H__

#include "r2/Game/ECS/System.h"

namespace r2::math
{
	struct Transform;
}

namespace r2::ecs
{

	struct HierarchyComponent;
	struct TransformComponent;
	struct TransformDirtyComponent;

	//The purpose of this system is to update the modelMatrix of each entity if the entity needs it 
	//ie. if there is a TransformDirtyComponent attached to the entity
	//this way, we only update modelMatrices when needed instead of every frame
	class SceneGraphTransformUpdateSystem : public System
	{
	public:

		SceneGraphTransformUpdateSystem();
		~SceneGraphTransformUpdateSystem();
		void Update();

	private:

		void UpdateEntityTransformComponent(const r2::math::Transform& parentTransform, const HierarchyComponent& entityHeirarchComponent, const TransformDirtyComponent& entityTransformDirtyComponent, TransformComponent& entityTransformComponent);
	};
}

#endif // __SCENE_GRAPH_TRANSFORM_UPDATE_SYSTEM_H__
