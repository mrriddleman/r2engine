#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Core/Layer/EditorLayer.h"
#include "r2/Core/Events/Events.h"

namespace r2
{
	EditorLayer::EditorLayer()
		:Layer("Editor", true)
		,mEnabled(false)
	{

	}

	void EditorLayer::Init()
	{
		editor.Init();
	}

	void EditorLayer::Shutdown()
	{
		editor.Shutdown();
	}

	void EditorLayer::Update()
	{
		if (!mEnabled)
			return;

		editor.Update();
	}

	void EditorLayer::Render(float alpha)
	{
		if (!mEnabled)
			return;

		editor.Render();
	}

	void EditorLayer::ImGuiRender(u32 dockingSpaceID)
	{
		if (!mEnabled)
			return;

		editor.RenderImGui(dockingSpaceID);
	}

	void EditorLayer::OnEvent(evt::Event& event)
	{
		r2::evt::EventDispatcher dispatcher(event);

		dispatcher.Dispatch<r2::evt::KeyPressedEvent>([this](const r2::evt::KeyPressedEvent& e)
		{
			if (e.KeyCode() == r2::io::KEY_F1 && ((e.Modifiers() & r2::io::Key::SHIFT_PRESSED_KEY) == r2::io::Key::SHIFT_PRESSED_KEY))
			{
				mEnabled = !mEnabled;

				editor.ReloadLevel();

				if (mEnabled)
				{
					editor.SetRenderToEditorCamera();
				}

				return true;
			}

			return false; 
		});

		if (mEnabled)
		{
			editor.OnEvent(event);
		}
	}

	bool EditorLayer::IsEnabled() const
	{
		return mEnabled;
	}
}

#endif
