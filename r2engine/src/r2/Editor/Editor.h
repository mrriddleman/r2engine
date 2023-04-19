#ifdef R2_EDITOR
#ifndef __EDITOR_H__
#define __EDITOR_H__

#include <vector>
#include "r2/Editor/EditorWidget.h"
#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/SceneGraph/SceneGraph.h"
#include "r2/Game/Level/Level.h"
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

namespace r2::draw
{
	struct AnimModel;
}

namespace r2
{
	struct LevelCache;


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

		void RegisterComponents();
		void RegisterSystems();
		void UnRegisterComponents();
		void UnRegisterSystems();

		//@TEST CODE: REMOVE!
		r2::SArray<ecs::RenderComponent>* HydrateRenderComponents(r2::SArray<ecs::RenderComponent>* tempRenderComponents);
		r2::SArray<ecs::SkeletalAnimationComponent>* HydrateSkeletalAnimationComponents(r2::SArray<ecs::SkeletalAnimationComponent>* tempSkeletalAnimationComponents);
		r2::SArray<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>* HydrateInstancedSkeletalAnimationComponents(r2::SArray<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>* tempInstancedSkeletalAnimationComponents);
		r2::SArray<ecs::InstanceComponentT<ecs::TransformComponent>>* HydrateInstancedTransformComponents(r2::SArray<ecs::InstanceComponentT<ecs::TransformComponent>>* tempInstancedTransformComponents);
		
		r2::util::Random mRandom;
		const r2::draw::AnimModel* microbatAnimModel;
		LevelCache* moptrLevelCache;
		r2::LevelHandle mLevelHandle;
		const flat::LevelData* mLevelData;

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