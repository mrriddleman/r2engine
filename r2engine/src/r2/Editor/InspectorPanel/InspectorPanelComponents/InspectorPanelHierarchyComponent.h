#ifdef R2_EDITOR
#ifndef __INSPECTOR_PANEL_HIERARCHY_COMPONENT__
#define __INSPECTOR_PANEL_HIERARCHY_COMPONENT__

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
	void InspectorPanelHierarchyComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator);
}

#endif
#endif