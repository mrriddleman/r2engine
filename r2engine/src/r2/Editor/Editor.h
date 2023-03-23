#ifdef R2_EDITOR
#ifndef __EDITOR_H__
#define __EDITOR_H__

#include <vector>
#include "r2/Editor/EditorWidget.h"
#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/SceneGraph/SceneGraph.h"

#include "r2/Utils/Random.h"

namespace r2::evt
{
	class Event;
	class EditorEvent;
}

namespace r2::ecs
{
	class RenderSystem;
	class SkeletalAnimationSystem;
#ifdef R2_DEBUG
	class DebugBonesRenderSystem;
	class DebugRenderSystem;
#endif
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
		void PostNewAction(std::unique_ptr<edit::EditorAction> action);
		void UndoLastAction();
		void RedoLastAction();
		void Save();

		void PostEditorEvent(r2::evt::EditorEvent& e);


		SceneGraph& GetSceneGraph();
		SceneGraph* GetSceneGraphPtr();
		ecs::ECSCoordinator* GetECSCoordinator();
		r2::mem::MallocArena& GetMemoryArena();

	private:

		void RegisterComponents();
		void RegisterSystems();
		void UnRegisterComponents();
		void UnRegisterSystems();


		//@TEST CODE:
		r2::util::Random mRandom;

		r2::mem::MallocArena mMallocArena;
		ecs::ECSCoordinator* mCoordinator;
		SceneGraph mSceneGraph;

		r2::ecs::RenderSystem* mnoptrRenderSystem;
		r2::ecs::SkeletalAnimationSystem* mnoptrSkeletalAnimationSystem;
#ifdef R2_DEBUG
		r2::ecs::DebugBonesRenderSystem* mnoptrDebugBonesRenderSystem;
		r2::ecs::DebugRenderSystem* mnoptrDebugRenderSystem;
#endif

		std::vector<void*> mComponentAllocations;

		std::vector<std::unique_ptr<edit::EditorWidget>> mEditorWidgets;
		std::vector<std::unique_ptr<edit::EditorAction>> mUndoStack;
		std::vector<std::unique_ptr<edit::EditorAction>> mRedoStack;
	};
}

#endif // __EDITOR_H__
#endif