#include "r2pch.h"
#ifdef R2_EDITOR
#include "r2/Editor/EditorEvents/EditorLevelEvents.h"

namespace r2::evt
{
	const r2::Level& EditorLevelEvent::GetLevel() const
	{
		return mLevel;
	}

	EditorLevelEvent::EditorLevelEvent(const Level& level, bool shouldConsume)
		:EditorEvent(shouldConsume)
		, mLevel(level)
	{
	}

	EditorLevelLoadedEvent::EditorLevelLoadedEvent(const Level& level)
		:EditorLevelEvent(level, false)
	{
	}

	std::string EditorLevelLoadedEvent::ToString() const
	{
		std::stringstream ss;
		ss << "EditorLevelLoadedEvent new level: " << GetLevel().GetLevelName();
		return ss.str();
	}

}


#endif