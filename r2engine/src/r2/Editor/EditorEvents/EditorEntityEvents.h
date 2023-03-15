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

	class EditorEntityNameChangedEvent : public EditorEntityEvent
	{
	public:
		EditorEntityNameChangedEvent(ecs::Entity entity, const std::string& newName);
		std::string ToString() const override;

		std::string GetNewName() const;

		EVENT_CLASS_TYPE(EVT_EDITOR_ENTITY_NAME_CHANGED)
	private:
		std::string mNewName;
	};

	class EditorEntityAttachedToNewParentEvent : public EditorEntityEvent
	{
	public:
		EditorEntityAttachedToNewParentEvent(ecs::Entity entity, ecs::Entity oldParent, ecs::Entity newParent);
		std::string ToString() const override;

		ecs::Entity GetOldParent() const;
		ecs::Entity GetNewParent() const;

		EVENT_CLASS_TYPE(EVT_EDITOR_ENTITY_ATTACHED_TO_NEW_PARENT)
	private:
		ecs::Entity mOldParent;
		ecs::Entity mNewParent;
	};
}

#endif

#endif // __EDITOR_ENTITY_EVENTS_H__
