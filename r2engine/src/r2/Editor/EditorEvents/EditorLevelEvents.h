#ifndef __EDITOR_LEVEL_EVENTS_H__
#define __EDITOR_LEVEL_EVENTS_H__

#ifdef R2_EDITOR

#include "r2/Editor/EditorEvents/EditorEvent.h"
#include "r2/Game/Level/Level.h"

namespace r2::evt
{
	class EditorLevelEvent : public EditorEvent
	{
	public:
		const Level& GetLevel() const;
		EVENT_CLASS_CATEGORY(ECAT_EDITOR | ECAT_EDITOR_LEVEL)
	protected:
		EditorLevelEvent(const Level& level, bool shouldConsume);
		Level mLevel;
	};

	class EditorLevelLoadedEvent : public EditorLevelEvent
	{
	public:
		EditorLevelLoadedEvent(const Level& level);
		std::string ToString() const override;
		EVENT_CLASS_TYPE(EVT_EDITOR_LEVEL_LOADED)
	};
}


#endif
#endif