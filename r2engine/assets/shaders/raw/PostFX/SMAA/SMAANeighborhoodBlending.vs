#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent; 
layout (location = 4) in uint DrawID;

#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Input/UniformBuffers/Vectors.glsl"

#define SMAA_PIXEL_SIZE (1.0 / fovAspectResXResY.zw)

out VS_OUT
{
	vec4 offsets[2];
	vec3 texCoords;
	vec2 pixelCoord;
	flat uint drawID;
} vs_out;

void SMAANeighborhoodBlendingVS(vec2 texcoord,
                                out vec4 offset[2]) 
{
    offset[0] = texcoord.xyxy + SMAA_PIXEL_SIZE.xyxy * vec4(-1.0, 0.0, 0.0, -1.0);
    offset[1] = texcoord.xyxy + SMAA_PIXEL_SIZE.xyxy * vec4( 1.0, 0.0, 0.0,  1.0);
}

void main()
{
	vec4 pos = models[DrawID] * vec4(aPosition, 1);
	
	gl_Position = vec4(pos.x, pos.y, 0, 1.0);

	SMAANeighborhoodBlendingVS(vec2(aTexCoord.x, aTexCoord.y), vs_out.offsets);

	vs_out.texCoords = vec3(aTexCoord.x, aTexCoord.y, 0.0);

	vs_out.drawID = DrawID;
}