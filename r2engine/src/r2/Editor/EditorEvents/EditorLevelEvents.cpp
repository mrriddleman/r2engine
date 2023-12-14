#include "r2pch.h"
#ifdef R2_EDITOR
#include "r2/Editor/EditorEvents/EditorLevelEvents.h"

namespace r2::evt
{
	const r2::LevelName& EditorLevelEvent::GetEditorLevel() const
	{
		return mLevel;
	}

	EditorLevelEvent::EditorLevelEvent(const LevelName& level, bool shouldConsume)
		:EditorEvent(shouldConsume)
		, mLevel(level)
	{
	}

	EditorLevelLoadedEvent::EditorLevelLoadedEvent(const LevelName& level, const std::string& filePathName)
		:EditorLevelEvent(level, false)
		,mFilePathName(filePathName)
	{
	}

	std::string EditorLevelLoadedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorLevelLoadedEvent level: " << mLevel.assetNameString;
		return ss.str();
	}


	EditorNewLevelCreatedEvent::EditorNewLevelCreatedEvent(LevelName& editorLevel)
		:EditorLevelEvent(editorLevel, false)
	{

	}

	std::string EditorNewLevelCreatedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorNewLevelCreatedEvent new level: " << mLevel.assetNameString;
		return ss.str();
	}


	EditorSetEditorLevel::EditorSetEditorLevel(LevelName& editorLevel)
		:EditorLevelEvent(editorLevel, false)
	{
	}

	std::string EditorSetEditorLevel::ToString() const
	{
		std::stringstream ss;
		ss << "EditorSetEditorLevel set to level: " << mLevel.assetNameString;
		return ss.str();
	}

	EditorLevelWillUnLoadEvent::EditorLevelWillUnLoadEvent(const LevelName& level)
		:EditorLevelEvent(level, false)
	{

	}

	std::string EditorLevelWillUnLoadEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorLevelWillUnLoadedEvent will unload level: " << mLevel.assetNameString;
		return ss.str();
	}

}


#endif