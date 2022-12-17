#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent; 
layout (location = 4) in uint DrawID;

#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Input/UniformBuffers/AAParams.glsl"
#include "Input/UniformBuffers/Vectors.glsl"

out VS_OUT
{
	vec4 offsets[3];
	vec3 texCoords;
	vec2 pixelCoord;
	flat uint drawID;
} vs_out;

void SMAABlendingWeightCalculationVS(vec2 pixelSize,
									 vec2 texcoord,
                                     out vec2 pixcoord,
                                     out vec4 offset[3])
{
	pixcoord = texcoord / pixelSize;

	// We will use these offsets for the searches later on (see @PSEUDO_GATHER4):
    offset[0] = texcoord.xyxy + pixelSize.xyxy * vec4(-0.25, -0.125,  1.25, -0.125);
    offset[1] = texcoord.xyxy + pixelSize.xyxy * vec4(-0.125, -0.25, -0.125,  1.25);

    // And these for the searches, they indicate the ends of the loops:
    offset[2] = vec4(offset[0].xz, offset[1].yw) + 
                vec4(-2.0, 2.0, -2.0, 2.0) *
                pixelSize.xxyy * float(smaa_maxSearchSteps);
}

void main()
{
	vec2 pixelSize = 1.0 / fovAspectResXResY.zw;
	
	vec4 pos = models[DrawID] * vec4(aPosition, 1);
	
	gl_Position = vec4(pos.x, pos.y, 0, 1.0);

	SMAABlendingWeightCalculationVS(pixelSize, aTexCoord.xy, vs_out.pixelCoord, vs_out.offsets);

	vs_out.texCoords = vec3(aTexCoord.x, aTexCoord.y, 0.0);

	vs_out.drawID = DrawID;
}