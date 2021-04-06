#ifndef __LIGHT_H__
#define __LIGHT_H__


#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SHashMap.h"
#include "glm/glm.hpp"

namespace r2::draw
{
	namespace light
	{
		const u32 MAX_NUM_LIGHTS = 100;
		
	}

	using LightSystemHandle = s64;

	struct LightHandle
	{
		LightSystemHandle lightSystemHandle = -1;
		s64 handle = -1;
	};

	using PointLightHandle = LightHandle;
	using DirectionLightHandle = LightHandle;
	using SpotLightHandle = LightHandle;


	struct LightProperties
	{
		glm::vec4 color = glm::vec4(1.0f);
		
		float fallOff = 1.0f;
		float intensity = 1.0f;
		s64 lightID = -1; //internal use only
	};

	struct DirectionLight
	{
		LightProperties lightProperties;
		glm::vec4 direction;
	};

	struct PointLight
	{
		LightProperties lightProperties;
		glm::vec4 position;	
	};

	struct SpotLight
	{
		LightProperties lightProperties;
		glm::vec4 position;
		glm::vec4 direction;
		//float radius; //cosine angle
		//float cutoff; //cosine angle
	};

	//@NOTE: right now this is the same format as the shaders. If we change the shader layout, we have to change this

	struct SceneLighting
	{
		PointLight mPointLights[light::MAX_NUM_LIGHTS];
		DirectionLight mDirectionLights[light::MAX_NUM_LIGHTS];
		SpotLight mSpotLights[light::MAX_NUM_LIGHTS];

		s32 mNumPointLights = 0;
		s32 mNumDirectionLights = 0;
		s32 mNumSpotLights = 0;
		s32 temp = 0;
	};


	struct SceneLightMetaData
	{
		r2::SHashMap<s64>* mPointLightMap = nullptr;
		r2::SHashMap<s64>* mDirectionLightMap = nullptr;
		r2::SHashMap<s64>* mSpotLightMap = nullptr;
	};

	struct LightSystem
	{
		LightSystemHandle mSystemHandle;
		SceneLightMetaData mMetaData;
		SceneLighting mSceneLighting;

		static u64 MemorySize(u64 alignment, u32 headerSize, u32 boundsChecking);
	};

	namespace lightsys
	{
		void Init(s64 pointLightID, s64 directionLightID, s64 spotLightID);

		template <class ARENA>
		LightSystem* CreateLightSystem(ARENA& arena);

		template <class ARENA>
		void DestroyLightSystem(ARENA& arena, LightSystem* system);

		PointLightHandle AddPointLight(LightSystem& system, const PointLight& pointLight);
		bool RemovePointLight(LightSystem& system, PointLightHandle lightHandle);

		DirectionLightHandle AddDirectionalLight(LightSystem& system, const DirectionLight& dirLight);
		bool RemoveDirectionalLight(LightSystem& system, DirectionLightHandle lightHandle);

		SpotLightHandle AddSpotLight(LightSystem& system, const SpotLight& spotLight);
		bool RemoveSpotLight(LightSystem& system, SpotLightHandle lightHandle);

		LightSystemHandle GenerateNewLightSystemHandle();
	}


	namespace lightsys
	{
		template <class ARENA>
		LightSystem* CreateLightSystem(ARENA& arena)
		{
			LightSystem* lightSystem = ALLOC(LightSystem, arena);
			R2_CHECK(lightSystem != nullptr, "We couldn't create the light system!");

			lightSystem->mMetaData.mPointLightMap = MAKE_SHASHMAP(arena, s64, light::MAX_NUM_LIGHTS * r2::SHashMap<s64>::LoadFactorMultiplier());
			R2_CHECK(lightSystem->mMetaData.mPointLightMap != nullptr, "We couldn't create the light system!");
			
			lightSystem->mMetaData.mDirectionLightMap = MAKE_SHASHMAP(arena, s64, light::MAX_NUM_LIGHTS * r2::SHashMap<s64>::LoadFactorMultiplier());
			R2_CHECK(lightSystem->mMetaData.mDirectionLightMap != nullptr, "We couldn't create the light system!");
			
			lightSystem->mMetaData.mSpotLightMap = MAKE_SHASHMAP(arena, s64, light::MAX_NUM_LIGHTS * r2::SHashMap<s64>::LoadFactorMultiplier());
			R2_CHECK(lightSystem->mMetaData.mSpotLightMap != nullptr, "We couldn't create the light system!");

			lightSystem->mSystemHandle = GenerateNewLightSystemHandle();


			memset(&lightSystem->mSceneLighting, 0, sizeof(SceneLighting));

			return lightSystem;
		}

		template <class ARENA>
		void DestroyLightSystem(ARENA& arena, LightSystem* system)
		{
			if (!system || !system->mMetaData.mSpotLightMap ||
				!system->mMetaData.mDirectionLightMap || !system->mMetaData.mPointLightMap)
			{
				R2_CHECK(false, "We probably shouldn't be destroying a null light system");
				return;
			}
			
			FREE(system->mMetaData.mSpotLightMap, arena);
			FREE(system->mMetaData.mDirectionLightMap, arena);
			FREE(system->mMetaData.mPointLightMap, arena);
			FREE(system, arena);
		}
	}
}


#endif