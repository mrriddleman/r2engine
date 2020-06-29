//
//  FlatbufferHelpers.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-24.
//
#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE

#include "FlatbufferHelpers.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include <filesystem>
#include <cstdlib>
#include <cstdio>

namespace r2::asset::pln::flathelp
{
    int RunSystemCommand(const char* command)
    {
        int result = system(command);
        if(result != 0)
        {
            R2_LOGE("Failed to run command: %s\n\n with result: %i\n", command, result);
        }
        return result;
    }
    
    bool GenerateFlatbufferJSONFile(const std::string& outputDir, const std::string& fbsPath, const std::string& sourcePath)
    {
        char command[Kilobytes(2)];
        std::string flatc = R2_FLATC;
        
        char sanitizedSourcePath[r2::fs::FILE_PATH_LENGTH];

        r2::fs::utils::SanitizeSubPath(sourcePath.c_str(), sanitizedSourcePath);
#ifdef R2_PLATFORM_WINDOWS
        sprintf_s(command, Kilobytes(2), "%s -t -o %s %s -- %s --raw-binary", flatc.c_str(), outputDir.c_str(), fbsPath.c_str(), sanitizedSourcePath);
#else
        sprintf(command, "%s -t -o %s %s -- %s --raw-binary", flatc.c_str(), outputDir.c_str(), fbsPath.c_str(), sanitizedSourcePath);
#endif
        return RunSystemCommand(command) == 0;
    }
    
    bool GenerateFlatbufferBinaryFile(const std::string& outputDir, const std::string& fbsPath, const std::string& sourcePath)
    {
        char command[Kilobytes(2)];
        std::string flatc = R2_FLATC;
        
        char sanitizedSourcePath[r2::fs::FILE_PATH_LENGTH];
        r2::fs::utils::SanitizeSubPath(sourcePath.c_str(), sanitizedSourcePath);

		char sanitizedOutputPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(outputDir.c_str(), sanitizedOutputPath);

#ifdef R2_PLATFORM_WINDOWS
        sprintf_s(command, Kilobytes(2), "%s -b -o %s %s %s", flatc.c_str(), sanitizedOutputPath, fbsPath.c_str(), sanitizedSourcePath);
#else
        sprintf(command, "%s -b -o %s %s %s", flatc.c_str(), outputDir.c_str(), fbsPath.c_str(), sanitizedSourcePath);
#endif
        return RunSystemCommand(command) == 0;
    }
    
    bool GenerateFlatbufferCodeFromSchema(const std::string& outputDir, const std::string& fbsPath)
    {
        char command[Kilobytes(2)];
        std::string flatc = R2_FLATC;
#ifdef R2_PLATFORM_WINDOWS
        sprintf_s(command, Kilobytes(2), "%s -c -o %s %s", flatc.c_str(), outputDir.c_str(), fbsPath.c_str());
#else
        sprintf(command, "%s -c -o %s %s", flatc.c_str(), outputDir.c_str(), fbsPath.c_str());
#endif
        return RunSystemCommand(command) == 0;
    }

    bool GenerateJSONAndBinary(byte* buffer, u32 size, const std::string& schemaPath, const std::string& binaryPath, const std::string& jsonFilePath)
    {
		bool wroteFile = utils::WriteFile(binaryPath, (char*)buffer, size);

        R2_CHECK(wroteFile, "Failed to write file: %s", binaryPath.c_str());

		std::filesystem::path p = binaryPath;

        std::filesystem::path jsonPath = jsonFilePath;

		bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(jsonPath.parent_path().string(), schemaPath, binaryPath);

		bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(p.parent_path().string(), schemaPath, jsonFilePath);

		return generatedJSON && generatedBinary;
    }

}

#endif
