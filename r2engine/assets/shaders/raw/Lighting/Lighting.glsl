#ifndef GLSL_LIGHTING
#define GLSL_LIGHTING

#include "Clusters/Clusters.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Lighting/Directional/DirectionalLighting.glsl"
#include "Lighting/PointLight/PointLighting.glsl"
#include "Lighting/SpotLight/SpotLighting.glsl"

vec3 CalculateLighting(in PixelData pixel)
{
	vec3 L0 = vec3(0);
	uint clusterTileIndex = GetClusterIndex(round(vec2(gl_FragCoord.xy) + vec2(0.01)), fovAspectResXResY.zw, exposureNearFar.yz, clusterScaleBias, clusterTileSizes, zPrePassSurface);

	EvalAllDirectionalLights(L0, pixel);
	EvalAllPointLights(clusterTileIndex, L0, pixel);
	EvalAllSpotLights(clusterTileIndex, L0, pixel);

	return L0;
}

#endif