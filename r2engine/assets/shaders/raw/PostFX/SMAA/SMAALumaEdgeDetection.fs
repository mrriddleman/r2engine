#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

#include "Input/UniformBuffers/AAParams.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"

in VS_OUT
{
	vec4 offsets[3];
	vec3 texCoords;
	flat uint drawID;
} fs_in;

void main()
{
	vec2 threshold = vec2(smaa_threshold);

	// Calculate lumas:
    vec3 weights 	= vec3(0.2126, 0.7152, 0.0722);
    float L 		= dot(SampleTexture(compositeSurface, fs_in.texCoords.xy,  ivec2(0)), weights);
    float Lleft 	= dot(SampleTexture(compositeSurface, fs_in.offsets[0].xy, ivec2(0)), weights);
    float Ltop  	= dot(SampleTexture(compositeSurface, fs_in.offsets[0].zw, ivec2(0)), weights);

    // We do the usual threshold:
    vec4 delta;
    delta.xy = abs(L - vec2(Lleft, Ltop));
    vec2 edges = step(threshold, delta.xy);

    // Then discard if there is no edge:
    if (dot(edges, vec2(1.0, 1.0)) == 0.0)
        discard;

    // Calculate right and bottom deltas:
    float Lright 	= dot(SampleTexture(compositeSurface, fs_in.offsets[1].xy, ivec2(0)), weights);
    float Lbottom  	= dot(SampleTexture(compositeSurface, fs_in.offsets[1].zw, ivec2(0)), weights);
    delta.zw = abs(L - vec2(Lright, Lbottom));

    // Calculate the maximum delta in the direct neighborhood:
    vec2 maxDelta = max(delta.xy, delta.zw);
    maxDelta = max(maxDelta.xx, maxDelta.yy);

    // Calculate left-left and top-top deltas:
    float Lleftleft = dot(SampleTexture(compositeSurface, fs_in.offsets[2].xy, ivec2(0)), weights);
    float Ltoptop 	= dot(SampleTexture(compositeSurface, fs_in.offsets[2].zw, ivec2(0)), weights);
    delta.zw = abs(vec2(Lleft, Ltop) - vec2(Lleftleft, Ltoptop));

    // Calculate the final maximum delta:
    maxDelta = max(maxDelta.xy, delta.zw);

    /**
     * Each edge with a delta in luma of less than 50% of the maximum luma
     * surrounding this pixel is discarded. This allows to eliminate spurious
     * crossing edges, and is based on the fact that, if there is too much
     * contrast in a direction, that will hide contrast in the other
     * neighbors.
     * This is done after the discard intentionally as this situation doesn't
     * happen too frequently (but it's important to do as it prevents some 
     * edges from going undetected).
     */
    edges.xy *= step(0.5 * maxDelta, delta.xy);

    FragColor = vec4(edges, 0.0, 0.0);
}