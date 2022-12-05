#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec3 NormalColor;
layout (location = 2) out vec4 SpecularColor;

#include "Common/Defines.glsl"
#include "Common/CommonFunctions.glsl"
#include "Common/MaterialInput.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
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

vec4 splitColors[NUM_FRUSTUM_SPLITS] = {vec4(2, 0.0, 0.0, 1.0), vec4(0.0, 2, 0.0, 1.0), vec4(0.0, 0.0, 2, 1.0), vec4(2, 2, 0.0, 1.0)};

vec4 DebugFrustumSplitColor()
{
	if(numDirectionLights > 0)
	{
		vec4 projectionPosInCSMSplitSpace = (gShadowMatrix[0] * vec4(fs_in.fragPos, 1.0));

		vec3 projectionPos = projectionPosInCSMSplitSpace.xyz;

		uint layer = NUM_FRUSTUM_SPLITS - 1;

		for(int i = int(layer); i >= 0; --i)
		{
			vec3 scale = gScale[i][0].xyz;
			vec3 bias = gBias[i][0].xyz;

			vec3 cascadePos = projectionPos + bias;
			cascadePos *= scale;
			cascadePos = abs(cascadePos - 0.5f);
			if(cascadePos.x <= 0.5 && cascadePos.y <= 0.5 && cascadePos.z <= 0.5)
			{
				layer = i;
			}
		}

		if(layer == -1)
		{
			layer = NUM_FRUSTUM_SPLITS-1;
		}

		return splitColors[layer];
	}
	

	return vec4(1);
}

void main()
{
	//texCoords = ParallaxMapping(fs_in.drawID, texCoords, viewDirTangent);

	//if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
 	//    discard;

 	PixelData pixel;

 	DefaultCharacterMaterialFunction(fs_in.fragPos, fs_in.drawID, fs_in.texCoords, fs_in.TBN, fs_in.tangent, fs_in.normal, pixel);

	vec3 lightingResult = CalculateLightingNoClearCoatNoAnisotropy(pixel);

	FragColor = vec4(lightingResult + pixel.emission , 1.0);// * DebugFrustumSplitColor();
	
	NormalColor = fs_in.viewNormal;

	SpecularColor = vec4(pixel.F0, 1.0 - pixel.roughness);
}