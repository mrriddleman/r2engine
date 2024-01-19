#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/DynamicVertexInput.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Common/ModelFunctions.glsl"

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
	mat4 vertexTransform = GetAnimatedModel(DrawID);
	vec4 modelPos = vertexTransform * vec4(aPos, 1.0);

	mat3 normalMatrix = GetNormalMatrix(vertexTransform);

	vs_out.normal = normalize(normalMatrix * aNormal);
	mat3 viewNormalMatrix = transpose(inverse(mat3(view * models[DrawID])));
	vs_out.viewNormal = normalize(viewNormalMatrix * aNormal);

//	vs_out.worldNormal = (vertexTransform * vec4(aNormal, 0));

	vec3 T = normalize(normalMatrix * aTangent);

	T = normalize(T - dot(T, vs_out.normal) * vs_out.normal);

	vec3 B = cross(vs_out.normal, T);

	vs_out.tangent = T;
	vs_out.bitangent = B;

	vs_out.TBN = mat3(T, B, vs_out.normal);
	mat3 TBN = transpose(vs_out.TBN);

	vs_out.texCoords = aTexCoord;
	vs_out.drawID = DrawID;
	vs_out.fragPos = modelPos.xyz / modelPos.w;

	vs_out.fragPosTangent = TBN * vs_out.fragPos;
	vs_out.viewPosTangent = TBN * cameraPosTimeW.xyz;

	gl_Position = projection * view * modelPos;

}