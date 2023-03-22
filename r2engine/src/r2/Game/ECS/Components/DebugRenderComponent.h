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
		r2::SArray<float>* radii;
		
		//for Arrow this is length
		//for Cone and Cylinder - this is height
		//for a line - this is length of the line
		r2::SArray<float>* scales;
		
		//For Lines, this is used for the direction of p1 
		//we use the scale to determine the p1 of the line
		r2::SArray<glm::vec3>* directions;

		r2::SArray<glm::vec4>* colors;
		b32 filled;
		b32 depthTest;
	};
}

#endif
#endif
