#ifndef __TRANSFORM_WIDGET_INPUT_H__
#define __TRANSFORM_WIDGET_INPUT_H__

#ifdef R2_EDITOR

#include "r2/Utils/Utils.h"

namespace r2::evt
{
	class KeyPressedEvent;
}

namespace r2::edit
{
	bool DoImGuizmoOperationKeyInput(const r2::evt::KeyPressedEvent& keyEvent, u32& mCurrentOperation);
}

#endif

#endif // __TRANSFORM_WIDGET_INPUT_H__
