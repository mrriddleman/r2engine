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

		//for Arrow this is headBaseRadius
		float radius;
		
		//for Arrow this is length
		//for Cone and Cylinder - this is height
		//for a line - this is length of the line
		glm::vec3 scale;
		
		//For Lines, this is used for the direction of p1 
		//we use the scale to determine the p1 of the line
		//for Quads, this will be the normal
		glm::vec3 direction;

		//some offset to give the position of the render debug component
		glm::vec3 offset;

		glm::vec4 color;
		b32 filled;
		b32 depthTest;
	};
}

#endif
#endif
