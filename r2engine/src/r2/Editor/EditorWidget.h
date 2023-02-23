#ifdef R2_EDITOR
#ifndef __EDITOR_WIDGET_H__
#define __EDITOR_WIDGET_H__

namespace r2
{
	class Editor;
}

namespace r2::evt
{
	class Event;
}

namespace r2::edit
{
	class EditorWidget
	{
	public:
		EditorWidget():mnoptrEditor(nullptr){}
		virtual ~EditorWidget() {}
		virtual void Init(Editor* noptrEditor) = 0;
		virtual void Shutdown() = 0;
		virtual void OnEvent(evt::Event& e) = 0;
		virtual void Update() = 0;
		virtual void Render(u32 dockingSpaceID) = 0;

	protected:
		Editor* mnoptrEditor;
	};
}

#endif // __EDITOR_WIDGET_H__
#endif