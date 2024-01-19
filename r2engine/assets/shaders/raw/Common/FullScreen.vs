#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/StaticVertexInput.glsl"
#include "Input/ShaderBufferObjects/ModelData.glsl"

out VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} vs_out;

void main()
{
	vec4 pos = models[DrawID] * vec4(aPos, 1);
	gl_Position = vec4(pos.x, pos.y, 0, 1.0);

	vs_out.normal = aNormal;
	vs_out.texCoords = vec3(aTexCoord.x, aTexCoord.y, 0.0);
	vs_out.drawID = DrawID;
}