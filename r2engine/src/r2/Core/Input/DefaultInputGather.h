#ifndef __DEFAULT_INPUT_GATHER_H__
#define __DEFAULT_INPUT_GATHER_H__

#include "r2/Core/Input/IInputGather.h"
#include "r2/Core/Input/InputState.h"

namespace r2::io
{
	class DefaultInputGather : public IInputGather
	{
	public:
		DefaultInputGather();
		
		virtual const InputState& GetCurrentInputState() const override;
		virtual void OnEvent(r2::evt::Event& event) override;

		void SetControllerIDConnected(r2::io::ControllerID controllerID, r2::io::ControllerInstanceID instanceID);

	private:

		void ProcessKeyboardKey(s32 keyCode, b32 repeated, u8 state);
		void ProcessMouseButton(s8 mouseButton, s8 state, u8 numClicks);

		InputState mInputState;
	};
}

#endif