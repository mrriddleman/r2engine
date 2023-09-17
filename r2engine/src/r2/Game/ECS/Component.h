#ifndef __COMPONENT_H__
#define __COMPONENT_H__

#include <bitset>

namespace r2::ecs
{
	static const u32 MAX_NUM_COMPONENTS = 128;
	using Signature = std::bitset<MAX_NUM_COMPONENTS>;
	using ComponentType = s32;
	using FreeComponentFunc = std::function<void(void* componentPtr)>;
}

#endif // __COMPONENT_H__
