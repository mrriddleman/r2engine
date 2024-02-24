#include "r2pch.h"
#include "r2/Core/Input/DefaultInputGather.h"

#include "r2/Core/Events/GameControllerEvent.h"
#include "r2/Core/Events/KeyEvent.h"
#include "r2/Core/Events/MouseEvent.h"

namespace r2::io
{

	DefaultInputGather::DefaultInputGather()
	{

	}

	const r2::io::InputState& DefaultInputGather::GetCurrentInputState() const
	{
		return mInputState;
	}

	void DefaultInputGather::OnEvent(r2::evt::Event& event)
	{
		r2::evt::EventDispatcher dispatcher(event);

		dispatcher.Dispatch<r2::evt::KeyPressedEvent>([this](const r2::evt::KeyPressedEvent& e) {

			ProcessKeyboardKey(e.KeyCode(), e.Repeated(), r2::io::BUTTON_PRESSED);
			return true;
		});

		dispatcher.Dispatch<r2::evt::KeyReleasedEvent>([this](const r2::evt::KeyReleasedEvent& e) {

			ProcessKeyboardKey(e.KeyCode(), false , r2::io::BUTTON_RELEASED);
			return true;
		});

		dispatcher.Dispatch<r2::evt::MouseButtonPressedEvent>([this](const r2::evt::MouseButtonPressedEvent& e) {

			ProcessMouseButton(e.MouseButton(), r2::io::BUTTON_PRESSED, e.NumClicks());
			return true;
		});

		dispatcher.Dispatch<r2::evt::MouseButtonReleasedEvent>([this](const r2::evt::MouseButtonReleasedEvent& e) {

			ProcessMouseButton(e.MouseButton(), r2::io::BUTTON_RELEASED, 0);
			return true;
		});

		dispatcher.Dispatch<r2::evt::MouseMovedEvent>([this](const r2::evt::MouseMovedEvent& e) {

			mInputState.mMouseState.mousePositionState.x = e.X();
			mInputState.mMouseState.mousePositionState.y = e.Y();

			return true;
		});

		dispatcher.Dispatch<r2::evt::MouseWheelEvent>([this](const r2::evt::MouseWheelEvent& e) {

			mInputState.mMouseState.mouseWheelState.direction = e.Direction();
			mInputState.mMouseState.mouseWheelState.x = e.XAmount();
			mInputState.mMouseState.mouseWheelState.y = e.YAmount();

			return true;
		});

		dispatcher.Dispatch<r2::evt::GameControllerAxisEvent>([this](const r2::evt::GameControllerAxisEvent& e) {

			ControllerID controllerID = e.GetControllerID();
			mInputState.mControllersState[controllerID].axes[e.AxisName()].value = e.AxisValue();

			return true;
		});

		dispatcher.Dispatch<r2::evt::GameControllerButtonEvent>([this](const r2::evt::GameControllerButtonEvent& e) {

			ControllerID controllerID = e.GetControllerID();
			if (mInputState.mControllersState[controllerID].buttons[e.ButtonName()].IsPressed() && e.ButtonState() == BUTTON_PRESSED)
			{
				mInputState.mControllersState[controllerID].buttons[e.ButtonName()].held = true;
			}
			else
			{
				mInputState.mControllersState[controllerID].buttons[e.ButtonName()].held = false;
			}

			mInputState.mControllersState[controllerID].buttons[e.ButtonName()].state = e.ButtonState();

			return true;
		});
	}

	void DefaultInputGather::ProcessKeyboardKey(s32 keyCode, b32 repeated, u8 state)
	{
		if (keyCode == KEY_RETURN)
		{
			mInputState.mKeyboardState.key_return.state = state;
			mInputState.mKeyboardState.key_return.repeated = repeated;
		}
		else if (keyCode == KEY_ESCAPE)
		{
			mInputState.mKeyboardState.key_escape.state = state;
			mInputState.mKeyboardState.key_escape.repeated = repeated;
		}
		else if (keyCode == KEY_BACKSPACE)
		{
			mInputState.mKeyboardState.key_backspace.state = state;
			mInputState.mKeyboardState.key_backspace.repeated = repeated;
		}
		else if (keyCode == KEY_TAB)
		{
			mInputState.mKeyboardState.key_tab.state = state;
			mInputState.mKeyboardState.key_tab.repeated = repeated;
		}
		else if (keyCode == KEY_SPACE)
		{
			mInputState.mKeyboardState.key_space.state = state;
			mInputState.mKeyboardState.key_space.repeated = repeated;
		}
		else if (keyCode == KEY_EXCLAIM)
		{
			mInputState.mKeyboardState.key_exclaim.state = state;
			mInputState.mKeyboardState.key_exclaim.repeated = repeated;
		}
		else if (keyCode == KEY_QUOTEDBL)
		{
			mInputState.mKeyboardState.key_quotedbl.state = state;
			mInputState.mKeyboardState.key_quotedbl.repeated = repeated;
		}
		else if (keyCode == KEY_HASH)
		{
			mInputState.mKeyboardState.key_hash.state = state;
			mInputState.mKeyboardState.key_hash.repeated = repeated;
		}
		else if (keyCode == KEY_PERCENT)
		{
			mInputState.mKeyboardState.key_percent.state = state;
			mInputState.mKeyboardState.key_percent.repeated = repeated;
		}
		else if (keyCode == KEY_DOLLAR)
		{
			mInputState.mKeyboardState.key_dollar.state = state;
			mInputState.mKeyboardState.key_dollar.repeated = repeated;
		}
		else if (keyCode == KEY_AMPERSAND)
		{
			mInputState.mKeyboardState.key_ampersand.state = state;
			mInputState.mKeyboardState.key_ampersand.repeated = repeated;
		}
		else if (keyCode == KEY_QUOTE)
		{
			mInputState.mKeyboardState.key_quote.state = state;
			mInputState.mKeyboardState.key_quote.repeated = repeated;
		}
		else if (keyCode == KEY_LEFTPAREN)
		{
			mInputState.mKeyboardState.key_leftparen.state = state;
			mInputState.mKeyboardState.key_leftparen.repeated = repeated;
		}
		else if (keyCode == KEY_RIGHTPAREN)
		{
			mInputState.mKeyboardState.key_rightparen.state = state;
			mInputState.mKeyboardState.key_rightparen.repeated = repeated;
		}
		else if (keyCode == KEY_ASTERISK)
		{
			mInputState.mKeyboardState.key_asterisk.state = state;
			mInputState.mKeyboardState.key_asterisk.repeated = repeated;
		}
		else if (keyCode == KEY_PLUS)
		{
			mInputState.mKeyboardState.key_plus.state = state;
			mInputState.mKeyboardState.key_plus.repeated = repeated;
		}
		else if (keyCode == KEY_COMMA)
		{
			mInputState.mKeyboardState.key_comma.state = state;
			mInputState.mKeyboardState.key_comma.repeated = repeated;
		}
		else if (keyCode == KEY_MINUS)
		{
			mInputState.mKeyboardState.key_minus.state = state;
			mInputState.mKeyboardState.key_minus.repeated = repeated;
		}
		else if (keyCode == KEY_PERIOD)
		{
			mInputState.mKeyboardState.key_period.state = state;
			mInputState.mKeyboardState.key_period.repeated = repeated;
		}
		else if (keyCode == KEY_SLASH)
		{
			mInputState.mKeyboardState.key_slash.state = state;
			mInputState.mKeyboardState.key_slash.repeated = repeated;
		}
		else if (keyCode == KEY_0)
		{
			mInputState.mKeyboardState.key_0.state = state;
			mInputState.mKeyboardState.key_0.repeated = repeated;
		}
		else if (keyCode == KEY_1)
		{
			mInputState.mKeyboardState.key_1.state = state;
			mInputState.mKeyboardState.key_1.repeated = repeated;
		}
		else if (keyCode == KEY_2)
		{
			mInputState.mKeyboardState.key_2.state = state;
			mInputState.mKeyboardState.key_2.repeated = repeated;
		}
		else if (keyCode == KEY_3)
		{
			mInputState.mKeyboardState.key_3.state = state;
			mInputState.mKeyboardState.key_3.repeated = repeated;
		}
		else if( keyCode == KEY_4)
		{
			mInputState.mKeyboardState.key_4.state = state;
			mInputState.mKeyboardState.key_4.repeated = repeated;
		}
		else if( keyCode == KEY_5)
		{
			mInputState.mKeyboardState.key_5.state = state;
			mInputState.mKeyboardState.key_5.repeated = repeated;
		}
		else if( keyCode == KEY_6)
		{
			mInputState.mKeyboardState.key_6.state = state;
			mInputState.mKeyboardState.key_6.repeated = repeated;
		}
		else if( keyCode == KEY_7)
		{
			mInputState.mKeyboardState.key_7.state = state;
			mInputState.mKeyboardState.key_7.repeated = repeated;
		}
		else if( keyCode == KEY_8)
		{
			mInputState.mKeyboardState.key_8.state = state;
			mInputState.mKeyboardState.key_8.repeated = repeated;
		}
		else if (keyCode == KEY_9) 
		{
			mInputState.mKeyboardState.key_9.state = state;
			mInputState.mKeyboardState.key_9.repeated = repeated;
		}
		else if (keyCode == KEY_COLON)
		{
			mInputState.mKeyboardState.key_colon.state = state;
			mInputState.mKeyboardState.key_colon.repeated = repeated;
		}
		else if (keyCode == KEY_SEMICOLON)
		{
			mInputState.mKeyboardState.key_semicolon.state = state;
			mInputState.mKeyboardState.key_semicolon.repeated = repeated;
		}
		else if (keyCode == KEY_LESS)
		{
			mInputState.mKeyboardState.key_less.state = state;
			mInputState.mKeyboardState.key_less.repeated = repeated;
		}
		else if (keyCode == KEY_EQUALS)
		{
			mInputState.mKeyboardState.key_equals.state = state;
			mInputState.mKeyboardState.key_equals.repeated = repeated;
		}
		else if (keyCode == KEY_GREATER)
		{
			mInputState.mKeyboardState.key_greater.state = state;
			mInputState.mKeyboardState.key_greater.repeated = repeated;
		}
		else if (keyCode == KEY_QUESTION)
		{
			mInputState.mKeyboardState.key_question.state = state;
			mInputState.mKeyboardState.key_question.repeated = repeated;
		}
		else if (keyCode == KEY_AT)
		{
			mInputState.mKeyboardState.key_at.state = state;
			mInputState.mKeyboardState.key_at.repeated = repeated;
		}
		else if (keyCode == KEY_LEFTBRACKET)
		{
			mInputState.mKeyboardState.key_leftbracket.state = state;
			mInputState.mKeyboardState.key_leftbracket.repeated = repeated;
		}
		else if (keyCode == KEY_BACKSLASH)
		{
			mInputState.mKeyboardState.key_backslash.state = state;
			mInputState.mKeyboardState.key_backslash.repeated = repeated;
		}
		else if (keyCode == KEY_RIGHTBRACKET)
		{
			mInputState.mKeyboardState.key_rightbracket.state = state;
			mInputState.mKeyboardState.key_rightbracket.repeated = repeated;
		}
		else if (keyCode == KEY_CARET)
		{
			mInputState.mKeyboardState.key_caret.state = state;
			mInputState.mKeyboardState.key_caret.repeated = repeated;
		}
		else if (keyCode == KEY_UNDERSCORE)
		{
			mInputState.mKeyboardState.key_underscore.state = state;
			mInputState.mKeyboardState.key_underscore.repeated = repeated;
		}
		else if (keyCode == KEY_BACKQUOTE)
		{
			mInputState.mKeyboardState.key_backquote.state = state;
			mInputState.mKeyboardState.key_backquote.repeated = repeated;
		}
		else if( keyCode == KEY_a)
		{
			mInputState.mKeyboardState.key_a.state = state;
			mInputState.mKeyboardState.key_a.repeated = repeated;
		}
		else if( keyCode == KEY_b)
		{
			mInputState.mKeyboardState.key_b.state = state;
			mInputState.mKeyboardState.key_b.repeated = repeated;
		}
		else if( keyCode == KEY_c)
		{
			mInputState.mKeyboardState.key_c.state = state;
			mInputState.mKeyboardState.key_c.repeated = repeated;
		}
		else if( keyCode == KEY_d)
		{
			mInputState.mKeyboardState.key_d.state = state;
			mInputState.mKeyboardState.key_d.repeated = repeated;
		}
		else if( keyCode == KEY_e)
		{
			mInputState.mKeyboardState.key_e.state = state;
			mInputState.mKeyboardState.key_e.repeated = repeated;
		}
		else if( keyCode == KEY_f)
		{
			mInputState.mKeyboardState.key_f.state = state;
			mInputState.mKeyboardState.key_f.repeated = repeated;
		}
		else if( keyCode == KEY_g)
		{
			mInputState.mKeyboardState.key_g.state = state;
			mInputState.mKeyboardState.key_g.repeated = repeated;
		}
		else if( keyCode == KEY_h)
		{
			mInputState.mKeyboardState.key_h.state = state;
			mInputState.mKeyboardState.key_h.repeated = repeated;
		}
		else if( keyCode == KEY_i)
		{
			mInputState.mKeyboardState.key_i.state = state;
			mInputState.mKeyboardState.key_i.repeated = repeated;
		}
		else if( keyCode == KEY_j)
		{
			mInputState.mKeyboardState.key_j.state = state;
			mInputState.mKeyboardState.key_j.repeated = repeated;
		}
		else if( keyCode == KEY_k)
		{
			mInputState.mKeyboardState.key_k.state = state;
			mInputState.mKeyboardState.key_k.repeated = repeated;
		}
		else if( keyCode == KEY_l)
		{
			mInputState.mKeyboardState.key_l.state = state;
			mInputState.mKeyboardState.key_l.repeated = repeated;
		}
		else if( keyCode == KEY_m)
		{
			mInputState.mKeyboardState.key_m.state = state;
			mInputState.mKeyboardState.key_m.repeated = repeated;
		}
		else if( keyCode == KEY_n)
		{
			mInputState.mKeyboardState.key_n.state = state;
			mInputState.mKeyboardState.key_n.repeated = repeated;
		}
		else if( keyCode == KEY_o)
		{
			mInputState.mKeyboardState.key_o.state = state;
			mInputState.mKeyboardState.key_o.repeated = repeated;
		}
		else if( keyCode == KEY_p)
		{
			mInputState.mKeyboardState.key_p.state = state;
			mInputState.mKeyboardState.key_p.repeated = repeated;
		}
		else if( keyCode == KEY_q)
		{
			mInputState.mKeyboardState.key_q.state = state;
			mInputState.mKeyboardState.key_q.repeated = repeated;
		}
		else if( keyCode == KEY_r)
		{
			mInputState.mKeyboardState.key_r.state = state;
			mInputState.mKeyboardState.key_r.repeated = repeated;
		}
		else if( keyCode == KEY_s)
		{
			mInputState.mKeyboardState.key_s.state = state;
			mInputState.mKeyboardState.key_s.repeated = repeated;
		}
		else if( keyCode == KEY_t)
		{
			mInputState.mKeyboardState.key_t.state = state;
			mInputState.mKeyboardState.key_t.repeated = repeated;
		}
		else if( keyCode == KEY_u)
		{
			mInputState.mKeyboardState.key_u.state = state;
			mInputState.mKeyboardState.key_u.repeated = repeated;
		}
		else if( keyCode == KEY_v)
		{
			mInputState.mKeyboardState.key_v.state = state;
			mInputState.mKeyboardState.key_v.repeated = repeated;
		}
		else if( keyCode == KEY_w)
		{
			mInputState.mKeyboardState.key_w.state = state;
			mInputState.mKeyboardState.key_w.repeated = repeated;
		}
		else if( keyCode == KEY_x)
		{
			mInputState.mKeyboardState.key_x.state = state;
			mInputState.mKeyboardState.key_x.repeated = repeated;
		}
		else if( keyCode == KEY_y)
		{
			mInputState.mKeyboardState.key_y.state = state;
			mInputState.mKeyboardState.key_y.repeated = repeated;
		}
		else if (keyCode == KEY_z) 
		{
			mInputState.mKeyboardState.key_z.state = state;
			mInputState.mKeyboardState.key_z.repeated = repeated;
		}
		else if( keyCode == KEY_CAPSLOCK)
		{
			mInputState.mKeyboardState.key_capslock.state = state;
			mInputState.mKeyboardState.key_capslock.repeated = repeated;
		}
		else if( keyCode == KEY_F1)
		{
			mInputState.mKeyboardState.key_f1.state = state;
			mInputState.mKeyboardState.key_f1.repeated = repeated;
		}
		else if( keyCode == KEY_F2)
		{
			mInputState.mKeyboardState.key_f2.state = state;
			mInputState.mKeyboardState.key_f2.repeated = repeated;
		}
		else if( keyCode == KEY_F3)
		{
			mInputState.mKeyboardState.key_f3.state = state;
			mInputState.mKeyboardState.key_f3.repeated = repeated;
		}
		else if( keyCode == KEY_F4)
		{
			mInputState.mKeyboardState.key_f4.state = state;
			mInputState.mKeyboardState.key_f4.repeated = repeated;
		}
		else if( keyCode == KEY_F5)
		{
			mInputState.mKeyboardState.key_f5.state = state;
			mInputState.mKeyboardState.key_f5.repeated = repeated;
		}
		else if( keyCode == KEY_F6)
		{
			mInputState.mKeyboardState.key_f6.state = state;
			mInputState.mKeyboardState.key_f6.repeated = repeated;
		}
		else if( keyCode == KEY_F7)
		{
			mInputState.mKeyboardState.key_f7.state = state;
			mInputState.mKeyboardState.key_f7.repeated = repeated;
		}
		else if( keyCode == KEY_F8)
		{
			mInputState.mKeyboardState.key_f8.state = state;
			mInputState.mKeyboardState.key_f8.repeated = repeated;
		}
		else if (keyCode == KEY_F9)
		{
			mInputState.mKeyboardState.key_f9.state = state;
			mInputState.mKeyboardState.key_f9.repeated = repeated;
		}
		else if( keyCode == KEY_F10)
		{
			mInputState.mKeyboardState.key_f10.state = state;
			mInputState.mKeyboardState.key_f10.repeated = repeated;
		}
		else if( keyCode == KEY_F11)
		{
			mInputState.mKeyboardState.key_f11.state = state;
			mInputState.mKeyboardState.key_f11.repeated = repeated;
		}
		else if (keyCode == KEY_F12)
		{
			mInputState.mKeyboardState.key_f12.state = state;
			mInputState.mKeyboardState.key_f12.repeated = repeated;
		}
		else if (keyCode == KEY_PRINTSCREEN)
		{
			mInputState.mKeyboardState.key_printscreen.state = state;
			mInputState.mKeyboardState.key_printscreen.repeated = repeated;
		}
		else if (keyCode == KEY_SCROLLLOCK)
		{
			mInputState.mKeyboardState.key_scrolllock.state = state;
			mInputState.mKeyboardState.key_scrolllock.repeated = repeated;
		}
		else if (keyCode == KEY_PAUSE)
		{
			mInputState.mKeyboardState.key_pause.state = state;
			mInputState.mKeyboardState.key_pause.repeated = repeated;
		}
		else if (keyCode == KEY_INSERT)
		{
			mInputState.mKeyboardState.key_insert.state = state;
			mInputState.mKeyboardState.key_insert.repeated = repeated;
		}
		else if (keyCode == KEY_HOME)
		{
			mInputState.mKeyboardState.key_home.state = state;
			mInputState.mKeyboardState.key_home.repeated = repeated;
		}
		else if (keyCode == KEY_PAGEUP)
		{
			mInputState.mKeyboardState.key_pageup.state = state;
			mInputState.mKeyboardState.key_pageup.repeated = repeated;
		}
		else if (keyCode == KEY_DELETE)
		{
			mInputState.mKeyboardState.key_delete.state = state;
			mInputState.mKeyboardState.key_delete.repeated = repeated;
		}
		else if (keyCode == KEY_END)
		{
			mInputState.mKeyboardState.key_end.state = state;
			mInputState.mKeyboardState.key_end.repeated = repeated;
		}
		else if (keyCode == KEY_PAGEDOWN)
		{
			mInputState.mKeyboardState.key_pagedown.state = state;
			mInputState.mKeyboardState.key_pagedown.repeated = repeated;
		}
		else if (keyCode == KEY_RIGHT)
		{
			mInputState.mKeyboardState.key_right.state = state;
			mInputState.mKeyboardState.key_right.repeated = repeated;
		}
		else if (keyCode == KEY_LEFT)
		{
			mInputState.mKeyboardState.key_left.state = state;
			mInputState.mKeyboardState.key_left.repeated = repeated;
		}
		else if (keyCode == KEY_DOWN)
		{
			mInputState.mKeyboardState.key_down.state = state;
			mInputState.mKeyboardState.key_down.repeated = repeated;
		}		
		else if (keyCode == KEY_UP)
		{
			mInputState.mKeyboardState.key_up.state = state;
			mInputState.mKeyboardState.key_up.repeated = repeated;
		}
	}

	void DefaultInputGather::ProcessMouseButton(s8 mouseButton, s8 state, u8 numClicks)
	{
		if (mouseButton == r2::io::MOUSE_BUTTON_LEFT)
		{
			mInputState.mMouseState.leftMouseButton.state = state;
			mInputState.mMouseState.leftMouseButton.numClicks = numClicks;
		}
		else if (mouseButton == r2::io::MOUSE_BUTTON_MIDDLE)
		{
			mInputState.mMouseState.middleMouseButton.state = state;
			mInputState.mMouseState.middleMouseButton.numClicks = numClicks;
		}
		else if (mouseButton == r2::io::MOUSE_BUTTON_RIGHT)
		{
			mInputState.mMouseState.rightMouseButton.state = state;
			mInputState.mMouseState.rightMouseButton.numClicks = numClicks;
		}
	}

}