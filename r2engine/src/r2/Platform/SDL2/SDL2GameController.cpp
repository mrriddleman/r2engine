#include "r2pch.h"
#include "r2/Platform/SDL2/SDL2GameController.h"


namespace r2
{
	SDL2GameController::SDL2GameController()
		:mControllerIndex(-1)
		,mSDLGameController(nullptr)
	{
	}

	bool SDL2GameController::Open(s32 controllerIndex)
	{
		if (SDL_IsGameController(controllerIndex))
		{
			mSDLGameController = SDL_GameControllerOpen(controllerIndex);
			mControllerIndex = controllerIndex;
			return true;
		}

		return false;
	}

	void SDL2GameController::Close()
	{
		if (IsOpen())
		{
			SDL_GameControllerClose(mSDLGameController);
			mSDLGameController = nullptr;
			mControllerIndex = -1;
		}
	}

	bool SDL2GameController::IsOpen() const
	{
		return mSDLGameController != nullptr;
	}

	bool SDL2GameController::IsAttached() const
	{
		if (mSDLGameController)
		{
			return SDL_GameControllerGetAttached(mSDLGameController);
		}

		return false;
	}

	r2::io::ControllerInstanceID SDL2GameController::GetControllerInstanceID() const
	{
		if (mControllerIndex == -1 || mSDLGameController == nullptr)
		{
			return io::INVALID_CONTROLLER_ID;
		}

		SDL_Joystick* j = SDL_GameControllerGetJoystick(mSDLGameController);

		if (!j)
		{
			return io::INVALID_CONTROLLER_ID;
		}

		SDL_JoystickID joyID = SDL_JoystickInstanceID(j);

		return joyID;
	}

	const char* SDL2GameController::GetMapping() const
	{
		if (IsOpen())
		{
			return SDL_GameControllerMapping(mSDLGameController);
		}

		return "Unknown";
	}

	const char* SDL2GameController::GetControllerName() const
	{
		if (IsOpen())
		{
			return SDL_GameControllerName(mSDLGameController);
		}

		return "Unknown";
	}

	const char* SDL2GameController::GetButtonName(io::ControllerButtonName name)
	{
		return SDL_GameControllerGetStringForButton((SDL_GameControllerButton)name);
	}

	const char* SDL2GameController::GetAxisName(io::ControllerAxisName name)
	{
		return SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)name);
	}

	bool SDL2GameController::IsGameController(io::ControllerID controllerID)
	{
		return SDL_IsGameController(controllerID);
	}

	u32 SDL2GameController::GetNumberOfConnectedGameControllers()
	{
		return SDL_NumJoysticks();
	}

	s32 SDL2GameController::GetPlayerIndex() const
	{
		if (IsOpen())
		{
			return SDL_GameControllerGetPlayerIndex(mSDLGameController);
		}

		return -1;
	}

	void SDL2GameController::SetPlayerIndex(s32 playerIndex)
	{
		if (IsOpen())
		{
			SDL_GameControllerSetPlayerIndex(mSDLGameController, playerIndex);
		}
	}

	r2::io::ControllerType SDL2GameController::GetControllerType() const
	{
		if (IsOpen())
		{
			return static_cast<io::ControllerType>(SDL_GameControllerGetType(mSDLGameController));
		}
		
		return io::ControllerType::CONTROLLER_TYPE_UNKNOWN;
	}

}