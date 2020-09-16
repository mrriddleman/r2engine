#include "r2pch.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLUtils.h"
#include <glad/glad.h>

void GLClearError()
{
	while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function)
{
	while (GLenum err = glGetError() )
	{
		R2_LOGE("%s failed with error: %i", err);
		return false;
	} 
	return true;
}