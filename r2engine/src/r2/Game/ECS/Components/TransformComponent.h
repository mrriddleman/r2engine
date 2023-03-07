#ifndef __TRANSFORM_COMPONENT_H__
#define __TRANSFORM_COMPONENT_H__

#include "r2/Core/Math/Transform.h"

namespace r2::ecs
{
	struct TransformComponent
	{
		r2::math::Transform transform;
		glm::mat4 modelMatrix;
	};
}

#endif // __TRANSFORM_COMPONENT_H__
