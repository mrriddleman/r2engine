#version 450 core

in vec4 FragPos;
in vec3 LightPos;
in float FarPlane;

void main()
{
	float lightDistance = length(FragPos.xyz - LightPos);
	lightDistance = lightDistance / FarPlane;

	gl_FragDepth = lightDistance;
}