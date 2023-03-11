#ifndef __EDITOR_EVENT_H__
#define __EDITOR_EVENT_H__

#ifdef R2_EDITOR

#include "r2/Core/Events/Event.h"

namespace r2::evt
{
	class EditorEvent : public Event
	{
	public:

		inline bool ShouldConsume() const { return mShouldConsume; }
		EVENT_CLASS_CATEGORY(ECAT_EDITOR)
	protected:
		EditorEvent(bool shouldConsume)
			:mShouldConsume(shouldConsume)
		{
		}
		bool mShouldConsume;
	};
}

#endif
#endif

