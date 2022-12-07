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
#ifdef R2_DEBUG
    struct ShaderLineOffset
    {
        size_t shaderLine = 0;
        size_t offset = 0;
    };

    struct ShaderDebugInfo
    {
        std::string globalShaderFilename = "";
        std::string shaderPartFilename = "";
        size_t globalLineStart = 0;
        size_t numLines = 0;
        size_t includeLines = 0;

        std::vector<ShaderLineOffset> shaderLineOffsets;
    };
    
    //0 - vertex
    //1 - fragment
    //2 - geometry
    //3 - compute
    std::unordered_map<std::string, ShaderDebugInfo> g_shaderDebugInfo[4];
#endif

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

    void ShowOpenGLShaderError(GLuint shaderHandle, size_t debugInfoIndex)
    {
#ifdef R2_DEBUG
		const int max_length = 2048;
		
        int actual_length = 0;
		
        char slog[2048];
		
        glGetShaderInfoLog(shaderHandle, max_length, &actual_length, slog);
		
        R2_LOGE("Shader info log for GL index %u:\n%s\n", shaderHandle, slog);

        struct ShaderError
        {
            std::string lineNumber;
            std::string errorCode;
            std::string errorString;
        };

        std::vector<ShaderError> shaderErrors;

        static std::regex lineExpression("^[0-9]*\\([0-9]+\\)");
        static std::regex errorCodeExpression("error\\s[A-Za-z]*[0-9]*");
        static std::regex errorDescExpression("^\\s*:\\s");
		std::smatch match;
        std::string slogString = slog;
        while (std::regex_search(slogString, match, lineExpression))
        {
            ShaderError shaderError;

            R2_CHECK(match.size() == 1, "Should only have 1");
            for (auto x : match)
            {
                size_t indexOfOpenParen = x.str().find_first_of('(');
                size_t indexOfCloseParen = x.str().find_first_of(')');

                shaderError.lineNumber = x.str().substr(indexOfOpenParen + 1, indexOfCloseParen - indexOfOpenParen - 1);
            }

            std::smatch errorCodeMatch;
            std::string restOfLine = match.suffix().str();
            if (std::regex_search(restOfLine, errorCodeMatch, errorCodeExpression))
            {
                R2_CHECK(errorCodeMatch.size() == 1, "Should only have 1");
                for (auto x : errorCodeMatch)
                {
                    shaderError.errorCode = x;
                }

                std::string errorDesc = errorCodeMatch.suffix().str();
                if (std::regex_search(errorDesc, errorCodeMatch, errorDescExpression))
                {
                    R2_CHECK(errorCodeMatch.size() == 1, "Should only have 1");
					for (auto x : errorCodeMatch)
					{
                        size_t newlinePos = errorCodeMatch.suffix().str().find_first_of('\n');

                        if (std::string::npos == newlinePos)
                        {
                            shaderError.errorString = errorCodeMatch.suffix();
                        }
                        else
                        {
                            shaderError.errorString = errorCodeMatch.suffix().str().substr(0, newlinePos);
                        }
					}
                }
            }

            shaderErrors.push_back(shaderError);
            slogString = match.suffix().str();
        }

        for (const auto& shaderError : shaderErrors)
        {
            //find the file this shaderError belongs to
            std::string filePath = "";
            std::string originalFile = "";
            size_t shaderLineNumber = 0;

            for (auto iter = g_shaderDebugInfo[debugInfoIndex].begin(); iter != g_shaderDebugInfo[debugInfoIndex].end(); ++iter)
            {
                size_t lineNum = std::stoull(shaderError.lineNumber);
                if (iter->second.globalLineStart <= lineNum &&
                    iter->second.globalLineStart + iter->second.numLines >= lineNum)
                {
                    size_t lineOffset = 0;
                    for (auto searchIter = g_shaderDebugInfo[debugInfoIndex][iter->second.shaderPartFilename].shaderLineOffsets.rbegin();
                        searchIter != g_shaderDebugInfo[debugInfoIndex][iter->second.shaderPartFilename].shaderLineOffsets.rend();
                        ++searchIter)
                    {
                        if (searchIter->shaderLine < lineNum)
                        {
                            lineOffset = searchIter->offset;
                            break;
                        }
                    }

                    filePath = iter->second.shaderPartFilename;
                    originalFile = iter->second.globalShaderFilename;
                    shaderLineNumber = lineNum - iter->second.globalLineStart + 1 + lineOffset;

                }
            }

            R2_LOGE("\nOriginal File: %s\nPart File: %s\nLine#: %zu\nError code: %s\nDescription: %s\n",
                originalFile.c_str(), filePath.c_str(), shaderLineNumber, shaderError.errorCode.c_str(), shaderError.errorString.c_str());
        }
#endif
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

                ShowOpenGLShaderError(vertexShaderHandle, 0);

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

                ShowOpenGLShaderError(fragmentShaderHandle, 1);

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

                ShowOpenGLShaderError(geometryShaderHandle, 2);

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

                ShowOpenGLShaderError(computeShaderHandle, 3);

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
#ifdef R2_DEBUG
    void UpdateShaderDebugInfo(const char* saveptr1, size_t& numLines, size_t& includeLines, size_t& globalLines, size_t debugIndex, const char* file)
    {
		numLines++; //maybe should be +2?
		globalLines++;
		size_t lenLength = strlen(saveptr1);
        bool finished = false;
		for (int i = 0; !finished && i < lenLength; ++i)
		{
			if (saveptr1[i] == '\n')
			{
                //skip the first newline
                for (int j = i + 1;!finished && j < lenLength; ++j)
                {
                    if (saveptr1[j] == '\n')
                    {
                        includeLines++;

                        auto iter= std::find_if(g_shaderDebugInfo[debugIndex][file].shaderLineOffsets.begin(), g_shaderDebugInfo[debugIndex][file].shaderLineOffsets.end(), [globalLines](const ShaderLineOffset& shaderLineOffset)
                            {
                                return shaderLineOffset.shaderLine == globalLines;
                            });
                        if (iter == g_shaderDebugInfo[debugIndex][file].shaderLineOffsets.end())
                        {
							ShaderLineOffset offset;
							offset.shaderLine = globalLines;
							offset.offset = includeLines;

							g_shaderDebugInfo[debugIndex][file].shaderLineOffsets.push_back(offset);
                        }
                        else
                        {
                            iter->offset = includeLines;
                        }

                    }
                    else if(saveptr1[j] != '\n' && saveptr1[j] != '\r' && saveptr1[j] != '\t' && saveptr1[j] != ' ')
                    {

                        finished = true;
                    }
                }
			}
			else if (saveptr1[i] != '\n' && saveptr1[i] != '\r' && saveptr1[i] != '\t' && saveptr1[i] != ' ')
			{
				break;
			}
		}
    }
#endif
    void ReadAndParseShaderData(u64 hashName, ShaderName shaderStage, const char* originalFilePath, const char* shaderFilePath, r2::SArray<char*>* shaderSourceFiles, r2::SArray<char*>& includedPaths, r2::SArray<void*>* tempAllocations, size_t debugIndex, size_t& globalLines)
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

		static std::regex includeExpression("^#include \".*\"[ \t]*$");
		static std::regex filePathExpression("\".*\"");
        static std::regex versionExpression("^#version [0-9][0-9][0-9] core$");

        char* shaderParsedOutIncludes = (char*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, lengthOfShaderFile + 32 + 50 * fs::FILE_PATH_LENGTH, sizeof(char)); //+32 for each potential '\0' in the file
        shaderParsedOutIncludes[0] = '\0';
        u32 lengthOfParsedShaderData = 0;

        r2::sarr::Push(*tempAllocations, (void*)shaderParsedOutIncludes);


#ifdef R2_DEBUG

        if (globalLines == 0)
        {
            globalLines = 1;
        }
		const auto iter = g_shaderDebugInfo[debugIndex].find(shaderFilePath);

		if (iter == g_shaderDebugInfo[debugIndex].end())
		{
			ShaderDebugInfo debugInfo;
			debugInfo.globalShaderFilename = originalFilePath;
			debugInfo.shaderPartFilename = shaderFilePath;
			debugInfo.numLines = 0;
			debugInfo.globalLineStart = globalLines;

			g_shaderDebugInfo[debugIndex][shaderFilePath] = debugInfo;
		}
#endif

        char* saveptr1;
        char* delimiter = "\r\n";
        char* pch = strtok_s(shaderFileDataCopy, delimiter, &saveptr1);


        if (pch != nullptr)
        {
#ifdef R2_DEBUG
//			UpdateShaderDebugInfo(saveptr1, g_shaderDebugInfo[debugIndex][shaderFilePath].numLines, g_shaderDebugInfo[debugIndex][shaderFilePath].includeLines, globalLines);
#endif
        }

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

                pch = strtok_s(NULL, delimiter, &saveptr1);

#ifdef R2_DEBUG
                UpdateShaderDebugInfo(saveptr1, g_shaderDebugInfo[debugIndex][shaderFilePath].numLines, g_shaderDebugInfo[debugIndex][shaderFilePath].includeLines, globalLines, debugIndex, shaderFilePath);
#endif

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

#ifdef R2_DEBUG
                    UpdateShaderDebugInfo(saveptr1, g_shaderDebugInfo[debugIndex][shaderFilePath].numLines, g_shaderDebugInfo[debugIndex][shaderFilePath].includeLines, globalLines, debugIndex, shaderFilePath);
#endif

                    lengthOfParsedShaderData += strLen + 1;
                }

                FREE(materialShaderDefines, *MEM_ENG_SCRATCH_PTR);

                u32 strLen = strlen(pch);

				strcat(&shaderParsedOutIncludes[currentOffset], pch);
				strcat(&shaderParsedOutIncludes[currentOffset], "\n");

                lengthOfParsedShaderData += strLen + 1;


				hasInjectedDefines = true;

				pch = strtok_s(NULL, delimiter, &saveptr1);

#ifdef R2_DEBUG
				UpdateShaderDebugInfo(saveptr1, g_shaderDebugInfo[debugIndex][shaderFilePath].numLines, g_shaderDebugInfo[debugIndex][shaderFilePath].includeLines, globalLines, debugIndex, shaderFilePath);
#endif

				continue;
            }


            if (!std::regex_match(pch, includeExpression))
            {
                u32 strLen = strlen(pch);

                strcat(&shaderParsedOutIncludes[currentOffset], pch);

                strcat(&shaderParsedOutIncludes[currentOffset], "\n");

                lengthOfParsedShaderData += strLen + 1;

                pch = strtok_s(NULL, delimiter, &saveptr1);

#ifdef R2_DEBUG
				UpdateShaderDebugInfo(saveptr1, g_shaderDebugInfo[debugIndex][shaderFilePath].numLines, g_shaderDebugInfo[debugIndex][shaderFilePath].includeLines, globalLines, debugIndex, shaderFilePath);
#endif

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

                pch = strtok_s(NULL, delimiter, &saveptr1);

#ifdef R2_DEBUG
				UpdateShaderDebugInfo(saveptr1, g_shaderDebugInfo[debugIndex][shaderFilePath].numLines, g_shaderDebugInfo[debugIndex][shaderFilePath].includeLines, globalLines, debugIndex, shaderFilePath);
#endif
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

                size_t dummy = 0;
                g_shaderDebugInfo[debugIndex][shaderFilePath].includeLines++;
#ifdef R2_DEBUG
                UpdateShaderDebugInfo(saveptr1, dummy, g_shaderDebugInfo[debugIndex][shaderFilePath].includeLines, dummy, debugIndex, shaderFilePath);
               // g_shaderDebugInfo[debugIndex][shaderFilePath].includeLines++;
#endif
                pch = strtok_s(NULL, delimiter, &saveptr1);
                continue;                
            }

#if defined(R2_ASSET_PIPELINE)
            shadersystem::AddShaderToShaderPartList(STRING_ID(quotelessPath), originalFilePath);
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

#ifdef R2_DEBUG
            //I think we should include the include directive because that will match with the source file
			g_shaderDebugInfo[debugIndex][shaderFilePath].numLines++;
            g_shaderDebugInfo[debugIndex][shaderFilePath].includeLines++;
	//		globalLines++;
#endif

            ReadAndParseShaderData(hashName, shaderStage, originalFilePath, fullIncludePath, shaderSourceFiles, includedPaths, tempAllocations, debugIndex, globalLines);

            pch = strtok_s(NULL, delimiter, &saveptr1);
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

        r2::SArray<void*>* tempAllocations = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, void*, 256);

        r2::SArray<char*>* vertexShaderParts = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char*, 32);

        r2::sarr::Push(*tempAllocations, (void*) vertexShaderParts);

        r2::SArray<char*>* fragmentShaderParts = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char*, 64);

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

            size_t globalLines = 0;

            ReadAndParseShaderData(hashName, shaderName, vertexShaderFilePath, vertexShaderFilePath, vertexShaderParts, *includePaths, tempAllocations, 0, globalLines);

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

            size_t globalLines = 0;

            ReadAndParseShaderData(hashName, shaderName, fragmentShaderFilePath, fragmentShaderFilePath, fragmentShaderParts, *includePaths, tempAllocations, 1, globalLines);

			//if (std::string(fileNameWithExtension) == "AnimModel.fs")
			//{
			//	const auto numParts = r2::sarr::Size(*fragmentShaderParts);

			//	for (int i = 0; i < numParts; ++i)
			//	{
			//		printf("%s", r2::sarr::At(*fragmentShaderParts, i));
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
            
            size_t globalLines = 0;
            
            ReadAndParseShaderData(hashName, shaderName, geometryShaderFilePath, geometryShaderFilePath, geometryShaderParts, *includePaths, tempAllocations, 2, globalLines);

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

            size_t globalLines = 0;

			ReadAndParseShaderData(hashName, shaderName, computeShaderFilePath, computeShaderFilePath, computeShaderParts, *includePaths, tempAllocations, 3, globalLines);
			
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