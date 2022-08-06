#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in uint DrawID;

#define NUM_FRUSTUM_SPLITS 4

layout (std140, binding = 0) buffer Models
{
	mat4 models[];
};

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
    mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
    mat4 inverseProjection;
};

invariant gl_Position;

void main()
{
    vec4 modelPos = models[DrawID] * vec4(aPos, 1.0);
	gl_Position = projection * view * modelPos;
}