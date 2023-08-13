//
//  main.cpp
//  Sandbox
//
//  Created by Serge Lansiquot on 2019-02-02.
//  Copyright © 2019 Serge Lansiquot. All rights reserved.
//

#include "r2.h"
#include "r2/Core/EntryPoint.h"


#include "BreakoutLevelSchema_generated.h"
#include "BreakoutPowerupSchema_generated.h"
#include "BreakoutPlayerSettings_generated.h"
#include "BreakoutHighScoreSchema_generated.h"

#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/File.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/AssetCache.h"
#include "BreakoutLevelsFile.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetFiles/RawAssetFile.h"
#include "r2/Core/Assets/AssetFiles/ZipAssetFile.h"
#include "r2/Core/Assets/AssetLib.h"

#include "r2/Render/Renderer/Renderer.h"

#include "r2/Utils/Hash.h"
#include "r2/Utils/Random.h"
#include "r2/Render/Camera/PerspectiveCameraController.h"
#include "glm/gtc/type_ptr.hpp"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Model/Light.h"

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/System.h"
//
#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Render/Model/Materials/MaterialParamsPackHelpers.h"
#include "r2/Render/Model/Textures/TexturePacksCache.h"
#include "r2/Render/Model/Materials/MaterialParamsPack_generated.h"

#include "r2/Game/ECS/Components/AudioEmitterActionComponent.h"
#include "r2/Game/ECSWorld/ECSWorld.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetManifest.h"
#endif

#include "r2/Audio/AudioEngine.h"
#include <mutex>

struct DummyComponent
{
    int dummy;
};

namespace r2::ecs
{
    class ECSCoordinator;

	template<>
	void SerializeComponentArray<DummyComponent>(flatbuffers::FlatBufferBuilder& builder, const r2::SArray<DummyComponent>& components)
	{
		R2_CHECK(false, "You need to implement a serializer for this type");
	}
}

namespace
{
    constexpr r2::util::Size g_resolutions[] = { {640, 480}, {1024, 768}, {1280, 768}, {1920, 1080}, {2560, 1440} };

    constexpr u32 RESOLUTIONS_COUNT = COUNT_OF(g_resolutions);

    void WriteBuffer(const char* name, flatbuffers::FlatBufferBuilder& builder)
    {
        byte* buf = builder.GetBufferPointer();
        u32 size = builder.GetSize();
        
        r2::fs::FileMode mode;
        mode = r2::fs::Mode::Write;
        mode |= r2::fs::Mode::Binary;
        
        char pathToWrite[r2::fs::FILE_PATH_LENGTH];
        
        r2::fs::utils::AppendSubPath(CPLAT.RootPath().c_str(), pathToWrite, name);
        
        r2::fs::File* file = r2::fs::FileSystem::Open(r2::fs::DeviceConfig(), pathToWrite, mode);
        auto result = file->Write(buf, size);
        
        R2_LOGI("Wrote out %llu bytes", result);
        
        r2::fs::FileSystem::Close(file);
    }
    
    void MakeLevels()
    {
        flatbuffers::FlatBufferBuilder builder;
        
        Breakout::color4 redColor(1.0f, 0.0f, 0.0f, 1.0f);
        Breakout::color4 blueColor(0.0f, 0.0f, 1.0f, 1.0f);
        Breakout::color4 greenColor(0.0f, 1.0f, 0.0f, 1.0f);
        
        auto redBlock = Breakout::CreateBlock(builder, 'r', &redColor, 1);
        auto blueBlock = Breakout::CreateBlock(builder, 'b', &blueColor, 1);
        auto greenBlock = Breakout::CreateBlock(builder, 'g', &greenColor, 1);
        std::vector<flatbuffers::Offset<Breakout::Block>> blocksVec = {redBlock, blueBlock, greenBlock};
        
        auto layout1 = Breakout::CreateLayout(builder, 10, 3, builder.CreateString("rrrrrrrrrr\nbbbbbbbbbb\ngggggggggg"));
        auto layout2 = Breakout::CreateLayout(builder, 10, 3, builder.CreateString("bbbbbbbbbb\nrrrrrrrrrr\ngggggggggg"));
        auto layout3 = Breakout::CreateLayout(builder, 10, 3, builder.CreateString("gggggggggg\nbbbbbbbbbb\nrrrrrrrrrr"));
        
        auto level1 = Breakout::CreateLevel(builder, builder.CreateString("Level1"), 1, builder.CreateVector(blocksVec), layout1);
        auto level2 = Breakout::CreateLevel(builder, builder.CreateString("Level2"), 2, builder.CreateVector(blocksVec), layout2);
        auto level3 = Breakout::CreateLevel(builder, builder.CreateString("Level3"), 3, builder.CreateVector(blocksVec), layout3);
        
        std::vector<flatbuffers::Offset<Breakout::Level>> levelsVec = {level1, level2, level3};
        
        auto levelPack = Breakout::CreateLevelPack(builder, builder.CreateVector(levelsVec));
        
        builder.Finish(levelPack);
        
        WriteBuffer("breakout_level_pack.bin", builder);
    }
    
    void MakePowerups()
    {
        flatbuffers::FlatBufferBuilder builder;
        
        auto doubleBallSizePwr = Breakout::CreatePowerup(builder, Breakout::BreakoutPowerupType::BreakoutPowerupType_BPUT_INCREASE_BALL_SIZE, 2.0, 0);
        
        auto doublePaddleSizePwr = Breakout::CreatePowerup(builder, Breakout::BreakoutPowerupType::BreakoutPowerupType_BPUT_INCREASE_PADDLE_SIZE, 2.0, 0);
        
        auto doubleBallPwr = Breakout::CreatePowerup(builder, Breakout::BreakoutPowerupType::BreakoutPowerupType_BPUT_MULTIBALL, 1.0, 2);
        
        auto tripleBallPwr = Breakout::CreatePowerup(builder, Breakout::BreakoutPowerupType::BreakoutPowerupType_BPUT_MULTIBALL, 1.0, 3);
        
        auto addLifePwr = Breakout::CreatePowerup(builder, Breakout::BreakoutPowerupType_BPUT_LIFE_UP, 1.0f, 1);
        
        auto ignoreCollisionPwr = Breakout::CreatePowerup(builder, Breakout::BreakoutPowerupType::BreakoutPowerupType_BPUT_IGNORE_COLLISIONS, 0, 0);
        
        auto laserPwr = Breakout::CreatePowerup(builder, Breakout::BreakoutPowerupType::BreakoutPowerupType_BPUT_PADDLE_LASER, 0, 0);
        
        auto halfBallSizePwr = Breakout::CreatePowerup(builder, Breakout::BreakoutPowerupType::BreakoutPowerupType_BPUT_DECREASE_BALL_SIZE, 0.5f, 0);
        
        auto halfPaddleSizePwr = Breakout::CreatePowerup(builder, Breakout::BreakoutPowerupType::BreakoutPowerupType_BPUT_DECREASE_PADDLE_SIZE, 0.5f, 0);
        
        auto increaseBrickPwr = Breakout::CreatePowerup(builder, Breakout::BreakoutPowerupType::BreakoutPowerupType_BPUT_INCREASE_BRICK_HP, 0, 2);
        
        std::vector<flatbuffers::Offset<Breakout::Powerup>> powerupVec = {doubleBallSizePwr, doublePaddleSizePwr, doubleBallPwr, tripleBallPwr, addLifePwr, ignoreCollisionPwr, laserPwr, halfBallSizePwr, halfPaddleSizePwr, increaseBrickPwr};
        
        auto powerups = Breakout::CreatePowerups(builder, builder.CreateVector(powerupVec));
        
        builder.Finish(powerups);
        
        WriteBuffer("breakout_powerups.bin", builder);
        
    }
    
    void MakeHighScores()
    {
        flatbuffers::FlatBufferBuilder builder;
        
        auto score1 = Breakout::CreateHighScore(builder, builder.CreateString("John Smith"), 10);
        auto score2 = Breakout::CreateHighScore(builder, builder.CreateString("A. Adams"), 50);
        auto score3 = Breakout::CreateHighScore(builder, builder.CreateString("B. Bana"), 100);
        auto score4 = Breakout::CreateHighScore(builder, builder.CreateString("S. Lansiquot"), 1000000);
        
        std::vector<flatbuffers::Offset<Breakout::HighScore>> highScoreVec = {score1, score2, score3, score4};
        
        auto scores = Breakout::CreateHighScores(builder, builder.CreateVector(highScoreVec));
        
        builder.Finish(scores);
        
        WriteBuffer("breakout_high_scores.bin", builder);
    }
    
    void MakePlayerSettings()
    {
        flatbuffers::FlatBufferBuilder builder;
        
        auto settings = Breakout::CreatePlayerSettings(builder, 0, 3, 0);
        
        builder.Finish(settings);
        
        WriteBuffer("breakout_player_save.bin", builder);
    }

    void MakeAllData()
    {
        MakeLevels();
        MakePowerups();
        MakeHighScores();
        MakePlayerSettings();
    }
    
    void MakeAssetManifest(const std::string& manifestPath)
    {
#ifdef R2_ASSET_PIPELINE
        std::string assetBinDir = ASSET_BIN_DIR;
        std::string assetRawDir = ASSET_RAW_DIR;
        std::string assetFBSSchemaDir = ASSET_FBS_SCHEMA_DIR;
        
        r2::asset::pln::AssetManifest manifest;
        manifest.outputType = r2::asset::pln::AssetType::Zip;
        manifest.assetOutputPath = assetBinDir + r2::fs::utils::PATH_SEPARATOR + "AllBreakoutData.zip";
        
        r2::asset::pln::AssetFileCommand inputFile;
        inputFile.command.compile = r2::asset::pln::AssetCompileCommand::FlatBufferCompile;
        inputFile.command.schemaPath = assetFBSSchemaDir + r2::fs::utils::PATH_SEPARATOR + "BreakoutHighScoreSchema.fbs";
        
        inputFile.inputPath = assetRawDir + r2::fs::utils::PATH_SEPARATOR + "breakout_high_scores.json";
        inputFile.outputFileName = "breakout_high_scores.scores";
        manifest.fileCommands.push_back(inputFile);
        
        inputFile.command.compile = r2::asset::pln::AssetCompileCommand::FlatBufferCompile;
        inputFile.command.schemaPath = assetFBSSchemaDir + r2::fs::utils::PATH_SEPARATOR + "BreakoutLevelSchema.fbs";
        inputFile.inputPath = assetRawDir + r2::fs::utils::PATH_SEPARATOR + "breakout_level_pack.json";
        inputFile.outputFileName = "breakout_level_pack.breakout_level";
        manifest.fileCommands.push_back(inputFile);
        
        inputFile.command.compile = r2::asset::pln::AssetCompileCommand::FlatBufferCompile;
        inputFile.command.schemaPath = assetFBSSchemaDir + r2::fs::utils::PATH_SEPARATOR + "BreakoutPlayerSettings.fbs";
        inputFile.inputPath = assetRawDir + r2::fs::utils::PATH_SEPARATOR + "breakout_player_save.json";
        inputFile.outputFileName = "breakout_player_save.player";
        manifest.fileCommands.push_back(inputFile);
        
        inputFile.command.compile = r2::asset::pln::AssetCompileCommand::FlatBufferCompile;
        inputFile.command.schemaPath = assetFBSSchemaDir + r2::fs::utils::PATH_SEPARATOR + "BreakoutPowerupSchema.fbs";
        inputFile.inputPath = assetRawDir + r2::fs::utils::PATH_SEPARATOR + "breakout_powerups.json";
        inputFile.outputFileName = "breakout_powerups.powerup";
        manifest.fileCommands.push_back(inputFile);
        
        r2::asset::pln::WriteAssetManifest(manifest, "BreakoutData.aman", manifestPath);
#endif
    }
}

namespace
{
    bool ResolveCategoryPath(u32 category, char* path);
}

class Sandbox: public r2::Application
{
public:
    
    const u64 NUM_DRAWS = 15;
    const u64 NUM_DRAW_COMMANDS = 30;
    const u64 NUM_BONES = 1000;

    virtual bool Init() override
    {
        memoryAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("SandboxArea");

        R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "Invalid memory area");
        
        r2::mem::MemoryArea* sandBoxMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
        R2_CHECK(sandBoxMemoryArea != nullptr, "Failed to get the memory area!");

        auto result = sandBoxMemoryArea->Init(Megabytes(4), 0);
        R2_CHECK(result == true, "Failed to initialize memory area");
        
        subMemoryAreaHandle = sandBoxMemoryArea->AddSubArea(Megabytes(4));
        R2_CHECK(subMemoryAreaHandle != r2::mem::MemoryArea::SubArea::Invalid, "sub area handle is invalid!");
        
        auto subMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle)->GetSubArea(subMemoryAreaHandle);
        
        linearArenaPtr = EMPLACE_LINEAR_ARENA(*subMemoryArea);
        
        R2_CHECK(linearArenaPtr != nullptr, "Failed to create linear arena!");
        
        mPersController.Init(4.0f, 70.0f, static_cast<float>(CENG.DisplaySize().width) / static_cast<float>(CENG.DisplaySize().height), 0.1f, 100.f, glm::vec3(0.0f, -1.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        r2::draw::renderer::SetRenderCamera(mPersController.GetCameraPtr());
        
        modelMats = MAKE_SARRAY(*linearArenaPtr, glm::mat4, NUM_DRAWS);
        /*animModelMats = MAKE_SARRAY(*linearArenaPtr, glm::mat4, NUM_DRAWS);

        mTransparentWindowMats = MAKE_SARRAY(*linearArenaPtr, glm::mat4, NUM_DRAWS);
        mTransparentWindowModelRefHandle = r2::draw::renderer::GetDefaultModelRef(r2::draw::QUAD);
        mTransparentWindowDrawFlags = MAKE_SARRAY(*linearArenaPtr, r2::draw::DrawFlags, NUM_DRAWS);*/

        r2::draw::DrawFlags drawFlags;
        drawFlags.Set(r2::draw::eDrawFlags::DEPTH_TEST);
        
        r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

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


		char materialsPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(SANDBOX_MATERIALS_MANIFESTS_BIN, materialsPath, "SandboxMaterialParamsPack.mppk");

		char texturePackPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(SANDBOX_TEXTURES_MANIFESTS_BIN, texturePackPath, "SandboxTexturePack.tman");


        r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

		const byte* materialManifestData = r2::asset::lib::GetManifestData(assetLib, r2::asset::Asset::GetAssetNameForFilePath(materialsPath, r2::asset::EngineAssetType::MATERIAL_PACK_MANIFEST));

		const flat::MaterialParamsPack* gameMaterialPack = flat::GetMaterialParamsPack(materialManifestData);

		gameAssetManager.LoadMaterialTextures(gameMaterialPack);

		u32 totalNumberOfTextures = 0;
		u32 totalNumberOfTexturePacks = 0;
		u32 totalNumCubemaps = 0;
		u32 cacheSize = 0;
		r2::draw::texche::GetTexturePacksCacheSizes(texturePackPath, totalNumberOfTextures, totalNumberOfTexturePacks, totalNumCubemaps, cacheSize);

		r2::SArray<r2::draw::tex::Texture>* gameTextures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::Texture, totalNumberOfTextures);
		r2::SArray<r2::draw::tex::CubemapTexture>* gameCubemaps = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::CubemapTexture, totalNumCubemaps);

		gameAssetManager.GetTexturesForMaterialParamsPack(gameMaterialPack, gameTextures, gameCubemaps);

        r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();
		r2::draw::rmat::UploadMaterialTextureParamsArray(*renderMaterialCache, gameMaterialPack, gameTextures, gameCubemaps);

		FREE(gameCubemaps, *MEM_ENG_SCRATCH_PTR);
		FREE(gameTextures, *MEM_ENG_SCRATCH_PTR);


        //auto microbatHandle = gameAssetManager.LoadAsset(r2::asset::Asset("micro_bat.rmdl", r2::asset::RMODEL));
        //mMicroBatModel = gameAssetManager.GetAssetDataConst<r2::draw::AnimModel>(microbatHandle);


        //auto skeletonHandle = gameAssetManager.LoadAsset(r2::asset::Asset("skeleton_archer_allinone.rmdl", r2::asset::RMODEL));
        //mSkeletonModel = gameAssetManager.GetAssetDataConst<r2::draw::AnimModel>(skeletonHandle);

        //auto ellenHandle = gameAssetManager.LoadAsset(r2::asset::Asset("EllenIdle.rmdl", r2::asset::RMODEL));
        //mEllenModel = gameAssetManager.GetAssetDataConst<r2::draw::AnimModel>(ellenHandle);

        //mSelectedAnimModel = mSkeletonModel;

        auto sponzaHandle = gameAssetManager.LoadAsset(r2::asset::Asset("Sponza.rmdl", r2::asset::RMODEL));
        mSponzaModel = gameAssetManager.GetAssetDataConst<r2::draw::Model>(sponzaHandle);


        glm::mat4 sponzaModelMatrix = glm::mat4(1.0);

        sponzaModelMatrix = sponzaModelMatrix * mSponzaModel->globalInverseTransform;

        sponzaModelMatrix = glm::rotate(sponzaModelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));

        r2::sarr::Push(*modelMats, sponzaModelMatrix);

        r2::util::Random randomizer;
        
        //color grading
        const r2::draw::tex::Texture* colorGradingLUT = gameAssetManager.GetAlbedoTextureForMaterialName(gameMaterialPack, STRING_ID("ColorGradingLUT")); 

        r2::draw::renderer::SetColorGradingLUT(colorGradingLUT, 32);
        r2::draw::renderer::SetColorGradingContribution(0.2);
        r2::draw::renderer::EnableColorGrading(true);

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

        mSkyboxModelRef = r2::draw::renderer::GetDefaultModelRef(r2::draw::SKYBOX);

        //mSkeletonModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
        //mEllenModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;

        mSponzaModelRefHandle = r2::draw::renderer::UploadModel(mSponzaModel);

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

        r2::draw::renderer::SetClearColor(glm::vec4(0.f, 0.f, 0.f, 1.f));
        //setup the lights
        {
            const auto* prefilteredCubemap = gameAssetManager.GetCubemapTextureForMaterialName(gameMaterialPack, STRING_ID("NewportPrefiltered")); 
            s32 numMips = prefilteredCubemap->numMipLevels;

            r2::draw::renderer::AddSkyLight(
                *r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, STRING_ID("NewportConvolved")),
                *r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, STRING_ID("NewportPrefiltered")),
                *r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, STRING_ID("NewportLUTDFG")), numMips);

			r2::draw::DirectionLight dirLight;
			dirLight.lightProperties.color = glm::vec4(1, 0.5, 80.f / 255.f, 1.0f);

			dirLight.direction = glm::normalize(glm::vec4(0.0f) - glm::vec4(0.0f, 0.0f, 100.0f, 0.0f));
			dirLight.lightProperties.intensity = 2;
            dirLight.lightProperties.castsShadowsUseSoftShadows = glm::uvec4(1, 0, 0, 0);

            r2::draw::renderer::AddDirectionLight(dirLight);

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


			//for (int i = 0; i < 50; ++i)
			//{
			//	r2::draw::PointLight pointLight;

			//	auto zPos = (int)randomizer.RandomNum(1, 10);
			//	auto xPos = (int)randomizer.RandomNum(0, 13) - (int)randomizer.RandomNum(0, 13);
			//	auto yPos = (int)randomizer.RandomNum(0, 10) - (int)randomizer.RandomNum(0, 10);

			//	auto r = randomizer.RandomNum(0, 255);
			//	auto g = randomizer.RandomNum(0, 255);
			//	auto b = randomizer.RandomNum(0, 255);

			//	pointLight.position = glm::vec4(xPos, yPos, zPos, 1.0);

			//	pointLight.lightProperties.color = glm::vec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);

			//	pointLight.lightProperties.intensity = 100.0;
			//	pointLight.lightProperties.fallOff = 0.1;

			//	pointLight.lightProperties.castsShadowsUseSoftShadows = glm::uvec4(0, 0, 0, 0);

			//	r2::draw::renderer::AddPointLight(pointLight);
			//}

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

		dispatcher.Dispatch<r2::evt::WindowResizeEvent>([this](const r2::evt::WindowResizeEvent& e)
			{
                if (!ShouldKeepAspectRatio())
                {
                    mPersController.SetAspect(static_cast<float>(e.NewWidth()) / static_cast<float>(e.NewHeight()));
                }

				//r2::draw::renderer::WindowResized(e.Width(), e.Height());
				// r2::draw::OpenGLResizeWindow(e.Width(), e.Height());
				return true;
			});

		dispatcher.Dispatch<r2::evt::MouseButtonPressedEvent>([this](const r2::evt::MouseButtonPressedEvent& e)
			{
				if (e.MouseButton() == r2::io::MOUSE_BUTTON_LEFT)
				{
					r2::math::Ray ray = r2::cam::CalculateRayFromMousePosition(mPersController.GetCamera(), e.XPos(), e.YPos());

					//  r2::draw::OpenGLDrawRay(ray);
					return true;
				}

				return false;
			});

		dispatcher.Dispatch<r2::evt::KeyPressedEvent>([this](const r2::evt::KeyPressedEvent& e) {
			if (e.KeyCode() == r2::io::KEY_LEFT)
			{
     //           if (mSelectedAnimModel)
     //           {
     //               s64 numAnimations = r2::sarr::Size(*mAnimationsHandles) / 2;
					//if (mSelectedAnimationID - 1 < 0)
					//{
					//	mSelectedAnimationID = (s32)numAnimations - 1;
					//}
					//else
					//{
					//	--mSelectedAnimationID;
					//}
     //           }
                
				return true;
			}
			else if (e.KeyCode() == r2::io::KEY_RIGHT)
			{
                //if (mSelectedAnimModel)
                //{
                //    u64 numAnimations = r2::sarr::Size(*mAnimationsHandles) / 2;
                //    mSelectedAnimationID = size_t(mSelectedAnimationID + 1) % numAnimations;
                //}
                
				return true;
			}
            else if (e.KeyCode() == r2::io::KEY_F1 && (e.Modifiers() & r2::io::Key::SHIFT_PRESSED_KEY) == 0)
            {
                mDrawDebugBones = !mDrawDebugBones;
                return true;
            }
            else if (e.KeyCode() == r2::io::KEY_UP)
            {
                s32 prevResolution = mResolution;
                mResolution = (mResolution + 1) % RESOLUTIONS_COUNT;
                MENG.SetResolution(g_resolutions[prevResolution], g_resolutions[mResolution]);
                PrintResolution();


                return true;
            }
            else if (e.KeyCode() == r2::io::KEY_DOWN)
            {
                s32 prevResolution = mResolution;

                --mResolution;
                if (mResolution < 0)
                {
                    mResolution = RESOLUTIONS_COUNT - 1;
                }
                
                MENG.SetResolution(g_resolutions[prevResolution], g_resolutions[mResolution]);

                PrintResolution();

                return true;
            }
            else if (e.KeyCode() == r2::io::KEY_f)
            {
                MENG.SetFullScreen();
                return true;
            }
            else if (e.KeyCode() == r2::io::KEY_i)
            {

				printf("======================= Capacity =============================\n");

				auto staticVertexBufferCapacity = r2::draw::renderer::GetStaticVertexBufferCapacity();
				auto animVertexBufferCapacity = r2::draw::renderer::GetAnimVertexBufferCapacity();

				printf("Static Vertex Buffer vbo - capacity: %u\n", staticVertexBufferCapacity.vertexBufferSizes[0]);
				printf("Static Vertex Buffer ibo - capacity: %u\n", staticVertexBufferCapacity.indexBufferSize);

				for (int i = 0; i < animVertexBufferCapacity.numVertexBuffers; ++i)
				{
					printf("Anim Vertex Buffer vbo[%i] - capacity: %u\n", i, animVertexBufferCapacity.vertexBufferSizes[i]);
				}

				printf("Anim Vertex Buffer ibo - capacity: %u\n", animVertexBufferCapacity.indexBufferSize);


                printf("======================= Size =============================\n");

				auto staticVertexBufferSize = r2::draw::renderer::GetStaticVertexBufferSize();
				auto animVertexBufferSize = r2::draw::renderer::GetAnimVertexBufferSize();

				printf("Static Vertex Buffer vbo - size: %u\n", staticVertexBufferSize.vertexBufferSizes[0]);
				printf("Static Vertex Buffer ibo - size: %u\n", staticVertexBufferSize.indexBufferSize);

				for (int i = 0; i < animVertexBufferSize.numVertexBuffers; ++i)
				{
					printf("Anim Vertex Buffer vbo[%i] - size: %u\n", i, animVertexBufferSize.vertexBufferSizes[i]);
				}

				printf("Anim Vertex Buffer ibo - size: %u\n", animVertexBufferSize.indexBufferSize);

                printf("======================= Remaining Size =============================\n");

                auto staticVertexBufferRemainingSize = r2::draw::renderer::GetStaticVertexBufferRemainingSize();
                auto animVertexBufferRemainingSize = r2::draw::renderer::GetAnimVertexBufferRemainingSize();

                printf("Static Vertex Buffer vbo - remaining size: %u\n", staticVertexBufferRemainingSize.vertexBufferSizes[0] );
                printf("Static Vertex Buffer ibo - remaining size: %u\n", staticVertexBufferRemainingSize.indexBufferSize );

                for (int i = 0; i < animVertexBufferRemainingSize.numVertexBuffers; ++i)
                {
                    printf("Anim Vertex Buffer vbo[%i] - remaining size: %u\n", i, animVertexBufferRemainingSize.vertexBufferSizes[i]);
                }

                printf("Anim Vertex Buffer ibo - remaining size: %u\n", animVertexBufferRemainingSize.indexBufferSize);
            }
            else if (e.KeyCode() == r2::io::KEY_u)
            {
            //    r2::draw::renderer::UnloadAllAnimModels();
          //      mMicroBatModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
           //     mSkeletonModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
            //    mEllenModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
			/*if (r2::draw::renderer::IsModelLoaded(mSponzaModelRefHandle) &&
				r2::draw::renderer::IsModelLoaded(r2::sarr::At(*mAnimModelRefs, 0)) &&
				r2::draw::renderer::IsModelLoaded(r2::sarr::At(*mAnimModelRefs, 1)))
			{
				r2::draw::renderer::UnloadModel(mSponzaModelRefHandle);

				R2_CHECK(!r2::draw::renderer::IsModelRefHandleValid(mSponzaModelRefHandle), "This handle shouldn't be valid anymore!");

				mSponzaModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;

				r2::SArray<r2::draw::vb::GPUModelRefHandle>* modelRefsToUnload = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::vb::GPUModelRefHandle, 2);


				r2::sarr::Push(*modelRefsToUnload, r2::sarr::At(*mAnimModelRefs, 0));
				r2::sarr::Push(*modelRefsToUnload, r2::sarr::At(*mAnimModelRefs, 1));

				r2::draw::renderer::UnloadAnimModelRefHandles(modelRefsToUnload);

				for (int i = 0; i < r2::sarr::Size(*modelRefsToUnload); ++i)
				{
					R2_CHECK(!r2::draw::renderer::IsModelRefHandleValid(r2::sarr::At(*modelRefsToUnload, i)), "This handle shouldn't be valid anymore!");

					r2::sarr::At(*mAnimModelRefs, i) = r2::draw::vb::InvalidGPUModelRefHandle;
				}

				FREE(modelRefsToUnload, *MEM_ENG_SCRATCH_PTR);
			}
			else
			{
				mSponzaModelRefHandle = r2::draw::renderer::UploadModel(mSponzaModel);

				r2::SArray<const r2::draw::AnimModel*>* animModelsToUpload = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const r2::draw::AnimModel*, 2);

				r2::sarr::Push(*animModelsToUpload, mMicroBatModel);
				r2::sarr::Push(*animModelsToUpload, mSkeletonModel);

				r2::SArray<r2::draw::vb::GPUModelRefHandle>* animModelRefHandles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::vb::GPUModelRefHandle, 2);

				r2::draw::renderer::UploadAnimModels(*animModelsToUpload, *animModelRefHandles);

				for (int i = 0; i < r2::sarr::Size(*animModelRefHandles); ++i)
				{
					R2_CHECK(r2::draw::renderer::IsModelRefHandleValid(r2::sarr::At(*animModelRefHandles, i)), "This handle shouldn't be valid anymore!");

					r2::sarr::At(*mAnimModelRefs, i) = r2::sarr::At(*animModelRefHandles, i);
				}

				FREE(animModelRefHandles, *MEM_ENG_SCRATCH_PTR);

				FREE(animModelsToUpload, *MEM_ENG_SCRATCH_PTR);
			}*/
            }
            else if (e.KeyCode() == r2::io::KEY_j)
            {
			/* if (!r2::draw::renderer::IsModelLoaded(mMicroBatModelRefHandle))
			 {
				 mMicroBatModelRefHandle = r2::draw::renderer::UploadAnimModel(mMicroBatModel);
			 }*/
            }
            else if (e.KeyCode() == r2::io::KEY_k)
            {
			/*if (!r2::draw::renderer::IsModelLoaded(mSkeletonModelRefHandle))
			{
				mSkeletonModelRefHandle = r2::draw::renderer::UploadAnimModel(mSkeletonModel);
			}*/
            }
            else if (e.KeyCode() == r2::io::KEY_l)
            {
                //if (!r2::draw::renderer::IsModelLoaded(mEllenModelRefHandle))
                //{
                //    mEllenModelRefHandle = r2::draw::renderer::UploadAnimModel(mEllenModel);
                //}
            }
            else if (e.KeyCode() == r2::io::KEY_m)
            {

			    //r2::ecs::AudioEmitterActionComponent audioEmitterActionComponent;
			    //audioEmitterActionComponent.action = r2::ecs::AEA_PLAY;

       //         if (MENG.GetECSWorld().GetECSCoordinator()->NumLivingEntities() > 0)
       //         { 
       //             MENG.GetECSWorld().GetECSCoordinator()->AddComponent<r2::ecs::AudioEmitterActionComponent>(1, audioEmitterActionComponent);
       //         }
			   



                /*r2::audio::AudioEngine audioEngine;

                if (r2::audio::AudioEngine::IsEventInstanceHandleValid(mMusicEventHandle))
                {
                    audioEngine.PlayEvent(mMusicEventHandle, false);
                }
                else
                {
                    mMusicEventHandle = audioEngine.PlayEvent("event:/Music", false);
                }*/
            }
            else if (e.KeyCode() == r2::io::KEY_p)
            {
                r2::audio::AudioEngine audioEngine;
                if (r2::audio::AudioEngine::IsEventInstanceHandleValid(mMusicEventHandle))
                {
                    audioEngine.PauseEvent(mMusicEventHandle);
                }
            }
            else if (e.KeyCode() == r2::io::KEY_n)
            {
                r2::audio::AudioEngine audioEngine;
                if (r2::audio::AudioEngine::IsEventInstanceHandleValid(mMusicEventHandle))
                {
                    audioEngine.StopEvent(mMusicEventHandle, true);
                }
            }
            else if (e.KeyCode() == r2::io::KEY_o)
            {
                r2::audio::AudioEngine audioEngine;
                if (r2::audio::AudioEngine::IsEventInstanceHandleValid(mMusicEventHandle))
                {
                    bool isPlaying = audioEngine.IsEventPlaying(mMusicEventHandle);

                    if (isPlaying)
                    {
                        printf("Music is currently playing\n");
                    }
                    else
                    {
                        printf("Music is currently not playing\n");
                    }

                    bool isMusicPaused = audioEngine.IsEventPaused(mMusicEventHandle);
                    if (isMusicPaused)
                    {
                        printf("Music is paused\n");
                    }
                    else
                    {
                        printf("Music is not paused\n");
                    }

                    bool hasStopped = audioEngine.HasEventStopped(mMusicEventHandle);
                    if (hasStopped)
                    {
                        printf("Music has stopped\n");
                    }
                    else
                    {
                        printf("Music not stopped\n");
                    }
                }
            }
            else if (e.KeyCode() == r2::io::KEY_h)
            {
			    static r2::util::Random randomizer;

			    std::call_once(std::once_flag{}, [&]() {
				    randomizer.Randomize();
				});

                r2::audio::AudioEngine audioEngine;
                if (!r2::audio::AudioEngine::IsEventInstanceHandleValid(mEventInstanceHandle))
                {
                    mEventInstanceHandle = audioEngine.CreateEventInstance("event:/MyEvent");
                }

                R2_CHECK(r2::audio::AudioEngine::IsEventInstanceHandleValid(mEventInstanceHandle), "?");

                const r2::Camera& camera = mPersController.GetCamera();

				r2::audio::AudioEngine::Attributes3D attributes;
                
                float paramValue = randomizer.Randomf();

                r2::audio::AudioEngine::SetEventParameterByName(mEventInstanceHandle, "Parameter 1", paramValue);

				attributes.position.x = static_cast<s32>(randomizer.RandomNum(0, 20)) - 10;
                attributes.position.y = static_cast<s32>(randomizer.RandomNum(0, 20)) - 10;
                attributes.position.z = static_cast<s32>(randomizer.RandomNum(0, 20)) - 10;

				attributes.look = glm::vec3(0, 0, 1);
				attributes.up = camera.up;
				attributes.velocity = glm::vec3(0);

                audioEngine.PlayEvent(mEventInstanceHandle, attributes);
            }
			return false;
		});

		mPersController.OnEvent(e);
    }
    
    virtual void Update() override
    {
        mPersController.Update();

        //update listener
        {
			const r2::Camera& camera = mPersController.GetCamera();

			r2::audio::AudioEngine audioEngine;

			r2::audio::AudioEngine::Attributes3D attributes;
			attributes.position = camera.position;
			attributes.look = camera.facing;
			attributes.up = camera.up;
			attributes.velocity = glm::vec3(0);

			audioEngine.SetListener3DAttributes(r2::audio::AudioEngine::DEFAULT_LISTENER, attributes);
        }

        //r2::sarr::Clear(*mSkeletonBoneTransforms);
        //r2::sarr::Clear(*mEllenBoneTransforms);

        //r2::sarr::Clear(*mSkeletonDebugBones);
        //r2::sarr::Clear(*mEllenDebugBones);

      //  auto time = CENG.GetTicks();

      //  r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

        //@TODO(Serge): fix this other skeleton animation bugs - setting to 0 so it won't crash
     //   const r2::draw::Animation* skeletonAnimation = gameAssetManager.GetAssetDataConst<r2::draw::Animation>(r2::sarr::At(*mAnimationsHandles, mSelectedAnimationID)); // r2::draw::animcache::GetAnimation(*mAnimationCache, r2::sarr::At(*mAnimationsHandles, mSelectedAnimationID + 3));
     //   const r2::draw::Animation* ellenAnimation = gameAssetManager.GetAssetDataConst<r2::draw::Animation>(r2::sarr::At(*mAnimationsHandles, mSelectedAnimationID+3));//r2::draw::animcache::GetAnimation(*mAnimationCache, r2::sarr::At(*mAnimationsHandles, mSelectedAnimationID + 6));

		//r2::draw::PlayAnimationForAnimModel(time, 0, true, *mMicroBatModel, microbatAnimation, *mBatBoneTransforms, *mBatDebugBones, 0);

		//r2::draw::PlayAnimationForAnimModel(time, 0, true, *mMicroBatModel, microbatAnimation3, *mBat2BoneTransforms, *mBat2DebugBones, 0);

	//	r2::draw::PlayAnimationForAnimModel(time, 0, true, *mSkeletonModel, skeletonAnimation, *mSkeletonBoneTransforms, *mSkeletonDebugBones, 0);

	//	r2::draw::PlayAnimationForAnimModel(time, 0, true, *mEllenModel, ellenAnimation, *mEllenBoneTransforms, *mEllenDebugBones, 0);
    }

    virtual void Render(float alpha) override
    {
        r2::draw::DrawParameters drawWorldParams;
        drawWorldParams.layer = r2::draw::DL_WORLD;
        drawWorldParams.flags.Clear();
        drawWorldParams.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);

        r2::draw::renderer::SetDefaultCullState(drawWorldParams);
        r2::draw::renderer::SetDefaultStencilState(drawWorldParams);
        r2::draw::renderer::SetDefaultBlendState(drawWorldParams);

        r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();

        if (r2::draw::renderer::IsModelLoaded(mSponzaModelRefHandle))
        {
			r2::SArray<glm::mat4>* sponzaModelMatrices = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, 1);
			r2::sarr::Push(*sponzaModelMatrices, r2::sarr::At(*modelMats, 0));

			const r2::draw::vb::GPUModelRef* sponzaGPUModelRef = r2::draw::renderer::GetGPUModelRef(mSponzaModelRefHandle);

			r2::SArray<r2::draw::RenderMaterialParams>* renderMaterialParams = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::RenderMaterialParams, sponzaGPUModelRef->numMaterials);
			r2::SArray<r2::draw::ShaderHandle>* shaderHandles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::ShaderHandle, sponzaGPUModelRef->numMaterials);

			for (u32 i = 0; i < sponzaGPUModelRef->numMaterials; ++i)
			{
                const r2::draw::RenderMaterialParams* renderMaterial= r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, r2::sarr::At(*sponzaGPUModelRef->materialNames, i).name);

                R2_CHECK(renderMaterial != nullptr, "...");
                r2::sarr::Push(*renderMaterialParams, *renderMaterial);
                r2::sarr::Push(*shaderHandles, r2::sarr::At(*sponzaGPUModelRef->shaderHandles, i));
			}

			r2::draw::renderer::DrawModel(drawWorldParams, mSponzaModelRefHandle, *sponzaModelMatrices, 1, *renderMaterialParams, *shaderHandles, nullptr);

			FREE(shaderHandles, *MEM_ENG_SCRATCH_PTR);
			FREE(renderMaterialParams, *MEM_ENG_SCRATCH_PTR);
			FREE(sponzaModelMatrices, *MEM_ENG_SCRATCH_PTR);
        }

        //r2::draw::DrawParameters drawTransparentWindowParams;
        //drawTransparentWindowParams.layer = r2::draw::DL_TRANSPARENT;
        //drawTransparentWindowParams.flags.Clear();
        //drawTransparentWindowParams.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);
        //
        //r2::draw::renderer::SetDefaultCullState(drawTransparentWindowParams);
        //r2::draw::renderer::SetDefaultStencilState(drawTransparentWindowParams);
        //r2::draw::renderer::SetDefaultBlendState(drawTransparentWindowParams);

		char materialsPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(SANDBOX_MATERIALS_MANIFESTS_BIN, materialsPath, "SandboxMaterialParamsPack.mppk");

        u64 materialParamsPackName = r2::asset::Asset::GetAssetNameForFilePath(materialsPath, r2::asset::MATERIAL_PACK_MANIFEST);
        //draw transparent windows
  /*      const r2::draw::RenderMaterialParams* transparentWindowRenderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, STRING_ID("TransparentWindow"));
        r2::draw::ShaderHandle transparentWindowShaderHandle = r2::mat::GetShaderHandleForMaterialName({ STRING_ID("TransparentWindow"), materialParamsPackName });

        r2::SArray<r2::draw::RenderMaterialParams>* transparentWindowRenderMaterials = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::RenderMaterialParams, 1);
        r2::SArray<r2::draw::ShaderHandle>* transparentWindowShaderHandles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::ShaderHandle, 1);

        r2::sarr::Push(*transparentWindowRenderMaterials, *transparentWindowRenderMaterial);
        r2::sarr::Push(*transparentWindowShaderHandles, transparentWindowShaderHandle);
        r2::draw::renderer::DrawModel(drawTransparentWindowParams, mTransparentWindowModelRefHandle, *mTransparentWindowMats, r2::sarr::Size(*mTransparentWindowMats), *transparentWindowRenderMaterials,*transparentWindowShaderHandles, nullptr);

        FREE(transparentWindowShaderHandles, *MEM_ENG_SCRATCH_PTR);
        FREE(transparentWindowRenderMaterials, *MEM_ENG_SCRATCH_PTR);*/

		/*r2::draw::DrawParameters animDrawParams;
		animDrawParams.layer = r2::draw::DL_CHARACTER;
		animDrawParams.flags.Clear();
		animDrawParams.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);

		r2::draw::renderer::SetDefaultCullState(animDrawParams);
		r2::draw::renderer::SetDefaultStencilState(animDrawParams);
		r2::draw::renderer::SetDefaultBlendState(animDrawParams);

		animDrawParams.flags.Set(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);*/

        //Draw the Skeleton
   //     if (r2::draw::renderer::IsModelLoaded(mSkeletonModelRefHandle))
   //     {
			//r2::SArray<glm::mat4>* skeletonModelMats = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, 2);
			//r2::sarr::Push(*skeletonModelMats, r2::sarr::At(*animModelMats, 0));
			//r2::sarr::Push(*skeletonModelMats, r2::sarr::At(*animModelMats, 1));

			//const r2::draw::vb::GPUModelRef* skeletonGPUModelRef = r2::draw::renderer::GetGPUModelRef(mSkeletonModelRefHandle);

			//r2::SArray<r2::draw::RenderMaterialParams>* renderMaterialParams = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::RenderMaterialParams, skeletonGPUModelRef->numMaterials);
			//r2::SArray<r2::draw::ShaderHandle>* shaderHandles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::ShaderHandle, skeletonGPUModelRef->numMaterials);

			//for (u32 i = 0; i < skeletonGPUModelRef->numMaterials; ++i)
			//{
			//	const r2::draw::RenderMaterialParams* renderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, r2::sarr::At(*skeletonGPUModelRef->materialNames, i).name);

			//	R2_CHECK(renderMaterial != nullptr, "...");

			//	r2::sarr::Push(*renderMaterialParams, *renderMaterial);
			//	r2::sarr::Push(*shaderHandles, r2::sarr::At(*skeletonGPUModelRef->shaderHandles, i));
			//}

			//r2::draw::renderer::DrawModel(animDrawParams, mSkeletonModelRefHandle, *skeletonModelMats, 2, *renderMaterialParams, *shaderHandles, mSkeletonBoneTransforms);

			//FREE(shaderHandles, *MEM_ENG_SCRATCH_PTR);
			//FREE(renderMaterialParams, *MEM_ENG_SCRATCH_PTR);
			//FREE(skeletonModelMats, *MEM_ENG_SCRATCH_PTR);
   //     }

   //     //Draw Ellen

   //     if (r2::draw::renderer::IsModelLoaded(mEllenModelRefHandle))
   //     {
			//r2::SArray<glm::mat4>* ellenModelMats = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, 2);
			//r2::sarr::Push(*ellenModelMats, r2::sarr::At(*animModelMats, 2));
			//r2::sarr::Push(*ellenModelMats, r2::sarr::At(*animModelMats, 3));

			//animDrawParams.stencilState.op.stencilFail = r2::draw::KEEP;
			//animDrawParams.stencilState.op.depthFail = r2::draw::KEEP;
			//animDrawParams.stencilState.op.depthAndStencilPass = r2::draw::REPLACE;
			//animDrawParams.stencilState.stencilWriteEnabled = true;
			//animDrawParams.stencilState.stencilEnabled = true;
			//animDrawParams.stencilState.func.func = r2::draw::ALWAYS;
			//animDrawParams.stencilState.func.ref = 1;
			//animDrawParams.stencilState.func.mask = 0xFF;

			//const r2::draw::vb::GPUModelRef* ellenGPUModelRef = r2::draw::renderer::GetGPUModelRef(mEllenModelRefHandle);

			//r2::SArray<r2::draw::RenderMaterialParams>* renderMaterialParams = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::RenderMaterialParams, ellenGPUModelRef->numMaterials);
			//r2::SArray<r2::draw::ShaderHandle>* shaderHandles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::ShaderHandle, ellenGPUModelRef->numMaterials);

			//for (u32 i = 0; i < ellenGPUModelRef->numMaterials; ++i)
			//{
			//	const r2::draw::RenderMaterialParams* renderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, r2::sarr::At(*ellenGPUModelRef->materialNames, i).name);

			//	R2_CHECK(renderMaterial != nullptr, "...");

			//	r2::sarr::Push(*renderMaterialParams, *renderMaterial);
			//	r2::sarr::Push(*shaderHandles, r2::sarr::At(*ellenGPUModelRef->shaderHandles, i));
			//}

   //         r2::draw::renderer::DrawModel(animDrawParams, mEllenModelRefHandle, *ellenModelMats, 2, *renderMaterialParams, *shaderHandles, mEllenBoneTransforms);

			//FREE(shaderHandles, *MEM_ENG_SCRATCH_PTR);
			//FREE(renderMaterialParams, *MEM_ENG_SCRATCH_PTR);

			//animDrawParams.flags.Remove(r2::draw::eDrawFlags::DEPTH_TEST);
			//animDrawParams.layer = r2::draw::DL_EFFECT;
			//animDrawParams.stencilState.stencilEnabled = true;
			//animDrawParams.stencilState.stencilWriteEnabled = false;
			//animDrawParams.stencilState.func.func = r2::draw::NOTEQUAL;
			//animDrawParams.stencilState.func.ref = 1;
			//animDrawParams.stencilState.func.mask = 0xFF;

   //         r2::SArray<r2::draw::RenderMaterialParams>* outlineMaterialParams = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::RenderMaterialParams, 1);
   //         r2::SArray<r2::draw::ShaderHandle>* outlineShaderHandles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::ShaderHandle, 1);

   //         r2::sarr::Push(*outlineMaterialParams, r2::draw::renderer::GetDefaultOutlineRenderMaterialParams(false) );
   //         r2::sarr::Push(*outlineShaderHandles, r2::draw::renderer::GetDefaultOutlineShaderHandle(false));

			//r2::draw::renderer::DrawModel(animDrawParams, mEllenModelRefHandle, *ellenModelMats, 2, *outlineMaterialParams, *outlineShaderHandles, mEllenBoneTransforms);

   //         FREE(outlineShaderHandles, *MEM_ENG_SCRATCH_PTR);
   //         FREE(outlineMaterialParams, *MEM_ENG_SCRATCH_PTR);
			//FREE(ellenModelMats, *MEM_ENG_SCRATCH_PTR);
   //     }

        //Draw the Skybox
		r2::SArray<glm::mat4>* skyboxModelMatrices = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, 1);
		r2::sarr::Push(*skyboxModelMatrices, glm::mat4(1.0f));

        drawWorldParams.layer = r2::draw::DL_SKYBOX;

        r2::SArray<r2::draw::RenderMaterialParams>* skyboxRenderParams = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::RenderMaterialParams, 1);
        r2::SArray < r2::draw::ShaderHandle>* skyboxShaderHandles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::ShaderHandle, 1);

		const r2::draw::RenderMaterialParams* skyboxRenderMaterialParams = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, STRING_ID("NewportSkybox"));
		r2::draw::ShaderHandle skyboxShaderHandle = r2::mat::GetShaderHandleForMaterialName({ STRING_ID("NewportSkybox"), materialParamsPackName });

        r2::sarr::Push(*skyboxRenderParams, *skyboxRenderMaterialParams);
        r2::sarr::Push(*skyboxShaderHandles, skyboxShaderHandle);

        r2::draw::renderer::DrawModel(drawWorldParams, mSkyboxModelRef, *skyboxModelMatrices, 1, *skyboxRenderParams, *skyboxShaderHandles, nullptr);

        FREE(skyboxShaderHandles, *MEM_ENG_SCRATCH_PTR);
        FREE(skyboxRenderParams, *MEM_ENG_SCRATCH_PTR);
        FREE(skyboxModelMatrices, *MEM_ENG_SCRATCH_PTR);

        //Draw the axis
#ifdef R2_DEBUG
        r2::SArray<glm::vec3>* basePositions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 3);
        r2::SArray<glm::vec3>* directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, 3);
        r2::SArray<float>* lengths = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, 3);
        r2::SArray<float>* coneRadii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, 3);
        r2::SArray<glm::vec4>* colors = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec4, 3);

        r2::sarr::Push(*basePositions, glm::vec3(0));
        r2::sarr::Push(*basePositions, glm::vec3(0));
        r2::sarr::Push(*basePositions, glm::vec3(0));

        r2::sarr::Push(*directions, glm::vec3(1, 0, 0));
        r2::sarr::Push(*directions, glm::vec3(0, 1, 0));
        r2::sarr::Push(*directions, glm::vec3(0, 0, 1));

        r2::sarr::Push(*lengths, 2.0f);
        r2::sarr::Push(*lengths, 2.0f);
        r2::sarr::Push(*lengths, 2.0f);

        r2::sarr::Push(*coneRadii, 0.3f);
        r2::sarr::Push(*coneRadii, 0.3f);
        r2::sarr::Push(*coneRadii, 0.3f);

        r2::sarr::Push(*colors, glm::vec4(1, 0, 0, 1));
        r2::sarr::Push(*colors, glm::vec4(0, 1, 0, 1));
        r2::sarr::Push(*colors, glm::vec4(0, 0, 1, 1));

        r2::draw::renderer::DrawArrowInstanced(*basePositions, *directions, *lengths, *coneRadii, *colors, true, true);

        FREE(colors, *MEM_ENG_SCRATCH_PTR);
        FREE(coneRadii, *MEM_ENG_SCRATCH_PTR);
        FREE(lengths, *MEM_ENG_SCRATCH_PTR);
        FREE(directions, *MEM_ENG_SCRATCH_PTR);
        FREE(basePositions, *MEM_ENG_SCRATCH_PTR);

        if (mDrawDebugBones)
        {
         //   r2::draw::renderer::DrawDebugBones(*mSkeletonDebugBones, r2::sarr::At(*animModelMats, 0), glm::vec4(1, 1, 0, 1));
          //  r2::draw::renderer::DrawDebugBones(*mEllenDebugBones, r2::sarr::At(*animModelMats, 2), glm::vec4(1, 1, 0, 1));
        }
#endif
    }
    
    virtual void Shutdown() override
    {

        FREE(modelMats, *linearArenaPtr);

  //      FREE(mAnimationsHandles, *linearArenaPtr);
  //      FREE(animModelMats, *linearArenaPtr);

  //      FREE(mEllenBoneTransforms, *linearArenaPtr);
  //      FREE(mEllenDebugBones, *linearArenaPtr);

		//FREE(mSkeletonBoneTransforms, *linearArenaPtr);
		//FREE(mSkeletonDebugBones, *linearArenaPtr);

  //      FREE(mTransparentWindowMats, *linearArenaPtr);
  //      FREE(mTransparentWindowDrawFlags, *linearArenaPtr);

        FREE_EMPLACED_ARENA(linearArenaPtr);
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
            SANDBOX_MATERIALS_MANIFESTS_BIN + std::string("/SandboxMaterialParamsPack.mppk")
		};
		return manifestBinPaths;
    }

    virtual std::vector<std::string> GetMaterialPackManifestsRawPaths() const override
    {
		std::vector<std::string> manifestRawPaths
		{
            SANDBOX_MATERIALS_MANIFESTS + std::string("/SandboxMaterialParamsPack.json")
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
		r2::fs::utils::AppendSubPath(SANDBOX_MATERIALS_MANIFESTS_BIN, materialsPath, "SandboxMaterialParamsPack.mppk");
		return {std::string( materialsPath )};
	}

    r2::util::Size GetAppResolution() const override
    {
        return g_resolutions[mResolution];
    }

    virtual u32 GetAssetMemorySize() const override
    {
        //@TODO(Serge): calculate this much later
        return Megabytes(350);
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

    virtual bool AddLooseAssetFiles(r2::SArray<r2::asset::AssetFile*>* fileList) const override
    {
		char modelFilePath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "MicroBat/micro_bat.rmdl", modelFilePath);

		r2::asset::RawAssetFile* batModelFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)batModelFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "Skeleton/skeleton_archer_allinone.rmdl", modelFilePath);

		r2::asset::RawAssetFile* skeletonFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)skeletonFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "Ellen/EllenIdle.rmdl", modelFilePath);

		r2::asset::RawAssetFile* ellenFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)ellenFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "Sponza/Sponza.rmdl", modelFilePath);

		r2::asset::RawAssetFile* sponzaFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)sponzaFile);

		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "MicroBat/micro_bat_idle.ranm", modelFilePath);
		r2::asset::RawAssetFile* idleAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)idleAnimFile);

		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "MicroBat/micro_bat_invert_idle.ranm", modelFilePath);
		r2::asset::RawAssetFile* invertIdleAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)invertIdleAnimFile);

		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "MicroBat/micro_bat_attack.ranm", modelFilePath);
		r2::asset::RawAssetFile* attackAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)attackAnimFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Skeleton/skeleton_archer_allinone.ranm", modelFilePath);

		r2::asset::RawAssetFile* skeletonIdleAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)skeletonIdleAnimFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Skeleton/walk.ranm", modelFilePath);

		r2::asset::RawAssetFile* skeletonWalkAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)skeletonWalkAnimFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Skeleton/run.ranm", modelFilePath);

		r2::asset::RawAssetFile* skeletonRoarAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)skeletonRoarAnimFile);



		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Ellen/EllenIdle.ranm", modelFilePath);

		r2::asset::RawAssetFile* ellenIdleAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)ellenIdleAnimFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Ellen/EllenRunForward.ranm", modelFilePath);

		r2::asset::RawAssetFile* ellenRunForwardAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)ellenRunForwardAnimFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Ellen/EllenSpawn.ranm", modelFilePath);

		r2::asset::RawAssetFile* ellenSpawnAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)ellenSpawnAnimFile);


		return true;
    }

	std::string GetLevelPackDataBinPath() const
	{
		return SANDBOX_LEVELS_DIR_BIN;
	}

	std::string GetLevelPackDataJSONPath() const
	{
		return SANDBOX_LEVELS_DIR_RAW;
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
        return {SANDBOX_MATERIAL_PARAMS_DIR };
    }

	std::vector<std::string> GetMaterialPacksBinaryPaths() const
	{
		return {SANDBOX_MATERIALS_PARAMS_DIR_BIN };
	}

    std::vector<r2::asset::pln::FindMaterialPackManifestFileFunc> GetFindMaterialManifestsFuncs() const
    {
        return { r2::asset::pln::FindMaterialParamsPackManifestFile };
    }

    std::vector<r2::asset::pln::GenerateMaterialPackManifestFromDirectoriesFunc> GetGenerateMaterialManifestsFromDirectoriesFuncs() const
    {
        return { r2::asset::pln::GenerateMaterialParamsPackManifestFromDirectories };
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

    void PrintResolution() const
    {
        printf("Resolution is %d by %d\n", g_resolutions[mResolution].width, g_resolutions[mResolution].height);
    }
    
#ifdef R2_EDITOR
    virtual void RegisterComponentImGuiWidgets(r2::edit::InspectorPanel& inspectorPanel) const override
    {

    }
#endif

private:
    r2::mem::MemoryArea::Handle memoryAreaHandle;
    r2::mem::MemoryArea::SubArea::Handle subMemoryAreaHandle;
    r2::cam::PerspectiveController mPersController;

    r2::mem::LinearArena* linearArenaPtr;

  //  r2::draw::vb::GPUModelRefHandle mSkeletonModelRefHandle;
 //   r2::draw::vb::GPUModelRefHandle mEllenModelRefHandle;

    r2::draw::vb::GPUModelRefHandle mSkyboxModelRef;

    r2::SArray<glm::mat4>* modelMats;
   // r2::SArray<glm::mat4>* animModelMats;

 //   r2::draw::vb::GPUModelRefHandle mTransparentWindowModelRefHandle;
  //  r2::SArray<glm::mat4>* mTransparentWindowMats;
  //  r2::SArray<r2::draw::DrawFlags>* mTransparentWindowDrawFlags;

	//r2::SArray<r2::draw::ShaderBoneTransform>* mSkeletonBoneTransforms;
	//r2::SArray<r2::draw::DebugBone>* mSkeletonDebugBones;

	//r2::SArray<r2::draw::ShaderBoneTransform>* mEllenBoneTransforms;
	//r2::SArray<r2::draw::DebugBone>* mEllenDebugBones;

    //r2::SArray<r2::asset::AssetHandle>* mAnimationsHandles;

   // const r2::draw::AnimModel* mSkeletonModel = nullptr;
   // const r2::draw::AnimModel* mEllenModel = nullptr;
    //const r2::draw::AnimModel* mSelectedAnimModel = nullptr;
    const r2::draw::Model* mSponzaModel = nullptr;
    r2::draw::vb::GPUModelRefHandle mSponzaModelRefHandle;

    s32 mSelectedAnimationID = 0;
    bool mDrawDebugBones = false;

    float mExposure = 1.0f;
    s32 mResolution = 3;

    r2::audio::AudioEngine::BankHandle mTestBankHandle;
    r2::audio::AudioEngine::EventInstanceHandle mMusicEventHandle;
    r2::audio::AudioEngine::EventInstanceHandle mEventInstanceHandle;
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
            case r2::fs::utils::LEVELS:
                r2::util::PathCpy(subpath, "assets_bin/Sandbox_Levels");
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
