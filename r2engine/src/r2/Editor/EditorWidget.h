#ifdef R2_EDITOR
#ifndef __EDITOR_WIDGET_H__
#define __EDITOR_WIDGET_H__

#include <string>
#include "r2/Utils/Utils.h"

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
		EditorWidget();
		virtual ~EditorWidget();
		virtual void Init(Editor* noptrEditor) = 0;
		virtual void Shutdown() = 0;
		virtual void OnEvent(evt::Event& e) = 0;
		virtual void Update() = 0;
		virtual void Render(u32 dockingSpaceID) = 0;

	protected:

		//@NOTE(Serge): this is in the same order as the imgui type
		enum DockingDirection
		{
			LEFT = 0,
			RIGHT,
			UP,
			DOWN
		};

		static void DockWindowInViewport(u32 dockingSpaceID, const std::string& windowName, DockingDirection dir, float amount);
		//static std::unordered_map<unsigned int, std::vector<float>> mDockingSizes;

		Editor* mnoptrEditor;
	};
}

#endif // __EDITOR_WIDGET_H__
#endif