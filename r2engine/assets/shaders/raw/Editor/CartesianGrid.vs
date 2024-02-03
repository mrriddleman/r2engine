#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/StaticVertexInput.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Common/CommonFunctions.glsl"
#include "Input/ShaderBufferObjects/DebugData.glsl"

out VS_OUT
{
    vec3 voutNearPoint;
    vec3 voutFarPoint;

} vs_out;

void main()
{
    vec4 modelPosition = constants[DrawID].modelMatrix * vec4(aPos, 1);

    vs_out.voutNearPoint = UnprojectPoint(modelPosition.x, modelPosition.y, 0, view, projection).xyz; // unprojecting on the near plane
    vs_out.voutFarPoint = UnprojectPoint(modelPosition.x, modelPosition.y, 1, view, projection).xyz;
    
    gl_Position = vec4(modelPosition.xyz, 1.0);
}
