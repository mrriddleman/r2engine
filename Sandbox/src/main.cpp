//
//  main.cpp
//  Sandbox
//
//  Created by Serge Lansiquot on 2019-02-02.
//  Copyright © 2019 Serge Lansiquot. All rights reserved.
//

#include "r2.h"
#include "r2/Core/EntryPoint.h"

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/System.h"
#include "r2/Core/Events/Events.h"

#include "r2/Game/ECSWorld/ECSWorld.h"
#include "PlayerCommandComponent.h"
#include "PlayerCommandSystem.h"
#include "CharacterControllerSystem.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"

#include "FacingComponent.h"
#include "r2/Core/Input/DefaultInputGather.h"
#include "FacingUpdateSystem.h"

#include "FacingComponentSerialization.h"
#include "GridPositionComponentSerialization.h"

#ifdef R2_EDITOR
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponentDataSource.h"
#include "FacingComponentInspectorPanelDataSource.h"
#endif

#include "r2/Game/ECS/ComponentArrayData_generated.h"

#include "../../r2engine/vendor/imgui/imgui.h"

namespace
{
    constexpr r2::util::Size g_resolutions[] = { {640, 480}, {1024, 768}, {1280, 768}, {1920, 1080}, {2560, 1440} };

    constexpr u32 RESOLUTIONS_COUNT = COUNT_OF(g_resolutions);
}

namespace
{
    bool ResolveCategoryPath(u32 category, char* path);
}

class Sandbox: public r2::Application
{
public:
    
    //const u64 NUM_DRAWS = 15;
    //const u64 NUM_DRAW_COMMANDS = 30;
    //const u64 NUM_BONES = 1000;

    virtual bool Init() override
    {
        //memoryAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("SandboxArea");

        //R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "Invalid memory area");
        //
        //r2::mem::MemoryArea* sandBoxMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
        //R2_CHECK(sandBoxMemoryArea != nullptr, "Failed to get the memory area!");

        //auto result = sandBoxMemoryArea->Init(Megabytes(4), 0);
        //R2_CHECK(result == true, "Failed to initialize memory area");
        //
        //subMemoryAreaHandle = sandBoxMemoryArea->AddSubArea(Megabytes(4));
        //R2_CHECK(subMemoryAreaHandle != r2::mem::MemoryArea::SubArea::Invalid, "sub area handle is invalid!");
        //
        //auto subMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle)->GetSubArea(subMemoryAreaHandle);
        
       // linearArenaPtr = EMPLACE_LINEAR_ARENA(*subMemoryArea);
        
       // R2_CHECK(linearArenaPtr != nullptr, "Failed to create linear arena!");
   //     mCamera.fov = glm::radians(70.0f);
   //     mCamera.aspectRatio = static_cast<float>(CENG.DisplaySize().width) / static_cast<float>(CENG.DisplaySize().height);
   //     mCamera.nearPlane = 0.1;
   //     mCamera.farPlane = 1000.0f;


   //     mPersController.SetCamera(&mCamera, glm::vec3(0.0f, -1.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), 10.0f);

   ////     mPersController.Init(10.0f, 70.0f, static_cast<float>(CENG.DisplaySize().width) / static_cast<float>(CENG.DisplaySize().height), 0.1f, 100.f, glm::vec3(0.0f, -1.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
   //     r2::draw::renderer::SetRenderCamera(mPersController.GetCameraPtr());
        
        //modelMats = MAKE_SARRAY(*linearArenaPtr, glm::mat4, NUM_DRAWS);
        /*animModelMats = MAKE_SARRAY(*linearArenaPtr, glm::mat4, NUM_DRAWS);

        mTransparentWindowMats = MAKE_SARRAY(*linearArenaPtr, glm::mat4, NUM_DRAWS);
        mTransparentWindowModelRefHandle = r2::draw::renderer::GetDefaultModelRef(r2::draw::QUAD);
        mTransparentWindowDrawFlags = MAKE_SARRAY(*linearArenaPtr, r2::draw::DrawFlags, NUM_DRAWS);*/

        //r2::draw::DrawFlags drawFlags;
        //drawFlags.Set(r2::draw::eDrawFlags::DEPTH_TEST);
        
    //    r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();
    //    r2::draw::TexturePacksCache& texturePacksCache = CENG.GetTexturePacksCache();

		//mSkeletonBoneTransforms = MAKE_SARRAY(*linearArenaPtr, r2::draw::ShaderBoneTransform, NUM_BONES);
		//mSkeletonDebugBones = MAKE_SARRAY(*linearArenaPtr, r2::draw::DebugBone, NUM_BONES);

		//mEllenBoneTransforms = MAKE_SARRAY(*linearArenaPtr, r2::draw::ShaderBoneTransform, NUM_BONES);
		//mEllenDebugBones = MAKE_SARRAY(*linearArenaPtr, r2::draw::DebugBone, NUM_BONES);


  //      glm::mat4 skeletonModel = glm::mat4(1.0f);
  //      skeletonModel = glm::translate(skeletonModel, glm::vec3(-3, 0, 0.1));
  //      skeletonModel = glm::rotate(skeletonModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
  //      skeletonModel = glm::scale(skeletonModel, glm::vec3(0.01f));
  //      r2::sarr::Push(*animModelMats, skeletonModel);

		//glm::mat4 skeletonModel2 = glm::mat4(1.0f);
		//skeletonModel2 = glm::translate(skeletonModel2, glm::vec3(-3, 1, 0.1));
		//skeletonModel2 = glm::rotate(skeletonModel2, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		//skeletonModel2 = glm::scale(skeletonModel2, glm::vec3(0.01f));
		//r2::sarr::Push(*animModelMats, skeletonModel2);


  //      glm::mat4 ellenModel = glm::mat4(1.0f);
  //      ellenModel = glm::translate(ellenModel, glm::vec3(3, 0, 0.1));
  //      ellenModel = glm::rotate(ellenModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
  //      ellenModel = glm::scale(ellenModel, glm::vec3(0.01f));
  //      r2::sarr::Push(*animModelMats, ellenModel);

  //      glm::mat4 ellenModel2 = glm::mat4(1.0);
  //      ellenModel2 = glm::translate(ellenModel2, glm::vec3(3, 1, 0.1));
		//ellenModel2 = glm::rotate(ellenModel2, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		//ellenModel2 = glm::scale(ellenModel2, glm::vec3(0.01f));

  //      r2::sarr::Push(*animModelMats, ellenModel2);

  //      mAnimationsHandles = MAKE_SARRAY(*linearArenaPtr, r2::asset::AssetHandle, 20);

        //@TODO(Serge): get rid of all of this once the engine can sustain itself
        //              at the moment we still need this for skybox and color grading
		//char materialsPath[r2::fs::FILE_PATH_LENGTH];
  //      r2::fs::utils::AppendSubPath(SANDBOX_MATERIALS_MANIFESTS_BIN, materialsPath, "SandboxMaterialPack.mpak");//"SandboxMaterialParamsPack.mppk");

		//char texturePackPath[r2::fs::FILE_PATH_LENGTH];
		//r2::fs::utils::AppendSubPath(SANDBOX_TEXTURES_MANIFESTS_BIN, texturePackPath, "SandboxTexturePack.tman");


  //      r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

		//const byte* materialManifestData = r2::asset::lib::GetManifestData(assetLib, r2::asset::Asset::GetAssetNameForFilePath(materialsPath, r2::asset::EngineAssetType::MATERIAL_PACK_MANIFEST));

		//const flat::MaterialPack* gameMaterialPack = flat::GetMaterialPack(materialManifestData);

  //      r2::draw::texche::LoadMaterialTextures(texturePacksCache, gameMaterialPack);
		////gameAssetManager.LoadMaterialTextures(gameMaterialPack);

		//u32 totalNumberOfTextures = 0;
		//u32 totalNumberOfTexturePacks = 0;
		//u32 totalNumCubemaps = 0;
		//u32 cacheSize = 0;
		//r2::draw::texche::GetTexturePacksCacheSizes(texturePackPath, totalNumberOfTextures, totalNumberOfTexturePacks, totalNumCubemaps, cacheSize);

		//r2::SArray<r2::draw::tex::Texture>* gameTextures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::Texture, totalNumberOfTextures);
		//r2::SArray<r2::draw::tex::CubemapTexture>* gameCubemaps = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::CubemapTexture, totalNumCubemaps);

  //      r2::draw::texche::GetTexturesForMaterialPack(texturePacksCache, gameMaterialPack, gameTextures, gameCubemaps);

		////gameAssetManager.GetTexturesForMaterialPack(gameMaterialPack, gameTextures, gameCubemaps);

  //      r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();
		//r2::draw::rmat::UploadMaterialTextureParamsArray(*renderMaterialCache, gameMaterialPack, gameTextures, gameCubemaps);

		//FREE(gameCubemaps, *MEM_ENG_SCRATCH_PTR);
		//FREE(gameTextures, *MEM_ENG_SCRATCH_PTR);

    //    auto sponzaHandle = gameAssetManager.LoadAsset(r2::asset::Asset("Sponza.rmdl", r2::asset::RMODEL));
      //  mSponzaModel = gameAssetManager.GetAssetDataConst<r2::draw::Model>(sponzaHandle);


  //      glm::mat4 sponzaModelMatrix = glm::mat4(1.0);

    //    sponzaModelMatrix = sponzaModelMatrix * mSponzaModel->globalInverseTransform;

      //  sponzaModelMatrix = glm::rotate(sponzaModelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));

        //r2::sarr::Push(*modelMats, sponzaModelMatrix);

   //     r2::util::Random randomizer;
        
        //color grading
        //const r2::draw::tex::Texture* colorGradingLUT = r2::draw::texche::GetAlbedoTextureForMaterialName(texturePacksCache, gameMaterialPack, STRING_ID("ColorGradingLUT")); //gameAssetManager.GetAlbedoTextureForMaterialName(gameMaterialPack, STRING_ID("ColorGradingLUT")); 

        //r2::draw::renderer::SetColorGradingLUT(colorGradingLUT, 32);
        //r2::draw::renderer::SetColorGradingContribution(0.05);
        //r2::draw::renderer::EnableColorGrading(true);

        //float startingX = 10.0f;

        //for (u32 i = 0; i < NUM_DRAWS; ++i)
        //{
        //    r2::sarr::Push(*mTransparentWindowDrawFlags, drawFlags);

        //    glm::mat4 modelMat = glm::mat4(1.0);

        //    modelMat = glm::translate(modelMat, glm::vec3(startingX, 0, 1.0f));
        //    modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0, 1, 0));
        //    modelMat = glm::scale(modelMat, glm::vec3(2));

        //    r2::sarr::Push(*mTransparentWindowMats, modelMat);

        //    startingX -= 20.f / static_cast<float>(NUM_DRAWS);
        //}

      //  mSkyboxModelRef = r2::draw::renderer::GetDefaultModelRef(r2::draw::SKYBOX);

        //mSkeletonModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
        //mEllenModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;

   //     mSponzaModelRefHandle = r2::draw::renderer::UploadModel(mSponzaModel);

      //  r2::SArray<r2::asset::Asset>* animationAssets = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::asset::Asset, 20);

        //r2::sarr::Push(*animationAssets, r2::asset::Asset("micro_bat_idle.ranm", r2::asset::RANIMATION));
        //r2::sarr::Push(*animationAssets, r2::asset::Asset("micro_bat_invert_idle.ranm", r2::asset::RANIMATION));
        //r2::sarr::Push(*animationAssets, r2::asset::Asset("micro_bat_attack.ranm", r2::asset::RANIMATION));

        //r2::sarr::Push(*animationAssets, r2::asset::Asset("skeleton_archer_allinone.ranm", r2::asset::RANIMATION));
        //r2::sarr::Push(*animationAssets, r2::asset::Asset("walk.ranm", r2::asset::RANIMATION));
        //r2::sarr::Push(*animationAssets, r2::asset::Asset("run.ranm", r2::asset::RANIMATION));

        //r2::sarr::Push(*animationAssets, r2::asset::Asset("EllenIdle.ranm", r2::asset::RANIMATION));
        //r2::sarr::Push(*animationAssets, r2::asset::Asset("EllenRunForward.ranm", r2::asset::RANIMATION));
        //r2::sarr::Push(*animationAssets, r2::asset::Asset("EllenSpawn.ranm", r2::asset::RANIMATION));

        //const auto numAnimations = r2::sarr::Size(*animationAssets);
        //for (u32 i= 0; i < numAnimations; ++i)
        //{
        //    auto animationHandle = gameAssetManager.LoadAsset(r2::sarr::At(*animationAssets, i));
        //    r2::sarr::Push(*mAnimationsHandles, animationHandle);
        //}

        //FREE(animationAssets, *MEM_ENG_SCRATCH_PTR);

  //      r2::draw::renderer::SetClearColor(glm::vec4(0.f, 0.f, 0.f, 1.f));
        //setup the lights
        {
            //const auto* prefilteredCubemap = r2::draw::texche::GetCubemapTextureForMaterialName(texturePacksCache, gameMaterialPack, STRING_ID("NewportPrefiltered"));//gameAssetManager.GetCubemapTextureForMaterialName(gameMaterialPack, STRING_ID("NewportPrefiltered")); 
            //s32 numMips = prefilteredCubemap->numMipLevels;

            //r2::draw::renderer::AddSkyLight(
            //    *r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, STRING_ID("NewportConvolved")),
            //    *r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, STRING_ID("NewportPrefiltered")),
            //    *r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, STRING_ID("NewportLUTDFG")), numMips);

			//r2::draw::DirectionLight dirLight;
			//dirLight.lightProperties.color = glm::vec4(1, 1, 1, 1.0f);

			//dirLight.direction = glm::normalize(glm::vec4(0.0f) - glm::vec4(40.0f, 0.0f, 100.0f, 0.0f));
			//dirLight.lightProperties.intensity = 1;
   //         dirLight.lightProperties.castsShadowsUseSoftShadows = glm::uvec4(1, 0, 0, 0);

   //         r2::draw::renderer::AddDirectionLight(dirLight);

            //r2::draw::DirectionLight dirLight2;
            //dirLight2.lightProperties.color = glm::vec4(1.0f);
            //dirLight2.direction = glm::normalize(glm::vec4(0.0f) - glm::vec4(4.0f, 0.0f, 4.0f, 0.0f));
            //dirLight2.lightProperties.intensity = 2;
            //dirLight2.lightProperties.castsShadowsUseSoftShadows = glm::uvec4(1, 0, 0, 0);

            //r2::draw::renderer::AddDirectionLight(dirLight2);

			//r2::draw::lightsys::AddDirectionalLight(*mLightSystem, dirLight);

			/*for (int i = 0; i < 10; ++i)
			{
				auto zPos = (int)randomizer.RandomNum(1, 10);
				auto xPos = (int)randomizer.RandomNum(0, 13) - (int)randomizer.RandomNum(0, 13);
				auto yPos = (int)randomizer.RandomNum(0, 10) - (int)randomizer.RandomNum(0, 10);

				auto r = randomizer.RandomNum(0, 255);
				auto g = randomizer.RandomNum(0, 255);
				auto b = randomizer.RandomNum(0, 255);

				r2::draw::SpotLight spotLight;
				spotLight.lightProperties.color = glm::vec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);

				spotLight.position = glm::vec4(xPos, yPos, zPos, glm::cos(glm::radians(30.f)));
				spotLight.direction = glm::vec4(glm::vec3(0, 0, -1), glm::cos(glm::radians(45.f)));

				spotLight.lightProperties.intensity = 100;
				spotLight.lightProperties.fallOff = 0.1;
				spotLight.lightProperties.castsShadowsUseSoftShadows = glm::uvec4(0, 0, 0, 0);

				r2::draw::renderer::AddSpotLight(spotLight);
			}*/


			/*for (int i = 0; i < 50; ++i)
			{
				r2::draw::PointLight pointLight;

				auto zPos = (int)randomizer.RandomNum(1, 10);
				auto xPos = (int)randomizer.RandomNum(0, 13) - (int)randomizer.RandomNum(0, 13);
				auto yPos = (int)randomizer.RandomNum(0, 10) - (int)randomizer.RandomNum(0, 10);

				auto r = randomizer.RandomNum(0, 255);
				auto g = randomizer.RandomNum(0, 255);
				auto b = randomizer.RandomNum(0, 255);

				pointLight.position = glm::vec4(xPos, yPos, zPos, 1.0);

				pointLight.lightProperties.color = glm::vec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);

				pointLight.lightProperties.intensity = 100.0;
				pointLight.lightProperties.fallOff = 0.1;

				pointLight.lightProperties.castsShadowsUseSoftShadows = glm::uvec4(0, 0, 0, 0); 

				r2::draw::renderer::AddPointLight(pointLight);
			}*/

        }

        //audio test setup
        {
            //load test bank 1
            //r2::audio::AudioEngine audioEngine;

            //char bankPath[r2::fs::FILE_PATH_LENGTH];
            //r2::fs::utils::BuildPathFromCategory(r2::fs::utils::SOUNDS, "TestBank1.bank", bankPath);

            //mTestBankHandle = audioEngine.LoadBank(bankPath, r2::audio::AudioEngine::LOAD_BANK_NORMAL);
        }


        return true;
    }

    virtual void OnEvent(r2::evt::Event& e) override
    {
		r2::evt::EventDispatcher dispatcher(e);

        dispatcher.Dispatch<r2::evt::GameControllerConnectedEvent>([this](const r2::evt::GameControllerConnectedEvent& e)
        {
                mDefaultInputGather.SetControllerIDConnected(e.GetControllerID(), e.GetInstanceID());

                if (e.GetControllerID() == 0)
                {					
                    mPlayerInputType.inputType = r2::io::INPUT_TYPE_CONTROLLER;
                    mPlayerInputType.controllerID = e.GetControllerID();

                    return true;
                }
                return false;
        });

        dispatcher.Dispatch<r2::evt::GameControllerDisconnectedEvent>([this](const r2::evt::GameControllerDisconnectedEvent& e)
        {
            if (e.GetControllerID() == 0)
            {
                mPlayerInputType.inputType = r2::io::INPUT_TYPE_KEYBOARD;
                mPlayerInputType.controllerID = r2::io::INVALID_CONTROLLER_ID;

                return true;
            }

            return false;
        });

		//dispatcher.Dispatch<r2::evt::KeyPressedEvent>([this](const r2::evt::KeyPressedEvent& e) {
  //          if (e.KeyCode() == r2::io::KEY_UP)
  //          {
  //              //s32 prevResolution = mResolution;
  //              //mResolution = (mResolution + 1) % RESOLUTIONS_COUNT;
  //              //MENG.SetResolution(g_resolutions[prevResolution], g_resolutions[mResolution]);
  //              //PrintResolution();


  //              //return true;
  //          }
  //          else if (e.KeyCode() == r2::io::KEY_DOWN)
  //          {
  //              //s32 prevResolution = mResolution;

  //              //--mResolution;
  //              //if (mResolution < 0)
  //              //{
  //              //    mResolution = RESOLUTIONS_COUNT - 1;
  //              //}
  //              //
  //              //MENG.SetResolution(g_resolutions[prevResolution], g_resolutions[mResolution]);

  //              //PrintResolution();

  //              //return true;
  //          }
  //          else if (e.KeyCode() == r2::io::KEY_f)
  //          {
  //              //MENG.SetFullScreen();
  //              //return true;
  //          }
  //          else if (e.KeyCode() == r2::io::KEY_i)
  //          {

		//		//printf("======================= Capacity =============================\n");

		//		//auto staticVertexBufferCapacity = r2::draw::renderer::GetStaticVertexBufferCapacity();
		//		//auto animVertexBufferCapacity = r2::draw::renderer::GetAnimVertexBufferCapacity();

		//		//printf("Static Vertex Buffer vbo - capacity: %u\n", staticVertexBufferCapacity.vertexBufferSizes[0]);
		//		//printf("Static Vertex Buffer ibo - capacity: %u\n", staticVertexBufferCapacity.indexBufferSize);

		//		//for (int i = 0; i < animVertexBufferCapacity.numVertexBuffers; ++i)
		//		//{
		//		//	printf("Anim Vertex Buffer vbo[%i] - capacity: %u\n", i, animVertexBufferCapacity.vertexBufferSizes[i]);
		//		//}

		//		//printf("Anim Vertex Buffer ibo - capacity: %u\n", animVertexBufferCapacity.indexBufferSize);


  //  //            printf("======================= Size =============================\n");

		//		//auto staticVertexBufferSize = r2::draw::renderer::GetStaticVertexBufferSize();
		//		//auto animVertexBufferSize = r2::draw::renderer::GetAnimVertexBufferSize();

		//		//printf("Static Vertex Buffer vbo - size: %u\n", staticVertexBufferSize.vertexBufferSizes[0]);
		//		//printf("Static Vertex Buffer ibo - size: %u\n", staticVertexBufferSize.indexBufferSize);

		//		//for (int i = 0; i < animVertexBufferSize.numVertexBuffers; ++i)
		//		//{
		//		//	printf("Anim Vertex Buffer vbo[%i] - size: %u\n", i, animVertexBufferSize.vertexBufferSizes[i]);
		//		//}

		//		//printf("Anim Vertex Buffer ibo - size: %u\n", animVertexBufferSize.indexBufferSize);

  //  //            printf("======================= Remaining Size =============================\n");

  //  //            auto staticVertexBufferRemainingSize = r2::draw::renderer::GetStaticVertexBufferRemainingSize();
  //  //            auto animVertexBufferRemainingSize = r2::draw::renderer::GetAnimVertexBufferRemainingSize();

  //  //            printf("Static Vertex Buffer vbo - remaining size: %u\n", staticVertexBufferRemainingSize.vertexBufferSizes[0] );
  //  //            printf("Static Vertex Buffer ibo - remaining size: %u\n", staticVertexBufferRemainingSize.indexBufferSize );

  //  //            for (int i = 0; i < animVertexBufferRemainingSize.numVertexBuffers; ++i)
  //  //            {
  //  //                printf("Anim Vertex Buffer vbo[%i] - remaining size: %u\n", i, animVertexBufferRemainingSize.vertexBufferSizes[i]);
  //  //            }

  //  //            printf("Anim Vertex Buffer ibo - remaining size: %u\n", animVertexBufferRemainingSize.indexBufferSize);
  //          }


  //        

		//	return false;
		//});


        mDefaultInputGather.OnEvent(e);

		//mPersController.OnEvent(e);
    }
    
    virtual void Update() override
    {

    }

    virtual void Render(float alpha) override
    {
        

        //Draw the axis
//#ifdef R2_DEBUG
//        r2::SArray<glm::vec3>* basePositions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 3);
//        r2::SArray<glm::vec3>* directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 3);
//        r2::SArray<float>* lengths = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, 3);
//        r2::SArray<float>* coneRadii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, 3);
//        r2::SArray<glm::vec4>* colors = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec4, 3);
//
//        r2::sarr::Push(*basePositions, glm::vec3(0));
//        r2::sarr::Push(*basePositions, glm::vec3(0));
//        r2::sarr::Push(*basePositions, glm::vec3(0));
//
//        r2::sarr::Push(*directions, glm::vec3(1, 0, 0));
//        r2::sarr::Push(*directions, glm::vec3(0, 1, 0));
//        r2::sarr::Push(*directions, glm::vec3(0, 0, 1));
//
//        r2::sarr::Push(*lengths, 2.0f);
//        r2::sarr::Push(*lengths, 2.0f);
//        r2::sarr::Push(*lengths, 2.0f);
//
//        r2::sarr::Push(*coneRadii, 0.3f);
//        r2::sarr::Push(*coneRadii, 0.3f);
//        r2::sarr::Push(*coneRadii, 0.3f);
//
//        r2::sarr::Push(*colors, glm::vec4(1, 0, 0, 1));
//        r2::sarr::Push(*colors, glm::vec4(0, 1, 0, 1));
//        r2::sarr::Push(*colors, glm::vec4(0, 0, 1, 1));
//
//        r2::draw::renderer::DrawArrowInstanced(*basePositions, *directions, *lengths, *coneRadii, *colors, true, true);
//
//        FREE(colors, *MEM_ENG_SCRATCH_PTR);
//        FREE(coneRadii, *MEM_ENG_SCRATCH_PTR);
//        FREE(lengths, *MEM_ENG_SCRATCH_PTR);
//        FREE(directions, *MEM_ENG_SCRATCH_PTR);
//        FREE(basePositions, *MEM_ENG_SCRATCH_PTR);
//
//    
//#endif
    }
    
    virtual void Shutdown() override
    {

    }
    
    virtual std::string GetSoundDefinitionPath() const override
    {
        char soundDefinitionPath[r2::fs::FILE_PATH_LENGTH];
        char result [r2::fs::FILE_PATH_LENGTH];
        ResolveCategoryPath(r2::fs::utils::SOUND_DEFINITIONS, soundDefinitionPath);
        r2::fs::utils::AppendSubPath(soundDefinitionPath, result, "sounds.sdef");
        
        return result;
    }
    
    virtual std::string GetShaderManifestsPath() const override
    {
        char shaderManifestsPath[r2::fs::FILE_PATH_LENGTH];
        char result [r2::fs::FILE_PATH_LENGTH];
        ResolveCategoryPath(r2::fs::utils::SHADERS_MANIFEST, shaderManifestsPath);
        r2::fs::utils::AppendSubPath(shaderManifestsPath, result, "shaders.sman");
        
        return result;
    }

    virtual std::vector<std::string> GetTexturePackManifestsBinaryPaths() const override
    {
        std::vector<std::string> manifestBinaryPaths
        {
            SANDBOX_TEXTURES_MANIFESTS_BIN + std::string("/SandboxTexturePack.tman")
        };
        return manifestBinaryPaths;
    }

    virtual std::vector<std::string> GetTexturePackManifestsRawPaths() const override
    {
		std::vector<std::string> manifestRawPaths
		{
			SANDBOX_TEXTURES_MANIFESTS + std::string("/SandboxTexturePack.json")
		};
		return manifestRawPaths;
    }

    virtual std::vector<std::string> GetMaterialPackManifestsBinaryPaths() const override
    {
        std::vector<std::string> manifestBinPaths
        {
            SANDBOX_MATERIALS_MANIFESTS_BIN + std::string("/SandboxMaterialPack.mpak"),
      //      SANDBOX_MATERIALS_MANIFESTS_BIN + std::string("/SandboxMaterialParamsPack.mppk")
		};
		return manifestBinPaths;
    }

    virtual std::vector<std::string> GetMaterialPackManifestsRawPaths() const override
    {
		std::vector<std::string> manifestRawPaths
		{
            SANDBOX_MATERIALS_MANIFESTS + std::string("/SandboxMaterialPack.json"),
      //      SANDBOX_MATERIALS_MANIFESTS + std::string("/SandboxMaterialParamsPack.json")
		};
		return manifestRawPaths;
    }

    virtual std::vector<std::string> GetModelBinaryPaths() const override
    {
        return {SANDBOX_MODELS_DIR_BIN};
    }

    virtual std::vector<std::string> GetModelRawPaths() const override
    {
        return {SANDBOX_MODELS_DIR_RAW};
    }

	std::vector<std::string> GetModelManifestRawPaths() const override
	{
		return { SANDBOX_MODELS_MANIFEST_DIR_RAW + std::string("/SandboxModels.json")};
	}

	std::vector<std::string> GetModelManifestBinaryPaths() const override
	{
		return { SANDBOX_MODELS_MANIFEST_DIR_BIN + std::string("/SandboxModels.rmmn" )};
	}

    virtual std::vector<std::string> GetAnimationBinaryPaths() const override
    {
        return { SANDBOX_ANIMATIONS_DIR_BIN };
    }

    virtual std::vector<std::string> GetAnimationRawPaths() const override
    {
        return { SANDBOX_ANIMATIONS_DIR_RAW };
    }

    std::vector<std::string> GetInternalShaderManifestsBinaryPaths() const
    {
        return { SANDBOX_SHADERS_INTERNAL_SHADER_DIR_BIN + std::string("/SandboxInternalShaders.sman")};
    }

    std::vector<std::string> GetInternalShaderManifestsRawPaths() const
    {
        return { SANDBOX_SHADERS_INTERNAL_SHADER_DIR_RAW + std::string("/SandboxInternalShaders.json")};
    }

	std::vector<std::string> GetMaterialPacksManifestsBinaryPaths() const
	{
		char materialsPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(SANDBOX_MATERIALS_MANIFESTS_BIN, materialsPath, "SandboxMaterialPack.mpak");
		return {std::string( materialsPath )};
	}

    r2::util::Size GetAppResolution() const override
    {
        return g_resolutions[mResolution];
    }

    virtual u32 GetAssetMemorySize() const override
    {
        //@TODO(Serge): calculate this much later
        return Megabytes(50);
    }

    virtual u32 GetTextureMemorySize() const override
    {
        return Megabytes(300);
    }

    virtual u32 GetMaxNumECSSystems() const override
    {
        return r2::ecs::MAX_NUM_SYSTEMS;
    }

    virtual u32 GetMaxNumECSEntities() const override
    {
        return r2::ecs::MAX_NUM_ENTITIES;
    }

    virtual u32 GetMaxNumComponents() const override
    {
        return r2::ecs::MAX_NUM_COMPONENTS;
    }

    u64 GetECSWorldAuxMemory() const override
    {
        //@TODO(Serge): we're assuming it's a stack header which we shouldn't be
		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();
		u32 boundsChecking = 0;
        const u32 ALIGNMENT = 16;

#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

        u64 memorySize = 0;

		r2::mem::utils::MemoryProperties memProperties;
		memProperties.alignment = ALIGNMENT;
		memProperties.boundsChecking = boundsChecking;
		memProperties.headerSize = stackHeaderSize;

        memorySize += r2::ecs::ComponentArray<PlayerCommandComponent>::MemorySize(GetMaxNumECSEntities(), ALIGNMENT, stackHeaderSize, boundsChecking);
        memorySize += r2::ecs::ComponentArray<FacingComponent>::MemorySize(GetMaxNumECSEntities(), ALIGNMENT, stackHeaderSize, boundsChecking);
        memorySize += r2::ecs::ComponentArray<GridPositionComponent>::MemorySize(GetMaxNumECSEntities(), ALIGNMENT, stackHeaderSize, boundsChecking);
        memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::ecs::ECSCoordinator::MemorySizeOfSystemType<PlayerCommandSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);
        memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::ecs::ECSCoordinator::MemorySizeOfSystemType<CharacterControllerSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);
        memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::ecs::ECSCoordinator::MemorySizeOfSystemType<FacingUpdateSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);

        return memorySize;
    }

	std::string GetLevelPackDataBinPath() const
	{
		return SANDBOX_LEVELS_DIR_BIN;
	}

	std::string GetLevelPackDataJSONPath() const
	{
		return SANDBOX_LEVELS_DIR_RAW;
	}

	std::string GetLevelPackDataManifestBinPath() const
	{
		return SANDBOX_LEVELS_DIR_BIN + std::string("/manifests/SandboxLevels.rlpk");
	}

	std::string GetLevelPackDataManifestJSONPath() const
	{
        return SANDBOX_LEVELS_DIR_RAW + std::string("/manifests/SandboxLevels.json");
	}
    
    virtual void RegisterECSData(r2::ecs::ECSWorld& ecsWorld) override
    {

       ecsWorld.RegisterComponent<PlayerCommandComponent>("PlayerCommandComponent", false, false, nullptr);
       ecsWorld.RegisterComponent<FacingComponent>("FacingComponent", true, false, nullptr);
       ecsWorld.RegisterComponent<GridPositionComponent>("GridPositionComponent", true, false, nullptr); //@TODO(Serge): should actually be instanced
       r2::ecs::ECSCoordinator* ecsCoordinator = ecsWorld.GetECSCoordinator();
       r2::ecs::Signature playerCommandSystemSignature;

	   const auto playerComponentType = ecsCoordinator->GetComponentType<r2::ecs::PlayerComponent>();

	   const auto playerCommandComponentType = ecsCoordinator->GetComponentType<PlayerCommandComponent>();
	   const auto skeletonAnimationComponentType = ecsCoordinator->GetComponentType<r2::ecs::SkeletalAnimationComponent>();
	   const auto transformComponentType = ecsCoordinator->GetComponentType<r2::ecs::TransformComponent>();
	   const auto facingComponentType = ecsCoordinator->GetComponentType<FacingComponent>();
       const auto transformDirtyComponentType = ecsCoordinator->GetComponentType<r2::ecs::TransformDirtyComponent>();

       s32 sortOrder = 0;

	   r2::ecs::Signature facingUpdateSystemSignature;
	   facingUpdateSystemSignature.set(transformComponentType);
	   facingUpdateSystemSignature.set(facingComponentType);
	   //facingUpdateSystemSignature.set(transformDirtyComponentType);

	   FacingUpdateSystem* facingUpdateSystem = ecsWorld.RegisterSystem<FacingUpdateSystem>(facingUpdateSystemSignature);

       ecsWorld.RegisterAppSystem(facingUpdateSystem, sortOrder++, true);

	   playerCommandSystemSignature.set(playerComponentType);
	   PlayerCommandSystem* playerCommandSystem = ecsWorld.RegisterSystem<PlayerCommandSystem>(playerCommandSystemSignature);
	   playerCommandSystem->SetInputGather(&mDefaultInputGather);
       ecsWorld.RegisterAppSystem(playerCommandSystem, sortOrder++, false);

       r2::ecs::Signature characterControllerSystemSignature;
       characterControllerSystemSignature.set(playerCommandComponentType);
       characterControllerSystemSignature.set(skeletonAnimationComponentType);
       characterControllerSystemSignature.set(transformComponentType);
       characterControllerSystemSignature.set(facingComponentType);

       CharacterControllerSystem* characterControllerSystem = ecsWorld.RegisterSystem<CharacterControllerSystem>(characterControllerSystemSignature);

       ecsWorld.RegisterAppSystem(characterControllerSystem, sortOrder++, false);

       //set the input type for the player
       //@TODO(Serge): move to the OnEvent so that we can switch back and forth 
       r2::io::InputType player0InputType;
       player0InputType.inputType = r2::io::INPUT_TYPE_KEYBOARD;
       //ecsWorld.SetPlayerInputType(0, player0InputType);

    }

    virtual void UnRegisterECSData(r2::ecs::ECSWorld& ecsWorld) override
    {
        ecsWorld.UnRegisterSystem<CharacterControllerSystem>();
        ecsWorld.UnRegisterSystem<PlayerCommandSystem>();
        ecsWorld.UnRegisterSystem<FacingUpdateSystem>();
        ecsWorld.UnRegisterComponent<GridPositionComponent>();
        ecsWorld.UnRegisterComponent<FacingComponent>();
        ecsWorld.UnRegisterComponent<PlayerCommandComponent>();

    }

#ifdef R2_ASSET_PIPELINE

    virtual std::vector<std::string> GetAssetWatchPaths() const override
    {
        std::vector<std::string> assetWatchPaths = {
            ASSET_RAW_DIR
        };
        
        return assetWatchPaths;
    }
    
    virtual std::string GetAssetManifestPath() const override
    {
        return MANIFESTS_DIR;
    }
    
    virtual std::string GetAssetCompilerTempPath() const override
    {
        return ASSET_TEMP_DIR;
    }
    
    virtual std::vector<std::string> GetSoundDirectoryWatchPaths() const override
    {
        char soundsPath[r2::fs::FILE_PATH_LENGTH];

        ResolveCategoryPath(r2::fs::utils::SOUNDS, soundsPath);
        
        std::vector<std::string> soundPaths = {
            std::string(soundsPath)
        };
        
        return soundPaths;
    }

    virtual std::vector<std::string> GetTexturePacksWatchPaths() const override
    {
        return {SANDBOX_TEXTURES_DIR};
    }

	virtual std::vector<std::string> GetTexturePacksBinaryDirectoryPaths() const override
	{
		return { SANDBOX_TEXTURES_PACKS_BIN };
	}

    virtual std::vector<std::string> GetMaterialPacksWatchPaths() const override
    {
        return  { SANDBOX_MATERIALS_DIR };// , SANDBOX_MATERIAL_PARAMS_DIR};
    }

	std::vector<std::string> GetMaterialPacksBinaryPaths() const
	{
        return { SANDBOX_MATERIALS_PACKS_DIR_BIN };// , SANDBOX_MATERIALS_PARAMS_DIR_BIN};
	}

    std::vector<r2::asset::pln::FindMaterialPackManifestFileFunc> GetFindMaterialManifestsFuncs() const
    {
        return { r2::asset::pln::FindMaterialPackManifestFile };//, r2::asset::pln::FindMaterialParamsPackManifestFile};
    }

    std::vector<r2::asset::pln::GenerateMaterialPackManifestFromDirectoriesFunc> GetGenerateMaterialManifestsFromDirectoriesFuncs() const
    {
        return { r2::asset::pln::GenerateMaterialPackManifestFromDirectories };// , r2::asset::pln::GenerateMaterialParamsPackManifestFromDirectories};
    }

    r2::asset::pln::InternalShaderPassesBuildFunc GetInternalShaderPassesBuildFunc() const
    {
        return r2::asset::pln::BuildShaderManifestsFromJsonIO;
    }

    std::string GetRawSoundDefinitionsPath() const
    {
        return SANDBOX_SOUND_DEFINITIONS_RAW + std::string("/sounds.json");
    }

#endif
    
    char* GetApplicationName() const
    {
        return "Sandbox";
    }

    r2::asset::PathResolver GetPathResolver() const override
    {
        return ResolveCategoryPath;
    }

    virtual void GetSubPathForDirectory(r2::fs::utils::Directory directory, char* subpath) const override
    {
		switch (directory)
		{
		case r2::fs::utils::ROOT:
			r2::util::PathCpy(subpath, "");
			break;
		case r2::fs::utils::SOUND_DEFINITIONS:
			r2::util::PathCpy(subpath, "assets_bin/Sandbox_Sounds/sound_definitions");
			break;
		case r2::fs::utils::SOUNDS:
			r2::util::PathCpy(subpath, "assets/Sandbox_Sounds/sounds");
			break;
		case r2::fs::utils::TEXTURES:
			r2::util::PathCpy(subpath, "assets_bin/Sandbox_Textures/packs");
			break;
		case r2::fs::utils::SHADERS_BIN:
			r2::util::PathCpy(subpath, "assets_bin/Sandbox_Shaders");
			break;
		case r2::fs::utils::SHADERS_RAW:
			r2::util::PathCpy(subpath, "assets/shaders/raw");
			break;
		case r2::fs::utils::SHADERS_MANIFEST:
			r2::util::PathCpy(subpath, "assets_bin/Sandbox_Shaders/manifests");
			break;
		case r2::fs::utils::MODELS:
			r2::util::PathCpy(subpath, "assets_bin/Sandbox_Models");
			break;
		case r2::fs::utils::ANIMATIONS:
			r2::util::PathCpy(subpath, "assets_bin/Sandbox_Animations");
			break;
		case r2::fs::utils::LEVELS_BIN:
			r2::util::PathCpy(subpath, "assets_bin/Sandbox_Levels");
			break;
        case r2::fs::utils::MATERIALS_RAW:
            r2::util::PathCpy(subpath, "assets_bin/Sandbox_Materials");
            break;
        case r2::fs::utils::MATERIALS_BIN:
			r2::util::PathCpy(subpath, "assets/Sandbox_Materials");
			break;
		default:
            r2::util::PathCpy(subpath, "");
			break;
		}
    }

    void PrintResolution() const
    {
        printf("Resolution is %d by %d\n", g_resolutions[mResolution].width, g_resolutions[mResolution].height);
    }
    
#ifdef R2_EDITOR
    virtual void RegisterComponentImGuiWidgets(r2::edit::InspectorPanel& inspectorPanel) const override
    {
        std::shared_ptr<InspectorPanelFacingDataSource> facingDataSource = std::make_shared<InspectorPanelFacingDataSource>(inspectorPanel.GetEditor(), inspectorPanel.GetEditor()->GetECSCoordinator(), &inspectorPanel);

        r2::edit::InspectorPanelComponentWidget facingComponentWidget = r2::edit::InspectorPanelComponentWidget(10, facingDataSource);
        inspectorPanel.RegisterComponentWidget(facingComponentWidget);

    }
#endif

    virtual const r2::io::InputType& GetInputTypeForPlayer(s32 player) const override
    {
        return mPlayerInputType;
    }

private:
    s32 mResolution = 3;

    r2::io::DefaultInputGather mDefaultInputGather;

    r2::io::InputType mPlayerInputType;

};

namespace
{
    bool ResolveCategoryPath(u32 category, char* path)
    {
        char subpath[r2::fs::FILE_PATH_LENGTH];
        bool result = true;
        switch (category)
        {
            case r2::fs::utils::ROOT:
                r2::util::PathCpy(subpath, "");
                break;
            case r2::fs::utils::SOUND_DEFINITIONS:
                r2::util::PathCpy(subpath, "assets_bin/Sandbox_Sounds/sound_definitions");
                break;
            case r2::fs::utils::SOUNDS:
                r2::util::PathCpy(subpath, "assets/Sandbox_Sounds/sounds");
                break;
            case r2::fs::utils::TEXTURES:
                r2::util::PathCpy(subpath, "assets_bin/Sandbox_Textures/packs");
                break;
            case r2::fs::utils::SHADERS_BIN:
                r2::util::PathCpy(subpath, "assets_bin/Sandbox_Shaders");
                break;
            case r2::fs::utils::SHADERS_RAW:
                r2::util::PathCpy(subpath, "assets/shaders/raw");
                break;
            case r2::fs::utils::SHADERS_MANIFEST:
                r2::util::PathCpy(subpath, "assets_bin/Sandbox_Shaders/manifests");
                break;
            case r2::fs::utils::MODELS:
                r2::util::PathCpy(subpath, "assets_bin/Sandbox_Models");
                break;
            case r2::fs::utils::ANIMATIONS:
                r2::util::PathCpy(subpath, "assets_bin/Sandbox_Animations");
                break;
            case r2::fs::utils::LEVELS_BIN:
                r2::util::PathCpy(subpath, "assets_bin/Sandbox_Levels");
                break;
			case r2::fs::utils::MATERIALS_RAW:
				r2::util::PathCpy(subpath, "assets_bin/Sandbox_Materials");
				break;
			case r2::fs::utils::MATERIALS_BIN:
				r2::util::PathCpy(subpath, "assets/Sandbox_Materials");
				break;
            default:
                result = false;
                break;
        }
        
        if (result)
        {
            //@TODO(Serge): fix APP_DIR such that it's correct when making a release build
            //this is just for testing right now
            r2::fs::utils::AppendSubPath(APP_DIR, path, subpath);
        }
        
        return result;
    }
}

std::unique_ptr<r2::Application> r2::CreateApplication()
{
    return std::make_unique<Sandbox>();
}
