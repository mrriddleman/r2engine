#ifndef GLSL_DEFINES
#define GLSL_DEFINES

#define PI 3.141592653589793238462
#define TWO_PI 2.0 * PI
#define NUM_FRUSTUM_SPLITS 4

#define MAX_NUM_DIRECTIONAL_LIGHTS 50
#define MAX_NUM_POINT_LIGHTS 4096
#define MAX_NUM_SPOT_LIGHTS MAX_NUM_POINT_LIGHTS

const uint NUM_SIDES_FOR_POINTLIGHT = 6;
const vec3 GLOBAL_UP = vec3(0, 0, 1);

#define NUM_SPOTLIGHT_LAYERS 1
#define NUM_POINTLIGHT_LAYERS 6
#define NUM_DIRECTIONLIGHT_LAYERS NUM_FRUSTUM_SPLITS

struct PixelData
{
	vec3 baseColor;
	vec3 N; //normal to use
	vec3 V; //view vector (camera look)
	vec3 uv; //texture coords
	vec3 diffuseColor;
	vec3 R; //reflection vector
	vec3 F0; //specular reflectance at 0
	vec3 Fd; //diffuse reflectance
	float reflectance; //reflectance amount
	float metallic; 
	float ao; //ambient occlusion amount
	float roughness; 
	float NoV;
	float ToV;
	float BoV;

	vec3 energyCompensation;
	float ggxVTerm;

	float anisotropy;
	float at;
	float ab;
	vec3 anisotropicT;
	vec3 anisotropicB;

	float clearCoat;
	float clearCoatRoughness;
	float clearCoatPerceptualRoughness;
	vec3 clearCoatNormal;
};

#endif