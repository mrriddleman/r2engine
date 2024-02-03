#ifndef GLSL_COMMON_FUNCTIONS
#define GLSL_COMMON_FUNCTIONS
#include "Common/Defines.glsl"

float Saturate(float x)
{
	return clamp(x, 0.0, 1.0);
}

vec3 Saturate(vec3 x)
{
	return clamp(x, 0.0, 1.0);
}

vec2 Saturate(vec2 x)
{
	return clamp(x, 0.0, 1.0);
}

vec4 Saturate(vec4 x)
{
	return clamp(x, 0.0, 1.0);
}

mat4 MatInverse(mat4 mat)
{
	return inverse(mat);
}

mat4 LookAt(vec3 eye, vec3 center, vec3 up)
{
	vec3 f = normalize(center - eye);
	vec3 s = normalize(cross(f, up));
	vec3 u = cross(s, f);

	mat4 result = mat4(1.0);

	result[0][0] = s.x;
	result[1][0] = s.y;
	result[2][0] = s.z;
	result[0][1] = u.x;
	result[1][1] = u.y;
	result[2][1] = u.z;
	result[0][2] = -f.x;
	result[1][2] = -f.y;
	result[2][2] = -f.z;
	result[3][0] = -dot(s, eye);
	result[3][1] = -dot(u, eye);
	result[3][2] = 	dot(f, eye);

	return result;
}

mat4 Projection(float fov, float aspect, float near, float far)
{
	mat4 result = mat4(0.0);

	float tanHalfFovy = tan(fov*0.5);

	result[0][0] = 1.0 / (aspect * tanHalfFovy);
	result[1][1] = 1.0 / (tanHalfFovy);
	result[2][2] = - (far + near) / (far - near);
	result[2][3] = -1.0;
	result[3][2] = (-2.0 * far * near) / (far - near);

	return result;
}

mat4 Projection_ZO(float fov, float aspect, float near, float far)
{
	mat4 result = mat4(0.0);

	float tanHalfFovy = tan(fov / 2.0);

	result[0][0] = 1.0 / (aspect * tanHalfFovy);
	result[1][1] = 1.0 / (tanHalfFovy);
	result[2][2] = - far / (far - near);
	result[2][3] = -1.0;
	result[3][2] = - (far * near) / (far - near);


	return result;
}

mat4 Ortho(float left, float right, float bottom, float top, float near, float far)
{
	mat4 result = mat4(1.0);

	result[0][0] = 2.0 / (right - left);
	result[1][1] = 2.0 / (top - bottom);
	result[2][2] = -2.0 / (far - near);
	result[3][0] = -(right + left) / (right - left);
	result[3][1] = -(top + bottom) / (top - bottom);
	result[3][2] = -(far + near) / (far - near);

	return result;
}

mat4 Ortho_ZO(float left, float right, float bottom, float top, float near, float far)
{
	mat4 result = mat4(1.0);

	result[0][0] = 2.0 / (right - left);
	result[1][1] = 2.0 / (top - bottom);
	result[2][2] = -1.0 / (far - near);
	result[3][0] = -(right + left) / (right - left);
	result[3][1] = -(top + bottom) / (top - bottom);
	result[3][2] = -near / (far - near);

	return result;
}

float GetDistanceAttenuation(vec3 posToLight, float falloff)
{
	float distanceSquare = dot(posToLight, posToLight);

    float factor = distanceSquare * falloff;

    float smoothFactor = Saturate(1.0 - factor * factor);

    float attenuation = smoothFactor * smoothFactor;

    return attenuation * 1.0 / max(distanceSquare, 1e-4);
}

vec2 EncodeNormal(vec3 normal)
{
	return vec2(normal.xy * 0.5 + 0.5);
}

vec3 DecodeNormal(vec2 enc)
{
	vec3 n;

	n.xy = enc * 2 - 1;
	n.z = sqrt(1 - dot(n.xy, n.xy));
	return n;
}

// calculate floating point numbers equality accurately
bool IsApproximatelyEqual(float a, float b)
{
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}

// get the max value between three values
float Max3(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

uint GetLocalMeshOrMaterialIndex(vec3 uv)
{
	highp uint meshLocalIndex = uint(round(uv.z));
	return meshLocalIndex;
}

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);
    vec4 unprojectedPoint =  viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}


#endif