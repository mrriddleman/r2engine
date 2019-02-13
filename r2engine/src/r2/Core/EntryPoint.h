//
//  EntryPoint.hpp
//  r2engine
///Users/serge/Documents/Projects/r2engine/premake5.lua
//  Created by Serge Lansiquot on 2019-02-03.
//

#ifndef EntryPoint_h
#define EntryPoint_h

#include "r2/Platform/Platform.h"

extern std::unique_ptr<r2::Application> r2::CreateApplication();


int main(int argc, char ** argv)
{
    auto& platform = r2::Platform::Get();
    auto app = r2::CreateApplication();
    
    platform.Init(std::move(app));
    platform.Run();
    platform.Shutdown();
    
    return 0;
}

#endif /* EntryPoint_hpp */
