#include "r2pch.h"
#if defined(R2_EDITOR) && defined(R2_IMGUI)

#include "r2/Core/Events/Events.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorMainMenuBar.h"
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/EditorAssetPanel.h"
#include "r2/Editor/EditorScenePanel.h"
#include "imgui.h"

namespace r2
{
	Editor::Editor()
		:mMallocArena(r2::mem::utils::MemBoundary())
		,mCoordinator(nullptr)
	{

	}

	void Editor::Init()
	{
		mCoordinator = ALLOC(ecs::ECSCoordinator, mMallocArena);
		mCoordinator->Init<mem::MallocArena>(mMallocArena, ecs::MAX_NUM_COMPONENTS, ecs::MAX_NUM_ENTITIES, 0, ecs::MAX_NUM_SYSTEMS);
		mSceneGraph.Init<mem::MallocArena>(mMallocArena, mCoordinator);

		//Do all of the panels/widgets setup here
		std::unique_ptr<edit::MainMenuBar> mainMenuBar = std::make_unique<edit::MainMenuBar>();
		mEditorWidgets.push_back(std::move(mainMenuBar));

		std::unique_ptr<edit::InspectorPanel> inspectorPanel = std::make_unique<edit::InspectorPanel>();
		mEditorWidgets.push_back(std::move(inspectorPanel));

		std::unique_ptr<edit::AssetPanel> assetPanel = std::make_unique<edit::AssetPanel>();
		mEditorWidgets.push_back(std::move(assetPanel));

		std::unique_ptr<edit::ScenePanel> scenePanel = std::make_unique<edit::ScenePanel>();
		mEditorWidgets.push_back(std::move(scenePanel));

		//now init all of the widgets
		for (const auto& widget : mEditorWidgets)
		{
			widget->Init(this);
		}
	}

	void Editor::Shutdown()
	{
		for (const auto& widget : mEditorWidgets)
		{
			widget->Shutdown();
		}

		mEditorWidgets.clear();

		mSceneGraph.Shutdown<mem::MallocArena>(mMallocArena);
		mCoordinator->Shutdown<mem::MallocArena>(mMallocArena);

		FREE(mCoordinator, mMallocArena);
	}

	void Editor::OnEvent(evt::Event& e)
	{
		r2::evt::EventDispatcher dispatcher(e);

		dispatcher.Dispatch<r2::evt::KeyPressedEvent>([this](const r2::evt::KeyPressedEvent& e)
			{
				if (e.KeyCode() == r2::io::KEY_z &&
					((e.Modifiers() & r2::io::Key::CONTROL_PRESSED) == r2::io::Key::CONTROL_PRESSED) &&
					((e.Modifiers() & r2::io::Key::SHIFT_PRESSED_KEY) == 0))
				{
					UndoLastAction();
					return true;
				}
				else if (e.KeyCode() == r2::io::KEY_z && 
					((e.Modifiers() & r2::io::Key::CONTROL_PRESSED) == r2::io::Key::CONTROL_PRESSED) &&
					((e.Modifiers() & r2::io::Key::SHIFT_PRESSED_KEY) == r2::io::Key::SHIFT_PRESSED_KEY))
				{
					RedoLastAction();
					return true;
				}
				else if (e.KeyCode() == r2::io::KEY_s &&
					((e.Modifiers() & r2::io::Key::CONTROL_PRESSED) == r2::io::Key::CONTROL_PRESSED))
				{
					Save();
					return true;
				}

				return false;
			});


		for (const auto& widget : mEditorWidgets)
		{
			widget->OnEvent(e);
		}
	}

	void Editor::Update()
	{
		for (const auto& widget : mEditorWidgets)
		{
			widget->Update();
		}
	}

	void Editor::Render()
	{

	}

	void Editor::RenderImGui(u32 dockingSpaceID)
	{
		for (const auto& widget : mEditorWidgets)
		{
			widget->Render(dockingSpaceID);
		}
	}

	void Editor::PostNewAction(std::unique_ptr<edit::EditorAction> action)
	{
		//Do the action
		action->Redo();
		mUndoStack.push_back(std::move(action));
	}

	void Editor::UndoLastAction()
	{
		if (!mUndoStack.empty())
		{
			std::unique_ptr<edit::EditorAction> action = std::move(mUndoStack.back());

			mUndoStack.pop_back();

			action->Undo();

			mRedoStack.push_back(std::move(action));
		}
	}

	void Editor::RedoLastAction()
	{
		if (!mRedoStack.empty())
		{
			std::unique_ptr<edit::EditorAction> action = std::move(mRedoStack.back());

			mRedoStack.pop_back();

			action->Redo();

			mUndoStack.push_back(std::move(action));
		}
	}

	void Editor::Save()
	{
		printf("Save!\n");
	}

	SceneGraph& Editor::GetSceneGraph()
	{
		return mSceneGraph;
	}

	ecs::ECSCoordinator* Editor::GetECSCoordinator()
	{
		return mCoordinator;
	}

	r2::mem::MallocArena& Editor::GetMemoryArena()
	{
		return mMallocArena;
	}
}

#endif