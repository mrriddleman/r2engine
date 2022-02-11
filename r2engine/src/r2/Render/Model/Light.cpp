#include "r2pch.h"
#include "r2/Render/Model/Light.h"
#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Core/Math/MathUtils.h"

namespace r2::draw::light
{
	s64 s_lightSystemID = -1;
	
}


namespace r2::draw::light
{

	void GetDirectionalLightSpaceMatrices(LightSpaceMatrixData& lightSpaceMatrixData, const Camera& cam, const glm::vec3& lightDir, float zMult, const u32 shadowMapSizes[cam::NUM_FRUSTUM_SPLITS])
	{
		glm::vec3 frustumCorners[cam::NUM_FRUSTUM_CORNERS];
		
		for (u32 i = 0; i < r2::cam::NUM_FRUSTUM_SPLITS; ++i)
		{
			cam::GetFrustumCorners(cam.frustumProjections[i], cam.view, frustumCorners);

			GetDirectionalLightSpaceMatricesForCascade(lightSpaceMatrixData, lightDir, i, zMult, shadowMapSizes, frustumCorners);
		}
	}

	//Credit: http://www.alextardif.com/shadowmapping.html
	void GetDirectionalLightSpaceMatricesForCascade(LightSpaceMatrixData& lightSpaceMatrixData, const glm::vec3& lightDir, u32 cascadeIndex, float zMult, const u32 shadowMapSizes[cam::NUM_FRUSTUM_SPLITS], const glm::vec3 frustumCorners[cam::NUM_FRUSTUM_CORNERS])
	{
		glm::vec3 frustumCenter = cam::GetFrustumCenter(frustumCorners);

		const float diameter = glm::length((frustumCorners[0] - frustumCorners[6]));
		const float radius = diameter / 2.0f;

		float texelsPerUnit = (float)shadowMapSizes[cascadeIndex] / diameter;

		glm::mat4 scalar = glm::mat4(1.0f);
		scalar = glm::scale(scalar, glm::vec3(texelsPerUnit));

		constexpr glm::vec3 ZERO = glm::vec3(0.0f);
		glm::vec3 baseLookAt = -lightDir;

		glm::mat4 lookAtMat = glm::lookAt(ZERO, baseLookAt, math::GLOBAL_UP) * scalar;
		glm::mat4 lookAtMatInv = glm::inverse(lookAtMat);

		frustumCenter = lookAtMat * glm::vec4(frustumCenter, 1.0f);
		frustumCenter.x = (float)floor(frustumCenter.x);
		frustumCenter.y = (float)floor(frustumCenter.y);
		frustumCenter = lookAtMatInv * glm::vec4(frustumCenter, 1.0f);

		glm::vec3 eye = frustumCenter - (lightDir * diameter);

		lightSpaceMatrixData.lightViewMatrices[cascadeIndex] = glm::lookAt(eye, frustumCenter, math::GLOBAL_UP);

		constexpr float mult = 1.25f;

		lightSpaceMatrixData.lightProjMatrices[cascadeIndex] = glm::ortho(-radius * mult, radius * mult, -radius * mult, radius * mult, -radius * zMult, radius * zMult);
	}

}


namespace r2::draw::lightsys
{
	void CalculateDirectionLightProjView(LightSystem& system, const Camera& cam, const glm::vec3& radius, const glm::mat4& centerTransform, s64 directionLightIndex)
	{
		R2_CHECK(directionLightIndex >= 0 && directionLightIndex < system.mSceneLighting.mNumDirectionLights, "We should have a proper light index");

		DirectionLight& dirLight = system.mSceneLighting.mDirectionLights[directionLightIndex];

		glm::vec3 eye = -dirLight.direction;
		glm::vec3 up = cam::GetWorldRight(cam);

		glm::mat4 lightView = glm::lookAt(eye, glm::vec3(0), up);

		lightView *= centerTransform;

		glm::mat4 lightProj = glm::ortho(-radius.x, radius.x, -radius.y, radius.y, -radius.z / 10.0f, radius.z / 10.f);
		
		dirLight.cameraViewToLightProj = lightProj * lightView * cam.invView;

		for (u32 i = 0; i < cam::NUM_FRUSTUM_SPLITS; ++i)
		{
			dirLight.lightSpaceMatrixData.lightViewMatrices[i] = lightView;
			dirLight.lightSpaceMatrixData.lightProjMatrices[i] = lightProj;
		}
	}

	PointLightHandle GeneratePointLightHandle(const LightSystem& system)
	{
		R2_CHECK(r2::squeue::Size(*system.mMetaData.mDirectionlightIDs) > 0, "We don't have any more point lights that can be added");

		auto pointLightID = r2::squeue::First(*system.mMetaData.mPointLightIDs);

		r2::squeue::PopFront(*system.mMetaData.mPointLightIDs);

		return { system.mSystemHandle, pointLightID };
	}

	DirectionLightHandle GenerateDirectionLightHandle(const LightSystem& system)
	{
		R2_CHECK(r2::squeue::Size(*system.mMetaData.mDirectionlightIDs) > 0, "We don't have any more direction lights that can be added");

		auto directionLightID = r2::squeue::First(*system.mMetaData.mDirectionlightIDs);

		r2::squeue::PopFront(*system.mMetaData.mDirectionlightIDs);

		return { system.mSystemHandle, directionLightID };
	}

	SpotLightHandle GenerateSpotLightHandle(const LightSystem& system)
	{
		R2_CHECK(r2::squeue::Size(*system.mMetaData.mSpotlightIDs) > 0, "We don't have any more spot lights that can be added");

		auto spotlightID = r2::squeue::First(*system.mMetaData.mSpotlightIDs);

		r2::squeue::PopFront(*system.mMetaData.mSpotlightIDs);

		return { system.mSystemHandle, spotlightID };
	}

	SkyLightHandle GenerateSkyLightHandle(const LightSystem& system)
	{
		return { system.mSystemHandle, 0 };
	}

	void ReclaimPointLightHandle(LightSystem& system, PointLightHandle handle)
	{
		R2_CHECK(system.mSystemHandle == handle.lightSystemHandle, "We're trying to reclaim a point light handle that doesn't match the light system you passed in");
		R2_CHECK(handle.handle >= 0 && handle.handle < light::MAX_NUM_LIGHTS, "Passed in an invalid point light handle");

		r2::squeue::PushBack(*system.mMetaData.mPointLightIDs, handle.handle);
	}

	void ReclaimSpotLightHandle(LightSystem& system, SpotLightHandle handle)
	{
		R2_CHECK(system.mSystemHandle == handle.lightSystemHandle, "We're trying to reclaim a spot light handle that doesn't match the light system you passed in");
		R2_CHECK(handle.handle >= 0 && handle.handle < light::MAX_NUM_LIGHTS, "Passed in an invalid spot light handle");

		r2::squeue::PushBack(*system.mMetaData.mSpotlightIDs, handle.handle);
	}

	void ReclaimDirectionLightHandle(LightSystem& system, DirectionLightHandle handle)
	{
		R2_CHECK(system.mSystemHandle == handle.lightSystemHandle, "We're trying to reclaim a direction light handle that doesn't match the light system you passed in");
		R2_CHECK(handle.handle >= 0 && handle.handle < light::MAX_NUM_LIGHTS, "Passed in an invalid direction light handle");

		r2::squeue::PushBack(*system.mMetaData.mDirectionlightIDs, handle.handle);
	}

	void Init(s64 pointLightID, s64 directionLightID, s64 spotLightID)
	{
	}

	void ClearModifiedLights(LightSystem& system)
	{
		r2::sarr::Clear(*system.mMetaData.mPointLightsModifiedList);
		r2::sarr::Clear(*system.mMetaData.mDirectionLightsModifiedList);
		r2::sarr::Clear(*system.mMetaData.mSpotLightsModifiedList);
		system.mMetaData.mShouldUpdateSkyLight = false;
	}

	bool Update(LightSystem& system, const Camera& cam, glm::vec3& lightProjRadius)
	{

		const auto numDirLightsToUpdate = r2::sarr::Size(*system.mMetaData.mDirectionLightsModifiedList);
		const auto numPointLightsToUpdate = r2::sarr::Size(*system.mMetaData.mPointLightsModifiedList);
		const auto numSpotLightsToUpdate = r2::sarr::Size(*system.mMetaData.mSpotLightsModifiedList);

		bool needsUpdate = numDirLightsToUpdate > 0 || numPointLightsToUpdate > 0 || numSpotLightsToUpdate > 0 || system.mMetaData.mShouldUpdateSkyLight;

		if(needsUpdate)
			ClearModifiedLights(system);

		return needsUpdate;
	}


	void SetShouldUpdatePointLight(LightSystem& system, s64 lightIndex)
	{
		r2::sarr::Push(*system.mMetaData.mPointLightsModifiedList, lightIndex);
	}

	void SetShouldUpdateDirectionLight(LightSystem& system, s64 lightIndex)
	{
		r2::sarr::Push(*system.mMetaData.mDirectionLightsModifiedList, lightIndex);
	}

	void SetShouldUpdateSpotLight(LightSystem& system, s64 lightIndex)
	{
		r2::sarr::Push(*system.mMetaData.mSpotLightsModifiedList, lightIndex);
	}

	void SetShouldUpdateSkyLight(LightSystem& system)
	{
		system.mMetaData.mShouldUpdateSkyLight = true;
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

		SetShouldUpdatePointLight(system, pointLightIndex);

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


		r2::shashmap::Set(*system.mMetaData.mPointLightMap, lightHandle.handle, defaultIndex);

		//swap the point light with the last point light

		if (lightIndex == (system.mSceneLighting.mNumPointLights - 1))
		{
			system.mSceneLighting.mNumPointLights--;
		}
		else
		{
			const PointLight& lastPointLight = system.mSceneLighting.mPointLights[system.mSceneLighting.mNumPointLights - 1];

			system.mSceneLighting.mPointLights[lightIndex] = lastPointLight;

			r2::shashmap::Set(*system.mMetaData.mPointLightMap, lastPointLight.lightProperties.lightID, lightIndex);

			system.mSceneLighting.mNumPointLights--;
		}

		ReclaimPointLightHandle(system, lightHandle);

		SetShouldUpdatePointLight(system, lightIndex);

		return true;
	}

	PointLight* GetPointLightPtr(LightSystem& system, PointLightHandle handle)
	{
		s64 defaultIndex = -1;
		s64 lightIndex = r2::shashmap::Get(*system.mMetaData.mPointLightMap, handle.handle, defaultIndex);

		if (lightIndex != defaultIndex)
		{
			SetShouldUpdatePointLight(system, lightIndex);
			return &system.mSceneLighting.mPointLights[lightIndex];
		}

		return nullptr;
	}

	const PointLight* GetPointLightConstPtr(const LightSystem& system, PointLightHandle handle)
	{
		s64 defaultIndex = -1;
		s64 lightIndex = r2::shashmap::Get(*system.mMetaData.mPointLightMap, handle.handle, defaultIndex);

		if (lightIndex != defaultIndex)
		{
			return &system.mSceneLighting.mPointLights[lightIndex];
		}

		return nullptr;
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

		SetShouldUpdateDirectionLight(system, lightIndex);

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

		r2::shashmap::Set(*system.mMetaData.mDirectionLightMap, lightHandle.handle, defaultIndex);

		//swap the point light with the last point light
		
		if (lightIndex == (system.mSceneLighting.mNumDirectionLights - 1))
		{
			system.mSceneLighting.mNumDirectionLights--;
		}
		else
		{
			const DirectionLight& lastDirectionalLight = system.mSceneLighting.mDirectionLights[system.mSceneLighting.mNumDirectionLights-1];

			system.mSceneLighting.mDirectionLights[lightIndex] = lastDirectionalLight;

			r2::shashmap::Set(*system.mMetaData.mDirectionLightMap, lastDirectionalLight.lightProperties.lightID, lightIndex);

			system.mSceneLighting.mNumDirectionLights--;
		}

		ReclaimDirectionLightHandle(system, lightHandle);

		SetShouldUpdateDirectionLight(system, lightIndex);

		return true;
	}

	DirectionLight* GetDirectionLightPtr(LightSystem& system, DirectionLightHandle handle)
	{
		s64 defaultIndex = -1;
		s64 lightIndex = r2::shashmap::Get(*system.mMetaData.mDirectionLightMap, handle.handle, defaultIndex);

		if (lightIndex != defaultIndex)
		{
			SetShouldUpdateDirectionLight(system, lightIndex);
			return &system.mSceneLighting.mDirectionLights[lightIndex];
		}

		return nullptr;
	}

	const DirectionLight* GetDirectionLightConstPtr(const LightSystem& system, DirectionLightHandle handle)
	{
		s64 defaultIndex = -1;
		s64 lightIndex = r2::shashmap::Get(*system.mMetaData.mDirectionLightMap, handle.handle, defaultIndex);

		if (lightIndex != defaultIndex)
		{
			return &system.mSceneLighting.mDirectionLights[lightIndex];
		}

		return nullptr;
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


		SetShouldUpdateSpotLight(system, lightIndex);

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

		r2::shashmap::Set(*system.mMetaData.mSpotLightMap, lightHandle.handle, defaultIndex);

		//swap the point light with the last point light

		if (lightIndex == (system.mSceneLighting.mNumSpotLights - 1))
		{
			system.mSceneLighting.mNumSpotLights--;
		}
		else
		{
			const SpotLight& lastSpotLight = system.mSceneLighting.mSpotLights[system.mSceneLighting.mNumSpotLights - 1];

			system.mSceneLighting.mSpotLights[lightIndex] = lastSpotLight;

			r2::shashmap::Set(*system.mMetaData.mSpotLightMap, lastSpotLight.lightProperties.lightID, lightIndex);

			system.mSceneLighting.mNumSpotLights--;
		}

		SetShouldUpdateSpotLight(system, lightIndex);

		ReclaimSpotLightHandle(system, lightHandle);

		return true;
	}

	SpotLight* GetSpotLightPtr(LightSystem& system, SpotLightHandle handle)
	{
		s64 defaultIndex = -1;
		s64 lightIndex = r2::shashmap::Get(*system.mMetaData.mSpotLightMap, handle.handle, defaultIndex);

		if (lightIndex != defaultIndex)
		{
			SetShouldUpdateSpotLight(system, lightIndex);
			return &system.mSceneLighting.mSpotLights[lightIndex];
		}

		return nullptr;
	}

	const SpotLight* GetSpotLightConstPtr(const LightSystem& system, SpotLightHandle handle)
	{
		s64 defaultIndex = -1;
		s64 lightIndex = r2::shashmap::Get(*system.mMetaData.mSpotLightMap, handle.handle, defaultIndex);

		if (lightIndex != defaultIndex)
		{
			return &system.mSceneLighting.mSpotLights[lightIndex];
		}

		return nullptr;
	}


	SkyLightHandle AddSkyLight(LightSystem& system, const SkyLight& skylight, s32 numPrefilteredMips)
	{
		SkyLightHandle skylightHandle = GenerateSkyLightHandle(system);

		system.mSceneLighting.mSkyLight = skylight;

		system.mSceneLighting.mSkyLight.lightProperties.lightID = skylightHandle.handle;

		system.mSceneLighting.numPrefilteredRoughnessMips = numPrefilteredMips;

		SetShouldUpdateSkyLight(system);

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

		SetShouldUpdateSkyLight(system);

		return true;
	}

	SkyLight* GetSkyLightPtr(LightSystem& system, SkyLightHandle handle)
	{
		if (system.mSceneLighting.mSkyLight.lightProperties.lightID == handle.handle)
		{
			SetShouldUpdateSkyLight(system);
			return &system.mSceneLighting.mSkyLight;
		}

		return nullptr;
	}

	const SkyLight* GetSkyLightConstPtr(const LightSystem& system, SkyLightHandle handle)
	{
		if (system.mSceneLighting.mSkyLight.lightProperties.lightID == handle.handle)
		{
			return &system.mSceneLighting.mSkyLight;
		}

		return nullptr;
	}

	LightSystemHandle GenerateNewLightSystemHandle()
	{
		return ++light::s_lightSystemID;
	}

	void ClearAllLighting(LightSystem& system)
	{

	}

	void InitLightIDs(LightSystem& system)
	{
		r2::squeue::ConsumeAll(*system.mMetaData.mPointLightIDs);
		r2::squeue::ConsumeAll(*system.mMetaData.mSpotlightIDs);
		r2::squeue::ConsumeAll(*system.mMetaData.mDirectionlightIDs);

		for (s64 i = 0; i < light::MAX_NUM_LIGHTS; ++i)
		{
			r2::squeue::PushBack(*system.mMetaData.mPointLightIDs, i);
			r2::squeue::PushBack(*system.mMetaData.mSpotlightIDs, i);
			r2::squeue::PushBack(*system.mMetaData.mDirectionlightIDs, i);
		}
	}

	void ClearShadowMapPages(LightSystem& system)
	{
		for (u32 i = 0; i < light::NUM_SPOTLIGHT_SHADOW_PAGES; ++i)
		{
			system.mShadowMapPages.mSpotLightShadowMapPages[i] = -1;
		}

		for (u32 i = 0; i < light::NUM_POINTLIGHT_SHADOW_PAGES; ++i)
		{
			system.mShadowMapPages.mPointLightShadowMapPages[i] = -1;
		}

		for (u32 i = 0; i < light::NUM_DIRECTIONLIGHT_SHADOW_PAGES; ++i)
		{
			system.mShadowMapPages.mDirectionLightShadowMapPages[i] = -1;
		}
	}
	
}

namespace r2::draw
{
	u64 LightSystem::MemorySize(u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(LightSystem), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<s64>::MemorySize(light::MAX_NUM_LIGHTS * r2::SHashMap<u32>::LoadFactorMultiplier()), alignment, headerSize, boundsChecking) * 3 +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<s64>::MemorySize(light::MAX_NUM_LIGHTS), alignment, headerSize, boundsChecking) * 3 +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SQueue<s64>::MemorySize(light::MAX_NUM_LIGHTS), alignment, headerSize, boundsChecking) * 3;
	}
}