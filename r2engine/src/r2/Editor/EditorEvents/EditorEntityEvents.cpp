#include "r2pch.h"
#ifdef R2_EDITOR
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include <sstream>

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

	EditorEntitySelectedEvent::EditorEntitySelectedEvent(ecs::Entity entitySelected, s32 selectedInstance, ecs::Entity prevSelectedEntity, s32 prevSelectedInstance)
		:EditorEntityEvent(entitySelected, false)
		,mPrevSelectedEntity(prevSelectedEntity)
		,mSelectedInstance(selectedInstance)
		,mPrevSelectedInstance(prevSelectedInstance)
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

	s32 EditorEntitySelectedEvent::GetSelectedInstance() const
	{
		return mSelectedInstance;
	}

	s32 EditorEntitySelectedEvent::GetPrevSelectedInstance() const
	{
		return mPrevSelectedInstance;
	}

	EditorEntityNameChangedEvent::EditorEntityNameChangedEvent(ecs::Entity entity, const std::string& newName)
		:EditorEntityEvent(entity, false)
		,mNewName(newName)
	{
	}

	std::string EditorEntityNameChangedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorEntityNameChangedEvent entity: " << GetEntity() << ", to new name: " << mNewName;
		return ss.str();
	}

	std::string EditorEntityNameChangedEvent::GetNewName() const
	{
		return mNewName;
	}

	EditorEntityAttachedToNewParentEvent::EditorEntityAttachedToNewParentEvent(ecs::Entity entity, ecs::Entity oldParent, ecs::Entity newParent)
		:EditorEntityEvent(entity, false)
		,mOldParent(oldParent)
		,mNewParent(newParent)
	{

	}

	std::string EditorEntityAttachedToNewParentEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorEntityAttachedToNewParentEvent entity: " << GetEntity() << ", attached from: " << mOldParent << " to new parent: " << mNewParent;
		return ss.str();
	}

	r2::ecs::Entity EditorEntityAttachedToNewParentEvent::GetOldParent() const
	{
		return mOldParent;
	}

	r2::ecs::Entity EditorEntityAttachedToNewParentEvent::GetNewParent() const
	{
		return mNewParent;
	}

	EditorEntityEditorFlagChangedEvent::EditorEntityEditorFlagChangedEvent(ecs::Entity entity, u32 flag, bool enabled)
		:EditorEntityEvent(entity, false)
		,mFlag(flag)
		,mEnabled(enabled)
	{

	}

	std::string EditorEntityEditorFlagChangedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorEntityEditorFlagChangedEvent entity: " << GetEntity() << ", changed flag: " << mFlag << " to: " << mEnabled;
		return ss.str();
	}

	u32 EditorEntityEditorFlagChangedEvent::GetFlag() const
	{
		return mFlag;
	}

	bool EditorEntityEditorFlagChangedEvent::GetEnabled() const
	{
		return mEnabled;
	}

	EditorEntityTransformComponentChangedEvent::EditorEntityTransformComponentChangedEvent(ecs::Entity entity, s32 instance, r2::ecs::eTransformDirtyFlags dirtyFlags)
		:EditorEntityEvent(entity, false)
		,mInstanceTransformed(instance)
		,mFlags(dirtyFlags)
	{

	}

	std::string EditorEntityTransformComponentChangedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorEntityTransformComponentChangedEvent entity: " << GetEntity() << ", instance: " << mInstanceTransformed;
		return ss.str();
	}

	s32 EditorEntityTransformComponentChangedEvent::GetInstance() const
	{
		return mInstanceTransformed;
	}

	r2::ecs::eTransformDirtyFlags EditorEntityTransformComponentChangedEvent::GetTransformDirtyFlags() const
	{
		return mFlags;
	}

}


#endif // R2_EDITOR
