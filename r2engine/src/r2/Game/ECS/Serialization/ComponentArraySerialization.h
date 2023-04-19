#ifndef __COMPONENT_ARRAY_SERIALIZATION_H__
#define __COMPONENT_ARRAY_SERIALIZATION_H__

#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Logging/Log.h"
#include "r2/Game/ECS/Entity.h"

namespace flat
{
	class ComponentArrayData;
}

namespace flexbuffers
{
	class Builder;
}

namespace r2::ecs
{
	template<typename Component>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<Component>& components)
	{
		R2_CHECK(false, "You need to implement a serializer for this type");
	}

	template<typename Component>
	inline void DeSerializeComponentArray(r2::SArray<Component>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(false, "You need to implement a deserializer for this type");
	}


	template<typename Component>
	inline void CleanupDeserializeComponentArray(r2::SArray<Component>& components)
	{

	}


}


#endif // __COMPONENT_ARRAY_SERIALIZATION_H__
