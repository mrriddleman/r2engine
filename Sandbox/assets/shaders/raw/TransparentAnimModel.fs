#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 Accum;
layout (location = 1) out float Reveal;

#include "Common/Defines.glsl"
#include "Common/CommonFunctions.glsl"
#include "Common/MaterialInput.glsl"
#include "Lighting/Lighting.glsl"

in VS_OUT
{
	vec3 texCoords; 
	vec3 fragPos;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	mat3 TBN;

	vec3 fragPosTangent;
	vec3 viewPosTangent;

	vec3 viewNormal;

	flat uint drawID;
} fs_in;

void main()
{
	PixelData pixel;

	DefaultCharacterMaterialFunction(fs_in.fragPos, fs_in.drawID, fs_in.texCoords, fs_in.TBN, fs_in.tangent, fs_in.normal, pixel);

	vec3 lightingResult = CalculateLightingNoClearCoatNoAnisotropy(pixel);

	float weight = TransparentWeight(lightingResult, pixel.alpha, gl_FragCoord.z);

	// blend func: GL_ONE, GL_ONE
	// switch to pre-multiplied alpha and weight
    Accum = vec4(lightingResult * pixel.alpha, pixel.alpha) * weight;

    // blend func: GL_ZERO, GL_ONE_MINUS_SRC_COLOR
    Reveal = pixel.alpha;
}