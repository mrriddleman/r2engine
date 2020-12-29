#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "glm/glm.hpp"

namespace r2::draw
{

	struct Light
	{
		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;
		glm::vec3 emission;
	};

	struct AttenuationState
	{
		float constant;
		float linear;
		float quadratic;
	};

	struct LightProperties
	{
		Light light;
		glm::vec3 color;
		float specular;
		float strength;

		AttenuationState attenuation;
	};

	struct DirectionLight
	{
		LightProperties lightProperties;
		glm::vec3 direction;
	};

	struct PointLight
	{
		LightProperties lightPropterties;
		glm::vec3 position;
	};

	struct SpotLight
	{
		LightProperties lightProperties;
		glm::vec3 position;
		glm::vec3 direction;
		float radius; //cosine angle
		float cutoff; //cosine angle
	};
}


#endif