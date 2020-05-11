#ifndef __SHADER_SYSTEM_H__
#define __SHADER_SYSTEM_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/Shader.h"


#ifdef R2_ASSET_PIPELINE
namespace r2::asset::pln
{
    struct ShaderManifest;
}
#endif // R2_As


namespace r2::draw::shadersystem
{
    
    bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 capacity, const char* shaderManifestPath);
    void Update();

    ShaderHandle AddShader(const r2::draw::Shader& shader);
    ShaderHandle FindShaderHandle(const r2::draw::Shader& shader);
    ShaderHandle FindShaderHandle(u64 shaderName);

    const Shader* GetShader(ShaderHandle handle);

    void Shutdown();

    u64 GetMemorySize(u64 capacity);

#ifdef R2_ASSET_PIPELINE
    void ReloadManifestFile(const std::string& manifestFilePath);
    void ReloadShader(const r2::asset::pln::ShaderManifest& manifest);
#endif

}

#endif