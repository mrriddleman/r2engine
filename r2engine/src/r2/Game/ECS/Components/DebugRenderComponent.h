#ifndef __DEBUG_RENDER_COMPONENT_H__
#define __DEBUG_RENDER_COMPONENT_H__

#ifdef R2_DEBUG

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Core/Containers/SArray.h"

namespace r2::ecs
{
	struct DebugRenderComponent
	{
		r2::draw::DebugModelType debugModelType;

		//@TODO(Serge): remove the SArrays here, this won't be the normal case

		//for Arrow this is headBaseRadius
		float radius;
		
		//for Arrow this is length
		//for Cone and Cylinder - this is height
		//for a line - this is length of the line
		float scale;
		
		//For Lines, this is used for the direction of p1 
		//we use the scale to determine the p1 of the line
		glm::vec3 direction;

		glm::vec4 color;
		b32 filled;
		b32 depthTest;
	};
}

#endif
#endif
