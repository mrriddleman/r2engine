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
#include <cstdlib>
#include <cstdio>

namespace r2::asset::pln::flat
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
#ifdef R2_PLATFORM_WINDOWS
        sprintf_s(command, Kilobytes(2), "%s -b -o %s %s %s", flatc.c_str(), outputDir.c_str(), fbsPath.c_str(), sanitizedSourcePath);
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
}

#endif
