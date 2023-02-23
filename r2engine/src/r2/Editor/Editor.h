#ifdef R2_EDITOR
#ifndef __EDITOR_H__
#define __EDITOR_H__

#include <vector>
#include "r2/Editor/EditorWidget.h"

namespace r2::evt
{
	class Event;
}

namespace r2
{
	class Editor
	{
	public:
		Editor();
		void Init();
		void Shutdown();
		void OnEvent(evt::Event& e);
		void Update();
		void Render(u32 dockingSpaceID);

	private:

		std::vector<std::unique_ptr<edit::EditorWidget>> mEditorWidgets;

	};
}

#endif // __EDITOR_H__
#endif