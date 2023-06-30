#ifdef R2_EDITOR
#ifndef __EDITOR_MAIN_MENU_BAR_H__
#define __EDITOR_MAIN_MENU_BAR_H__

#include "r2/Editor/EditorWidget.h"
#include <deque>

namespace r2::edit
{
	class MainMenuBar : public EditorWidget
	{
	public:
		MainMenuBar();
		~MainMenuBar();
		virtual void Init(Editor* noptrEditor) override;
		virtual void Shutdown() override;
		virtual void OnEvent(evt::Event& e) override;
		virtual void Update() override;
		virtual void Render(u32 dockingSpaceID) override;
	private:

		void LoadLevel(const std::string& filePath);
		void UnloadLevel();

		void SaveLevelToRecents(const std::string& filePath);

		void LoadRecentsFile();
		void SaveRecentsFile();

		std::string mGroupName;
		std::string mLevelName;
		
		std::deque<std::string> mLastLevelPathsOpened;

	};
}


#endif // __EDITOR_MAIN_MENU_BAR_H__
#endif