#ifndef __OPENGL_UTILS_H__
#define __OPENGL_UTILS_H__

#include "r2/Core/Logging/Log.h"

#if defined R2_DEBUG
#define GLCall(x) GLClearError();\
	x;\
	R2_CHECK(GLLogCall(#x), "OpenGL Function: %s failed in file: %s on line %d", #x, __FILE__, __LINE__);
#elif 
#define GLCall(x) x;
#endif


void GLClearError();
bool GLLogCall(const char* function);

#endif // __OPENGL_UTILS_H__
