#ifndef __SHADER_H__
#define __SHADER_H__

#include "r2/Render/Renderer/RendererTypes.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/ShaderManifest.h"
#endif

namespace r2::draw
{
    enum eMeshPass : u16
    {
        MP_FORWARD = 0,
        MP_TRANSPARENT,
        MP_DEPTH,
        MP_SHADOW_DIRECTIONAL,
        MP_SHADOW_POINT,
        MP_SHADOW_SPOTLIGHT,
        //Maybe add more here in the future
        NUM_MESH_PASSES
    };

    enum eShaderEffectType : u8
    {
        SET_STATIC = 0,
        SET_DYNAMIC,
        NUM_SHADER_EFFECT_TYPES
    };

    using ShaderHandle = u64;
    using ShaderName = u64;
    static const ShaderHandle InvalidShader = 0;

    struct ShaderEffect
    {
        ShaderHandle staticShaderHandle = InvalidShader;
        ShaderHandle dynamicShaderHandle = InvalidShader;
    };

    struct ShaderEffectPasses
    {
        ShaderEffect meshPasses[NUM_MESH_PASSES];
    };

    struct Shader
    {
        //maybe have a hash here?
        ShaderName shaderID = 0;
        u32 shaderProg = 0;
        //@TODO(Serge): may have to be moved out of the asset pipeline so that materials can compare
#ifdef R2_ASSET_PIPELINE
        r2::asset::pln::ShaderManifest manifest;
#endif
    };

    namespace shader
    {
        void Use(const Shader& shader);
        void SetBool(const Shader& shader, const char* name, bool value);
        void SetInt(const Shader& shader, const char* name, s32 value);
        void SetFloat(const Shader& shader, const char* name, f32 value);
        void SetVec2(const Shader& shader, const char* name, const glm::vec2& value);
        void SetVec3(const Shader& shader, const char* name, const glm::vec3& value);
        void SetVec4(const Shader& shader, const char* name, const glm::vec4& value);
        void SetMat3(const Shader& shader, const char* name, const glm::mat3& value);
        void SetMat4(const Shader& shader, const char* name, const glm::mat4& value);
        void SetIVec2(const Shader& shader, const char* name, const glm::ivec2& value);
        void SetIVec3(const Shader& shader, const char* name, const glm::ivec3& value);
        void SetIVec4(const Shader& shader, const char* name, const glm::ivec4& value);

        void Delete(Shader& shader);
        bool IsValid(const Shader& shader);


        u32 GetMaxNumberOfGeometryShaderInvocations();

        //u32 CreateShaderProgramFromStrings(const char* vertexShaderStr, const char* fragShaderStr, const char* geometryShaderStr, const char* computeShaderStr);
        
        //@TODO(Serge): move this to shader system
        Shader CreateShaderProgramFromRawFiles(
            u64 hashName,
            const char* vertexShaderFilePath,
            const char* fragmentShaderFilePath,
            const char* geometryShaderFilePath,
            const char* computeShaderFilePath,
            const char* manifestBasePath,
            bool assertOnFail);
        
        //@TODO(Serge): maybe move this to the shader system and load through the shader handle
        void ReloadShaderProgramFromRawFiles(u32* program, u64 hashName, const char* vertexShaderFilePath, const char* fragmentShaderFilePath, const char* geometryShaderFilePath, const char* computeShaderFilePath, const char* basePath);
    }


}

#endif