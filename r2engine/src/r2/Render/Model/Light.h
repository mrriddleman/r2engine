#ifndef __LIGHT_H__
#define __LIGHT_H__


#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SHashMap.h"
#include <glm/glm.hpp>
#include "r2/Render/Model/Material.h"
#include "r2/Render/Camera/Camera.h"

namespace r2::draw
{
	namespace light
	{
		enum LightType : u8
		{
			LT_DIRECTIONAL_LIGHT = 0,
			LT_SPOT_LIGHT,
			LT_POINT_LIGHT,
			NUM_LIGHT_TYPES
		};


		const u32 MAX_NUM_LIGHTS = 50;
		const u32 SHADOW_MAP_SIZE = 1024;

		const u32 NUM_SPOTLIGHT_LAYERS = 1;
		const u32 NUM_POINTLIGHT_LAYERS = 6;
		const u32 NUM_DIRECTIONLIGHT_LAYERS = cam::NUM_FRUSTUM_SPLITS;

		constexpr u32 NUM_SPOTLIGHT_SHADOW_PAGES = MAX_NUM_LIGHTS ;
		constexpr u32 NUM_POINTLIGHT_SHADOW_PAGES = MAX_NUM_LIGHTS ;
		constexpr u32 NUM_DIRECTIONLIGHT_SHADOW_PAGES = MAX_NUM_LIGHTS ;
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
	using SkyLightHandle = LightHandle;

	struct LightProperties
	{
		glm::vec4 color = glm::vec4(1.0f);
		glm::uvec4 castsShadowsUseSoftShadows = glm::uvec4(0);
		float fallOff = 1.0f;
		float intensity = 1.0f;
		s64 lightID = -1; //internal use only
	};

	struct LightSpaceMatrixData
	{
		glm::mat4 lightViewMatrices[cam::NUM_FRUSTUM_SPLITS];
		glm::mat4 lightProjMatrices[cam::NUM_FRUSTUM_SPLITS];
	};

	struct DirectionLight
	{
		LightProperties lightProperties;
		glm::vec4 direction; //@TODO(Serge): make these vec3?
		

		glm::mat4 cameraViewToLightProj;


		//calculated automatically
		LightSpaceMatrixData lightSpaceMatrixData;
	};

	struct PointLight
	{
		LightProperties lightProperties;
		glm::vec4 position;	//@TODO(Serge): make these vec3?

		glm::mat4 lightSpaceMatrices[light::NUM_POINTLIGHT_LAYERS];
	};

	struct SpotLight
	{
		LightProperties lightProperties;
		glm::vec4 position;//@TODO(Serge): make these vec3?
		glm::vec4 direction;//@TODO(Serge): make these vec3?
		//float radius; //cosine angle
		//float cutoff; //cosine angle

		glm::mat4 lightSpaceMatrix;
	};

	struct SkyLight
	{
		LightProperties lightProperties;
		tex::TextureAddress diffuseIrradianceTexture;
		tex::TextureAddress prefilteredRoughnessTexture;
		tex::TextureAddress lutDFGTexture;
		//int numPrefilteredRoughnessMips;
	};

	//@NOTE: right now this is the same format as the shaders. If we change the shader layout, we have to change this

	struct SceneLighting
	{
		PointLight mPointLights[light::MAX_NUM_LIGHTS];
		DirectionLight mDirectionLights[light::MAX_NUM_LIGHTS];
		SpotLight mSpotLights[light::MAX_NUM_LIGHTS];
		SkyLight mSkyLight;

		s32 mNumPointLights = 0;
		s32 mNumDirectionLights = 0;
		s32 mNumSpotLights = 0;
		s32 numPrefilteredRoughnessMips = 0;
		s32 useSDSMShadows = 0;
	};

	struct SceneLightMetaData
	{
		r2::SHashMap<s64>* mPointLightMap = nullptr;
		r2::SHashMap<s64>* mDirectionLightMap = nullptr;
		r2::SHashMap<s64>* mSpotLightMap = nullptr;

		r2::SArray<s64>* mPointLightsModifiedList = nullptr;
		r2::SArray<s64>* mDirectionLightsModifiedList = nullptr;
		r2::SArray<s64>* mSpotLightsModifiedList = nullptr;

		r2::SQueue<s64>* mPointLightIDs = nullptr;
		r2::SQueue<s64>* mDirectionlightIDs = nullptr;
		r2::SQueue<s64>* mSpotlightIDs = nullptr;

		b32 mShouldUpdateSkyLight;
	};

	struct ShadowMapPages
	{
		float mSpotLightShadowMapPages[light::NUM_SPOTLIGHT_SHADOW_PAGES];
		float mPointLightShadowMapPages[light::NUM_POINTLIGHT_SHADOW_PAGES];
		float mDirectionLightShadowMapPages[light::NUM_DIRECTIONLIGHT_SHADOW_PAGES];
	};

	struct LightSystem
	{
		LightSystemHandle mSystemHandle;
		SceneLightMetaData mMetaData;
		SceneLighting mSceneLighting;
		ShadowMapPages mShadowMapPages;

		static u64 MemorySize(u64 alignment, u32 headerSize, u32 boundsChecking);
	};

	namespace light
	{
		void GetDirectionalLightSpaceMatrices(LightSpaceMatrixData& lightSpaceMatrixData, const Camera& cam, const glm::vec3& lightDir, float zMult, const u32 shadowMapSizes[cam::NUM_FRUSTUM_SPLITS]);
		void GetDirectionalLightSpaceMatricesForCascade(LightSpaceMatrixData& lightSpaceMatrixData, const glm::vec3& lightDir, u32 cascadeIndex, float zMult, const u32 shadowMapSizes[cam::NUM_FRUSTUM_SPLITS], const glm::vec3 frustumCorners[cam::NUM_FRUSTUM_CORNERS]);
	}

	namespace lightsys
	{
		void Init(s64 pointLightID, s64 directionLightID, s64 spotLightID);

		bool Update(LightSystem& system, const Camera& cam, glm::vec3& lightProjRadius);

		template <class ARENA>
		LightSystem* CreateLightSystem(ARENA& arena);

		template <class ARENA>
		void DestroyLightSystem(ARENA& arena, LightSystem* system);

		PointLightHandle AddPointLight(LightSystem& system, const PointLight& pointLight);
		bool RemovePointLight(LightSystem& system, PointLightHandle lightHandle);
		PointLight* GetPointLightPtr(LightSystem& system, PointLightHandle handle);
		const PointLight* GetPointLightConstPtr(const LightSystem& system, PointLightHandle handle);

		DirectionLightHandle AddDirectionalLight(LightSystem& system, const DirectionLight& dirLight);
		bool RemoveDirectionalLight(LightSystem& system, DirectionLightHandle lightHandle);
		DirectionLight* GetDirectionLightPtr(LightSystem& system, DirectionLightHandle handle);
		const DirectionLight* GetDirectionLightConstPtr(const LightSystem& system, DirectionLightHandle handle);

		SpotLightHandle AddSpotLight(LightSystem& system, const SpotLight& spotLight);
		bool RemoveSpotLight(LightSystem& system, SpotLightHandle lightHandle);
		SpotLight* GetSpotLightPtr(LightSystem& system, SpotLightHandle handle);
		const SpotLight* GetSpotLightConstPtr(const LightSystem& system, SpotLightHandle handle);

		SkyLightHandle AddSkyLight(LightSystem& system, const SkyLight& skylight, s32 numPrefilteredMips);
		SkyLightHandle AddSkyLight(LightSystem& system, const MaterialHandle& diffuseMaterial, const MaterialHandle& prefilteredMaterial, const MaterialHandle& lutDFG);
		bool RemoveSkyLight(LightSystem& system, SkyLightHandle skylightHandle);
		SkyLight* GetSkyLightPtr(LightSystem& system, SkyLightHandle handle);
		const SkyLight* GetSkyLightConstPtr(const LightSystem& system, SkyLightHandle handle);

		LightSystemHandle GenerateNewLightSystemHandle();


		void ClearAllLighting(LightSystem& system);

		void InitLightIDs(LightSystem& system);

		void ClearShadowMapPages(LightSystem& system);
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


			lightSystem->mMetaData.mPointLightsModifiedList = MAKE_SARRAY(arena, s64, light::MAX_NUM_LIGHTS);

			R2_CHECK(lightSystem->mMetaData.mPointLightsModifiedList != nullptr, "We couldn't create the modified list of the lights!");

			lightSystem->mMetaData.mDirectionLightsModifiedList = MAKE_SARRAY(arena, s64, light::MAX_NUM_LIGHTS);

			R2_CHECK(lightSystem->mMetaData.mDirectionLightsModifiedList != nullptr, "We couldn't create the modified list of the lights!");

			lightSystem->mMetaData.mSpotLightsModifiedList = MAKE_SARRAY(arena, s64, light::MAX_NUM_LIGHTS);

			R2_CHECK(lightSystem->mMetaData.mSpotLightsModifiedList != nullptr, "We couldn't create the modified list of the lights!");


			lightSystem->mMetaData.mPointLightIDs = MAKE_SQUEUE(arena, s64, light::MAX_NUM_LIGHTS);

			R2_CHECK(lightSystem->mMetaData.mPointLightIDs != nullptr, "We couldn't create the mPointLightIDs");

			lightSystem->mMetaData.mDirectionlightIDs = MAKE_SQUEUE(arena, s64, light::MAX_NUM_LIGHTS);

			R2_CHECK(lightSystem->mMetaData.mDirectionlightIDs != nullptr, "We couldn't create the mDirectionlightIDs");
			
			lightSystem->mMetaData.mSpotlightIDs = MAKE_SQUEUE(arena, s64, light::MAX_NUM_LIGHTS);

			R2_CHECK(lightSystem->mMetaData.mSpotlightIDs != nullptr, "We couldn't create the mSpotlightIDs");


			lightSystem->mSystemHandle = GenerateNewLightSystemHandle();


			memset(&lightSystem->mSceneLighting, 0, sizeof(SceneLighting));


			InitLightIDs(*lightSystem);
			ClearShadowMapPages(*lightSystem);

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

			FREE(system->mMetaData.mSpotlightIDs, arena);
			FREE(system->mMetaData.mDirectionlightIDs, arena);
			FREE(system->mMetaData.mPointLightIDs, arena);

			FREE(system->mMetaData.mSpotLightsModifiedList, arena);
			FREE(system->mMetaData.mDirectionLightsModifiedList, arena);
			FREE(system->mMetaData.mPointLightsModifiedList, arena);
			
			FREE(system->mMetaData.mSpotLightMap, arena);
			FREE(system->mMetaData.mDirectionLightMap, arena);
			FREE(system->mMetaData.mPointLightMap, arena);
			FREE(system, arena);
		}
	}
}


#endif