//
//  EntryPoint.hpp
//  r2engine
///Users/serge/Documents/Projects/r2engine/premake5.lua
//  Created by Serge Lansiquot on 2019-02-03.
//

#ifndef EntryPoint_h
#define EntryPoint_h

#include <memory>
#include "r2/Platform/Platform.h"

extern std::unique_ptr<r2::Application> r2::CreateApplication();
extern std::unique_ptr<r2::Platform> r2::CreatePlatform();

int main(int argc, char ** argv)
{
    auto platform = r2::CreatePlatform();
    auto app = r2::CreateApplication();
    
    platform->Init(std::move(app));
    platform->Run();
    platform->Shutdown();
    
    return 0;
}

#endif /* EntryPoint_hpp */
