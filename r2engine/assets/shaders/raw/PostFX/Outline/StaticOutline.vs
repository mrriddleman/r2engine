#version 450 core
#extension GL_NV_gpu_shader5 : enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in uint DrawID;


#define NUM_FRUSTUM_SPLITS 4 //TODO(Serge): pass in


layout (std140, binding = 0) uniform Matrices 
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
    mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
    mat4 inverseProjection;
    mat4 inverseView;
	mat4 vpMatrix;
	mat4 prevProjection;
	mat4 prevView;
	mat4 prevVPMatrix;
};

layout (std140, binding = 0) buffer Models
{
	mat4 models[];
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposureNearFar;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
	vec4 fovAspectResXResY;
    uint64_t frame;
    vec2 clusterScaleBias;
    uvec4 clusterTileSizes; //{tileSizeX, tileSizeY, tileSizeZ, tileSizePx}
    vec4 jitter;
};

out VS_OUT
{
	vec3 texCoords; 
	flat uint drawID;

} vs_out;

invariant gl_Position;

void main()
{
	vec4 modelPos = models[DrawID] * vec4(aPos, 1.0);
	
	vec4 Normal = vec4(mat3(transpose(inverse(models[DrawID]))) * aNormal, 0.0);

	gl_Position = projection * view * (modelPos + Normal*0.0001);

	vs_out.texCoords = aTexCoord;
	
	vs_out.drawID = DrawID;
}
