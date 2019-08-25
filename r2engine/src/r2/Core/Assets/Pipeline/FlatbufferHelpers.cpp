//
//  FlatbufferHelpers.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-24.
//

#include "FlatbufferHelpers.h"
#include <cstdlib>
#include <cstdio>

namespace r2::asset::pln::flat
{
    void RunSystemCommand(const char* command)
    {
        int result = system(command);
        R2_LOGI("RunSystemCommand generated result: %i\n", result);
    }
    
    bool GenerateFlatbufferJSONFile(const std::string& outputDir, const std::string& fbsPath, const std::string& sourcePath)
    {
        char command[Kilobytes(2)];
        std::string flatc = R2_FLATC;
        
#ifdef R2_PLATFORM_WINDOWS
        sprintf_s(command, sizeof(command), "\"%s\" -t -o \"%s\" \"%s\" -- \"%s\" --raw-binary", flatc.c_str(), outputDir.c_str(), fbsPath.c_str(), sourcePath.c_str());
#else
        sprintf(command, "\"%s\" -t -o \"%s\" \"%s\" -- \"%s\" --raw-binary", flatc.c_str(), outputDir.c_str(), fbsPath.c_str(), sourcePath.c_str());
#endif
        RunSystemCommand(command);
        return true;
    }
    
    bool GenerateFlatbufferBinaryFile(const std::string& outputDir, const std::string& fbsPath, const std::string& sourcePath)
    {
        char command[Kilobytes(2)];
        std::string flatc = R2_FLATC;
        
#ifdef R2_PLATFORM_WINDOWS
        sprintf_s(command, sizeof(command), "\"%s\" -b -o \"%s\" \"%s\" \"%s\"", flatc.c_str(), outputDir.c_str(), fbsPath.c_str(), sourcePath.c_str());
#else
        sprintf(command, "\"%s\" -b -o \"%s\" \"%s\" \"%s\"", flatc.c_str(), outputDir.c_str(), fbsPath.c_str(), sourcePath.c_str());
#endif
        RunSystemCommand(command);
        return true;
    }
    
    bool GenerateFlatbufferCodeFromSchema(const std::string& outputDir, const std::string& fbsPath)
    {
        char command[Kilobytes(2)];
        std::string flatc = R2_FLATC;
        
#ifdef R2_PLATFORM_WINDOWS
        sprintf_s(command, sizeof(command), "\"%s\" -c -o \"%s\" \"%s\"", flatc.c_str(), outputDir.c_str(), fbsPath.c_str());
#else
        sprintf(command, "\"%s\" -c -o \"%s\" \"%s\"", flatc.c_str(), outputDir.c_str(), fbsPath.c_str());
#endif
        RunSystemCommand(command);
        return true;
    }
}
