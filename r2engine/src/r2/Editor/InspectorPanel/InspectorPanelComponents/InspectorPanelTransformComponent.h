#ifdef R2_EDITOR
#ifndef __INSPECTOR_PANEL_TRANSFORM_COMPONENT
#define __INSPECTOR_PANEL_TRANSFORM_COMPONENT

#include "r2/Game/ECS/Entity.h"

namespace r2
{
	class Editor;
}
namespace r2::ecs
{
	class ECSCoordinator;
}

namespace r2::edit
{
	void InspectorPanelTransformComponent(Editor* editor,r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator);
}


#endif
#endif