#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/Shader.h"

namespace r2::draw
{
	struct Material
	{
		u64 materialID = 0;
		ShaderHandle shaderId = InvalidShader;
		u32 textureId = 0;
		glm::vec4 color = glm::vec4(1.0f);
	};

	using MaterialHandle = s32;

}

namespace r2::draw::mat
{
	static const MaterialHandle InvalidMaterial;

	bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 capacity);
	MaterialHandle AddMaterial(const Material& mat);
	const Material* GetMaterial(MaterialHandle matID);
	void Shutdown();

	u64 GetMemorySize(u64 capacity);
}

#endif // !__MATERIAL_H__
