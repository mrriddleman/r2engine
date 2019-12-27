//
//  OpenGLShader.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//

#ifndef OpenGLShader_h
#define OpenGLShader_h

#include "glm/glm.hpp"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/ShaderManifest.h"
#endif

namespace r2::draw::opengl
{
    struct Shader
    {
        u32 shaderProg = 0;
#ifdef R2_ASSET_PIPELINE
        r2::asset::pln::ShaderManifest manifest;
#endif
        
        void UseShader();
        //U stand for uniform
        void SetUBool(const char* name, bool value) const;
        void SetUInt(const char* name, s32 value) const;
        void SetUFloat(const char* name, f32 value) const;
        void SetUVec2(const char* name, const glm::vec2& value) const;
        void SetUVec3(const char* name, const glm::vec3& value) const ;
        void SetUVec4(const char* name, const glm::vec4& value) const;
        void SetUMat3(const char* name, const glm::mat3& value) const;
        void SetUMat4(const char* name, const glm::mat4& value) const;
        void SetUIVec2(const char* name, const glm::ivec2& value) const;
        void SetUIVec3(const char* name, const glm::ivec3& value) const;
        void SetUIVec4(const char* name, const glm::ivec4& value) const;
    };
    
    u32 CreateShaderProgramFromStrings(const char* vertexShaderStr, const char* fragShaderStr);
    u32 CreateShaderProgramFromRawFiles(const char* vertexShaderFilePath, const char* fragmentShaderFilePath);
    void ReloadShaderProgramFromRawFiles(u32* program, const char* vertexShaderFilePath, const char* fragmentShaderFilePath);
}

#endif /* OpenGLShader_h */
