#include "r2pch.h"
#ifdef R2_EDITOR
#include "r2/Editor/EditorEvents/EditorLevelEvents.h"

namespace r2::evt
{
	const r2::EditorLevel& EditorLevelEvent::GetEditorLevel() const
	{
		return mLevel;
	}

	EditorLevelEvent::EditorLevelEvent(const EditorLevel& level, bool shouldConsume)
		:EditorEvent(shouldConsume)
		, mLevel(level)
	{
	}

	EditorLevelLoadedEvent::EditorLevelLoadedEvent(const EditorLevel& level)
		:EditorLevelEvent(level, false)
	{
	}

	std::string EditorLevelLoadedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorLevelLoadedEvent level: " << "Group Name: " << mLevel.GetGroupName() << ", Level Name: " << mLevel.GetLevelName();
		return ss.str();
	}


	EditorNewLevelCreatedEvent::EditorNewLevelCreatedEvent(EditorLevel& editorLevel)
		:EditorLevelEvent(editorLevel, false)
	{

	}

	std::string EditorNewLevelCreatedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorNewLevelCreatedEvent new level: " << "Group Name: " << mLevel.GetGroupName() << ", Level Name: " << mLevel.GetLevelName();
		return ss.str();
	}


	EditorSetEditorLevel::EditorSetEditorLevel(EditorLevel& editorLevel)
		:EditorLevelEvent(editorLevel, false)
	{
	}

	std::string EditorSetEditorLevel::ToString() const
	{
		std::stringstream ss;
		ss << "EditorSetEditorLevel set to level: " << "Group Name: " << mLevel.GetGroupName() << ", Level Name: " << mLevel.GetLevelName();
		return ss.str();
	}

}


#endif