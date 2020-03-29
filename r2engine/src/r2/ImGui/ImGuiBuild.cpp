//
//  ImGuiBuild.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-10-05.
//

#include "r2pch.h"

#ifdef R2_PLATFORM_WINDOWS
#pragma warning( disable : 4996 )
#pragma warning( disable : 4005 )
#endif


#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "examples/imgui_impl_opengl3.cpp"
#include "examples/imgui_impl_sdl.cpp"
