#ifndef GLSL_COLOR_CORRECTION
#define GLSL_COLOR_CORRECTION

#include "Common/CommonFunctions.glsl"
#include "Common/Texture.glsl"
#include "Input/UniformBuffers/Vectors.glsl"


layout (std140, binding = 7) uniform ColorCorrectionValues
{
	float cc_contrast;
	float cc_brightness;
	float cc_saturation;
	float cc_gamma;

	float cc_filmGrainStrength;
	float cg_halfColX;
	float cg_halfColY;
	float cg_numSwatches;

	Tex2DAddress cg_lut;

	float cg_contribution;
	float cc_temperature;
	float cc_tint;
	float cc_unused;
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

vec4 ApplyColorGrading(vec4 color)
{
	float maxColor = cg_numSwatches - 1.0f;
	float threshold = maxColor/ cg_numSwatches;

	float xOffset = cg_halfColX + color.r * threshold / cg_numSwatches;
	float yOffset = cg_halfColY + color.g * threshold;
	float cell = floor(color.b * maxColor);

	vec2 lutPos = vec2(cell / cg_numSwatches + xOffset, yOffset);

	vec4 lutColor = SampleTextureIRGBA(cg_lut, lutPos.rg);

	return mix(color, lutColor, cg_contribution);
}

vec3 ApplyWhiteBalance(vec3 col) 
{
    float t1 = cc_temperature * 10.0f / 6.0f;
    float t2 = cc_tint * 10.0f / 6.0f;

    float x = 0.31271 - t1 * (t1 < 0 ? 0.1 : 0.05);
    float standardIlluminantY = 2.87 * x - 3 * x * x - 0.27509507;
    float y = standardIlluminantY + t2 * 0.05;

    vec3 w1 = vec3(0.949237, 1.03542, 1.08728);

    float Y = 1;
    float X = Y * x / y;
    float Z = Y * (1 - x - y) / y;
    float L = 0.7328 * X + 0.4296 * Y - 0.1624 * Z;
    float M = -0.7036 * X + 1.6975 * Y + 0.0061 * Z;
    float S = 0.0030 * X + 0.0136 * Y + 0.9834 * Z;
    vec3 w2 = vec3(L, M, S);

    vec3 balance = vec3(w1.x / w2.x, w1.y / w2.y, w1.z / w2.z);

    mat3 LIN_2_LMS_MAT = mat3(
        3.90405e-1, 5.49941e-1, 8.92632e-3,
        7.08416e-2, 9.63172e-1, 1.35775e-3,
        2.31082e-2, 1.28021e-1, 9.36245e-1
    );

    mat3 LMS_2_LIN_MAT = mat3(
        2.85847e+0, -1.62879e+0, -2.48910e-2,
        -2.10182e-1,  1.15820e+0,  3.24281e-4,
        -4.18120e-2, -1.18169e-1,  1.06867e+0
    );

    vec3 lms = LIN_2_LMS_MAT * col;
    lms *= balance;
    return LMS_2_LIN_MAT * lms;
}


#endif
