#ifndef GLSL_DEPTH_UTILS
#define GLSL_DEPTH_UTILS

#include "Common/Texture.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/UniformBuffers/Vectors.glsl"

float LinearizeDepth(float depth, float near, float far)
{
	float z = depth * 2.0-1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));
}

vec2 CalculateVelocity(Tex2DAddress currentDepthTex, vec2 texCoords)
{
	float depth = SampleTextureR(currentDepthTex, texCoords) * 2.0 - 1.0;

	vec4 clipSpacePosition = vec4(texCoords * 2.0 - 1.0, depth, 1.0);
	vec4 viewSpacePosition = inverseProjection * clipSpacePosition;

	viewSpacePosition /= viewSpacePosition.w;

	vec4 worldSpacePosition = inverseView * viewSpacePosition;

	vec3 curPos = (vpMatrix * vec4(worldSpacePosition.xyz, 1.0)).xyw;
	vec3 prevPos = (prevVPMatrix * vec4(worldSpacePosition.xyz, 1.0)).xyw;

	return (((curPos.xy / curPos.z) - jitter.xy) - ((prevPos.xy / prevPos.z) - jitter.zw)) * 0.5;
}

#endif