#ifndef GLSL_DEPTH_UTILS
#define GLSL_DEPTH_UTILS

float LinearizeDepth(float depth, float near, float far)
{
	float z = depth * 2.0-1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));
}

#endif