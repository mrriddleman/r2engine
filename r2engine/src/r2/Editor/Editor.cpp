#include "r2pch.h"
#if defined(R2_EDITOR) && defined(R2_IMGUI)

#include "r2/Core/Events/Events.h"
#include "r2/Editor/Editor.h"

#include "imgui.h"

namespace r2
{

	Editor::Editor()
	{

	}

	void Editor::Init()
	{

	}

	void Editor::Shutdown()
	{

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

				return false;
			});
	}

	void Editor::Update()
	{

	}

	void Editor::Render()
	{
		static bool show = false;
	    ImGui::ShowDemoWindow(&show);
	}

}

#endif