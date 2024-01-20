#version 450 core
#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/StaticVertexInput.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Common/CommonFunctions.glsl"

out VS_OUT
{
	vec3 texCoords; 
	flat uint materialIndex;
	flat uint drawID;

} vs_out;

invariant gl_Position;

void main()
{
	vec4 modelPos = models[DrawID] * vec4(aPos, 1.0);
	
	vec4 Normal = vec4(mat3(transpose(inverse(models[DrawID]))) * aNormal, 0.0);

	gl_Position = projection * view * (modelPos + Normal*0.0001);

	vs_out.texCoords = aTexCoord;
	vs_out.materialIndex = GetLocalMeshOrMaterialIndex(aTexCoord);
	vs_out.drawID = DrawID;
}
