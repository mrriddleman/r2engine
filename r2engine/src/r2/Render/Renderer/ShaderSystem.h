#ifndef __SHADER_SYSTEM_H__
#define __SHADER_SYSTEM_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/Shader.h"
#include "r2/Render/Model/Materials/MaterialParams_generated.h"
#include "r2/Render/Model/Materials/MaterialParamsPack_generated.h"

#ifdef R2_ASSET_PIPELINE
namespace r2::asset::pln
{
    struct ShaderManifest;
}
#endif

namespace r2
{
    struct ShaderManifests;
}

//@TODO(Serge): make this not a singleton?
namespace r2::draw::shadersystem
{
    
    bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 capacity, const char* shaderManifestPath, const char* internalShaderManifestPath, u32 numMaterialPacks);
    void Update();

    ShaderHandle AddShader(const r2::draw::Shader& shader);
    ShaderHandle FindShaderHandle(const r2::draw::Shader& shader);
    ShaderHandle FindShaderHandle(ShaderName shaderName);

    const Shader* GetShader(ShaderHandle handle);

    void Shutdown();

    u64 GetMemorySize(u64 capacity, u32 numMaterialPacks);

    const char* FindShaderPathByName(const char* name);
    
    void GetMaterialShaderPiecesForShader(ShaderName shaderName, ShaderName shaderStageName, r2::SArray<const flat::MaterialShaderParam*>& materialParts);
    void GetDefinesForShader(ShaderName shaderName, ShaderName shaderStageName, r2::SArray<const flat::MaterialShaderParam*>& defines);

#ifdef R2_ASSET_PIPELINE
    void ReloadShadersFromChangedPath(const std::string& path);
    void AddShaderToShaderPartList(const ShaderName& shaderPartName, const char* pathThatIncludedThePart);
    void AddShaderToShaderMap(const ShaderName& shaderNameInMap, const ShaderName& shaderName);
#endif
    const r2::ShaderManifests* FindShaderManifestByFullPath(const char* path);


}

#endif