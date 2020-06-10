#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/Shader.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace r2::draw
{
	struct Material
	{
		u64 materialID = 0;
		ShaderHandle shaderId = InvalidShader;
		u64 texturePackHandle = 0;
		glm::vec4 color = glm::vec4(1.0f);
	};

	using MaterialHandle = u32;

	template<class ARENA>
	Material MakeMaterial(ARENA& arena, u64 numTextures, const char* file, s32 line, const char* description);

	template<class ARENA>
	void FreeMaterial(ARENA& arena, Material& material, const char* file, s32 line, const char* description);
}

//@TODO(Serge): make this not a singleton
namespace r2::draw::mat
{
	static const MaterialHandle InvalidMaterial;

	bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, const char* materialPackPath, const char* texturePacksManifestPath);
	
	//@TODO(Serge): add a progress function here
	void LoadAllMaterialTexturesFromDisk();
	//@TODO(Serge): add function to upload only 1 material?

	void UploadAllMaterialTexturesToGPU();
	void UploadMaterialTexturesToGPUFromMaterialName(u64 materialName);
	void UploadMaterialTexturesToGPU(MaterialHandle matID);
	void UnloadAllMaterialTexturesFromGPU();

	const r2::SArray<r2::draw::tex::Texture>* GetTexturesForMaterial(MaterialHandle matID);
	MaterialHandle AddMaterial(const Material& mat);
	const Material* GetMaterial(MaterialHandle matID);
	MaterialHandle GetMaterialHandleFromMaterialName(u64 materialName);
	void Shutdown();

	u64 MemorySize(u64 capacity, u64 textureCacheInBytes, u64 numTextures, u64 numPacks, u64 maxTexturesInAPack);
}

namespace r2::draw
{
	template<class ARENA>
	Material MakeMaterial(ARENA& arena, u64 numTextures, const char* file, s32 line, const char* description)
	{
		Material newMaterial;

		newMaterial = MAKE_SARRAY_VERBOSE(arena, r2::draw::tex::Texture, numTextures, file, line, description);

		return newMaterial
	}

	template<class ARENA>
	void FreeMaterial(ARENA& arena, Material& material, const char* file, s32 line, const char* description)
	{
		FREE_VERBOSE(material.textures, arena, file, line, description);
	}
}

#endif // !__MATERIAL_H__
