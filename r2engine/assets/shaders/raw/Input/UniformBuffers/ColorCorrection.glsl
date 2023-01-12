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

float FilmGrainRandom(vec2 p)
{
	vec2 K1 = vec2(
    	23.14069263277926, // e^pi (Gelfond's constant)
    	2.665144142690225 // 2^sqrt(2) (Gelfond–Schneider constant)
  	);

	return fract( cos( dot(p,K1) * cameraPosTimeW.w ) * 12345.6789 );
}

vec3 ApplyFilmGrain(vec3 color, vec2 uv)
{
 	vec2 uvRandom = uv;
 	uvRandom.y *= FilmGrainRandom(vec2(uvRandom.y, cc_filmGrainStrength));
 	color += FilmGrainRandom(uvRandom) * cc_filmGrainStrength;

 	return color;
}

#endif
