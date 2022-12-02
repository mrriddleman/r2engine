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

float GetRadRotationTemporal()
{
    float rotations[] = {60.0, 300.0, 180.0, 240.0, 120.0, 0.0};
    return rotations[int(frame) % 6] * (1.0 / 360.0) * 2.0 * PI;
}

#endif