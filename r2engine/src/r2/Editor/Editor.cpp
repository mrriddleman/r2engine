#include "r2pch.h"
#if defined(R2_EDITOR) && defined(R2_IMGUI)

#include "r2/Core/Events/Events.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorMainMenuBar.h"
#include "r2/Editor/EditorInspectorPanel.h"
#include "imgui.h"

namespace r2
{
	Editor::Editor()
	{

	}

	void Editor::Init()
	{
		//Do all of the panels/widgets setup here
		std::unique_ptr<edit::MainMenuBar> mainMenuBar = std::make_unique<edit::MainMenuBar>();
		mEditorWidgets.push_back(std::move(mainMenuBar));

		std::unique_ptr<edit::InspectorPanel> inspectorPanel = std::make_unique<edit::InspectorPanel>();
		mEditorWidgets.push_back(std::move(inspectorPanel));

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
					printf("Undo\n");
					return true;
				}
				else if (e.KeyCode() == r2::io::KEY_z && 
					((e.Modifiers() & r2::io::Key::CONTROL_PRESSED) == r2::io::Key::CONTROL_PRESSED) &&
					((e.Modifiers() & r2::io::Key::SHIFT_PRESSED_KEY) == r2::io::Key::SHIFT_PRESSED_KEY))
				{
					printf("Redo\n");
					return true;
				}
				else if (e.KeyCode() == r2::io::KEY_s &&
					((e.Modifiers() & r2::io::Key::CONTROL_PRESSED) == r2::io::Key::CONTROL_PRESSED))
				{
					printf("Save\n");
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

	void Editor::Render(u32 dockingSpaceID)
	{
		for (const auto& widget : mEditorWidgets)
		{
			widget->Render(dockingSpaceID);
		}

		static bool show = false;
		ImGui::ShowDemoWindow(&show);
	}

}

#endif