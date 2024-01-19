#version 450 core
#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/StaticVertexInput.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/ShaderBufferObjects/ModelData.glsl"

out VS_OUT
{
	vec3 texCoords; 
	vec3 fragPos;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	mat3 TBN;

	vec3 fragPosTangent;
	vec3 viewPosTangent;

	vec3 viewNormal;

	flat uint drawID;

} vs_out;

invariant gl_Position;

void main()
{
	vec4 modelPos = models[DrawID] * vec4(aPos, 1.0);
	vs_out.fragPos = modelPos.xyz;
	gl_Position = projection * view * modelPos;

	mat3 normalMatrix = transpose(inverse(mat3(models[DrawID])));
	vs_out.normal = normalize(normalMatrix * aNormal);
	mat3 viewNormalMatrix = transpose(inverse(mat3(view * models[DrawID])));

	
	vs_out.viewNormal = normalize(viewNormalMatrix * aNormal);

	//vs_out.worldNormal = vs_out.normal;


	vec3 T = normalize(normalMatrix * aTangent);

	T = normalize(T - (dot(T, vs_out.normal) * vs_out.normal));

	vec3 B = normalize(cross(vs_out.normal, T));

	vs_out.tangent = T;
	vs_out.bitangent = B;

	vs_out.TBN = mat3(T, B, vs_out.normal);
	mat3 TBN = transpose(vs_out.TBN);

	vs_out.fragPosTangent = TBN * vs_out.fragPos;
	vs_out.viewPosTangent = TBN * cameraPosTimeW.xyz;

	vs_out.texCoords = aTexCoord;
	vs_out.drawID = DrawID;
}