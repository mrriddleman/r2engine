#ifndef GLSL_LIGHTING
#define GLSL_LIGHTING

#include "Clusters/Clusters.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Lighting/Directional/DirectionalLighting.glsl"
#include "Lighting/PointLight/PointLighting.glsl"
#include "Lighting/SpotLight/SpotLighting.glsl"
#include "Lighting/PBR/IBL.glsl"

vec3 CalculateLighting(inout PixelData pixel)
{
	//IBL
	vec3 iblColor = vec3(0);
	vec3 iblFr = vec3(0);
	vec3 iblFd = vec3(0);

	EvalIBL(skylight, pixel.multibounceAO, pixel, iblFd, iblFr);

	if(pixel.anisotropy == 0.0)
	{
		vec3 clearCoatR = reflect(-pixel.V, pixel.clearCoatNormal);
		EvalClearCoatIBL(skylight, clearCoatR, pixel, iblFd, iblFr);
	}

	iblColor += (iblFd + iblFr);

	//direct lighting
	vec3 L0 = vec3(0);
	uint clusterTileIndex = GetClusterIndex(round(vec2(gl_FragCoord.xy) + vec2(0.01)), fovAspectResXResY.zw, exposureNearFar.yz, clusterScaleBias, clusterTileSizes, zPrePassSurface);

	EvalAllDirectionalLights(L0, pixel);
	EvalAllPointLights(clusterTileIndex, L0, pixel);
	EvalAllSpotLights(clusterTileIndex, L0, pixel);

	return iblColor + L0;
}

vec3 CalculateLightingNoClearCoatNoAnisotropy(inout PixelData pixel)
{
	//IBL
	vec3 iblColor = vec3(0);
	vec3 iblFr = vec3(0);
	vec3 iblFd = vec3(0);

	EvalIBL(skylight, pixel.multibounceAO, pixel, iblFd, iblFr);

	iblColor += (iblFd + iblFr);

	//direct lighting
	vec3 L0 = vec3(0);
	uint clusterTileIndex = GetClusterIndex(round(vec2(gl_FragCoord.xy) + vec2(0.01)), fovAspectResXResY.zw, exposureNearFar.yz, clusterScaleBias, clusterTileSizes, zPrePassSurface);

	EvalAllDirectionalLightsNoClearCoatNoAnisotropy(L0, pixel);
	EvalAllPointLightsNoClearCoatNoAnisotropy(clusterTileIndex, L0, pixel);
	EvalAllSpotLightsNoClearCoatNoAnisotropy(clusterTileIndex, L0, pixel);

	return iblColor + L0;
}

vec3 CalculateLightingWithClearCoat(inout PixelData pixel)
{
	//IBL
	vec3 iblColor = vec3(0);
	vec3 iblFr = vec3(0);
	vec3 iblFd = vec3(0);

	EvalIBL(skylight, pixel.multibounceAO, pixel, iblFd, iblFr);
	
	vec3 clearCoatR = reflect(-pixel.V, pixel.clearCoatNormal);
	EvalClearCoatIBL(skylight, clearCoatR, pixel, iblFd, iblFr);

	iblColor += (iblFd + iblFr);

	//direct lighting
	vec3 L0 = vec3(0);
	uint clusterTileIndex = GetClusterIndex(round(vec2(gl_FragCoord.xy) + vec2(0.01)), fovAspectResXResY.zw, exposureNearFar.yz, clusterScaleBias, clusterTileSizes, zPrePassSurface);

	EvalAllDirectionalLightsWithClearCoat(L0, pixel);
	EvalAllPointLightsWithClearCoat(clusterTileIndex, L0, pixel);
	EvalAllSpotLightsWithClearCoat(clusterTileIndex, L0, pixel);

	return iblColor + L0;
}

vec3 CalculateLightingWithAnisotropy(inout PixelData pixel)
{
	//IBL
	vec3 iblColor = vec3(0);
	vec3 iblFr = vec3(0);
	vec3 iblFd = vec3(0);

	EvalIBL(skylight, pixel.multibounceAO, pixel, iblFd, iblFr);

	iblColor += (iblFd + iblFr);

	//direct lighting
	vec3 L0 = vec3(0);
	uint clusterTileIndex = GetClusterIndex(round(vec2(gl_FragCoord.xy) + vec2(0.01)), fovAspectResXResY.zw, exposureNearFar.yz, clusterScaleBias, clusterTileSizes, zPrePassSurface);

	EvalAllDirectionalLightsWithAnisotropy(L0, pixel);
	EvalAllPointLightsWithAnisotropy(clusterTileIndex, L0, pixel);
	EvalAllSpotLightsWithAnisotropy(clusterTileIndex, L0, pixel);

	return iblColor + L0;
}

#endif