#include "r2pch.h"
#include "r2/Game/SceneGraph/SceneGraph.h"

namespace r2
{
	SceneGraph::SceneGraph()
		:mnoptrSceneGraphSystem(nullptr)
		, mnoptrECSCoordinator(nullptr)
	{

	}

	SceneGraph::~SceneGraph()
	{
		R2_CHECK(mnoptrSceneGraphSystem == nullptr, "Did you forget to Shutdown the SceneGraph?");
		R2_CHECK(mnoptrECSCoordinator == nullptr, "Did you forget to Shutdown the SceneGraph?");
	}
}