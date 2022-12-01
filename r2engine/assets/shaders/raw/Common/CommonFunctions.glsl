#ifndef GLSL_COMMON_FUNCTIONS
#define GLSL_COMMON_FUNCTIONS

#include "Input/UniformBuffers/Vectors.glsl"

float Saturate(float x)
{
	return clamp(x, 0.0, 1.0);
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

float GetRadRotationTemporal()
{
	float rotations[] = {60.0, 300.0, 180.0, 240.0, 120.0, 0.0};
	return rotations[int(frame) % 6] * (1.0 / 360.0) * 2.0 * PI;
}

float GetDistanceAttenuation(vec3 posToLight, float falloff)
{
	float distanceSquare = dot(posToLight, posToLight);

    float factor = distanceSquare * falloff;

    float smoothFactor = Saturate(1.0 - factor * factor);

    float attenuation = smoothFactor * smoothFactor;

    return attenuation * 1.0 / max(distanceSquare, 1e-4);
}

#endif