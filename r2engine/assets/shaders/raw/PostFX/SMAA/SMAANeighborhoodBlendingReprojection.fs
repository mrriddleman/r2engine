#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;


#include "Input/UniformBuffers/AAParams.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Input/UniformBuffers/Vectors.glsl"

#define SMAA_PIXEL_SIZE (1.0 / fovAspectResXResY.zw)

in VS_OUT
{
	vec4 offsets[2];
	vec3 texCoords;
	vec2 pixelCoord;
	flat uint drawID;
} fs_in;


#ifndef SMAA_REPROJECTION
#define SMAA_REPROJECTION 1
#endif

//-----------------------------------------------------------------------------
// Neighborhood Blending Pixel Shader (Third Pass)

vec4 SMAANeighborhoodBlendingPS(vec2 texcoord,
                                vec4 offsets[2],
                                Tex2DAddress colorTex,
                                Tex2DAddress blendTex) 
{
    // Fetch the blending weights for current pixel:
    vec4 a;
    a.xz = SampleTextureRGBA(blendTex, texcoord).xz;
    a.y = SampleTextureRGBA(blendTex, offsets[1].zw).g;
    a.w = SampleTextureRGBA(blendTex, offsets[1].xy).a;

    // Is there any blending weight with a value greater than 0.0?
    if (dot(a, vec4(1.0, 1.0, 1.0, 1.0)) < 1e-5)
        return SampleTextureLodZeroRGBA(colorTex, texcoord);
    else 
    {
        vec4 color = vec4(0.0, 0.0, 0.0, 0.0);

        // Up to 4 lines can be crossing a pixel (one through each edge). We
        // favor blending by choosing the line with the maximum weight for each
        // direction:
        vec2 offset;
        offset.x = a.a > a.b? a.a : -a.b; // left vs. right 
        offset.y = a.g > a.r? a.g : -a.r; // top vs. bottom

        // Then we go in the direction that has the maximum weight:
        if (abs(offset.x) > abs(offset.y)) // horizontal vs. vertical
            offset.y = 0.0;
        else
            offset.x = 0.0;

        #if SMAA_REPROJECTION == 1
        // Fetch the opposite color and lerp by hand:
        vec4 C = SampleTextureLodZeroRGBA(colorTex, texcoord);
        texcoord += sign(offset) * SMAA_PIXEL_SIZE;
        vec4 Cop = SampleTextureLodZeroRGBA(colorTex, texcoord);
        float s = abs(offset.x) > abs(offset.y)? abs(offset.x) : abs(offset.y);

        // Unpack the velocity values:
        C.a *= C.a;
        Cop.a *= Cop.a;

        // Lerp the colors:
        vec4 Caa = mix(C, Cop, s);

        // Unpack velocity and return the resulting value:
        Caa.a = sqrt(Caa.a);
        return Caa;
        #else
        // Fetch the opposite color and lerp by hand:
        vec4 C = SampleTextureLodZeroRGBA(colorTex, texcoord);
        texcoord += sign(offset) * SMAA_PIXEL_SIZE;
        vec4 Cop = SampleTextureLodZeroRGBA(colorTex, texcoord);
        float s = abs(offset.x) > abs(offset.y)? abs(offset.x) : abs(offset.y);
        return mix(C, Cop, s);
        #endif
    }
}

void main()
{
	FragColor = SMAANeighborhoodBlendingPS(
									fs_in.texCoords.xy,
                                  	fs_in.offsets,
                                  	compositeSurface,
                                  	smaaBlendingWeightSurface);
}