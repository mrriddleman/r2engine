#ifndef GLSL_DEBUG_DATA
#define GLSL_DEBUG_DATA

struct DebugRenderConstants
{
	vec4 color;
	mat4 modelMatrix;
};

layout (std430, binding = 5) buffer DebugConstants
{
	DebugRenderConstants constants[];
};


#endif