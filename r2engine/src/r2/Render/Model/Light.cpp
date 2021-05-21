#include "r2pch.h"
#include "r2/Render/Model/Light.h"
#include "r2/Render/Model/Textures/TextureSystem.h"

namespace r2::draw::light
{
	s64 s_lightSystemID = -1;
}

namespace r2::draw::lightsys
{
	s32 s_PointLightID = 0;
	s32 s_DirectionalLightID = 0;
	s32 s_SpotLightID = 0;


	PointLightHandle GeneratePointLightHandle(const LightSystem& system)
	{
		return { system.mSystemHandle, s_PointLightID++ };
	}

	DirectionLightHandle GenerateDirectionLightHandle(const LightSystem& system)
	{
		return { system.mSystemHandle, s_DirectionalLightID++ };
	}

	SpotLightHandle GenerateSpotLightHandle(const LightSystem& system)
	{
		return { system.mSystemHandle, s_SpotLightID++ };
	}

	SkyLightHandle GenerateSkyLightHandle(const LightSystem& system)
	{
		return { system.mSystemHandle, 0 };
	}

	void Init(s64 pointLightID, s64 directionLightID, s64 spotLightID)
	{
		s_PointLightID = pointLightID;
		s_DirectionalLightID = directionLightID;
		s_SpotLightID = spotLightID;
	}

	PointLightHandle AddPointLight(LightSystem& system, const PointLight& pointLight)
	{
		if (system.mSceneLighting.mNumPointLights + 1 > light::MAX_NUM_LIGHTS)
		{
			R2_CHECK(false, "We're trying to add more point lights than we have space for");
			return {};	
		}

		PointLightHandle newPointLightHandle = GeneratePointLightHandle(system);

		s64 pointLightIndex = system.mSceneLighting.mNumPointLights;
		++system.mSceneLighting.mNumPointLights;
		system.mSceneLighting.mPointLights[pointLightIndex] = pointLight;

		system.mSceneLighting.mPointLights[pointLightIndex].lightProperties.lightID = newPointLightHandle.handle;

		r2::shashmap::Set(*system.mMetaData.mPointLightMap, newPointLightHandle.handle, pointLightIndex);

		return newPointLightHandle;
	}

	bool RemovePointLight(LightSystem& system, PointLightHandle lightHandle)
	{
		if (system.mSystemHandle != lightHandle.lightSystemHandle)
		{
			R2_CHECK(false, "We're trying to remove a light that doesn't belong to this system!");
			return false;
		}

		if (system.mSceneLighting.mNumPointLights <= 0)
		{
			return false;
		}

		s64 defaultIndex = -1;

		s64 lightIndex = r2::shashmap::Get(*system.mMetaData.mPointLightMap, lightHandle.handle, defaultIndex);

		if (lightIndex <= defaultIndex || lightIndex >= light::MAX_NUM_LIGHTS)
		{
			R2_CHECK(false, "We couldn't find the light!");
			return false;
		}

		//swap the point light with the last point light
		system.mSceneLighting.mNumPointLights--;

		if (system.mSceneLighting.mNumPointLights != lightIndex)
		{
			const PointLight& lastPointLight = system.mSceneLighting.mPointLights[system.mSceneLighting.mNumPointLights];

			system.mSceneLighting.mPointLights[lightIndex] = lastPointLight;

			r2::shashmap::Set(*system.mMetaData.mPointLightMap, lastPointLight.lightProperties.lightID, lightIndex);
		}

		return true;
	}

	DirectionLightHandle AddDirectionalLight(LightSystem& system, const DirectionLight& dirLight)
	{
		if (system.mSceneLighting.mNumDirectionLights + 1 > light::MAX_NUM_LIGHTS)
		{
			R2_CHECK(false, "We're trying to add more point lights than we have space for");
			return {};
		}

		DirectionLightHandle newDirectionalLightHandle = GenerateDirectionLightHandle(system);

		s64 lightIndex = system.mSceneLighting.mNumDirectionLights;
		++system.mSceneLighting.mNumDirectionLights;
		system.mSceneLighting.mDirectionLights[lightIndex] = dirLight;

		system.mSceneLighting.mDirectionLights[lightIndex].lightProperties.lightID = newDirectionalLightHandle.handle;

		r2::shashmap::Set(*system.mMetaData.mDirectionLightMap, newDirectionalLightHandle.handle, lightIndex);

		return newDirectionalLightHandle;
	}

	bool RemoveDirectionalLight(LightSystem& system, DirectionLightHandle lightHandle)
	{
		if (system.mSystemHandle != lightHandle.lightSystemHandle)
		{
			R2_CHECK(false, "We're trying to remove a light that doesn't belong to this system!");
			return false;
		}

		if (system.mSceneLighting.mNumDirectionLights <= 0)
		{
			return false;
		}

		s64 defaultIndex = -1;

		s64 lightIndex = r2::shashmap::Get(*system.mMetaData.mDirectionLightMap, lightHandle.handle, defaultIndex);

		if (lightIndex <= defaultIndex || lightIndex >= light::MAX_NUM_LIGHTS)
		{
			R2_CHECK(false, "We couldn't find the light!");
			return false;
		}

		//swap the point light with the last point light
		system.mSceneLighting.mNumDirectionLights--;

		if (system.mSceneLighting.mNumDirectionLights != lightIndex)
		{
			const DirectionLight& lastDirectionalLight = system.mSceneLighting.mDirectionLights[system.mSceneLighting.mNumDirectionLights];

			system.mSceneLighting.mDirectionLights[lightIndex] = lastDirectionalLight;

			r2::shashmap::Set(*system.mMetaData.mDirectionLightMap, lastDirectionalLight.lightProperties.lightID, lightIndex);
		}

		return true;
	}

	SpotLightHandle AddSpotLight(LightSystem& system, const SpotLight& spotLight)
	{
		if (system.mSceneLighting.mNumSpotLights + 1 > light::MAX_NUM_LIGHTS)
		{
			R2_CHECK(false, "We're trying to add more point lights than we have space for");
			return {};
		}

		SpotLightHandle newSpotLightHandle = GenerateSpotLightHandle(system);

		s64 lightIndex = system.mSceneLighting.mNumSpotLights;
		++system.mSceneLighting.mNumSpotLights;
		system.mSceneLighting.mSpotLights[lightIndex] = spotLight;

		system.mSceneLighting.mSpotLights[lightIndex].lightProperties.lightID = newSpotLightHandle.handle;

		r2::shashmap::Set(*system.mMetaData.mSpotLightMap, newSpotLightHandle.handle, lightIndex);

		return newSpotLightHandle;
	}

	bool RemoveSpotLight(LightSystem& system, SpotLightHandle lightHandle)
	{
		if (system.mSystemHandle != lightHandle.lightSystemHandle)
		{
			R2_CHECK(false, "We're trying to remove a light that doesn't belong to this system!");
			return false;
		}

		if (system.mSceneLighting.mNumSpotLights <= 0)
		{
			return false;
		}

		s64 defaultIndex = -1;

		s64 lightIndex = r2::shashmap::Get(*system.mMetaData.mSpotLightMap, lightHandle.handle, defaultIndex);

		if (lightIndex <= defaultIndex || lightIndex >= light::MAX_NUM_LIGHTS)
		{
			R2_CHECK(false, "We couldn't find the light!");
			return false;
		}

		//swap the point light with the last point light
		system.mSceneLighting.mNumSpotLights--;

		if (system.mSceneLighting.mNumSpotLights != lightIndex)
		{
			const SpotLight& lastSpotLight = system.mSceneLighting.mSpotLights[system.mSceneLighting.mNumSpotLights];

			system.mSceneLighting.mSpotLights[lightIndex] = lastSpotLight;

			r2::shashmap::Set(*system.mMetaData.mSpotLightMap, lastSpotLight.lightProperties.lightID, lightIndex);
		}

		return true;
	}

	SkyLightHandle AddSkyLight(LightSystem& system, const SkyLight& skylight, s32 numPrefilteredMips)
	{
		SkyLightHandle skylightHandle = GenerateSkyLightHandle(system);

		system.mSceneLighting.mSkyLight = skylight;

		system.mSceneLighting.mSkyLight.lightProperties.lightID = skylightHandle.handle;

		system.mSceneLighting.numPrefilteredRoughnessMips = numPrefilteredMips;

		return skylightHandle;
	}

	SkyLightHandle AddSkyLight(LightSystem& system, const MaterialHandle& diffuseMaterialHandle, const MaterialHandle& prefilteredMaterialHandle, const MaterialHandle& lutDFGHandle)
	{
		SkyLight skyLight;

		r2::draw::MaterialSystem* matSystem = r2::draw::matsys::GetMaterialSystem(diffuseMaterialHandle.slot);
		R2_CHECK(matSystem != nullptr, "Failed to get the material system!");

		const Material* diffuseMaterial = mat::GetMaterial(*matSystem, diffuseMaterialHandle);
		
		R2_CHECK(diffuseMaterial != nullptr, "Skylight - The diffuse material is null!");
		const Material* prefilteredMaterial = mat::GetMaterial(*matSystem, prefilteredMaterialHandle);
		R2_CHECK(prefilteredMaterial != nullptr, "Skylight - The prefiltered material is null!");
		const Material* lutDFGMaterial = mat::GetMaterial(*matSystem, lutDFGHandle);
		R2_CHECK(lutDFGMaterial != nullptr, "Skylight - The lutDFG material is null!");

		//@TODO(Serge): would be nice to have some kind of verification here?
		skyLight.diffuseIrradianceTexture = texsys::GetTextureAddress(diffuseMaterial->diffuseTexture);
		skyLight.prefilteredRoughnessTexture = texsys::GetTextureAddress(prefilteredMaterial->diffuseTexture);
		skyLight.lutDFGTexture = texsys::GetTextureAddress(lutDFGMaterial->diffuseTexture);

		const auto* prefilteredCubemap = mat::GetCubemapTexture(*matSystem, prefilteredMaterialHandle);

		R2_CHECK(prefilteredCubemap != nullptr, "We should always get a proper cubemap here!");

		return AddSkyLight(system, skyLight, texsys::GetNumberOfMipMaps(*prefilteredCubemap));
	}

	bool RemoveSkyLight(LightSystem& system, SkyLightHandle skylightHandle)
	{
		if (system.mSystemHandle != skylightHandle.lightSystemHandle)
		{
			R2_CHECK(false, "We're trying to remove a light that doesn't belong to this system!");
			return false;
		}

		if (system.mSceneLighting.mSkyLight.lightProperties.lightID == skylightHandle.handle)
		{
			system.mSceneLighting.mSkyLight = {};
		}

		return true;
	}

	LightSystemHandle GenerateNewLightSystemHandle()
	{
		return ++light::s_lightSystemID;
	}

	
	
}

namespace r2::draw
{
	u64 LightSystem::MemorySize(u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(LightSystem), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<s64>::MemorySize(light::MAX_NUM_LIGHTS * r2::SHashMap<u32>::LoadFactorMultiplier()), alignment, headerSize, boundsChecking) * 3;
	}
}