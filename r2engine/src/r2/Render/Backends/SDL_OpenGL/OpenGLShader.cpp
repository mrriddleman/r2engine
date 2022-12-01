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
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Containers/SArray.h"
#include <string.h>

#include "r2/Utils/Hash.h"

//Don't love using these...
#include <string>
#include <regex>

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/ShaderManifest_generated.h"
#endif

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

    u32 GetMaxNumberOfGeometryShaderInvocations()
    {
        GLint maxInvocations;
        glGetIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS, &maxInvocations);

        return maxInvocations;
    }

    u32 CreateShaderProgramFromStrings(const r2::SArray<char*>* vertexShaderStrings, const r2::SArray<char*>* fragShaderStrings, const r2::SArray<char*>* geometryShaderStrings, const r2::SArray<char*>* computeShaderStrings)
    {
        if (r2::sarr::Size(*computeShaderStrings) == 0)
        {
            R2_CHECK(r2::sarr::Size(*vertexShaderStrings) > 0 && r2::sarr::Size(*fragShaderStrings) > 0, "Vertex and/or Fragment shader strings are nullptr");
        }

		GLuint shaderProgram = glCreateProgram();

		GLuint vertexShaderHandle;
		if (r2::sarr::Size(*vertexShaderStrings) > 0)
		{
			vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
		}

		GLuint fragmentShaderHandle;
		if (r2::sarr::Size(*fragShaderStrings) > 0)
		{
			fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
		}

		GLuint geometryShaderHandle;
		if (r2::sarr::Size(*geometryShaderStrings) > 0)
		{
			geometryShaderHandle = glCreateShader(GL_GEOMETRY_SHADER);
		}

		GLuint computeShaderHandle;
		if (r2::sarr::Size(*computeShaderStrings) > 0)
		{
			computeShaderHandle = glCreateShader(GL_COMPUTE_SHADER);
		}

		if (r2::sarr::Size(*vertexShaderStrings) > 0)
		{ // compile shader and check for errors
			glShaderSource(vertexShaderHandle, r2::sarr::Size(*vertexShaderStrings), r2::sarr::Begin(*vertexShaderStrings), NULL);
			glCompileShader(vertexShaderHandle);
			int lparams = -1;
			glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &lparams);

			if (GL_TRUE != lparams) {
				R2_LOGE("ERROR: vertex shader index %u did not compile\n", vertexShaderHandle);

				const int max_length = 2048;
				int actual_length = 0;
				char slog[2048];
				glGetShaderInfoLog(vertexShaderHandle, max_length, &actual_length, slog);
				R2_LOGE("Shader info log for GL index %u:\n%s\n", vertexShaderHandle,
					slog);

				glDeleteShader(vertexShaderHandle);
				glDeleteShader(fragmentShaderHandle);
				if (r2::sarr::Size(*geometryShaderStrings) > 0)
				{
					glDeleteShader(geometryShaderHandle);
				}
				glDeleteProgram(shaderProgram);
				return 0;
			}
		}

		if (r2::sarr::Size(*fragShaderStrings) > 0)
		{ // compile shader and check for errors
			glShaderSource(fragmentShaderHandle, r2::sarr::Size(*fragShaderStrings), r2::sarr::Begin(*fragShaderStrings), NULL);
			glCompileShader(fragmentShaderHandle);
			int lparams = -1;
			glGetShaderiv(fragmentShaderHandle, GL_COMPILE_STATUS, &lparams);

			if (GL_TRUE != lparams) {
				R2_LOGE("ERROR: fragment shader index %u did not compile\n",
					fragmentShaderHandle);

				const int max_length = 2048;
				int actual_length = 0;
				char slog[2048];
				glGetShaderInfoLog(fragmentShaderHandle, max_length, &actual_length, slog);
				R2_LOGE("Shader info log for GL index %u:\n%s\n", fragmentShaderHandle,
					slog);

				glDeleteShader(vertexShaderHandle);
				glDeleteShader(fragmentShaderHandle);

				if (r2::sarr::Size(*geometryShaderStrings) > 0)
				{
					glDeleteShader(geometryShaderHandle);
				}

				glDeleteProgram(shaderProgram);
				return 0;
			}
		}

		if (r2::sarr::Size(*geometryShaderStrings) > 0)
		{
			glShaderSource(geometryShaderHandle, r2::sarr::Size(*geometryShaderStrings), r2::sarr::Begin(*geometryShaderStrings), NULL);
			glCompileShader(geometryShaderHandle);
			int lparams = -1;
			glGetShaderiv(geometryShaderHandle, GL_COMPILE_STATUS, &lparams);

			if (GL_TRUE != lparams) {
				R2_LOGE("ERROR: geometry shader index %u did not compile\n", geometryShaderHandle);

				const int max_length = 2048;
				int actual_length = 0;
				char slog[2048];
				glGetShaderInfoLog(geometryShaderHandle, max_length, &actual_length, slog);
				R2_LOGE("Shader info log for GL index %u:\n%s\n", geometryShaderHandle,
					slog);

				glDeleteShader(vertexShaderHandle);
				glDeleteShader(fragmentShaderHandle);
				glDeleteShader(geometryShaderHandle);

				glDeleteProgram(shaderProgram);
				return 0;
			}
		}

		if (r2::sarr::Size(*computeShaderStrings) > 0)
		{
			glShaderSource(computeShaderHandle, r2::sarr::Size(*computeShaderStrings), r2::sarr::Begin(*computeShaderStrings), nullptr);
			glCompileShader(computeShaderHandle);
			int lparams = -1;
			glGetShaderiv(computeShaderHandle, GL_COMPILE_STATUS, &lparams);

			if (GL_TRUE != lparams)
			{
				R2_LOGE("ERROR: compute shader index %u did not compile\n", computeShaderHandle);

				const int max_length = 2048;
				int actual_length = 0;
				char slog[2048];
				glGetShaderInfoLog(computeShaderHandle, max_length, &actual_length, slog);
				R2_LOGE("Shader info log for GL index %u:\n%s\n", computeShaderHandle, slog);

				glDeleteShader(computeShaderHandle);
				glDeleteProgram(shaderProgram);

				return 0;
			}
		}

		if (r2::sarr::Size(*computeShaderStrings) == 0)
		{
			glAttachShader(shaderProgram, fragmentShaderHandle);
			glAttachShader(shaderProgram, vertexShaderHandle);
			if (r2::sarr::Size(*geometryShaderStrings) > 0)
			{
				glAttachShader(shaderProgram, geometryShaderHandle);
			}

			{ // link program and check for errors
				glLinkProgram(shaderProgram);
				glDeleteShader(vertexShaderHandle);
				glDeleteShader(fragmentShaderHandle);
				if (r2::sarr::Size(*geometryShaderStrings) > 0)
				{
					glDeleteShader(geometryShaderHandle);
				}
			}
		}
		else
		{
			glAttachShader(shaderProgram, computeShaderHandle);
			glLinkProgram(shaderProgram);
			glDeleteShader(computeShaderHandle);
		}

		int lparams = -1;
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &lparams);

		if (GL_TRUE != lparams)
		{
			R2_LOGE("ERROR: could not link shader program GL index %u\n",
				shaderProgram);

			const int max_length = 2048;
			int actual_length = 0;
			char plog[2048];
			glGetProgramInfoLog(shaderProgram, max_length, &actual_length, plog);
			R2_LOGE("Program info log for GL index %u:\n%s", shaderProgram, plog);

			glDeleteProgram(shaderProgram);
			return 0;
		}

        return shaderProgram;
    }
    
    u32 CreateShaderProgramFromStrings(const char* vertexShaderStr, const char* fragShaderStr, const char* geometryShaderStr, const char* computeShaderStr)
    {
        if (!computeShaderStr)
        {
            R2_CHECK(vertexShaderStr != nullptr && fragShaderStr != nullptr, "Vertex and/or Fragment shader are nullptr");
        }
        
        GLuint shaderProgram = glCreateProgram();

        GLuint vertexShaderHandle;
        if (vertexShaderStr)
        {
            vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
        }
        
        GLuint fragmentShaderHandle;
        if (fragShaderStr)
        {
            fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
        }
        
        GLuint geometryShaderHandle;
        if (geometryShaderStr)
        {
            geometryShaderHandle = glCreateShader(GL_GEOMETRY_SHADER);
        }
        
        GLuint computeShaderHandle;
        if (computeShaderStr)
        {
            computeShaderHandle = glCreateShader(GL_COMPUTE_SHADER);
        }

        if(vertexShaderStr != nullptr)
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
        
        if(fragShaderStr != nullptr)
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

        if (computeShaderStr != nullptr)
        {
            glShaderSource(computeShaderHandle, 1, &computeShaderStr, nullptr);
            glCompileShader(computeShaderHandle);
            int lparams = -1;
            glGetShaderiv(computeShaderHandle, GL_COMPILE_STATUS, &lparams);

            if (GL_TRUE != lparams)
            {
                R2_LOGE("ERROR: compute shader index %u did not compile\n", computeShaderHandle);

                const int max_length = 2048;
                int actual_length = 0;
                char slog[2048];
                glGetShaderInfoLog(computeShaderHandle, max_length, &actual_length, slog);
                R2_LOGE("Shader info log for GL index %u:\n%s\n", computeShaderHandle, slog);

                glDeleteShader(computeShaderHandle);
                glDeleteProgram(shaderProgram);

                return 0;
            }
        }

        if (computeShaderStr == nullptr)
        {
			glAttachShader(shaderProgram, fragmentShaderHandle);
			glAttachShader(shaderProgram, vertexShaderHandle);
			if (geometryShaderStr)
			{
				glAttachShader(shaderProgram, geometryShaderHandle);
			}

			{ // link program and check for errors
				glLinkProgram(shaderProgram);
				glDeleteShader(vertexShaderHandle);
				glDeleteShader(fragmentShaderHandle);
				if (geometryShaderStr)
				{
					glDeleteShader(geometryShaderHandle);
				}
			}
        }
        else
        {
            glAttachShader(shaderProgram, computeShaderHandle);
            glLinkProgram(shaderProgram);
            glDeleteShader(computeShaderHandle);
        }
        
		int lparams = -1;
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &lparams);

		if (GL_TRUE != lparams) 
        {
			R2_LOGE("ERROR: could not link shader program GL index %u\n",
				shaderProgram);

			const int max_length = 2048;
			int actual_length = 0;
			char plog[2048];
			glGetProgramInfoLog(shaderProgram, max_length, &actual_length, plog);
			R2_LOGE("Program info log for GL index %u:\n%s", shaderProgram, plog);

			glDeleteProgram(shaderProgram);
			return 0;
		}
        
        return shaderProgram;
    }

    char* ReadShaderData(const char* shaderFilePath)
    {
		r2::fs::File* shaderFile = r2::fs::FileSystem::Open(DISK_CONFIG, shaderFilePath, r2::fs::Read | r2::fs::Binary);

		R2_CHECK(shaderFile != nullptr, "Failed to open file: %s\n", shaderFilePath);
		if (!shaderFile)
		{
			R2_LOGE("Failed to open file: %s\n", shaderFilePath);
			return nullptr;
		}

		u64 fileSize = shaderFile->Size();

		char* shaderFileData = (char*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, fileSize + 1, sizeof(char));

		if (!shaderFileData)
		{
			R2_CHECK(false, "Could not allocate: %llu bytes", fileSize + 1);
			return nullptr;
		}

		bool success = shaderFile->ReadAll(shaderFileData);

		if (!success)
		{
			FREE(shaderFileData, *MEM_ENG_SCRATCH_PTR);
			R2_LOGE("Failed to read file: %s", shaderFilePath);
			return nullptr;
		}

        shaderFileData[fileSize] = '\0';
		r2::fs::FileSystem::Close(shaderFile);

        return shaderFileData;
    }

    void FindFullPathForShader(const char* shaderName, char * fullPath)
    {
        strcpy(fullPath, shadersystem::FindShaderPathByName(shaderName));
    }

    void ReadAndParseShaderData(u64 hashName, ShaderName shaderStage, const char* shaderFilePath, r2::SArray<char*>* shaderSourceFiles, r2::SArray<char*>& includedPaths, r2::SArray<void*>* tempAllocations)
	{
        char* shaderFileData = ReadShaderData(shaderFilePath);

        if (!shaderFileData)
        {
            R2_CHECK(false, "Failed to get the shader data for: %s", shaderFilePath);
            return;
        }
        
        r2::sarr::Push(*tempAllocations, (void*)shaderFileData);

        u32 lengthOfShaderFile = strlen(shaderFileData);

        char* shaderFileDataCopy = (char*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, lengthOfShaderFile + 1, sizeof(char));

        strcpy(shaderFileDataCopy, shaderFileData);

        r2::sarr::Push(*tempAllocations, (void*)shaderFileDataCopy);

		std::regex includeExpression("^#include \".*\"[ \t]*$");
		std::regex filePathExpression("\".*\"");
        std::regex versionExpression("^#version [0-9][0-9][0-9] core$");

        char* shaderParsedOutIncludes = (char*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, lengthOfShaderFile + 32 + 50 * fs::FILE_PATH_LENGTH, sizeof(char)); //+32 for each potential '\0' in the file
        shaderParsedOutIncludes[0] = '\0';
        u32 lengthOfParsedShaderData = 0;

        r2::sarr::Push(*tempAllocations, (void*)shaderParsedOutIncludes);

        char* saveptr1;
        char* pch = strtok_s(shaderFileDataCopy, "\r\n", &saveptr1);
       
        u32 currentOffset = 0;
        
        bool hasPassedVersion = false;
        bool hasInjectedDefines = false;

        while (pch != nullptr)
        {
            if (!hasPassedVersion && std::regex_match(pch, versionExpression))
            {
                u32 strLen = strlen(pch);

                strcat(&shaderParsedOutIncludes[currentOffset], pch);
                strcat(&shaderParsedOutIncludes[currentOffset], "\n");

                hasPassedVersion = true;
                lengthOfParsedShaderData += strLen + 1;


                pch = strtok_s(NULL, "\r\n", &saveptr1);
                continue;
            }

            if (hasPassedVersion && !hasInjectedDefines)
            {
				r2::SArray<const flat::MaterialShaderParam*>* materialShaderDefines = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::MaterialShaderParam*, 50); //I dunno how many?

                shadersystem::GetDefinesForShader(hashName, shaderStage, *materialShaderDefines);

                auto numDefines = r2::sarr::Size(*materialShaderDefines);

                for (u32 i = 0; i < numDefines; ++i)
                {
                    const flat::MaterialShaderParam* shaderDefine = r2::sarr::At(*materialShaderDefines, i);
                    u32 strLen = strlen(shaderDefine->value()->c_str());
                    const char* define = shaderDefine->value()->c_str();
                    strcat(&shaderParsedOutIncludes[currentOffset], define);
                    strcat(&shaderParsedOutIncludes[currentOffset], "\n");
                    lengthOfParsedShaderData += strLen + 1;
                }

                FREE(materialShaderDefines, *MEM_ENG_SCRATCH_PTR);

                u32 strLen = strlen(pch);

				strcat(&shaderParsedOutIncludes[currentOffset], pch);
				strcat(&shaderParsedOutIncludes[currentOffset], "\n");

                lengthOfParsedShaderData += strLen + 1;

				hasInjectedDefines = true;

				pch = strtok_s(NULL, "\r\n", &saveptr1);
				continue;
            }


            if (!std::regex_match(pch, includeExpression))
            {
                u32 strLen = strlen(pch);

                strcat(&shaderParsedOutIncludes[currentOffset], pch);

                strcat(&shaderParsedOutIncludes[currentOffset], "\n");

                lengthOfParsedShaderData += strLen + 1;

                pch = strtok_s(NULL, "\r\n", &saveptr1);
                continue;
            }

            std::smatch match;
            std::string pchString = std::string(pch);
            if (!std::regex_search(pchString, match, filePathExpression))
            {
                u32 strLen = strlen(pch);

				strcat(&shaderParsedOutIncludes[currentOffset], pch);

				strcat(&shaderParsedOutIncludes[currentOffset], "\n");

				lengthOfParsedShaderData += strLen + 1;

                pch = strtok_s(NULL, "\r\n", &saveptr1);
                continue;
            }

            std::string stringMatch(match[0]);

            char* quotelessPath = (char*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, stringMatch.size() - 1, sizeof(char));

            strncpy(quotelessPath, stringMatch.c_str() + 1, stringMatch.size()-2); //make sure not to include the quotes

            quotelessPath[stringMatch.size() - 2] = '\0';
            
            r2::sarr::Push(*tempAllocations, (void*)quotelessPath);

          //  bool success = fs::utils::CopyFileNameWithExtension(quotelessPath, quotelessPath);

       //     R2_CHECK(success, "Couldn't copy the filename!");

            const auto numIncludePaths = r2::sarr::Size(includedPaths);
            bool found = false;
            for (u64 i = 0; i < numIncludePaths; ++i)
            {
                const char* nextIncludePath = r2::sarr::At(includedPaths, i);
                if (strcmp(quotelessPath, nextIncludePath) == 0)
                {
                    found = true;
                    break;
                }
            }

            if (found)
            {
                pch = strtok_s(NULL, "\r\n", &saveptr1);
                continue;                
            }

#if defined(R2_ASSET_PIPELINE)
            shadersystem::AddShaderToShaderPartList(STRING_ID(quotelessPath), shaderFilePath);
#endif

            char* nextPiece = &shaderParsedOutIncludes[currentOffset];

            shaderParsedOutIncludes[lengthOfParsedShaderData] = '\0';
            lengthOfParsedShaderData++;
            currentOffset = lengthOfParsedShaderData;

            shaderParsedOutIncludes[currentOffset] = '\0';

            if (strlen(nextPiece) > 0)
            {
                r2::sarr::Push(*shaderSourceFiles, nextPiece);
            }

            r2::sarr::Push(includedPaths, quotelessPath);

            char fullIncludePath[fs::FILE_PATH_LENGTH];
            FindFullPathForShader(quotelessPath, fullIncludePath);

            R2_CHECK(strlen(fullIncludePath) > 0, "We should have a proper path here!");

            ReadAndParseShaderData(hashName, shaderStage, fullIncludePath, shaderSourceFiles, includedPaths, tempAllocations);

            pch = strtok_s(NULL, "\r\n", &saveptr1);
        }

        shaderParsedOutIncludes[lengthOfParsedShaderData] = '\0';

        if (shaderSourceFiles)
        {
            char* nextPiece = &shaderParsedOutIncludes[currentOffset];
            r2::sarr::Push(*shaderSourceFiles, nextPiece);

         //   printf("%s\n", nextPiece);
        }


	}


    void ClearTemporaryAllocations(r2::SArray<void*>* tempAllocations)
    {
        if (!tempAllocations)
            return;

		const auto numAllocations = r2::sarr::Size(*tempAllocations);

		for (s32 i = (s32)numAllocations - 1; i >= 0; i--)
		{
			FREE(r2::sarr::At(*tempAllocations, i), *MEM_ENG_SCRATCH_PTR);
		}

		r2::sarr::Clear(*tempAllocations);
		FREE(tempAllocations, *MEM_ENG_SCRATCH_PTR);
    }
    
    Shader CreateShaderProgramFromRawFiles(u64 hashName, const char* vertexShaderFilePath, const char* fragmentShaderFilePath, const char* geometryShaderFilePath, const char* computeShaderFilePath, const char* manifestBasePath, bool assertOnFailure)
    {
        Shader newShader;

        if (!computeShaderFilePath)
        {
			R2_CHECK(vertexShaderFilePath != nullptr, "Vertex shader is null");
			R2_CHECK(fragmentShaderFilePath != nullptr, "Fragment shader is null");
        }
        
        char* vertexFileData = nullptr;
        char* fragmentFileData = nullptr;
        char* geometryFileData = nullptr;
        char* computeFileData = nullptr;

        r2::SArray<void*>* tempAllocations = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, void*, 128);

        r2::SArray<char*>* vertexShaderParts = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char*, 32);

        r2::sarr::Push(*tempAllocations, (void*) vertexShaderParts);

        r2::SArray<char*>* fragmentShaderParts = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char*, 32);

        r2::sarr::Push(*tempAllocations, (void*)fragmentShaderParts);

        r2::SArray<char*>* geometryShaderParts = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char*, 32);

        r2::sarr::Push(*tempAllocations, (void*)geometryShaderParts);

        r2::SArray<char*>* computeShaderParts = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char*, 32);

        r2::sarr::Push(*tempAllocations, (void*)computeShaderParts);

        if(vertexShaderFilePath && strlen(vertexShaderFilePath) > 0)
        {

			const r2::ShaderManifests* shaderManifests = shadersystem::FindShaderManifestByFullPath(vertexShaderFilePath);
			char fileNameWithExtension[fs::FILE_PATH_LENGTH];
			fs::utils::GetRelativePath(shaderManifests->basePath()->c_str(), vertexShaderFilePath, fileNameWithExtension);

			auto shaderName = STRING_ID(fileNameWithExtension);

            r2::SArray<char*>* includePaths = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char*, 24);

            r2::sarr::Push(*tempAllocations, (void*)includePaths);

            ReadAndParseShaderData(hashName, shaderName, vertexShaderFilePath, vertexShaderParts, *includePaths, tempAllocations);

#ifdef R2_ASSET_PIPELINE
            r2::draw::shadersystem::AddShaderToShaderMap(shaderName, hashName);
#endif
        }
        
        if(fragmentShaderFilePath && strlen(fragmentShaderFilePath) > 0)
        {
			const r2::ShaderManifests* shaderManifests = shadersystem::FindShaderManifestByFullPath(fragmentShaderFilePath);
			char fileNameWithExtension[fs::FILE_PATH_LENGTH];
			fs::utils::GetRelativePath(shaderManifests->basePath()->c_str(), fragmentShaderFilePath, fileNameWithExtension);

            

			auto shaderName = STRING_ID(fileNameWithExtension);

			r2::SArray<char*>* includePaths = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char*, 24);

			r2::sarr::Push(*tempAllocations, (void*)includePaths);

            ReadAndParseShaderData(hashName, shaderName, fragmentShaderFilePath, fragmentShaderParts, *includePaths, tempAllocations);

			//if (std::string(fileNameWithExtension) == "AnimModel.fs")
			//{
			//	const auto numParts = r2::sarr::Size(*fragmentShaderParts);

			//	for (int i = 0; i < numParts; ++i)
			//	{
			//		printf("%s\n", r2::sarr::At(*fragmentShaderParts, i));
			//	}
			//}


#ifdef R2_ASSET_PIPELINE
			r2::draw::shadersystem::AddShaderToShaderMap(shaderName, hashName);
#endif
        }
        
        if(geometryShaderFilePath && strlen(geometryShaderFilePath) > 0)
        {
			const r2::ShaderManifests* shaderManifests = shadersystem::FindShaderManifestByFullPath(geometryShaderFilePath);
			char fileNameWithExtension[fs::FILE_PATH_LENGTH];
			fs::utils::GetRelativePath(shaderManifests->basePath()->c_str(), geometryShaderFilePath, fileNameWithExtension);

			auto shaderName = STRING_ID(fileNameWithExtension);

			r2::SArray<char*>* includePaths = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char*, 24);

			r2::sarr::Push(*tempAllocations, (void*)includePaths);

            ReadAndParseShaderData(hashName, shaderName, geometryShaderFilePath, geometryShaderParts, *includePaths, tempAllocations);

#ifdef R2_ASSET_PIPELINE
			r2::draw::shadersystem::AddShaderToShaderMap(shaderName, hashName);
#endif
        }

        if (computeShaderFilePath && strlen(computeShaderFilePath) > 0)
        {
			const r2::ShaderManifests* shaderManifests = shadersystem::FindShaderManifestByFullPath(computeShaderFilePath);
			char fileNameWithExtension[fs::FILE_PATH_LENGTH];
			fs::utils::GetRelativePath(shaderManifests->basePath()->c_str(), computeShaderFilePath, fileNameWithExtension);

			auto shaderName = STRING_ID(fileNameWithExtension);

			r2::SArray<char*>* includePaths = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char*, 24);

			r2::sarr::Push(*tempAllocations, (void*)includePaths);

			ReadAndParseShaderData(hashName, shaderName, computeShaderFilePath, computeShaderParts, *includePaths, tempAllocations);
			
#ifdef R2_ASSET_PIPELINE
			r2::draw::shadersystem::AddShaderToShaderMap(shaderName, hashName);
#endif
        }
        
        u32 shaderProg = CreateShaderProgramFromStrings(vertexShaderParts, fragmentShaderParts, geometryShaderParts, computeShaderParts);
        
        if (!shaderProg)
        {
            if (assertOnFailure)
            {
                if (computeShaderFilePath && strlen(computeShaderFilePath) > 0)
                {
                    R2_CHECK(false, "Failed to create the shader program for compute shader: %s\n", computeShaderFilePath);
                }
                else
                {
                    if (geometryShaderFilePath && strlen(geometryShaderFilePath) > 0)
                    {
                        R2_CHECK(false, "Failed to create the shader program for vertex shader: %s\nAND\nFragment shader: %s\nAND\nGeometry shader: %s\n", vertexShaderFilePath, fragmentShaderFilePath, geometryShaderFilePath);
                    }
                    else
                    {
                        R2_CHECK(false, "Failed to create the shader program for vertex shader: %s\nAND\nFragment shader: %s\n", vertexShaderFilePath, fragmentShaderFilePath);
                    }
                    
                }
            }
           
            if (computeShaderFilePath && strlen(computeShaderFilePath) > 0)
            {
                R2_LOGE("Failed to create the shader program for compute shader: %s\n", computeShaderFilePath);
            }
            else
            {
                if (geometryShaderFilePath && strlen(geometryShaderFilePath) > 0)
                {
                    R2_LOGE("Failed to create the shader program for vertex shader: %s\nAND\nFragment shader: %s\nAND\nGeometry shader: %s\n", vertexShaderFilePath, fragmentShaderFilePath, geometryShaderFilePath);
                }
                else
                {
                    R2_LOGE("Failed to create the shader program for vertex shader: %s\nAND\nFragment shader: %s\n", vertexShaderFilePath, fragmentShaderFilePath);
                }
                
            }
            ClearTemporaryAllocations(tempAllocations);
			
            
            return newShader;
        }
        
        newShader.shaderProg = shaderProg;
        newShader.shaderID = hashName;
#ifdef R2_ASSET_PIPELINE
        newShader.manifest.hashName = hashName;
        newShader.manifest.vertexShaderPath = vertexShaderFilePath;
        newShader.manifest.fragmentShaderPath = fragmentShaderFilePath;
        newShader.manifest.geometryShaderPath = geometryShaderFilePath;
        newShader.manifest.computeShaderPath = computeShaderFilePath;
#endif

        ClearTemporaryAllocations(tempAllocations);

        return newShader;
    }
    
    void ReloadShaderProgramFromRawFiles(u32* program, u64 hashName, const char* vertexShaderFilePath, const char* fragmentShaderFilePath, const char* geometryShaderFilePath, const char* computeShaderFilePath, const char* basePath)
    {
        R2_CHECK(program != nullptr, "Shader Program is nullptr");

        if (computeShaderFilePath != nullptr)
		{
			R2_CHECK(vertexShaderFilePath != nullptr, "vertex shader file path is nullptr");
			R2_CHECK(fragmentShaderFilePath != nullptr, "fragment shader file path is nullptr");
        }
        else
        {
            R2_CHECK(vertexShaderFilePath == nullptr, "vertex shader file path is not nullptr");
            R2_CHECK(fragmentShaderFilePath == nullptr, "fragment shader file path is not nullptr");
            R2_CHECK(geometryShaderFilePath == nullptr, "geometry shader file path is not nullptr");
        }

        Shader reloadedShaderProgram = CreateShaderProgramFromRawFiles(hashName, vertexShaderFilePath, fragmentShaderFilePath, geometryShaderFilePath, computeShaderFilePath, basePath, false);
        
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