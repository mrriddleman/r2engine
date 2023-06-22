#ifndef __EDITOR_LEVEL_EVENTS_H__
#define __EDITOR_LEVEL_EVENTS_H__

#ifdef R2_EDITOR

#include "r2/Editor/EditorEvents/EditorEvent.h"
#include "r2/Editor/EditorLevel.h"

namespace r2::evt
{
	class EditorLevelEvent : public EditorEvent
	{
	public:
		const EditorLevel& GetEditorLevel() const;
		EVENT_CLASS_CATEGORY(ECAT_EDITOR | ECAT_EDITOR_LEVEL)
	protected:
		EditorLevelEvent(const EditorLevel& level, bool shouldConsume);
		EditorLevel mLevel;
	};

	class EditorLevelLoadedEvent : public EditorLevelEvent
	{
	public:
		EditorLevelLoadedEvent(const EditorLevel& level);
		std::string ToString() const override;
		EVENT_CLASS_TYPE(EVT_EDITOR_LEVEL_LOADED)
	};

	class EditorNewLevelCreatedEvent : public EditorLevelEvent
	{
	public:
		EditorNewLevelCreatedEvent(EditorLevel& editorLevel);
		std::string ToString() const override;
		EVENT_CLASS_TYPE(EVT_EDITOR_LEVEL_CREATED)
	};

	class EditorSetEditorLevel : public EditorLevelEvent
	{
	public:
		EditorSetEditorLevel(EditorLevel& editorLevel);
		std::string ToString() const override;
		EVENT_CLASS_TYPE(EVT_EDITOR_LEVEL_SET)
	};
}


#endif
#endif