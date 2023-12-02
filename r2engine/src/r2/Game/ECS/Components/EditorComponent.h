#ifndef __EDITOR_NAME_COMPONENT_H__
#define __EDITOR_NAME_COMPONENT_H__

#include <string>
#include "r2/Utils/Flags.h"

namespace r2::ecs
{
#if defined R2_EDITOR
	enum eEditorFlags : u32
	{
		EDITOR_FLAG_SHOW_ENTITY = 1 << 0,
		EDITOR_FLAG_ENABLE_ENTITY = 1 << 1,
		EDITOR_FLAG_SELECTED_ENTITY = 1 << 2
	};
#endif

	struct EditorComponent
	{
		std::string editorName;
		Flags<u32, u32> flags;
	};
}


#endif // __EDITOR_NAME_COMPONENT_H__
