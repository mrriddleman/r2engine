#ifndef GLSL_MATRICES
#define GLSL_MATRICES

#include "Common/Defines.glsl"

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
    mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];//depricated
    mat4 inverseProjection;
    mat4 inverseView;
    mat4 vpMatrix;
    mat4 prevProjection;
    mat4 prevView;
    mat4 prevVPMatrix;
};

#endif