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
		const LevelName& GetLevel() const;
		EVENT_CLASS_CATEGORY(ECAT_EDITOR | ECAT_EDITOR_LEVEL)
	protected:
		EditorLevelEvent(const LevelName& level, bool shouldConsume);
		LevelName mLevel;
	};

	class EditorLevelLoadedEvent : public EditorLevelEvent
	{
	public:
		EditorLevelLoadedEvent(const LevelName& level, const std::string& filePathName);
		std::string ToString() const override;
		const std::string& GetFilePath() const { return mFilePathName; }
		EVENT_CLASS_TYPE(EVT_EDITOR_LEVEL_LOADED)

	private:
		std::string mFilePathName;
	};

	class EditorLevelWillUnLoadEvent : public EditorLevelEvent
	{
	public:
		EditorLevelWillUnLoadEvent(const LevelName& level);
		std::string ToString() const override;
		EVENT_CLASS_TYPE(EVT_EDITOR_LEVEL_WILL_UNLOAD)
	};

	class EditorNewLevelCreatedEvent : public EditorLevelEvent
	{
	public:
		EditorNewLevelCreatedEvent(LevelName& editorLevel);
		std::string ToString() const override;
		EVENT_CLASS_TYPE(EVT_EDITOR_LEVEL_CREATED)
	};

	class EditorSetEditorLevel : public EditorLevelEvent
	{
	public:
		EditorSetEditorLevel(LevelName& editorLevel);
		std::string ToString() const override;
		EVENT_CLASS_TYPE(EVT_EDITOR_LEVEL_SET)
	};


	class EditorAssetsImportedIntoLevel : public EditorLevelEvent
	{
	public:
		EditorAssetsImportedIntoLevel(LevelName& editorLevel);
		std::string ToString() const override;
		EVENT_CLASS_TYPE(EVT_EDITOR_LEVEL_ASSETS_IMPORTED)
	};
}


#endif
#endif