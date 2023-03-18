#ifndef __RENDER_COMPONENT_H__
#define __RENDER_COMPONENT_H__

#include "r2/Render/Model/Material.h"
#include "r2/Render/Renderer/VertexBufferLayoutSystem.h"
#include "r2/Render/Renderer/RendererTypes.h"

namespace r2::ecs
{
	struct RenderComponent
	{
		r2::draw::PrimitiveType primitiveType;
		r2::draw::DrawParameters drawParameters;
		r2::draw::vb::GPUModelRefHandle gpuModelRefHandle;
		r2::SArray<r2::draw::MaterialHandle>* optrOverrideMaterials;
	};
}

#endif // __RENDER_COMPONENT_H__
