#ifdef R2_EDITOR
#ifndef __INSPECTOR_PANEL_EDITOR_COMPONENT_H__
#define __INSPECTOR_PANEL_EDITOR_COMPONENT_H__

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
	void InspectorPanelEditorComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator);
}

#endif // __INSPECTOR_PANEL_EDITOR_COMPONENT__
#endif