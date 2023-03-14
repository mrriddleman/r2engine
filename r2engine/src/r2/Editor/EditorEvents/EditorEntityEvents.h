#ifndef __EDITOR_ENTITY_EVENTS_H__
#define __EDITOR_ENTITY_EVENTS_H__

#ifdef R2_EDITOR

#include "r2/Game/ECS/Entity.h"
#include "r2/Editor/EditorEvents/EditorEvent.h"
#include <sstream>

namespace r2::evt
{
	class EditorEntityEvent : public EditorEvent 
	{
	public:
		ecs::Entity GetEntity() const;
		EVENT_CLASS_CATEGORY(ECAT_EDITOR | ECAT_EDITOR_ENTITY)
	protected:

		EditorEntityEvent(ecs::Entity entity, bool shouldConsume);
		ecs::Entity mEntity;
	};

	class EditorEntityCreatedEvent : public EditorEntityEvent
	{
	public:
		EditorEntityCreatedEvent(ecs::Entity newEntity);
		std::string ToString() const override;
		EVENT_CLASS_TYPE(EVT_EDITOR_CREATED_NEW_ENTITY)

	};

	class EditorEntityDestroyedEvent : public EditorEntityEvent
	{
	public:
		EditorEntityDestroyedEvent(ecs::Entity destroyedEntity);
		std::string ToString() const override;
		EVENT_CLASS_TYPE(EVT_EDITOR_DESTROYED_ENTITY)

	};

	class EditorEntityTreeDestroyedEvent : public EditorEntityEvent
	{
	public:
		EditorEntityTreeDestroyedEvent(ecs::Entity destroyedEntityParent);
		std::string ToString() const override;
		EVENT_CLASS_TYPE(EVT_EDITOR_DESTROYED_ENTITY_TREE)

	};

	class EditorEntitySelectedEvent : public EditorEntityEvent
	{
	public:
		EditorEntitySelectedEvent(ecs::Entity entitySelected, ecs::Entity prevSelectedEntity);
		std::string ToString() const override;
		ecs::Entity GetPreviouslySelectedEntity() const;

		EVENT_CLASS_TYPE(EVT_EDITOR_ENTITY_SELECTED)


	private:
		ecs::Entity mPrevSelectedEntity;
	};
}

#endif

#endif // __EDITOR_ENTITY_EVENTS_H__
