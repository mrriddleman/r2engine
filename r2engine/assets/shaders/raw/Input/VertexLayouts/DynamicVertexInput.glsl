#ifndef GLSL_DYNAMIC_VERTEX_INPUT
#define GLSL_DYNAMIC_VERTEX_INPUT

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec3 aTexCoord;
layout (location = 4) in vec3 aTexCoord1;
layout (location = 5) in vec4 BoneWeights;
layout (location = 6) in ivec4 BoneIDs;
layout (location = 7) in uint DrawID;

#endif