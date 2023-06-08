#ifndef __RENDER_COMPONENT_H__
#define __RENDER_COMPONENT_H__

#include "r2/Render/Model/Materials/MaterialTypes.h"
#include "r2/Render/Renderer/VertexBufferLayoutSystem.h"
#include "r2/Render/Renderer/RendererTypes.h"

namespace r2::ecs
{
	struct RenderComponent
	{
		u64 assetModelHash;
		u32 primitiveType;
		b32 isAnimated;
		r2::draw::DrawParameters drawParameters;
		r2::draw::vb::GPUModelRefHandle gpuModelRefHandle;
		r2::SArray<r2::mat::MaterialName>* optrMaterialOverrideNames;
	};
}

#endif // __RENDER_COMPONENT_H__
