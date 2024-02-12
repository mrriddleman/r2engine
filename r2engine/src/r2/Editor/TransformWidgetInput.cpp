#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Editor/TransformWidgetInput.h"
#include "r2/Core/Events/KeyEvent.h"
#include "imgui.h"
#include "ImGuizmo.h"


namespace r2::edit
{

	bool DoImGuizmoOperationKeyInput(const r2::evt::KeyPressedEvent& keyEvent, u32& currentOperation)
	{

		if (keyEvent.KeyCode() == r2::io::KEY_g)
		{
			//translation
			currentOperation = ImGuizmo::TRANSLATE;
			return true;
		}
		else if (keyEvent.KeyCode() == r2::io::KEY_r)
		{
			//rotate
			currentOperation = ImGuizmo::ROTATE;
			return true;
		}
		else if (keyEvent.KeyCode() == r2::io::KEY_e)
		{
			//scale
			currentOperation = ImGuizmo::SCALE;
			return true;
		}
		else if (keyEvent.KeyCode() == r2::io::KEY_x && ((keyEvent.Modifiers() & r2::io::ALT_PRESSED) != r2::io::ALT_PRESSED))
		{
			if (currentOperation == ImGuizmo::TRANSLATE ||
				currentOperation == ImGuizmo::TRANSLATE_X ||
				currentOperation == ImGuizmo::TRANSLATE_Y ||
				currentOperation == ImGuizmo::TRANSLATE_Z ||
				currentOperation == (ImGuizmo::TRANSLATE_Y | ImGuizmo::TRANSLATE_Z))
			{
				currentOperation = ImGuizmo::TRANSLATE_X;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::SCALE ||
				currentOperation == ImGuizmo::SCALE_X ||
				currentOperation == ImGuizmo::SCALE_Y ||
				currentOperation == ImGuizmo::SCALE_Z ||
				currentOperation == (ImGuizmo::SCALE_Y | ImGuizmo::SCALE_Z))
			{
				currentOperation = ImGuizmo::SCALE_X;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::ROTATE ||
				currentOperation == ImGuizmo::ROTATE_X ||
				currentOperation == ImGuizmo::ROTATE_Y ||
				currentOperation == ImGuizmo::ROTATE_Z ||
				currentOperation == (ImGuizmo::ROTATE_Y | ImGuizmo::ROTATE_Z))
			{
				currentOperation = ImGuizmo::ROTATE_X;
				return true;
			}
		}
		else if (keyEvent.KeyCode() == r2::io::KEY_z && ((keyEvent.Modifiers() & r2::io::ALT_PRESSED) != r2::io::ALT_PRESSED))
		{
			if (currentOperation == ImGuizmo::TRANSLATE ||
				currentOperation == ImGuizmo::TRANSLATE_X ||
				currentOperation == ImGuizmo::TRANSLATE_Y ||
				currentOperation == ImGuizmo::TRANSLATE_Z ||
				currentOperation == (ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Y))
			{
				currentOperation = ImGuizmo::TRANSLATE_Z;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::SCALE ||
				currentOperation == ImGuizmo::SCALE_X ||
				currentOperation == ImGuizmo::SCALE_Y ||
				currentOperation == ImGuizmo::SCALE_Z ||
				currentOperation == (ImGuizmo::SCALE_X | ImGuizmo::SCALE_Y))
			{
				currentOperation = ImGuizmo::SCALE_Z;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::ROTATE ||
				currentOperation == ImGuizmo::ROTATE_X ||
				currentOperation == ImGuizmo::ROTATE_Y ||
				currentOperation == ImGuizmo::ROTATE_Z ||
				currentOperation == (ImGuizmo::ROTATE_X | ImGuizmo::ROTATE_Y))
			{
				currentOperation = ImGuizmo::ROTATE_Z;
				return true;
			}
		}
		else if (keyEvent.KeyCode() == r2::io::KEY_y && ((keyEvent.Modifiers() & r2::io::ALT_PRESSED) != r2::io::ALT_PRESSED))
		{
			if (currentOperation == ImGuizmo::TRANSLATE ||
				currentOperation == ImGuizmo::TRANSLATE_X ||
				currentOperation == ImGuizmo::TRANSLATE_Y ||
				currentOperation == ImGuizmo::TRANSLATE_Z ||
				currentOperation == (ImGuizmo::TRANSLATE_Z | ImGuizmo::TRANSLATE_X))
			{
				currentOperation = ImGuizmo::TRANSLATE_Y;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::SCALE ||
				currentOperation == ImGuizmo::SCALE_X ||
				currentOperation == ImGuizmo::SCALE_Y ||
				currentOperation == ImGuizmo::SCALE_Z ||
				currentOperation == (ImGuizmo::SCALE_Z | ImGuizmo::SCALE_X))
			{
				currentOperation = ImGuizmo::SCALE_Y;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::ROTATE ||
				currentOperation == ImGuizmo::ROTATE_X ||
				currentOperation == ImGuizmo::ROTATE_Y ||
				currentOperation == ImGuizmo::ROTATE_Z ||
				currentOperation == (ImGuizmo::ROTATE_Z | ImGuizmo::ROTATE_X))
			{
				currentOperation = ImGuizmo::ROTATE_Y;
				return true;
			}
		}
		else if (keyEvent.KeyCode() == r2::io::KEY_x && ((keyEvent.Modifiers() & r2::io::ALT_PRESSED) == r2::io::ALT_PRESSED))
		{
			if (currentOperation == ImGuizmo::TRANSLATE ||
				currentOperation == ImGuizmo::TRANSLATE_X ||
				currentOperation == ImGuizmo::TRANSLATE_Y ||
				currentOperation == ImGuizmo::TRANSLATE_Z ||
				currentOperation == (ImGuizmo::TRANSLATE_Y | ImGuizmo::TRANSLATE_Z) ||
				currentOperation == (ImGuizmo::TRANSLATE_Y | ImGuizmo::TRANSLATE_X) ||
				currentOperation == (ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Z))
			{
				currentOperation = ImGuizmo::TRANSLATE_Y | ImGuizmo::TRANSLATE_Z;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::SCALE ||
				currentOperation == ImGuizmo::SCALE_X ||
				currentOperation == ImGuizmo::SCALE_Y ||
				currentOperation == ImGuizmo::SCALE_Z ||
				currentOperation == (ImGuizmo::SCALE_Y | ImGuizmo::SCALE_Z) ||
				currentOperation == (ImGuizmo::SCALE_Y | ImGuizmo::SCALE_X) ||
				currentOperation == (ImGuizmo::SCALE_X | ImGuizmo::SCALE_Z))
			{
				currentOperation = ImGuizmo::SCALE_Y | ImGuizmo::SCALE_Z;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::ROTATE ||
				currentOperation == ImGuizmo::ROTATE_X ||
				currentOperation == ImGuizmo::ROTATE_Y ||
				currentOperation == ImGuizmo::ROTATE_Z ||
				currentOperation == (ImGuizmo::ROTATE_Y | ImGuizmo::ROTATE_Z) ||
				currentOperation == (ImGuizmo::ROTATE_Y | ImGuizmo::ROTATE_X) ||
				currentOperation == (ImGuizmo::ROTATE_X | ImGuizmo::ROTATE_Z))
			{
				currentOperation = ImGuizmo::ROTATE_Y | ImGuizmo::ROTATE_Z;
				return true;
			}

		}
		else if (keyEvent.KeyCode() == r2::io::KEY_y && ((keyEvent.Modifiers() & r2::io::ALT_PRESSED) == r2::io::ALT_PRESSED))
		{
			if (currentOperation == ImGuizmo::TRANSLATE ||
				currentOperation == ImGuizmo::TRANSLATE_X ||
				currentOperation == ImGuizmo::TRANSLATE_Y ||
				currentOperation == ImGuizmo::TRANSLATE_Z ||
				currentOperation == (ImGuizmo::TRANSLATE_Y | ImGuizmo::TRANSLATE_Z) ||
				currentOperation == (ImGuizmo::TRANSLATE_Y | ImGuizmo::TRANSLATE_X) ||
				currentOperation == (ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Z))
			{
				currentOperation = ImGuizmo::TRANSLATE_Z | ImGuizmo::TRANSLATE_X;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::SCALE ||
				currentOperation == ImGuizmo::SCALE_X ||
				currentOperation == ImGuizmo::SCALE_Y ||
				currentOperation == ImGuizmo::SCALE_Z ||
				currentOperation == (ImGuizmo::SCALE_Y | ImGuizmo::SCALE_Z) ||
				currentOperation == (ImGuizmo::SCALE_Y | ImGuizmo::SCALE_X) ||
				currentOperation == (ImGuizmo::SCALE_X | ImGuizmo::SCALE_Z))
			{
				currentOperation = ImGuizmo::SCALE_Z | ImGuizmo::SCALE_X;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::ROTATE ||
				currentOperation == ImGuizmo::ROTATE_X ||
				currentOperation == ImGuizmo::ROTATE_Y ||
				currentOperation == ImGuizmo::ROTATE_Z ||
				currentOperation == (ImGuizmo::ROTATE_Y | ImGuizmo::ROTATE_Z) ||
				currentOperation == (ImGuizmo::ROTATE_Y | ImGuizmo::ROTATE_X) ||
				currentOperation == (ImGuizmo::ROTATE_X | ImGuizmo::ROTATE_Z))
			{
				currentOperation = ImGuizmo::ROTATE_Z | ImGuizmo::ROTATE_X;
				return true;
			}
		}
		else if (keyEvent.KeyCode() == r2::io::KEY_z && ((keyEvent.Modifiers() & r2::io::ALT_PRESSED) == r2::io::ALT_PRESSED))
		{
			if (currentOperation == ImGuizmo::TRANSLATE ||
				currentOperation == ImGuizmo::TRANSLATE_X ||
				currentOperation == ImGuizmo::TRANSLATE_Y ||
				currentOperation == ImGuizmo::TRANSLATE_Z ||
				currentOperation == (ImGuizmo::TRANSLATE_Y | ImGuizmo::TRANSLATE_Z) ||
				currentOperation == (ImGuizmo::TRANSLATE_Y | ImGuizmo::TRANSLATE_X) ||
				currentOperation == (ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Z))
			{
				currentOperation = ImGuizmo::TRANSLATE_Y | ImGuizmo::TRANSLATE_X;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::SCALE ||
				currentOperation == ImGuizmo::SCALE_X ||
				currentOperation == ImGuizmo::SCALE_Y ||
				currentOperation == ImGuizmo::SCALE_Z ||
				currentOperation == (ImGuizmo::SCALE_Y | ImGuizmo::SCALE_Z) ||
				currentOperation == (ImGuizmo::SCALE_Y | ImGuizmo::SCALE_X) ||
				currentOperation == (ImGuizmo::SCALE_X | ImGuizmo::SCALE_Z))
			{
				currentOperation = ImGuizmo::SCALE_Y | ImGuizmo::SCALE_X;
				return true;
			}
			else if (
				currentOperation == ImGuizmo::ROTATE ||
				currentOperation == ImGuizmo::ROTATE_X ||
				currentOperation == ImGuizmo::ROTATE_Y ||
				currentOperation == ImGuizmo::ROTATE_Z ||
				currentOperation == (ImGuizmo::ROTATE_Y | ImGuizmo::ROTATE_Z) ||
				currentOperation == (ImGuizmo::ROTATE_Y | ImGuizmo::ROTATE_X) ||
				currentOperation == (ImGuizmo::ROTATE_X | ImGuizmo::ROTATE_Z))
			{
				currentOperation = ImGuizmo::ROTATE_Y | ImGuizmo::ROTATE_X;
				return true;
			}
		}

		return false;
	}

}

#endif