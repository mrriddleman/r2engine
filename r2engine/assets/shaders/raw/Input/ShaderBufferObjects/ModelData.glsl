#ifndef GLSL_MODEL_DATA
#define GLSL_MODEL_DATA

layout (std140, binding = 0) buffer Models
{
	mat4 models[];
};

#endif