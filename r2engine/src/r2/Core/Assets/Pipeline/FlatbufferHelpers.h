//
//  FlatbufferHelpers.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-24.
//

#ifndef FlatbufferHelpers_h
#define FlatbufferHelpers_h

#ifdef R2_DEBUG
#include <string>

namespace r2::asset::pln::flat
{
    bool GenerateFlatbufferJSONFile(const std::string& outputDir, const std::string& fbsPath, const std::string& sourcePath);
    bool GenerateFlatbufferBinaryFile(const std::string& outputDir, const std::string& fbsPath, const std::string& sourcePath);
    bool GenerateFlatbufferCodeFromSchema(const std::string& outputDir, const std::string& fbsPath);
}

#endif
#endif /* FlatbufferHelpers_h */
