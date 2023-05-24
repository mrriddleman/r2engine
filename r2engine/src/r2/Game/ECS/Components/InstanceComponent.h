#ifndef __INSTANCE_COMPONENT_H__
#define __INSTANCE_COMPONENT_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SArray.h"

namespace r2::ecs
{
	template<typename T>
	struct InstanceComponentT
	{
		u32 numInstances;
		r2::SArray<T>* instances;

		static u64 MemorySize(u32 numInstances, u32 alignment, u32 headerSize, u32 boundsChecking)
		{
			return r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<T>::MemorySize(numInstances), alignment, headerSize, boundsChecking);
		}

	};
}

#endif // __INSTANCE_COMPONENT_H__
