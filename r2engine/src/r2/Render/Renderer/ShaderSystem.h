#ifndef __SHADER_SYSTEM_H__
#define __SHADER_SYSTEM_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/Shader.h"


#ifdef R2_ASSET_PIPELINE
namespace r2::asset::pln
{
    struct ShaderManifest;
}

namespace r2
{
    struct ShaderManifests;
}
#endif // R2_As

//@TODO(Serge): make this not a singleton?
namespace r2::draw::shadersystem
{
    
    bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 capacity, const char* shaderManifestPath, const char* internalShaderManifestPath);
    void Update();

    ShaderHandle AddShader(const r2::draw::Shader& shader);
    ShaderHandle FindShaderHandle(const r2::draw::Shader& shader);
    ShaderHandle FindShaderHandle(ShaderName shaderName);

    const Shader* GetShader(ShaderHandle handle);

    void Shutdown();

    u64 GetMemorySize(u64 capacity);

    const char* FindShaderPathByName(const char* name);
    
#ifdef R2_ASSET_PIPELINE
    void ReloadManifestFile(const std::string& manifestFilePath);
 //   void ReloadShader(const r2::asset::pln::ShaderManifest& manifest, bool isPartPath );
    void ReloadShadersFromChangedPath(const std::string& path);
    void AddShaderToShaderPartList(const ShaderName& shaderPartName, const char* pathThatIncludedThePart);
    void AddShaderToShaderMap(const ShaderName& shaderNameInMap, const ShaderName& shaderName);

    const r2::ShaderManifests* FindShaderManifestByFullPath(const char* path);
#endif

}

#endif