#ifdef R2_EDITOR

#ifndef __INSPECTOR_PANEL_COMPONENT_H__
#define __INSPECTOR_PANEL_COMPONENT_H__

#include <functional>

#include "r2/Game/ECS/Entity.h"

namespace r2::ecs
{
	class ECSCoordinator;
}

namespace r2::edit
{
	using InspectorPanelComponentWidgetFunc = std::function<void(r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)>;
}

#endif
#endif
