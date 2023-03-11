#ifndef __EDITOR_ENTITY_EVENTS_H__
#define __EDITOR_ENTITY_EVENTS_H__

#ifdef R2_EDITOR

#include "r2/Game/ECS/Entity.h"
#include "r2/Editor/EditorEvents/EditorEvent.h"
#include <sstream>

namespace r2::evt
{
	class EditorEntityCreatedEvent : public EditorEvent
	{
	public:
		EditorEntityCreatedEvent(ecs::Entity newEntity);
		std::string ToString() const override;
		ecs::Entity GetNewEntity() const;

		EVENT_CLASS_TYPE(EVT_EDITOR_CREATED_NEW_ENTITY)
	private:
		ecs::Entity mNewEntity;
	};

	class EditorEntityDestroyedEvent : public EditorEvent
	{
	public:
		EditorEntityDestroyedEvent(ecs::Entity destroyedEntity);
		std::string ToString() const override;
		ecs::Entity GetDestroyedEntity() const;

		EVENT_CLASS_TYPE(EVT_EDITOR_DESTROYED_ENTITY)
	private:
		ecs::Entity mDestroyedEntity;
	};

}

#endif

#endif // __EDITOR_ENTITY_EVENTS_H__
