#ifndef __SDL2_GAME_CONTROLLER_H__
#define __SDL2_GAME_CONTROLLER_H__

#include "r2/Utils/Utils.h"
#include "r2/Platform/IO.h"
#include <SDL2/SDL.h>

namespace r2
{
	class SDL2GameController
	{
	public:
		SDL2GameController();

		bool Open(s32 controllerIndex);
		void Close();
		bool IsOpen() const;

		bool IsAttached() const;
		io::ControllerInstanceID GetControllerInstanceID() const;
		inline io::ControllerID GetControllerID() const { return mControllerIndex; }
		const char* GetMapping() const;
		const char* GetControllerName() const;
		static const char* GetButtonName(io::ControllerButtonName name);
		static const char* GetAxisName(io::ControllerAxisName name);
		static bool IsGameController(io::ControllerID controllerID);
		static u32 GetNumberOfConnectedGameControllers();

		s32 GetPlayerIndex() const;
		void SetPlayerIndex(s32 playerIndex);
		io::ControllerType GetControllerType()const;


	private:

		s32 mControllerIndex;
		SDL_GameController* mSDLGameController;
	};
}

#endif