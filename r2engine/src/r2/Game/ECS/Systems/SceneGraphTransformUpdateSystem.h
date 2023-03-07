#ifndef __SCENE_GRAPH_TRANSFORM_UPDATE_SYSTEM_H__
#define __SCENE_GRAPH_TRANSFORM_UPDATE_SYSTEM_H__

#include "r2/Game/ECS/System.h"

namespace r2::ecs
{
	class SceneGraphTransformUpdateSystem : public System
	{
	public:

		SceneGraphTransformUpdateSystem();
		~SceneGraphTransformUpdateSystem();
		void Update();

	private:
	};
}

#endif // __SCENE_GRAPH_TRANSFORM_UPDATE_SYSTEM_H__
