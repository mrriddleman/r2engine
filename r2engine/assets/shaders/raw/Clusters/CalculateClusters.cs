#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

#include "Common/Defines.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/ShaderBufferObjects/ClusterData.glsl"

//Function prototypes
vec4 ClipToView(vec4 clip);
vec4 ScreenToView(vec4 screen);
vec3 LineIntersectionToZPlane(vec3 A, vec3 B, float zDistance);

void main()
{
	const vec3 eyePos = vec3(0.0);

	uint tileSizePix = clusterTileSizes[3];
	uint tileIndex   = gl_WorkGroupID.x + 
					   gl_WorkGroupID.y * gl_NumWorkGroups.x +
					   gl_WorkGroupID.z * (gl_NumWorkGroups.x * gl_NumWorkGroups.y);


	vec4 maxPointSS = vec4(vec2(gl_WorkGroupID.x + 1, gl_WorkGroupID.y + 1) * tileSizePix, -1.0, 1.0); //max point in screen space (top right)
	vec4 minPointSS = vec4(gl_WorkGroupID.xy * tileSizePix, -1.0, 1.0); //bottom left in screen space

	vec3 maxPointVS = ScreenToView(maxPointSS).xyz;
	vec3 minPointVS = ScreenToView(minPointSS).xyz;

	//using DOOM 2016's tile partition formula
	float tileNear = -exposureNearFar.y * pow(exposureNearFar.z / exposureNearFar.y, gl_WorkGroupID.z / float(gl_NumWorkGroups.z));
	float tileFar  = -exposureNearFar.y * pow(exposureNearFar.z / exposureNearFar.y, (gl_WorkGroupID.z + 1) / float(gl_NumWorkGroups.z));

	vec3 minPointNear = LineIntersectionToZPlane(eyePos, minPointVS, tileNear);
	vec3 minPointFar  = LineIntersectionToZPlane(eyePos, minPointVS, tileFar);
	vec3 maxPointNear = LineIntersectionToZPlane(eyePos, maxPointVS, tileNear);
	vec3 maxPointFar  = LineIntersectionToZPlane(eyePos, maxPointVS, tileFar);

	vec3 minPointAABB = min(min(minPointNear, minPointFar), min(maxPointNear, maxPointFar));
	vec3 maxPointAABB = max(max(minPointNear, minPointFar), max(maxPointNear, maxPointFar));

	clusters[tileIndex].minPoint = vec4(minPointAABB, 0.0);
	clusters[tileIndex].maxPoint = vec4(maxPointAABB, 0.0);
}

vec4 ClipToView(vec4 clip)
{
	vec4 view = inverseProjection * clip;

	view = view / view.w;

	return view;
}

vec4 ScreenToView(vec4 screen)
{
	vec2 texCoord = screen.xy / fovAspectResXResY.zw;

	vec4 clip = vec4(vec2(texCoord.x, texCoord.y) * 2.0 - 1.0, screen.z, screen.w);

	return ClipToView(clip);
}

vec3 LineIntersectionToZPlane(vec3 A, vec3 B, float zDistance)
{
	vec3 normal = vec3(0.0, 0.0, 1.0);

	vec3 ab = B - A;

	float t = (zDistance - dot(normal, A)) / dot(normal, ab);

	vec3 result = A + t * ab;

	return result;
}