#ifdef R2_EDITOR
#ifndef __EDITOR_H__
#define __EDITOR_H__

#include <vector>
#include "r2/Editor/EditorWidget.h"
#include "r2/Editor/EditorActions/EditorAction.h"

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
		void Render();
		void RenderImGui(u32 dockingSpaceID);
		void PostNewEditorAction(std::unique_ptr<edit::EditorAction> action);
		void UndoLastAction();
		void RedoLastAction();
		void Save();

	private:

		std::vector<std::unique_ptr<edit::EditorWidget>> mEditorWidgets;
		std::vector<std::unique_ptr<edit::EditorAction>> mUndoStack;
		std::vector<std::unique_ptr<edit::EditorAction>> mRedoStack;

	};
}

#endif // __EDITOR_H__
#endif