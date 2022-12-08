#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in uint DrawID;

#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/ShaderBufferObjects/ModelData.glsl"

invariant gl_Position;

out VS_OUT
{
    flat uint drawID;
    vec3 texCoords; 
} vs_out;

void main()
{
    vec4 modelPos = models[DrawID] * vec4(aPos, 1.0);
	gl_Position = projection * view * modelPos;

    vs_out.drawID = DrawID;
    vs_out.texCoords= aTexCoord;
}