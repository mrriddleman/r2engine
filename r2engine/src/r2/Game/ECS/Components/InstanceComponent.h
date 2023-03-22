#ifndef __INSTANCE_COMPONENT_H__
#define __INSTANCE_COMPONENT_H__

#include <glm/glm.hpp>
#include "r2/Core/Containers/SArray.h"

namespace r2::ecs
{
	//@IDEA: ugly, but, we could make this a template given another component...
	struct InstanceComponent
	{
		//@TODO(Serge): right now this doesn't do any rotation or scales for the instances which we should probably have at some point
		r2::SArray<glm::vec3>* offsets;
		r2::SArray<glm::mat4>* instanceModels;
	};
}

#endif // __INSTANCE_COMPONENT_H__
