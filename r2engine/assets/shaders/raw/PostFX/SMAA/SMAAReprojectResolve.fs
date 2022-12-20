#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out float FragColor;

#include "Common/CommonFunctions.glsl"
#include "Depth/DepthUtils.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"

#define SMAA_REPROJECTION_WEIGHT_SCALE 30

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;


vec4 SMAAResolvePS(vec2 texcoord,
                   Tex2DAddress colorTexCurr,
                   Tex2DAddress colorTexPrev,
                   Tex2DAddress depthTex) 
{
    // Velocity is calculated from previous to current position, so we need to
    // inverse it:
    vec2 velocity = -CalculateVelocity(depthTex, texcoord);

    // Fetch current pixel:
    vec4 current = SampleTextureRGBA(colorTexCurr, texcoord);

    // Reproject current coordinates and fetch previous pixel:
    vec4 previous = SampleTextureRGBA(colorTexPrev, texcoord + velocity);

    // Attenuate the previous pixel if the velocity is different:
    float delta = abs(current.a * current.a - previous.a * previous.a) / 5.0;
    float weight = 0.5 * Saturate(1.0 - (sqrt(delta) * SMAA_REPROJECTION_WEIGHT_SCALE));

    // Blend the pixels according to the calculated weight:
    return mix(current, previous, weight);
}

void main()
{
	FragColor = SMAAResolvePS(
		fs_in.texCoords.xy,
		smaaNeighborhoodBlendingSurface[0],
		smaaNeighborhoodBlendingSurface[1],
		zPrePassSurface);
}