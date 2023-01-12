#ifndef GLSL_COLOR_CORRECTION
#define GLSL_COLOR_CORRECTION

#include "Common/CommonFunctions.glsl"
#include "Input/UniformBuffers/Vectors.glsl"


layout (std140, binding = 7) uniform ColorCorrectionValues
{
	float cc_contrast;
	float cc_brightness;
	float cc_saturation;
	float cc_gamma;
	float cc_filmGrainStrength;
};

vec3 ToGrayscaleColor(vec3 color)
{
	return vec3(dot(color, vec3(0.299, 0.587, 0.114)));
}

vec3 ApplyContrastAndBrightness(vec3 color, float contrast, float brightness)
{
	return Saturate( contrast * (color - vec3(0.5)) + vec3(0.5) + vec3(brightness));
}

vec3 ApplySaturation(vec3 color, float saturation)
{
	return Saturate( mix(ToGrayscaleColor(color), color, saturation));
}

vec3 GammaCorrect(vec3 color, float gamma)
{
    return pow(color, vec3(gamma));
}

vec3 GammaCorrect(vec3 color)
{
	return pow(color, vec3(cc_gamma));
}

vec3 ApplyFilmGrain(vec3 color, float strength)
{
	float randomIntensity = fract(10000 * sin(gl_FragCoord.x + gl_FragCoord.y * cameraPosTimeW.w) * PI);
	strength *= randomIntensity;
	return color + strength;
}

#endif
