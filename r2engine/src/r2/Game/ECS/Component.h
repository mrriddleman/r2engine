#ifndef __COMPONENT_H__
#define __COMPONENT_H__

#include <bitset>

namespace r2::ecs
{
	static const u32 MAX_NUM_COMPONENTS = 64;
	using Signature = std::bitset<MAX_NUM_COMPONENTS>;
}

#endif // __COMPONENT_H__
