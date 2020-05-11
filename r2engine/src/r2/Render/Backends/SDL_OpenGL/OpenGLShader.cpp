//
//  OpenGLShader.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//
#include "r2pch.h"

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Render/Renderer/Shader.h"
#include "r2/Render/Renderer/ShaderSystem.h"
#include "glad/glad.h"
#include "glm/gtc/type_ptr.hpp"

//For loading shader files
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/File.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Containers/SArray.h"

namespace r2::draw::shader
{
    const ShaderHandle InvalidShader = 0;

    void Use(const Shader& shader)
    {
        glUseProgram(shader.shaderProg);
    }
    
    //U stand for uniform
    void SetBool(const Shader& shader, const char* name, bool value)
    {
        glUniform1i(glGetUniformLocation(shader.shaderProg, name), (s32)value);
    }
    
    void SetInt(const Shader& shader, const char* name, s32 value)
    {
        glUniform1i(glGetUniformLocation(shader.shaderProg, name), (s32)value);
    }
    
    void SetFloat(const Shader& shader, const char* name, f32 value)
    {
        glUniform1f(glGetUniformLocation(shader.shaderProg, name), value);
    }
    
    void SetVec2(const Shader& shader, const char* name, const glm::vec2& value)
    {
        glUniform2fv(glGetUniformLocation(shader.shaderProg, name), 1, glm::value_ptr(value));
    }
    
    void SetVec3(const Shader& shader, const char* name, const glm::vec3& value)
    {
        glUniform3fv(glGetUniformLocation(shader.shaderProg, name), 1, glm::value_ptr(value));
    }
    
    void SetVec4(const Shader& shader, const char* name, const glm::vec4& value)
    {
        glUniform4fv(glGetUniformLocation(shader.shaderProg, name), 1, glm::value_ptr(value));
    }
    
    void SetMat3(const Shader& shader, const char* name, const glm::mat3& value)
    {
        glUniformMatrix3fv(glGetUniformLocation(shader.shaderProg, name), 1, GL_FALSE, glm::value_ptr(value));
    }
    
    void SetMat4(const Shader& shader, const char* name, const glm::mat4& value)
    {
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProg, name), 1, GL_FALSE, glm::value_ptr(value));
    }
    
    void SetIVec2(const Shader& shader, const char* name, const glm::ivec2& value)
    {
        glUniform2iv(glGetUniformLocation(shader.shaderProg, name), 1, glm::value_ptr(value));
    }
    
    void SetIVec3(const Shader& shader, const char* name, const glm::ivec3& value)
    {
        glUniform3iv(glGetUniformLocation(shader.shaderProg, name), 1, glm::value_ptr(value));
    }
    
    void SetIVec4(const Shader& shader, const char* name, const glm::ivec4& value)
    {
        glUniform4iv(glGetUniformLocation(shader.shaderProg, name), 1, glm::value_ptr(value));
    }
    
    void Delete(Shader& shader)
    {
        if (IsValid(shader))
        {
            glDeleteShader(shader.shaderProg);
            shader.shaderProg = InvalidShader;
        }
    }

    bool IsValid(const Shader& shader)
    {
        return shader.shaderProg != InvalidShader;
    }
    
    u32 CreateShaderProgramFromStrings(const char* vertexShaderStr, const char* fragShaderStr, const char* geometryShaderStr)
    {
        R2_CHECK(vertexShaderStr != nullptr && fragShaderStr != nullptr, "Vertex and/or Fragment shader are nullptr");
        
        GLuint shaderProgram          = glCreateProgram();
        GLuint vertexShaderHandle     = glCreateShader( GL_VERTEX_SHADER );
        GLuint fragmentShaderHandle   = glCreateShader( GL_FRAGMENT_SHADER );
        
        GLuint geometryShaderHandle;
        if (geometryShaderStr)
        {
            geometryShaderHandle = glCreateShader(GL_GEOMETRY_SHADER);
        }
        
        { // compile shader and check for errors
            glShaderSource( vertexShaderHandle, 1, &vertexShaderStr, NULL );
            glCompileShader( vertexShaderHandle );
            int lparams = -1;
            glGetShaderiv( vertexShaderHandle, GL_COMPILE_STATUS, &lparams );
            
            if ( GL_TRUE != lparams ) {
                R2_LOGE("ERROR: vertex shader index %u did not compile\n", vertexShaderHandle);
                
                const int max_length = 2048;
                int actual_length    = 0;
                char slog[2048];
                glGetShaderInfoLog( vertexShaderHandle, max_length, &actual_length, slog );
                R2_LOGE("Shader info log for GL index %u:\n%s\n", vertexShaderHandle,
                        slog );
                
                glDeleteShader( vertexShaderHandle );
                glDeleteShader( fragmentShaderHandle );
                if (geometryShaderStr)
                {
                    glDeleteShader( geometryShaderHandle );
                }
                glDeleteProgram( shaderProgram );
                return 0;
            }
            
        }
        
        { // compile shader and check for errors
            glShaderSource( fragmentShaderHandle, 1, &fragShaderStr, NULL );
            glCompileShader( fragmentShaderHandle );
            int lparams = -1;
            glGetShaderiv( fragmentShaderHandle, GL_COMPILE_STATUS, &lparams );
            
            if ( GL_TRUE != lparams ) {
                R2_LOGE("ERROR: fragment shader index %u did not compile\n",
                        fragmentShaderHandle );
                
                const int max_length = 2048;
                int actual_length    = 0;
                char slog[2048];
                glGetShaderInfoLog( fragmentShaderHandle, max_length, &actual_length, slog );
                R2_LOGE("Shader info log for GL index %u:\n%s\n", fragmentShaderHandle,
                        slog );
                
                glDeleteShader( vertexShaderHandle );
                glDeleteShader( fragmentShaderHandle );
                
                if (geometryShaderStr)
                {
                    glDeleteShader( geometryShaderHandle );
                }
                
                glDeleteProgram( shaderProgram );
                return 0;
            }
            
        }
        
        if (geometryShaderStr != nullptr)
        {
            glShaderSource( geometryShaderHandle, 1, &geometryShaderStr, NULL );
            glCompileShader( geometryShaderHandle );
            int lparams = -1;
            glGetShaderiv( geometryShaderHandle, GL_COMPILE_STATUS, &lparams );
            
            if ( GL_TRUE != lparams ) {
                R2_LOGE("ERROR: geometry shader index %u did not compile\n", geometryShaderHandle);
                
                const int max_length = 2048;
                int actual_length    = 0;
                char slog[2048];
                glGetShaderInfoLog( geometryShaderHandle, max_length, &actual_length, slog );
                R2_LOGE("Shader info log for GL index %u:\n%s\n", geometryShaderHandle,
                        slog );
                
                glDeleteShader( vertexShaderHandle );
                glDeleteShader( fragmentShaderHandle );
                glDeleteShader( geometryShaderHandle );
                
                glDeleteProgram( shaderProgram );
                return 0;
            }
        }
        
        glAttachShader( shaderProgram, fragmentShaderHandle );
        glAttachShader( shaderProgram, vertexShaderHandle );
        if (geometryShaderStr)
        {
            glAttachShader( shaderProgram, geometryShaderHandle );
        }
        
        { // link program and check for errors
            glLinkProgram( shaderProgram );
            glDeleteShader( vertexShaderHandle );
            glDeleteShader( fragmentShaderHandle );
            if (geometryShaderStr)
            {
                glDeleteShader( geometryShaderHandle );
            }
            
            int lparams = -1;
            glGetProgramiv( shaderProgram, GL_LINK_STATUS, &lparams );
            
            if ( GL_TRUE != lparams ) {
                R2_LOGE("ERROR: could not link shader program GL index %u\n",
                        shaderProgram );
                
                const int max_length = 2048;
                int actual_length    = 0;
                char plog[2048];
                glGetProgramInfoLog( shaderProgram, max_length, &actual_length, plog );
                R2_LOGE("Program info log for GL index %u:\n%s", shaderProgram, plog );
                
                glDeleteProgram( shaderProgram );
                return 0;
            }
        }
        
        return shaderProgram;
    }
    
    Shader CreateShaderProgramFromRawFiles(u64 hashName, const char* vertexShaderFilePath, const char* fragmentShaderFilePath, const char* geometryShaderFilePath)
    {
        Shader newShader;
        R2_CHECK(vertexShaderFilePath != nullptr, "Vertex shader is null");
        R2_CHECK(fragmentShaderFilePath != nullptr, "Fragment shader is null");
        
        char* vertexFileData = nullptr;
        char* fragmentFileData = nullptr;
        char* geometryFileData = nullptr;
        
        {
            r2::fs::File* vertexFile = r2::fs::FileSystem::Open(DISK_CONFIG, vertexShaderFilePath, r2::fs::Read | r2::fs::Binary);
            
            R2_CHECK(vertexFile != nullptr, "Failed to open file: %s\n", vertexShaderFilePath);
            if(!vertexFile)
            {
                R2_LOGE("Failed to open file: %s\n", vertexShaderFilePath);
                return newShader;
            }
            
            u64 fileSize = vertexFile->Size();
            
            vertexFileData = (char*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, fileSize+1, sizeof(char));
            
            if(!vertexFileData)
            {
                R2_CHECK(false, "Could not allocate: %llu bytes", fileSize+1);
                return newShader;
            }
            
            bool success = vertexFile->ReadAll(vertexFileData);
            
            if(!success)
            {
                FREE(vertexFileData, *MEM_ENG_SCRATCH_PTR);
                R2_LOGE("Failed to read file: %s", vertexShaderFilePath);
                return newShader;
            }
            
            vertexFileData[fileSize] = '\0';
            r2::fs::FileSystem::Close(vertexFile);
        }
        
        {
            r2::fs::File* fragmentFile = r2::fs::FileSystem::Open(DISK_CONFIG, fragmentShaderFilePath, r2::fs::Read | r2::fs::Binary);
            
            R2_CHECK(fragmentFile != nullptr, "Failed to open file: %s\n", fragmentShaderFilePath);
            if(!fragmentFile)
            {
                R2_LOGE("Failed to open file: %s\n", fragmentShaderFilePath);
                return newShader;
            }
            
            u64 fileSize = fragmentFile->Size();
            
            fragmentFileData = (char*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, fileSize+1, sizeof(char));
            
            if(!fragmentFileData)
            {
                R2_CHECK(false, "Could not allocate: %llu bytes", fileSize+1);
                return newShader;
            }
            
            bool success = fragmentFile->ReadAll(fragmentFileData);
            
            if(!success)
            {
                FREE(fragmentFileData, *MEM_ENG_SCRATCH_PTR);
                R2_LOGE("Failed to read file: %s", fragmentShaderFilePath);
                return newShader;
            }
            
            fragmentFileData[fileSize] = '\0';
            r2::fs::FileSystem::Close(fragmentFile);
        }
        
        if(geometryShaderFilePath != nullptr && strlen(geometryShaderFilePath) > 0)
        {
            r2::fs::File* geometryFile = r2::fs::FileSystem::Open(DISK_CONFIG, geometryShaderFilePath, r2::fs::Read | r2::fs::Binary);
            
            R2_CHECK(geometryFile != nullptr, "Failed to open file: %s\n", geometryShaderFilePath);
            if(!geometryFile)
            {
                R2_LOGE("Failed to open file: %s\n", geometryShaderFilePath);
                return newShader;
            }
            
            u64 fileSize = geometryFile->Size();
            
            geometryFileData = (char*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, fileSize+1, sizeof(char));
            
            if(!geometryFileData)
            {
                R2_CHECK(false, "Could not allocate: %llu bytes", fileSize+1);
                return newShader;
            }
            
            bool success = geometryFile->ReadAll(geometryFileData);
            
            if(!success)
            {
                FREE(geometryFileData, *MEM_ENG_SCRATCH_PTR);
                R2_LOGE("Failed to read file: %s", geometryShaderFilePath);
                return newShader;
            }
            
            geometryFileData[fileSize] = '\0';
            r2::fs::FileSystem::Close(geometryFile);
        }
        
        u32 shaderProg = CreateShaderProgramFromStrings(vertexFileData, fragmentFileData, geometryFileData);
        
        if (geometryFileData != nullptr)
        {
            FREE(geometryFileData, *MEM_ENG_SCRATCH_PTR);
        }
        
        FREE(fragmentFileData, *MEM_ENG_SCRATCH_PTR);//fragment first since it was allocated last - this is a stack currently
        FREE(vertexFileData, *MEM_ENG_SCRATCH_PTR);
        
        if (!shaderProg)
        {
            R2_CHECK(false, "Failed to create the shader program for vertex shader: %s\nAND\nFragment shader: %s\n", vertexShaderFilePath, fragmentShaderFilePath);
            
            R2_LOGE("Failed to create the shader program for vertex shader: %s\nAND\nFragment shader: %s\n", vertexShaderFilePath, fragmentShaderFilePath);
            return newShader;
        }
        
        newShader.shaderProg = shaderProg;
        newShader.shaderID = hashName;
#ifdef R2_ASSET_PIPELINE
        newShader.manifest.hashName = hashName;
        newShader.manifest.vertexShaderPath = vertexShaderFilePath;
        newShader.manifest.fragmentShaderPath = fragmentShaderFilePath;
        newShader.manifest.geometryShaderPath = geometryShaderFilePath;
#endif

        return newShader;
    }
    
    void ReloadShaderProgramFromRawFiles(u32* program, u64 hashName, const char* vertexShaderFilePath, const char* fragmentShaderFilePath, const char* geometryShaderFilePath)
    {
        R2_CHECK(program != nullptr, "Shader Program is nullptr");
        R2_CHECK(vertexShaderFilePath != nullptr, "vertex shader file path is nullptr");
        R2_CHECK(fragmentShaderFilePath != nullptr, "fragment shader file path is nullptr");
        
        Shader reloadedShaderProgram = CreateShaderProgramFromRawFiles(hashName, vertexShaderFilePath, fragmentShaderFilePath, geometryShaderFilePath);
        
        if (reloadedShaderProgram.shaderProg)
        {
            if(*program != 0)
            {
                glDeleteProgram(*program);
            }
            
            *program = reloadedShaderProgram.shaderProg;
        }
    }

   
}



#endif