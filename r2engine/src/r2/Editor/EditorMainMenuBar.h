#ifdef R2_EDITOR
#ifndef __EDITOR_MAIN_MENU_BAR_H__
#define __EDITOR_MAIN_MENU_BAR_H__

#include "r2/Editor/EditorWidget.h"

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
	};
}


#endif // __EDITOR_MAIN_MENU_BAR_H__
#endif