#ifdef R2_EDITOR
#ifndef __EDITOR_H__
#define __EDITOR_H__

#include <vector>
#include "r2/Editor/EditorWidget.h"
#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Utils/Random.h"

namespace r2::evt
{
	class Event;
	class EditorEvent;
}

namespace r2::ecs
{
	class ECSCoordinator;
}

namespace r2::draw
{
	struct AnimModel;
}

namespace r2
{
	class SceneGraph;

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
		void PostNewAction(std::unique_ptr<edit::EditorAction> action);
		void UndoLastAction();
		void RedoLastAction();
		void Save();
		void LoadLevel(const std::string& filePathName, const std::string& parentDirectory);

		std::string GetAppLevelPath() const;

		void PostEditorEvent(r2::evt::EditorEvent& e);


		SceneGraph& GetSceneGraph();
		SceneGraph* GetSceneGraphPtr();
		ecs::ECSCoordinator* GetECSCoordinator();
		r2::mem::MallocArena& GetMemoryArena();

	private:

		r2::mem::MemoryArea::Handle mEditorMemoryAreaHandle;
		r2::mem::MallocArena mMallocArena;


		std::vector<void*> mComponentAllocations;

		std::vector<std::unique_ptr<edit::EditorWidget>> mEditorWidgets;
		std::vector<std::unique_ptr<edit::EditorAction>> mUndoStack;
		std::vector<std::unique_ptr<edit::EditorAction>> mRedoStack;
	};
}

#endif // __EDITOR_H__
#endif