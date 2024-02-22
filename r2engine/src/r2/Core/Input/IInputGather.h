#ifndef __IINPUT_GATHER_H__
#define __IINPUT_GATHER_H__

namespace r2::evt
{
	class Event;
}

namespace r2::io
{
	struct InputState;

	class IInputGather
	{
	public:
		virtual ~IInputGather() {};
		virtual const InputState& GetCurrentInputState() = 0;
		virtual void OnEvent(r2::evt::Event& event) = 0;
	};
}

#endif