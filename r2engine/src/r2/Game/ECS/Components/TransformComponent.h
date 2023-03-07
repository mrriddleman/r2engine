#ifndef __TRANSFORM_COMPONENT_H__
#define __TRANSFORM_COMPONENT_H__

#include "r2/Core/Math/Transform.h"

namespace r2::ecs
{
	struct TransformComponent
	{
		r2::math::Transform localTransform; //local only
		r2::math::Transform accumTransform; //parent + local?
		glm::mat4 modelMatrix; //full world matrix including parents
	};
}

#endif // __TRANSFORM_COMPONENT_H__
