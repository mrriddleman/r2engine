#ifndef __INPUT_STATE_H__
#define __INPUT_STATE_H__


#include "r2/Platform/IO.h"

namespace r2::io
{
	struct InputState
	{
		MouseState mMouseState;

		KeyboardState mKeyboardState;

		ControllerState mControllersState[8];
	};
}

#endif // 
