#ifndef GLSL_VECTORS
#define GLSL_VECTORS

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

#endif