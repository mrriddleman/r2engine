//
//  FlatbufferHelpers.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-24.
//

#ifndef FlatbufferHelpers_h
#define FlatbufferHelpers_h

#ifdef R2_ASSET_PIPELINE
#include <string>

namespace r2::asset::pln::flathelp
{
    bool GenerateFlatbufferJSONFile(const std::string& outputDir, const std::string& fbsPath, const std::string& sourcePath);
    bool GenerateFlatbufferBinaryFile(const std::string& outputDir, const std::string& fbsPath, const std::string& sourcePath);
    bool GenerateFlatbufferCodeFromSchema(const std::string& outputDir, const std::string& fbsPath);
    bool GenerateJSONAndBinary(byte* buffer, u32 size, const std::string& schemaPath, const std::string& binaryPath, const std::string& jsonFilePath);
}

#endif
#endif /* FlatbufferHelpers_h */
