#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/StaticVertexInput.glsl"
#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Input/UniformBuffers/AAParams.glsl"
#include "Input/UniformBuffers/Vectors.glsl"

#define SMAA_PIXEL_SIZE (1.0 / fovAspectResXResY.zw)

out VS_OUT
{
	vec4 offsets[3];
	vec3 texCoords;
	vec2 pixelCoord;
	flat uint drawID;
} vs_out;

void SMAABlendingWeightCalculationVS(vec2 texcoord,
                                     out vec2 pixcoord,
                                     out vec4 offset[3])
{
	pixcoord = texcoord / SMAA_PIXEL_SIZE;

	// We will use these offsets for the searches later on (see @PSEUDO_GATHER4):
    offset[0] = texcoord.xyxy + SMAA_PIXEL_SIZE.xyxy * vec4(-0.25, -0.125,  1.25, -0.125);
    offset[1] = texcoord.xyxy + SMAA_PIXEL_SIZE.xyxy * vec4(-0.125, -0.25, -0.125,  1.25);

    // And these for the searches, they indicate the ends of the loops:
    offset[2] = vec4(offset[0].xz, offset[1].yw) + 
                vec4(-2.0, 2.0, -2.0, 2.0) *
                SMAA_PIXEL_SIZE.xxyy * float(smaa_maxSearchSteps);
}

void main()
{
	vec4 pos = models[DrawID] * vec4(aPos, 1);
	
	gl_Position = vec4(pos.x, pos.y, 0, 1.0);

	SMAABlendingWeightCalculationVS(aTexCoord.xy, vs_out.pixelCoord, vs_out.offsets);

	vs_out.texCoords = vec3(aTexCoord.x, aTexCoord.y, 0.0);

	vs_out.drawID = DrawID;
}