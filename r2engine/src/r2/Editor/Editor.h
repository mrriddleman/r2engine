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
	class SceneGraph;
}

namespace r2::draw
{
	struct AnimModel;
	struct Animation;
	struct Model;
}

namespace r2
{
	
	class Level;

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
		
		void UndoLastAction();
		void RedoLastAction();
		void Save();
		void LoadLevel(const std::string& filePathName);
		void UnloadCurrentLevel();
		void CreateNewLevel(const std::string& groupName, const std::string& levelName);
		void ReloadLevel();

		const Level& GetEditorLevelConst() const;
		Level& GetEditorLevelRef();

		void PostNewAction(std::unique_ptr<edit::EditorAction> action);
		void PostEditorEvent(r2::evt::EditorEvent& e);

		std::string GetAppLevelPath() const;
		ecs::SceneGraph& GetSceneGraph();
		ecs::ECSCoordinator* GetECSCoordinator();

		inline u32 GetEditorFolderImage() const { return mEditorFolderImage; }
		inline u32 GetEditorFileImage() const { return mEditorFileImage; }

	private:

		void AddModelToLevel(u64 modelAssetName, const r2::draw::Model& model);
		void AddSoundBankToLevel(u64 soundBankAssetName);

		Level* mCurrentEditorLevel;

		std::vector<std::unique_ptr<edit::EditorWidget>> mEditorWidgets;
		std::vector<std::unique_ptr<edit::EditorAction>> mUndoStack;
		std::vector<std::unique_ptr<edit::EditorAction>> mRedoStack;

		s32 mEditorFolderImageWidth;
		s32 mEditorFolderImageHeight;
		u32 mEditorFolderImage;

		s32 mEditorFileImageWidth;
		s32 mEditorFileImageHeight;
		u32 mEditorFileImage;
	};
}

#endif // __EDITOR_H__
#endif