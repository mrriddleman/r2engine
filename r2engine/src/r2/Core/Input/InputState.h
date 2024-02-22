#ifndef __INPUT_STATE_H__
#define __INPUT_STATE_H__

#include "r2/Platform/Platform.h"
#include "r2/Platform/IO.h"

namespace r2::io
{
	struct InputState
	{
		MouseState mMouseState;

		KeyboardState mKeyboardState;

		ControllerState mControllersState[Engine::NUM_PLATFORM_CONTROLLERS];
		
		u32 numControllers = 0;
	};
}

#endif // 
