#include "r2pch.h"
#ifdef R2_EDITOR
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"

namespace r2::evt
{
	r2::ecs::Entity EditorEntityEvent::GetEntity() const
	{
		return mEntity;
	}

	EditorEntityEvent::EditorEntityEvent(ecs::Entity entity, bool shouldConsume)
		:EditorEvent(shouldConsume)
		,mEntity(entity)
	{
	}

	EditorEntityCreatedEvent::EditorEntityCreatedEvent(ecs::Entity newEntity)
		: EditorEntityEvent(newEntity, false)
	{
	}

	std::string EditorEntityCreatedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorEntityCreatedEvent new entity: " << GetEntity();
		return ss.str();
	}


	EditorEntityDestroyedEvent::EditorEntityDestroyedEvent(ecs::Entity destroyedEntity)
		: EditorEntityEvent(destroyedEntity, false)
	{
	}

	std::string EditorEntityDestroyedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorEntityDestroyedEvent new entity: " << GetEntity();
		return ss.str();
	}

	EditorEntityTreeDestroyedEvent::EditorEntityTreeDestroyedEvent(ecs::Entity destroyedEntity)
		: EditorEntityEvent(destroyedEntity, false)
	{
	}

	std::string EditorEntityTreeDestroyedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorEntityTreeDestroyedEvent parent entity: " << GetEntity();
		return ss.str();
	}

	EditorEntitySelectedEvent::EditorEntitySelectedEvent(ecs::Entity entitySelected, ecs::Entity prevSelectedEntity)
		:EditorEntityEvent(entitySelected, false)
		,mPrevSelectedEntity(prevSelectedEntity)
	{
	}

	std::string EditorEntitySelectedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorEntitySelectedEvent selected entity: " << GetEntity() << ", previously selected entity: " << mPrevSelectedEntity;
		return ss.str();
	}

	ecs::Entity EditorEntitySelectedEvent::GetPreviouslySelectedEntity() const
	{
		return mPrevSelectedEntity;
	}
}


#endif // R2_EDITOR
