#ifndef __COMPONENT_ARRAY_SERIALIZATION_H__
#define __COMPONENT_ARRAY_SERIALIZATION_H__

#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Logging/Log.h"
#include "flatbuffers/flexbuffers.h"

namespace r2::ecs
{
	template<typename Component>
	void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<Component>& components)
	{
		R2_CHECK(false, "You need to implement a serializer for this type");
	}
}


#endif // __COMPONENT_ARRAY_SERIALIZATION_H__
