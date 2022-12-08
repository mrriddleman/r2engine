#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in uint DrawID;

#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/ShaderBufferObjects/DebugData.glsl"

// #define NUM_FRUSTUM_SPLITS 4

// layout (std140, binding = 0) uniform Matrices
// {
//     mat4 projection;
//     mat4 view;
//     mat4 skyboxView;
//     mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
//     mat4 inverseProjection;
//     mat4 inverseView;
//     mat4 vpMatrix;
//     mat4 prevProjection;
//     mat4 prevView;
//     mat4 prevVPMatrix;
// };

// struct DebugRenderConstants
// {
// 	vec4 color;
// 	mat4 modelMatrix;
// };

// layout (std430, binding = 5) buffer DebugConstants
// {
// 	DebugRenderConstants constants[];
// };

out VS_OUT
{
	vec4 color;
	flat uint drawID;
} vs_out;

void main()
{
	vs_out.drawID = DrawID;
	vs_out.color = constants[DrawID].color;
	gl_Position = projection * view * constants[DrawID].modelMatrix * vec4(aPos, 1.0);
}
