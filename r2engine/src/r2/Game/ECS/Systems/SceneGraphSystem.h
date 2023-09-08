#ifndef __SCENE_GRAPH_SYSTEM_H__
#define __SCENE_GRAPH_SYSTEM_H__

#include "r2/Game/ECS/System.h"

namespace r2::ecs
{
	class SceneGraph;
	class SceneGraphSystem : public System
	{
	public:
		
		SceneGraphSystem();
		~SceneGraphSystem();

		virtual s32 FindSortedPlacement(Entity e) override;

		void SetSceneGraph(SceneGraph* sceneGraph);

	private:
		SceneGraph* mnoptrSceneGraph;
	};
}

#endif

