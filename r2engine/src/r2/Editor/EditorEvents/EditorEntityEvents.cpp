#include "r2pch.h"
#ifdef R2_EDITOR
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"

namespace r2::evt
{
	EditorEntityCreatedEvent::EditorEntityCreatedEvent(ecs::Entity newEntity)
		: EditorEvent(false)
		,mNewEntity(newEntity)
	{
	}

	std::string EditorEntityCreatedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorEntityCreatedEvent new entity: " << mNewEntity;
		return ss.str();
	}

	ecs::Entity EditorEntityCreatedEvent::GetNewEntity() const
	{
		return mNewEntity;
	}

	EditorEntityDestroyedEvent::EditorEntityDestroyedEvent(ecs::Entity destroyedEntity)
		: EditorEvent(false)
		, mDestroyedEntity(destroyedEntity)
	{
	}

	std::string EditorEntityDestroyedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorEntityDestroyedEvent new entity: " << mDestroyedEntity;
		return ss.str();
	}

	ecs::Entity EditorEntityDestroyedEvent::GetDestroyedEntity() const
	{
		return mDestroyedEntity;
	}

	EditorEntityTreeDestroyedEvent::EditorEntityTreeDestroyedEvent(ecs::Entity destroyedEntity)
		: EditorEvent(false)
		,mDestroyedParentEntity(destroyedEntity)
	{

	}

	std::string EditorEntityTreeDestroyedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorEntityTreeDestroyedEvent parent entity: " << mDestroyedParentEntity;
		return ss.str();
	}

	r2::ecs::Entity EditorEntityTreeDestroyedEvent::GetDestroyedParentEntity() const
	{
		return mDestroyedParentEntity;
	}

}


#endif // R2_EDITOR
