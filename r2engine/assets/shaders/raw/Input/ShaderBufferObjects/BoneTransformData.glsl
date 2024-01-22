#ifndef GLSL_BONE_TRANSFORM_DATA
#define GLSL_BONE_TRANSFORM_DATA

struct BoneTransform
{
	mat4 transform;
	mat4 invBinPose;
};

layout (std430, binding = 2) buffer BoneTransforms
{
	BoneTransform bonesXForms[];
};

layout (std140, binding = 3) buffer BoneTransformOffsets
{
	ivec4 boneOffsets[];
};

#endif