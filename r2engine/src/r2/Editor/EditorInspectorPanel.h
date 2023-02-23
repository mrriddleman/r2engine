
#ifdef R2_EDITOR
#ifndef __EDITOR_INSPECTOR_PANEL_H__
#define __EDITOR_INSPECTOR_PANEL_H__

#include "r2/Editor/EditorWidget.h"

namespace r2
{
	class Editor;
}

namespace r2::edit
{
	class InspectorPanel : public EditorWidget
	{
	public:
		InspectorPanel();
		~InspectorPanel();
		virtual void Init(Editor* noptrEditor) override;
		virtual void Shutdown() override;
		virtual void OnEvent(evt::Event& e) override;
		virtual void Update() override;
		virtual void Render(u32 dockingSpaceID) override;
	};
}

#endif // __EDITOR_INSPECTOR_PANEL_H__
#endif