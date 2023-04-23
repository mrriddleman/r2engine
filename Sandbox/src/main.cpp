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
#include "r2/Core/Assets/AssetFiles/GLTFAssetFile.h"
#include "r2/Core/Assets/AssetFiles/RawAssetFile.h"
#include "r2/Core/Assets/AssetFiles/ZipAssetFile.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Render/Animation/AnimationCache.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Model/ModelSystem.h"
#include "r2/Render/Model/Material.h"
#include "r2/Utils/Hash.h"
#include "r2/Utils/Random.h"
#include "r2/Render/Camera/PerspectiveCameraController.h"
#include "glm/gtc/type_ptr.hpp"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Animation/AnimationPlayer.h"
#include "r2/Render/Model/Light.h"

#include "r2/Render/Model/Material_generated.h"
#include "r2/Render/Model/MaterialPack_generated.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"



#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetManifest.h"
#endif

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
    
    enum ConstantConfigHandles
    {
        VP_MATRICES = 0,
        VECTOR_CONSTANTS,
        MODEL_MATRICES,
        SUB_COMMANDS,
        MODEL_MATERIALS,
        BONE_TRANSFORMS,
        BONE_TRANSFORM_OFFSETS,
        LIGHTING,
        NUM_CONSTANT_CONFIGS
    };

    enum VertexConfigHandles
    {
        STATIC_MODELS_CONFIG = 0,
        ANIM_MODELS_CONFIG,


        DEBUG_CONFIG, //@NOTE: should be next to last always
        NUM_VERTEX_CONFIGS
    };
    
    const u64 NUM_DRAWS = 15;
    const u64 NUM_DRAW_COMMANDS = 30;
    const u64 NUM_BONES = 1000;


    
    virtual bool Init() override
    {
        memoryAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("SandboxArea");

        R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "Invalid memory area");
        
        r2::mem::MemoryArea* sandBoxMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
        R2_CHECK(sandBoxMemoryArea != nullptr, "Failed to get the memory area!");
        
        u64 materialMemorySystemSize = 0;

		
		char materialsPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(SANDBOX_MATERIALS_MANIFESTS_BIN, materialsPath, "SandboxMaterialParamsPack.mppk");

		char texturePackPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(SANDBOX_TEXTURES_MANIFESTS_BIN, texturePackPath, "SandboxTexturePack.tman");

      //  void* materialPackData = nullptr;
      //  void* texturePackManifestData = nullptr;
        materialMemorySystemSize = r2::draw::mat::GetMaterialBoundarySize(materialsPath, texturePackPath);//r2::draw::mat::LoadMaterialAndTextureManifests(materialsPath, texturePackPath, &materialPackData, &texturePackManifestData);
		
        R2_CHECK(materialMemorySystemSize != 0, "Didn't properly load the material and manifests!");

        auto result = sandBoxMemoryArea->Init(Megabytes(128) + materialMemorySystemSize, 0);
        R2_CHECK(result == true, "Failed to initialize memory area");
        
        subMemoryAreaHandle = sandBoxMemoryArea->AddSubArea(Megabytes(4) + materialMemorySystemSize);
        R2_CHECK(subMemoryAreaHandle != r2::mem::MemoryArea::SubArea::Invalid, "sub area handle is invalid!");
        
        auto subMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle)->GetSubArea(subMemoryAreaHandle);
        
        linearArenaPtr = EMPLACE_LINEAR_ARENA(*subMemoryArea);
        
        R2_CHECK(linearArenaPtr != nullptr, "Failed to create linear arena!");
        
        
        char filePath[r2::fs::FILE_PATH_LENGTH];
        
        //Pre-build asset data



        r2::fs::utils::AppendSubPath(ASSET_BIN_DIR, filePath, "AllBreakoutData.zip");
        
        r2::asset::ZipAssetFile* zipFile = r2::asset::lib::MakeZipAssetFile(filePath);
        r2::asset::FileList files = r2::asset::lib::MakeFileList(10);
        r2::sarr::Push(*files, (r2::asset::AssetFile*)zipFile);
        
        assetCacheBoundary = MAKE_BOUNDARY(*linearArenaPtr, Kilobytes(768), 64);
        assetCache = r2::asset::lib::CreateAssetCache(assetCacheBoundary, files);

        assetsBuffers = MAKE_SARRAY(*linearArenaPtr, r2::asset::AssetCacheRecord, 1000);
        
        reload = true;
        
#ifdef R2_ASSET_PIPELINE
        if (assetCache)
        {
            assetCache->AddReloadFunction([this](r2::asset::AssetHandle handle)
            {
                reload = true;
                
                if (assetsBuffers)
                {
                    u64 size = r2::sarr::Size(*assetsBuffers);
                    
                    for (u64 i = 0; i < size; ++i)
                    {
                        auto record = r2::sarr::At(*assetsBuffers, i);
                        
                        if(record.handle.handle == handle.handle)
                        {
                            assetCache->ReturnAssetBuffer(record);
                        }
                    }
                }

            });
        }
#endif

        mPersController.Init(4.0f, 70.0f, static_cast<float>(CENG.DisplaySize().width) / static_cast<float>(CENG.DisplaySize().height), 0.1f, 100.f, glm::vec3(0.0f, -1.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        r2::draw::renderer::SetRenderCamera(mPersController.GetCameraPtr());
        
        modelMats = MAKE_SARRAY(*linearArenaPtr, glm::mat4, NUM_DRAWS);
        animModelMats = MAKE_SARRAY(*linearArenaPtr, glm::mat4, NUM_DRAWS);
        mStaticModelDrawFlags = MAKE_SARRAY(*linearArenaPtr, r2::draw::DrawFlags, NUM_DRAWS);

        mStaticCubeModelMats = MAKE_SARRAY(*linearArenaPtr, glm::mat4, NUM_DRAWS);
        mStaticCubeMaterials = MAKE_SARRAY(*linearArenaPtr, r2::draw::MaterialHandle, NUM_DRAWS);
        mStaticCubesDrawFlags = MAKE_SARRAY(*linearArenaPtr, r2::draw::DrawFlags, NUM_DRAWS);
        mStaticCubeModelRefHandle = r2::draw::renderer::GetDefaultModelRef(r2::draw::CUBE);

        mTransparentWindowMats = MAKE_SARRAY(*linearArenaPtr, glm::mat4, NUM_DRAWS);
        mTransparentWindowMaterialHandles = MAKE_SARRAY(*linearArenaPtr, r2::draw::MaterialHandle, NUM_DRAWS);
        mTransparentWindowModelRefHandle = r2::draw::renderer::GetDefaultModelRef(r2::draw::QUAD);
        mTransparentWindowDrawFlags = MAKE_SARRAY(*linearArenaPtr, r2::draw::DrawFlags, NUM_DRAWS);

        r2::draw::DrawFlags drawFlags;
        drawFlags.Set(r2::draw::eDrawFlags::DEPTH_TEST);

        //for (u32 i = 0; i < r2::draw::FULLSCREEN_TRIANGLE; ++i)
        //{
        //    auto nextModel = static_cast<r2::draw::DefaultModel>(r2::draw::QUAD + i);

        //    if (nextModel == r2::draw::QUAD)
        //    {
        //        for (int i = 0; i < 4; ++i)
        //        {
        //            r2::sarr::Push(*mStaticModelDrawFlags, drawFlags);
        //        }
        //    }

        //    r2::sarr::Push(*mStaticModelDrawFlags, drawFlags);
        //}

        r2::sarr::Push(*mStaticModelDrawFlags, drawFlags); //for Sponza

        mStaticModelRefs = MAKE_SARRAY(*linearArenaPtr, r2::draw::vb::GPUModelRefHandle, NUM_DRAWS);
        mStaticModelMaterialHandles = MAKE_SARRAY(*linearArenaPtr, r2::draw::MaterialHandle, NUM_DRAWS);

		//mAnimModelRefs = MAKE_SARRAY(*linearArenaPtr, r2::draw::vb::GPUModelRefHandle, 3);

        mSkyboxMaterialHandles = MAKE_SARRAY(*linearArenaPtr, r2::draw::MaterialHandle, 1);
        
        mBatBoneTransforms = MAKE_SARRAY(*linearArenaPtr, r2::draw::ShaderBoneTransform, NUM_BONES);
        mBatDebugBones = MAKE_SARRAY(*linearArenaPtr, r2::draw::DebugBone, NUM_BONES);
        
        mBat2BoneTransforms = MAKE_SARRAY(*linearArenaPtr, r2::draw::ShaderBoneTransform, NUM_BONES);
        mBat2DebugBones = MAKE_SARRAY(*linearArenaPtr, r2::draw::DebugBone, NUM_BONES);

		mSkeletonBoneTransforms = MAKE_SARRAY(*linearArenaPtr, r2::draw::ShaderBoneTransform, NUM_BONES);
		mSkeletonDebugBones = MAKE_SARRAY(*linearArenaPtr, r2::draw::DebugBone, NUM_BONES);

		mEllenBoneTransforms = MAKE_SARRAY(*linearArenaPtr, r2::draw::ShaderBoneTransform, NUM_BONES);
		mEllenDebugBones = MAKE_SARRAY(*linearArenaPtr, r2::draw::DebugBone, NUM_BONES);



        glm::mat4 quadMat = glm::mat4(1.0f);
        
      //  quadMat = glm::rotate(quadMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
  //      quadMat = glm::scale(quadMat, glm::vec3(10.0f));
  //      r2::sarr::Push(*modelMats, quadMat);

  //      glm::mat4 quadMat1 = glm::mat4(1.0f);
  //      quadMat1 = glm::translate(quadMat1, glm::vec3(5.0f, 0.0f, 5.0f));
  //      quadMat1 = glm::rotate(quadMat1, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  //      quadMat1 = glm::scale(quadMat1, glm::vec3(10.0f));
  //     
  //      
  //      r2::sarr::Push(*modelMats, quadMat1);

		//glm::mat4 quadMat2 = glm::mat4(1.0f);
  //      quadMat2 = glm::translate(quadMat2, glm::vec3(-5.0f, 0.0f, 5.0f));
  //      quadMat2 = glm::rotate(quadMat2, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		//quadMat2 = glm::scale(quadMat2, glm::vec3(10.0f));
		//
  //      
		//r2::sarr::Push(*modelMats, quadMat2);

		//glm::mat4 quadMat3 = glm::mat4(1.0f);
  //      quadMat3 = glm::translate(quadMat3, glm::vec3(0.0f, 5.0f, 5.0f));
  //      quadMat3 = glm::rotate(quadMat3, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		//quadMat3 = glm::scale(quadMat3, glm::vec3(10.0f));
		//
		//
		//r2::sarr::Push(*modelMats, quadMat3);

		//glm::mat4 quadMat4 = glm::mat4(1.0f);
  //      quadMat4 = glm::translate(quadMat4, glm::vec3(0.0f, -5.0f, 5.0f));
  //      quadMat4 = glm::rotate(quadMat4, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		//quadMat4 = glm::scale(quadMat4, glm::vec3(10.0f));
		//
		//
		//r2::sarr::Push(*modelMats, quadMat4);


		//glm::mat4 cubeMat = glm::mat4(1.0f);
		//cubeMat = glm::translate(cubeMat, glm::vec3(1.5, 0, 2));
		//r2::sarr::Push(*modelMats, cubeMat);

  //      glm::mat4 sphereMat = glm::mat4(1.0f);
  //      sphereMat = glm::translate(sphereMat, glm::vec3(4, 0, 1.1));
  //      r2::sarr::Push(*modelMats, sphereMat);

		//glm::mat4 coneMat = glm::mat4(1.0f);
		//coneMat = glm::translate(coneMat, glm::vec3(-1, 0, 0.6));
		////coneMat = glm::rotate(coneMat, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
		//r2::sarr::Push(*modelMats, coneMat);

  //      glm::mat4 cylinderMat = glm::mat4(1.0f);
  //      
  //      cylinderMat = glm::translate(cylinderMat, glm::vec3(-4, 0.0, 1.6));
  //     // cylinderMat = glm::rotate(cylinderMat, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
  //      r2::sarr::Push(*modelMats, cylinderMat);



        glm::mat4 microbatMat = glm::mat4(1.0f);
        microbatMat = glm::translate(microbatMat, glm::vec3(0, 0, 0.1));
        microbatMat = glm::rotate(microbatMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        microbatMat = glm::scale(microbatMat, glm::vec3(0.01f));
        r2::sarr::Push(*animModelMats, microbatMat);

		glm::mat4 microbatMat2 = glm::mat4(1.0f);
		microbatMat2 = glm::translate(microbatMat2, glm::vec3(0, 1, 0.1));
		microbatMat2 = glm::rotate(microbatMat2, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		microbatMat2 = glm::scale(microbatMat2, glm::vec3(0.01f));
		r2::sarr::Push(*animModelMats, microbatMat2);

        glm::mat4 skeletonModel = glm::mat4(1.0f);
        skeletonModel = glm::translate(skeletonModel, glm::vec3(-3, 0, 0.1));
        skeletonModel = glm::rotate(skeletonModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        skeletonModel = glm::scale(skeletonModel, glm::vec3(0.01f));
        r2::sarr::Push(*animModelMats, skeletonModel);

		glm::mat4 skeletonModel2 = glm::mat4(1.0f);
		skeletonModel2 = glm::translate(skeletonModel2, glm::vec3(-3, 1, 0.1));
		skeletonModel2 = glm::rotate(skeletonModel2, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		skeletonModel2 = glm::scale(skeletonModel2, glm::vec3(0.01f));
		r2::sarr::Push(*animModelMats, skeletonModel2);


        glm::mat4 ellenModel = glm::mat4(1.0f);
        ellenModel = glm::translate(ellenModel, glm::vec3(3, 0, 0.1));
        ellenModel = glm::rotate(ellenModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ellenModel = glm::scale(ellenModel, glm::vec3(0.01f));
        r2::sarr::Push(*animModelMats, ellenModel);

        glm::mat4 ellenModel2 = glm::mat4(1.0);
        ellenModel2 = glm::translate(ellenModel2, glm::vec3(3, 1, 0.1));
		ellenModel2 = glm::rotate(ellenModel2, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		ellenModel2 = glm::scale(ellenModel2, glm::vec3(0.01f));

        r2::sarr::Push(*animModelMats, ellenModel2);

        mAnimationsHandles = MAKE_SARRAY(*linearArenaPtr, r2::draw::AnimationHandle, 20);

		r2::mem::utils::MemBoundary boundary = MAKE_BOUNDARY(*linearArenaPtr, materialMemorySystemSize, 64);

		//const flat::MaterialPack* materialPack = flat::GetMaterialPack(materialPackData);

		//R2_CHECK(materialPack != nullptr, "Failed to get the material pack from the data!");

		//const flat::TexturePacksManifest* texturePacks = flat::GetTexturePacksManifest(texturePackManifestData);

		//R2_CHECK(texturePacks != nullptr, "Failed to get the material pack from the data!");

		mMaterialSystem = r2::draw::matsys::CreateMaterialSystem(boundary, materialsPath, texturePackPath);

		if (!mMaterialSystem)
		{
			R2_CHECK(false, "We couldn't initialize the material system");
			return false;
		}

      //  FREE(texturePackManifestData, *MEM_ENG_SCRATCH_PTR);
      //  FREE(materialPackData, *MEM_ENG_SCRATCH_PTR);

		r2::draw::mat::LoadAllMaterialTexturesFromDisk(*mMaterialSystem);
		r2::draw::mat::UploadAllMaterialTexturesToGPU(*mMaterialSystem);

        r2::asset::FileList modelFiles = r2::asset::lib::MakeFileList(10);

		char modelFilePath[r2::fs::FILE_PATH_LENGTH];

        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "MicroBat/micro_bat.rmdl", modelFilePath);

		r2::asset::RawAssetFile* batModelFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

        r2::sarr::Push(*modelFiles, (r2::asset::AssetFile*)batModelFile);


        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "Skeleton/skeleton_archer_allinone.rmdl", modelFilePath);

        r2::asset::RawAssetFile* skeletonFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

        r2::sarr::Push(*modelFiles, (r2::asset::AssetFile*)skeletonFile);


        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "Ellen/EllenIdle.rmdl", modelFilePath);

        r2::asset::RawAssetFile* ellenFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

        r2::sarr::Push(*modelFiles, (r2::asset::AssetFile*)ellenFile);


        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "Sponza/Sponza.rmdl", modelFilePath);

        r2::asset::RawAssetFile* sponzaFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

        r2::sarr::Push(*modelFiles, (r2::asset::AssetFile*)sponzaFile);

        mModelSystem = r2::draw::modlsys::Init(memoryAreaHandle, Megabytes(64), false, modelFiles, "Sandbox Model System");

        if (!mModelSystem)
        {
            R2_CHECK(false, "Failed to create the model system!");
            return false;
        }
        r2::asset::FileList animationFiles = r2::asset::lib::MakeFileList(100);

        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "MicroBat/micro_bat_idle.ranm", modelFilePath);
        r2::asset::RawAssetFile* idleAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

        r2::sarr::Push(*animationFiles, (r2::asset::AssetFile*)idleAnimFile);

		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "MicroBat/micro_bat_invert_idle.ranm", modelFilePath);
		r2::asset::RawAssetFile* invertIdleAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

        r2::sarr::Push(*animationFiles, (r2::asset::AssetFile*)invertIdleAnimFile);

		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "MicroBat/micro_bat_attack.ranm", modelFilePath);
		r2::asset::RawAssetFile* attackAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*animationFiles, (r2::asset::AssetFile*)attackAnimFile);


        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Skeleton/skeleton_archer_allinone.ranm", modelFilePath);

        r2::asset::RawAssetFile* skeletonIdleAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

        r2::sarr::Push(*animationFiles, (r2::asset::AssetFile*)skeletonIdleAnimFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Skeleton/walk.ranm", modelFilePath);

		r2::asset::RawAssetFile* skeletonWalkAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*animationFiles, (r2::asset::AssetFile*)skeletonWalkAnimFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Skeleton/run.ranm", modelFilePath);

		r2::asset::RawAssetFile* skeletonRoarAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*animationFiles, (r2::asset::AssetFile*)skeletonRoarAnimFile);


        
		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Ellen/EllenIdle.ranm", modelFilePath);

		r2::asset::RawAssetFile* ellenIdleAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*animationFiles, (r2::asset::AssetFile*)ellenIdleAnimFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Ellen/EllenRunForward.ranm", modelFilePath);

		r2::asset::RawAssetFile* ellenRunForwardAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*animationFiles, (r2::asset::AssetFile*)ellenRunForwardAnimFile);


		r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::ANIMATIONS, "Ellen/EllenSpawn.ranm", modelFilePath);

		r2::asset::RawAssetFile* ellenSpawnAnimFile = r2::asset::lib::MakeRawAssetFile(modelFilePath);

		r2::sarr::Push(*animationFiles, (r2::asset::AssetFile*)ellenSpawnAnimFile);


        mAnimationCache = r2::draw::animcache::Init(memoryAreaHandle, Megabytes(16), animationFiles, "Sandbox Animation Cache");

        if (!mAnimationCache)
        {
            R2_CHECK(false, "Failed to create the animation cache");
            return false;
        }

        auto microbatHandle = r2::draw::modlsys::LoadModel(mModelSystem, r2::asset::Asset("micro_bat.rmdl", r2::asset::RMODEL));
        mMicroBatModel = r2::draw::modlsys::GetAnimModel(mModelSystem, microbatHandle);

        auto skeletonHandle = r2::draw::modlsys::LoadModel(mModelSystem, r2::asset::Asset("skeleton_archer_allinone.rmdl", r2::asset::RMODEL));
        mSkeletonModel = r2::draw::modlsys::GetAnimModel(mModelSystem, skeletonHandle);

        auto ellenHandle = r2::draw::modlsys::LoadModel(mModelSystem, r2::asset::Asset("EllenIdle.rmdl", r2::asset::RMODEL));
        mEllenModel = r2::draw::modlsys::GetAnimModel(mModelSystem, ellenHandle);

        mSelectedAnimModel = mMicroBatModel;

        auto sponzaHandle = r2::draw::modlsys::LoadModel(mModelSystem, r2::asset::Asset("Sponza.rmdl", r2::asset::RMODEL));
        mSponzaModel = r2::draw::modlsys::GetModel(mModelSystem, sponzaHandle);

        glm::mat4 sponzaModelMatrix = glm::mat4(1.0);

        sponzaModelMatrix = sponzaModelMatrix * mSponzaModel->globalInverseTransform;

        sponzaModelMatrix = glm::rotate(sponzaModelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));

        r2::sarr::Push(*modelMats, sponzaModelMatrix);
    //    r2::draw::MaterialHandle defaultStaticModelMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*mMaterialSystem, STRING_ID("StoneBlockWall"));
    //    R2_CHECK(r2::draw::mat::IsValid(defaultStaticModelMaterialHandle), "Failed to get a proper handle for the static material!");

        r2::draw::MaterialHandle blueClearCoatMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*mMaterialSystem, STRING_ID("BlueClearCoat"));
        R2_CHECK(r2::draw::mat::IsValid(blueClearCoatMaterialHandle), "Failed to get blue clear coat material handle");

        r2::util::Random randomizer;
   //     randomizer.Randomize();
        r2::sarr::Push(*mStaticCubeMaterials, blueClearCoatMaterialHandle);
        for (u32 i = 0; i < NUM_DRAWS; ++i)
        {
           
            r2::sarr::Push(*mStaticCubesDrawFlags, drawFlags);
            
            //figure out the position
			auto zPos = (int)randomizer.RandomNum(1, 10);
			auto xPos = (int)randomizer.RandomNum(0, 13) - (int)randomizer.RandomNum(0, 13);
			auto yPos = (int)randomizer.RandomNum(0, 10) - (int)randomizer.RandomNum(0, 10);

            glm::vec3 position = glm::vec3(xPos, yPos, zPos);

            glm::mat4 modelMat = glm::mat4(1.0);

            float scale = static_cast<float>(randomizer.RandomNum(2, 7)) / 5.0f;

            float rotationX = static_cast<float>(randomizer.RandomNum(0, 350));
            float rotationY = static_cast<float>(randomizer.RandomNum(0, 350));
            float rotationZ = static_cast<float>(randomizer.RandomNum(0, 350));


            modelMat = glm::translate(modelMat, position);
			modelMat = glm::rotate(modelMat, glm::radians(rotationX), glm::vec3(1, 0, 0));
			modelMat = glm::rotate(modelMat, glm::radians(rotationY), glm::vec3(0, 1, 0));
			modelMat = glm::rotate(modelMat, glm::radians(rotationZ), glm::vec3(0, 0, 1));
            modelMat = glm::scale(modelMat, glm::vec3(scale));
            
            

            r2::sarr::Push(*mStaticCubeModelMats, modelMat);
        }


        //color grading

        r2::draw::MaterialHandle colorGradingMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*mMaterialSystem, STRING_ID("ColorGradingLUT"));
        R2_CHECK(r2::draw::mat::IsValid(colorGradingMaterialHandle), "Failed to get color grading material handle");

        const r2::draw::tex::Texture& colorGradingLUT = r2::draw::mat::GetMaterialTextureAssetsForMaterial(*mMaterialSystem, colorGradingMaterialHandle).normalTextures.materialTexture.diffuseTexture;

        r2::draw::renderer::SetColorGradingLUT(&colorGradingLUT, 32);
        r2::draw::renderer::SetColorGradingContribution(0.2);
        r2::draw::renderer::EnableColorGrading(true);


        //transparent windows
        r2::draw::MaterialHandle transparentWindowMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*mMaterialSystem, STRING_ID("TransparentWindow"));
        R2_CHECK(r2::draw::mat::IsValid(transparentWindowMaterialHandle), "Failed to get transparent window material handle");


        r2::sarr::Push(*mTransparentWindowMaterialHandles, transparentWindowMaterialHandle);


        float startingX = 10.0f;

        for (u32 i = 0; i < NUM_DRAWS; ++i)
        {
            r2::sarr::Push(*mTransparentWindowDrawFlags, drawFlags);

            glm::mat4 modelMat = glm::mat4(1.0);

            modelMat = glm::translate(modelMat, glm::vec3(startingX, 0, 1.0f));
            modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0, 1, 0));
            modelMat = glm::scale(modelMat, glm::vec3(2));

            r2::sarr::Push(*mTransparentWindowMats, modelMat);

            startingX -= 20.f / static_cast<float>(NUM_DRAWS);
        }

    //    r2::draw::MaterialHandle brushedMetalMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*mMaterialSystem, STRING_ID("BrushedMetal"));
    //    R2_CHECK(r2::draw::mat::IsValid(brushedMetalMaterialHandle), "Failed to get the brushed metal material handle");

    //    for (u32 i = 0; i < r2::draw::FULLSCREEN_TRIANGLE; ++i)
    //    {
    //        auto nextModel = static_cast<r2::draw::DefaultModel>(r2::draw::QUAD + i);

    //        if (nextModel == r2::draw::QUAD)
    //        {
    //            for (int i = 0; i < 5; ++i)
    //            {
    //                r2::sarr::Push(*mStaticModelRefs, r2::draw::renderer::GetDefaultModelRef(nextModel));
    //                r2::sarr::Push(*mStaticModelMaterialHandles, defaultStaticModelMaterialHandle);
    //            }


    //           
    //        }
    //        
    //        else
    //        {
    //            r2::sarr::Push(*mStaticModelRefs, r2::draw::renderer::GetDefaultModelRef(nextModel));
				//// defaultStaticModelMaterialHandle = r2::draw::renderer::GetMaterialHandleForDefaultModel(nextModel);
				//if (nextModel == r2::draw::SPHERE)
				//{
				//	r2::sarr::Push(*mStaticModelMaterialHandles, defaultStaticModelMaterialHandle);
				//}
				//else
				//{
				//	r2::sarr::Push(*mStaticModelMaterialHandles, defaultStaticModelMaterialHandle);
				//}
    //        }
    //        
    //    }


        mSkyboxModelRef = r2::draw::renderer::GetDefaultModelRef(r2::draw::SKYBOX);


        r2::draw::MaterialHandle skyboxMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*mMaterialSystem, STRING_ID("NewportSkybox"));

        R2_CHECK(r2::draw::mat::IsValid(skyboxMaterialHandle), "Failed to get a proper handle for the skybox!");

        r2::sarr::Push(*mSkyboxMaterialHandles, skyboxMaterialHandle); //;

        //r2::SArray<const r2::draw::AnimModel*>* animModelsToDraw = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const r2::draw::AnimModel*, NUM_DRAWS);

        //r2::sarr::Push(*animModelsToDraw, mMicroBatModel);
        //r2::sarr::Push(*animModelsToDraw, mSkeletonModel);
        //r2::sarr::Push(*animModelsToDraw, mEllenModel);

        //r2::draw::renderer::UploadAnimModels(*animModelsToDraw, *mAnimModelRefs);

        //FREE(animModelsToDraw, *MEM_ENG_SCRATCH_PTR);

        mMicroBatModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
        mSkeletonModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
        mEllenModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;

        mSponzaModelRefHandle = r2::draw::renderer::UploadModel(mSponzaModel);


        r2::SArray<r2::asset::Asset>* animationAssets = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::asset::Asset, 20);

        r2::sarr::Push(*animationAssets, r2::asset::Asset("micro_bat_idle.ranm", r2::asset::RANIMATION));
        r2::sarr::Push(*animationAssets, r2::asset::Asset("micro_bat_invert_idle.ranm", r2::asset::RANIMATION));
        r2::sarr::Push(*animationAssets, r2::asset::Asset("micro_bat_attack.ranm", r2::asset::RANIMATION));

        r2::sarr::Push(*animationAssets, r2::asset::Asset("skeleton_archer_allinone.ranm", r2::asset::RANIMATION));
        r2::sarr::Push(*animationAssets, r2::asset::Asset("walk.ranm", r2::asset::RANIMATION));
        r2::sarr::Push(*animationAssets, r2::asset::Asset("run.ranm", r2::asset::RANIMATION));

        r2::sarr::Push(*animationAssets, r2::asset::Asset("EllenIdle.ranm", r2::asset::RANIMATION));
        r2::sarr::Push(*animationAssets, r2::asset::Asset("EllenRunForward.ranm", r2::asset::RANIMATION));
        r2::sarr::Push(*animationAssets, r2::asset::Asset("EllenSpawn.ranm", r2::asset::RANIMATION));

        r2::draw::animcache::LoadAnimations(*mAnimationCache, *animationAssets, *mAnimationsHandles);

        FREE(animationAssets, *MEM_ENG_SCRATCH_PTR);


        r2::draw::renderer::SetClearColor(glm::vec4(0.f, 0.f, 0.f, 1.f));
        //setup the lights
        {
         
            
            r2::draw::renderer::AddSkyLight(
                r2::draw::mat::GetMaterialHandleFromMaterialName(*mMaterialSystem, STRING_ID("NewportConvolved")),
                r2::draw::mat::GetMaterialHandleFromMaterialName(*mMaterialSystem, STRING_ID("NewportPrefiltered")),
                r2::draw::mat::GetMaterialHandleFromMaterialName(*mMaterialSystem, STRING_ID("NewportLUTDFG")));
               

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

            //r2::draw::PointLight pointLight;
            //pointLight.position = glm::vec4(0, 1.3, 1, 1.0);
            //pointLight.lightProperties.color = glm::vec4(1.0f);
            //pointLight.lightProperties.intensity = 100.0;
            //pointLight.lightProperties.fallOff = 0.1;
            //pointLight.lightProperties.castsShadowsUseSoftShadows = glm::uvec4(1, 0, 0, 0);
            //r2::draw::renderer::AddPointLight(pointLight);


			//r2::draw::SpotLight spotLight;
			//spotLight.lightProperties.color = glm::vec4(1.0f);

			//spotLight.position = glm::vec4(0, 2, 1, glm::cos(glm::radians(60.f)));
			//spotLight.direction = glm::vec4(glm::vec3(0, -1, 0), glm::cos(glm::radians(75.f)));

			//spotLight.lightProperties.intensity = 100;
			//spotLight.lightProperties.fallOff = 0.01;
			//spotLight.lightProperties.castsShadowsUseSoftShadows = glm::uvec4(1, 0, 0, 0);

			//r2::draw::renderer::AddSpotLight(spotLight);


   //         
			//r2::draw::PointLight pointLight2;

			//pointLight2.position = glm::vec4(-3, 2, -1, 1.0);
			//pointLight2.lightProperties.color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

			//pointLight2.lightProperties.intensity = 1;
			//pointLight2.lightProperties.fallOff = 0.01;




			//r2::draw::PointLight pointLight3;

   //         pointLight3.position = glm::vec4(-1, 2, 0, 1.0);
   //         pointLight3.lightProperties.color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

   //         pointLight3.lightProperties.intensity = 2;
   //         pointLight3.lightProperties.fallOff = 0.01;


			//r2::draw::lightsys::AddPointLight(*mLightSystem, pointLight3);


			//r2::draw::PointLight pointLight4;

			//pointLight4.position = glm::vec4(3, 2, 0, 1.0);
			//pointLight4.lightProperties.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

			//pointLight4.lightProperties.intensity = 5;
			//pointLight4.lightProperties.fallOff = 0;


//			r2::draw::lightsys::AddPointLight(*mLightSystem, pointLight4);

        }

      //  r2::draw::renderer::UpdateSceneLighting(*mLightSystem);


        return assetCache != nullptr;
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
                if (mSelectedAnimModel)
                {
                    s64 numAnimations = r2::sarr::Size(*mAnimationsHandles) / 3;
					if (mSelectedAnimationID - 1 < 0)
					{
						mSelectedAnimationID = (s32)numAnimations - 1;
					}
					else
					{
						--mSelectedAnimationID;
					}
                }
                
				return true;
			}
			else if (e.KeyCode() == r2::io::KEY_RIGHT)
			{
                if (mSelectedAnimModel)
                {
                    u64 numAnimations = r2::sarr::Size(*mAnimationsHandles) / 3;
                    mSelectedAnimationID = size_t(mSelectedAnimationID + 1) % numAnimations;
                }
                
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
                r2::draw::renderer::UnloadAllAnimModels();
                mMicroBatModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
                mSkeletonModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
                mEllenModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
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
                if (!r2::draw::renderer::IsModelLoaded(mMicroBatModelRefHandle))
                {
                    mMicroBatModelRefHandle = r2::draw::renderer::UploadAnimModel(mMicroBatModel);
                }
            }
            else if (e.KeyCode() == r2::io::KEY_k)
            {
                if (!r2::draw::renderer::IsModelLoaded(mSkeletonModelRefHandle))
                {
                    mSkeletonModelRefHandle = r2::draw::renderer::UploadAnimModel(mSkeletonModel);
                }
            }
            else if (e.KeyCode() == r2::io::KEY_l)
            {
                if (!r2::draw::renderer::IsModelLoaded(mEllenModelRefHandle))
                {
                    mEllenModelRefHandle = r2::draw::renderer::UploadAnimModel(mEllenModel);
                }
            }


			return false;
		});

		mPersController.OnEvent(e);
    }
    
    virtual void Update() override
    {
        if (reload)
        {
            reload = false;
            
            r2::sarr::Clear(*assetsBuffers);
            
            r2::asset::Asset levelsAsset("breakout_level_pack.breakout_level", r2::asset::DEFAULT);
            
            auto levelAssetHandle = assetCache->LoadAsset(levelsAsset);
            
            auto assetBuffer = assetCache->GetAssetBuffer(levelAssetHandle);
            
            r2::sarr::Push(*assetsBuffers, assetBuffer);
            
            R2_CHECK(assetBuffer.buffer != nullptr, "Asset buffer is nullptr");
            
            const byte* data = assetBuffer.buffer->Data();
            
            const auto levelPack = Breakout::GetLevelPack(data);
            
            const auto levels = levelPack->levels();
            
            printf("======================Levels==========================\n");
            
            for (u32 i = 0; i < levels->size(); ++i)
            {
                printf("Level: %s\n", levels->Get(i)->name()->c_str());
                
                printf("Hash: %llu\n", levels->Get(i)->hashName());
                
                const auto blocks = levels->Get(i)->blocks();
                
                for (u32 j = 0; j < blocks->size(); ++j)
                {
                    auto fillColor = blocks->Get(j)->fillColor();
                    printf("-------------------Block--------------------\n");
                    
                    printf("Color: r: %f g: %f b: %f a: %f\n", fillColor->r(), fillColor->g(), fillColor->b(), fillColor->a());
                    
                    printf("HP: %i\n", blocks->Get(j)->hp());
                    
                    printf("Symbol: %c\n", blocks->Get(j)->symbol());
                    
                    printf("--------------------------------------------\n");
                }
                
                printf("----------------------Layout-----------------\n");
                
                printf("Height: %u\n", levels->Get(i)->layout()->height());
                
                printf("Width: %u\n\n", levels->Get(i)->layout()->width());
                
                printf("%s\n", levels->Get(i)->layout()->layout()->c_str());
                
                printf("---------------------------------------------\n");
            }
            printf("======================================================\n");
            
            
            
            printf("======================Powerups==========================\n");
            
            r2::asset::Asset powerupsAsset("breakout_powerups.powerup", r2::asset::DEFAULT);
            
            auto powerupsAssetHandle = assetCache->LoadAsset(powerupsAsset);
            auto powerupAssetBuffer = assetCache->GetAssetBuffer(powerupsAssetHandle);
            
            r2::sarr::Push(*assetsBuffers, powerupAssetBuffer);
            
            R2_CHECK(powerupAssetBuffer.buffer != nullptr, "Asset buffer is nullptr");
            
            const byte* powerupsData = powerupAssetBuffer.buffer->Data();
            
            const auto powerupsfbb = Breakout::GetPowerups(powerupsData);
            
            const auto powerups = powerupsfbb->powerups();
            
            for (u32 i = 0; i < powerups->size(); ++i)
            {
                auto powerup = powerups->Get(i);
                printf("----------------------Powerup-----------------\n");
                
                printf("type: %i\n", powerup->type());
                
                printf("size multiplier: %f\n", powerup->sizeMultiplier());
                
                printf("increase amount: %i\n", powerup->increaseAmount());
                
                printf("----------------------------------------------\n");
            }
            
            printf("========================================================\n");
            
            r2::asset::Asset highScoresAsset("breakout_high_scores.scores", r2::asset::DEFAULT);
            
            auto highScoreAssetHandle = assetCache->LoadAsset(highScoresAsset);
            auto highScoreAssetBuffer = assetCache->GetAssetBuffer(highScoreAssetHandle);
            
            r2::sarr::Push(*assetsBuffers, highScoreAssetBuffer);
            
            R2_CHECK(highScoreAssetBuffer.buffer != nullptr, "Asset buffer is nullptr");
            
            const auto scores = Breakout::GetHighScores(highScoreAssetBuffer.buffer->Data())->scores();
            
            printf("======================Scores==========================\n");
            
            for (u32 i = 0; i < scores->size(); ++i)
            {
                auto score = scores->Get(i);
                
                printf("Player: %s\t\tScore: %u\n", score->name()->c_str(), score->points());
            }
            
            printf("======================================================\n");
            
            r2::asset::Asset playerSettingsAsset("breakout_player_save.player", r2::asset::DEFAULT);
            
            auto playerSettingsAssetHandle = assetCache->LoadAsset(playerSettingsAsset);
            auto settingsAssetBuffer = assetCache->GetAssetBuffer(playerSettingsAssetHandle);
            
            r2::sarr::Push(*assetsBuffers, settingsAssetBuffer);
            
            R2_CHECK(settingsAssetBuffer.buffer != nullptr, "Asset buffer is nullptr");
            
            const auto settings = Breakout::GetPlayerSettings(settingsAssetBuffer.buffer->Data());
            
            printf("======================Settings==========================\n");
            
            printf("Player Starting Level: %u\n", settings->startingLevel());
            
            printf("Player Starting Lives: %u\n", settings->lives());
            
            printf("Player points: %u\n", settings->points());
            
            printf("========================================================\n");
            
            auto oneMoreLevelRef = assetCache->GetAssetBuffer(levelAssetHandle);
            
            auto oneMoreScoreRef = assetCache->GetAssetBuffer(highScoreAssetHandle);
            
            r2::sarr::Push(*assetsBuffers, oneMoreLevelRef);
            
            r2::sarr::Push(*assetsBuffers, oneMoreScoreRef);
        }
        
        mPersController.Update();

		//r2::sarr::Clear(*mBoneTransforms);
        //r2::sarr::Clear(*mDebugBones);

        r2::sarr::Clear(*mBatBoneTransforms);
        r2::sarr::Clear(*mBat2BoneTransforms);
        r2::sarr::Clear(*mSkeletonBoneTransforms);
        r2::sarr::Clear(*mEllenBoneTransforms);

        r2::sarr::Clear(*mBatDebugBones);
        r2::sarr::Clear(*mBat2DebugBones);
        r2::sarr::Clear(*mSkeletonDebugBones);
        r2::sarr::Clear(*mEllenDebugBones);

        auto time = CENG.GetTicks();
  //      auto curTime = time;

        const r2::draw::Animation* microbatAnimation = r2::draw::animcache::GetAnimation(*mAnimationCache, r2::sarr::At(*mAnimationsHandles, mSelectedAnimationID));
        const r2::draw::Animation* microbatAnimation3 = r2::draw::animcache::GetAnimation(*mAnimationCache, r2::sarr::At(*mAnimationsHandles, mSelectedAnimationID + 2));

        const r2::draw::Animation* skeletonAnimation = r2::draw::animcache::GetAnimation(*mAnimationCache, r2::sarr::At(*mAnimationsHandles, mSelectedAnimationID + 3));
        const r2::draw::Animation* ellenAnimation = r2::draw::animcache::GetAnimation(*mAnimationCache, r2::sarr::At(*mAnimationsHandles, mSelectedAnimationID + 6));

		r2::draw::PlayAnimationForAnimModel(time, 0, true, *mMicroBatModel, microbatAnimation, *mBatBoneTransforms, *mBatDebugBones, 0);

		r2::draw::PlayAnimationForAnimModel(time, 0, true, *mMicroBatModel, microbatAnimation3, *mBat2BoneTransforms, *mBat2DebugBones, 0);

		r2::draw::PlayAnimationForAnimModel(time, 0, false, *mSkeletonModel, skeletonAnimation, *mSkeletonBoneTransforms, *mSkeletonDebugBones, 0);

		r2::draw::PlayAnimationForAnimModel(time, 0, true, *mEllenModel, ellenAnimation, *mEllenBoneTransforms, *mEllenDebugBones, 0);
        

	//	r2::draw::renderer::UpdateViewMatrix(mPersController.GetCameraPtr()->view);


	//	r2::draw::renderer::UpdateCameraPosition(mPersController.GetCameraPtr()->position);


        

        //r2::draw::renderer::UpdateCamera(mPersController.GetCamera());

		//r2::draw::renderer::UpdateExposure(mExposure);
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

        if (r2::draw::renderer::IsModelLoaded(mSponzaModelRefHandle))
        {
			r2::SArray<glm::mat4>* sponzaModelMatrices = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, 1);
			r2::sarr::Push(*sponzaModelMatrices, r2::sarr::At(*modelMats, 0));

			r2::draw::renderer::DrawModel(drawWorldParams, mSponzaModelRefHandle, *sponzaModelMatrices, 1, nullptr, nullptr);
			FREE(sponzaModelMatrices, *MEM_ENG_SCRATCH_PTR);
        }

        r2::draw::DrawParameters drawTransparentWindowParams;
        drawTransparentWindowParams.layer = r2::draw::DL_TRANSPARENT;
        drawTransparentWindowParams.flags.Clear();
        drawTransparentWindowParams.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);
        
        r2::draw::renderer::SetDefaultCullState(drawTransparentWindowParams);
        //drawTransparentWindowParams.cullState.frontFace = r2::draw::FRONT_AND_BACK;

        r2::draw::renderer::SetDefaultStencilState(drawTransparentWindowParams);
        r2::draw::renderer::SetDefaultBlendState(drawTransparentWindowParams);

        //drawTransparentWindowParams.blendState.blendingEnabled = true;
        //drawTransparentWindowParams.blendState.numBlendFunctions = 1;
        //drawTransparentWindowParams.blendState.blendFunctions[0].blendDrawBuffer = 0;
        //drawTransparentWindowParams.blendState.blendFunctions[0].sfactor = r2::draw::ONE;
        //drawTransparentWindowParams.blendState.blendFunctions[0].dfactor = r2::draw::ONE_MINUS_SRC_ALPHA;

        //draw transparent windows
        r2::draw::renderer::DrawModel(drawTransparentWindowParams, mTransparentWindowModelRefHandle, *mTransparentWindowMats, r2::sarr::Size(*mTransparentWindowMats), mTransparentWindowMaterialHandles, nullptr);


		r2::draw::DrawParameters animDrawParams;
		animDrawParams.layer = r2::draw::DL_CHARACTER;
		animDrawParams.flags.Clear();
		animDrawParams.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);
		
        r2::draw::renderer::SetDefaultCullState(animDrawParams);
		r2::draw::renderer::SetDefaultStencilState(animDrawParams);
		r2::draw::renderer::SetDefaultBlendState(animDrawParams);

        //Draw the bat
        if (r2::draw::renderer::IsModelLoaded(mMicroBatModelRefHandle))
        {
			r2::SArray<glm::mat4>* microBatModelMats = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, 2);
			r2::SArray<r2::draw::ShaderBoneTransform>* allMicroBatBoneTransforms = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::ShaderBoneTransform, NUM_BONES);

			r2::sarr::Push(*microBatModelMats, r2::sarr::At(*animModelMats, 0));
			r2::sarr::Push(*microBatModelMats, r2::sarr::At(*animModelMats, 1));

			r2::sarr::Append(*allMicroBatBoneTransforms, *mBatBoneTransforms);
			r2::sarr::Append(*allMicroBatBoneTransforms, *mBat2BoneTransforms);
			
			r2::draw::renderer::DrawModel(animDrawParams, mMicroBatModelRefHandle, *microBatModelMats, 2, nullptr, allMicroBatBoneTransforms);

			FREE(allMicroBatBoneTransforms, *MEM_ENG_SCRATCH_PTR);
			FREE(microBatModelMats, *MEM_ENG_SCRATCH_PTR);
        }

        animDrawParams.flags.Set(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);

        //Draw the Skeleton
        if (r2::draw::renderer::IsModelLoaded(mSkeletonModelRefHandle))
        {
			r2::SArray<glm::mat4>* skeletonModelMats = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, 2);
			r2::sarr::Push(*skeletonModelMats, r2::sarr::At(*animModelMats, 2));
			r2::sarr::Push(*skeletonModelMats, r2::sarr::At(*animModelMats, 3));

			r2::draw::renderer::DrawModel(animDrawParams, mSkeletonModelRefHandle, *skeletonModelMats, 2, nullptr, mSkeletonBoneTransforms);

			FREE(skeletonModelMats, *MEM_ENG_SCRATCH_PTR);
        }

        //Draw Ellen

        if (r2::draw::renderer::IsModelLoaded(mEllenModelRefHandle))
        {
			r2::SArray<glm::mat4>* ellenModelMats = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, 2);
			r2::sarr::Push(*ellenModelMats, r2::sarr::At(*animModelMats, 4));
			r2::sarr::Push(*ellenModelMats, r2::sarr::At(*animModelMats, 5));

			animDrawParams.stencilState.op.stencilFail = r2::draw::KEEP;
			animDrawParams.stencilState.op.depthFail = r2::draw::KEEP;
			animDrawParams.stencilState.op.depthAndStencilPass = r2::draw::REPLACE;
			animDrawParams.stencilState.stencilWriteEnabled = true;
			animDrawParams.stencilState.stencilEnabled = true;
			animDrawParams.stencilState.func.func = r2::draw::ALWAYS;
			animDrawParams.stencilState.func.ref = 1;
			animDrawParams.stencilState.func.mask = 0xFF;

			r2::draw::renderer::DrawModel(animDrawParams, mEllenModelRefHandle, *ellenModelMats, 2, nullptr, mEllenBoneTransforms);

			animDrawParams.flags.Remove(r2::draw::eDrawFlags::DEPTH_TEST);
			animDrawParams.layer = r2::draw::DL_EFFECT;
			animDrawParams.stencilState.stencilEnabled = true;
			animDrawParams.stencilState.stencilWriteEnabled = false;
			animDrawParams.stencilState.func.func = r2::draw::NOTEQUAL;
			animDrawParams.stencilState.func.ref = 1;
			animDrawParams.stencilState.func.mask = 0xFF;

			r2::SArray<r2::draw::MaterialHandle>* outlineMaterialHandles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::MaterialHandle, 1);

			r2::sarr::Push(*outlineMaterialHandles, r2::draw::renderer::GetDefaultOutlineMaterialHandle(false));

			r2::draw::renderer::DrawModel(animDrawParams, mEllenModelRefHandle, *ellenModelMats, 2, outlineMaterialHandles, mEllenBoneTransforms);

			FREE(outlineMaterialHandles, *MEM_ENG_SCRATCH_PTR);

			FREE(ellenModelMats, *MEM_ENG_SCRATCH_PTR);
        }
        

        //Draw the Skybox
		r2::SArray<glm::mat4>* skyboxModelMatrices = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, 1);
		r2::sarr::Push(*skyboxModelMatrices, glm::mat4(1.0f));

        drawWorldParams.layer = r2::draw::DL_SKYBOX;

        r2::draw::renderer::DrawModel(drawWorldParams, mSkyboxModelRef, *skyboxModelMatrices, 1, mSkyboxMaterialHandles, nullptr);
        FREE(skyboxModelMatrices, *MEM_ENG_SCRATCH_PTR);

        //Draw the axis

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

     //   r2::draw::renderer::DrawArrow(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), 1, 0.1, glm::vec4(1, 0, 0, 1), true);
     //   r2::draw::renderer::DrawArrow(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 1, 0.1, glm::vec4(0, 1, 0, 1), true);
     //   r2::draw::renderer::DrawArrow(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), 1, 0.1, glm::vec4(0, 0, 1, 1), true);


		//r2::draw::renderer::DrawSphere(glm::vec3(0, 5, -5), 0.5, glm::vec4(0.8, 0.6, 0.1, 1), true);
		//r2::draw::renderer::DrawSphere(glm::vec3(0, 5, 5), 0.5, glm::vec4(1, 0, 0, 1), true);
		//r2::draw::renderer::DrawSphere(glm::vec3(5, 5, 0), 0.5, glm::vec4(1, 0, 1, 1), true);
		//r2::draw::renderer::DrawSphere(glm::vec3(-5, 5, 0), 0.5, glm::vec4(1, 1, 0, 1), true);
		//r2::draw::renderer::DrawSphere(glm::vec3(5, 5, -5), 0.5, glm::vec4(0.6, 0.3, 0.7, 1), true);

  //      r2::draw::renderer::DrawArrow(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), 1, 0.1, glm::vec4(1, 0, 0, 1), false);

  //     // r2::draw::renderer::DrawCone(glm::vec3(5, 5, -5), glm::vec3(-5, -5, 5), 0.5, 1, glm::vec4(0.6, 0.3, 0.7, 1), true);

  //      r2::draw::renderer::DrawCylinder(glm::vec3(-5, 5, -5), glm::vec3(0, 1, 0), 0.5, 1, glm::vec4(0.5, 0.5, 1, 1), true);
		////r2::draw::renderer::DrawSphere(glm::vec3(-5, 5, -5), 0.5, glm::vec4(0.5, 0.5, 1, 1), true);
  //      r2::draw::renderer::DrawCube(glm::vec3(-5, 5, 5), 0.5, glm::vec4(0, 1, 1, 1), true);
		////r2::draw::renderer::DrawSphere(glm::vec3(-5, 5, 5), 0.5, glm::vec4(0, 1, 1, 1), true);
		//r2::draw::renderer::DrawSphere(glm::vec3(5, 5, 5), 0.5, glm::vec4(1, 1, 1, 1), false);

  //      r2::draw::renderer::DrawLine(glm::vec3(0), glm::vec3(0, 5, -5), glm::vec4(1, 1, 1, 1), true);
  //      r2::draw::renderer::DrawLine(glm::vec3(0), glm::vec3(0, 5,  5), glm::vec4(1, 1, 0, 1), true);
  //      r2::draw::renderer::DrawLine(glm::vec3(0), glm::vec3(5, 5,  0), glm::vec4(0, 1, 1, 1), true);
  //      r2::draw::renderer::DrawLine(glm::vec3(0), glm::vec3(-5, 5, 0), glm::vec4(1, 0, 1, 1), true);
  //      r2::draw::renderer::DrawLine(glm::vec3(0), glm::vec3(5, 5, -5), glm::vec4(0, 1, 0, 1), true);
  //      r2::draw::renderer::DrawLine(glm::vec3(0), glm::vec3(-5, 5, -5), glm::vec4(1, 1, 1, 1), true);
		//r2::draw::renderer::DrawLine(glm::vec3(0), glm::vec3(-5, 5, 5), glm::vec4(0, 0, 0, 1), true);
		//r2::draw::renderer::DrawLine(glm::vec3(0), glm::vec3(5, 5, 5), glm::vec4(0.5, 1, 0, 1), true);

       // glm::mat4 mat = r2::draw::renderer::DrawCylinder(glm::vec3(0, 5, -5), glm::vec3(0, 1, 0), 1, 5, glm::vec4(1, 1, 1, 1), false);
         
  //      r2::draw::renderer::DrawTangentVectors(r2::draw::CYLINDER, r2::sarr::At(*modelMats,r2::draw::CYLINDER));
  //      r2::draw::renderer::DrawTangentVectors(r2::draw::QUAD, r2::sarr::At(*modelMats, r2::draw::QUAD));
        if (mDrawDebugBones)
        {

            r2::draw::renderer::DrawDebugBones(*mBatDebugBones, r2::sarr::At(*animModelMats, 0), glm::vec4(1, 1, 0, 1));
            r2::draw::renderer::DrawDebugBones(*mSkeletonDebugBones, r2::sarr::At(*animModelMats, 2), glm::vec4(1, 1, 0, 1));
            r2::draw::renderer::DrawDebugBones(*mEllenDebugBones, r2::sarr::At(*animModelMats, 4), glm::vec4(1, 1, 0, 1));
          
        }

        
        
    }
    
    virtual void Shutdown() override
    {

        r2::draw::animcache::Shutdown(*mAnimationCache);
        r2::draw::modlsys::Shutdown(mModelSystem);
        
        void* materialBoundary = mMaterialSystem->mMaterialMemBoundary.location;
        r2::draw::matsys::FreeMaterialSystem(mMaterialSystem);
        FREE(materialBoundary, *linearArenaPtr);

        FREE(modelMats, *linearArenaPtr);


        FREE(mAnimationsHandles, *linearArenaPtr);
        FREE(animModelMats, *linearArenaPtr);

        FREE(mEllenBoneTransforms, *linearArenaPtr);
        FREE(mEllenDebugBones, *linearArenaPtr);

		FREE(mSkeletonBoneTransforms, *linearArenaPtr);
		FREE(mSkeletonDebugBones, *linearArenaPtr);

        FREE(mBat2BoneTransforms, *linearArenaPtr);
        FREE(mBat2DebugBones, *linearArenaPtr);

		FREE(mBatBoneTransforms, *linearArenaPtr);
		FREE(mBatDebugBones, *linearArenaPtr);

        
        
        FREE(mSkyboxMaterialHandles, *linearArenaPtr);

        FREE(mStaticModelMaterialHandles, *linearArenaPtr);
        FREE(mStaticModelRefs, *linearArenaPtr);
      //  FREE(mAnimModelRefs, *linearArenaPtr);

        
        FREE(mStaticModelDrawFlags, *linearArenaPtr);


        FREE(mTransparentWindowMats, *linearArenaPtr);
        FREE(mTransparentWindowMaterialHandles, *linearArenaPtr);
        FREE(mTransparentWindowDrawFlags, *linearArenaPtr);

        FREE(mStaticCubeModelMats, *linearArenaPtr);
        FREE(mStaticCubeMaterials, *linearArenaPtr);
        FREE(mStaticCubesDrawFlags, *linearArenaPtr);

        


        u64 size = r2::sarr::Size(*assetsBuffers);
        
        for (u64 i = 0; i < size; ++i)
        {
            auto record = r2::sarr::At(*assetsBuffers, i);
            
            assetCache->ReturnAssetBuffer(record);
        }
        
        FREE(assetCacheBoundary.location, *linearArenaPtr);
        FREE(assetsBuffers, *linearArenaPtr);
        r2::asset::lib::DestroyCache(assetCache);

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
           // SANDBOX_MATERIALS_MANIFESTS_BIN + std::string("/SandboxMaterialPack.mpak"),
            SANDBOX_MATERIALS_MANIFESTS_BIN + std::string("/SandboxMaterialParamsPack.mppk")
		};
		return manifestBinPaths;
    }

    virtual std::vector<std::string> GetMaterialPackManifestsRawPaths() const override
    {
		std::vector<std::string> manifestRawPaths
		{
			//SANDBOX_MATERIALS_MANIFESTS + std::string("/SandboxMaterialPack.json"),
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

	//@SO TEMPORARY!!!
#ifdef R2_EDITOR
    virtual std::vector<const r2::draw::Animation*> GetEditorAnimation() const override
    {
        std::vector<const r2::draw::Animation*> animations;
        animations.push_back(r2::draw::animcache::GetAnimation(*mAnimationCache, r2::sarr::At(*mAnimationsHandles, 0)));
        animations.push_back(r2::draw::animcache::GetAnimation(*mAnimationCache, r2::sarr::At(*mAnimationsHandles, 1)));
        animations.push_back(r2::draw::animcache::GetAnimation(*mAnimationCache, r2::sarr::At(*mAnimationsHandles, 2)));

        return animations;
    }


	r2::draw::ModelSystem* GetEditorModelSystem() const
	{
		return mModelSystem;
	}

	r2::draw::MaterialSystem* GetEditorMaterialSystem() const
	{
		return mMaterialSystem;
	}

    virtual void RegisterComponents(r2::ecs::ECSCoordinator* coordinator) const
    {
        /*coordinator->RegisterComponent<r2::mem::LinearArena, DummyComponent>(*linearArenaPtr, "DummyComponent", true);*/
    }
	
    virtual void UnRegisterComponents(r2::ecs::ECSCoordinator* coordinator) const
    {
        /*coordinator->UnRegisterComponent<r2::mem::LinearArena, DummyComponent>(*linearArenaPtr);*/
    }

    virtual void AddComponentsToEntity(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity e) const
    {
        //DummyComponent d;
        //d.dummy = 42;

        //coordinator->AddComponent<DummyComponent>(e, d);
    }

#endif
    
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
        char soundFXPath[r2::fs::FILE_PATH_LENGTH];
        char musicPath [r2::fs::FILE_PATH_LENGTH];
        ResolveCategoryPath(r2::fs::utils::SOUND_FX, soundFXPath);
        ResolveCategoryPath(r2::fs::utils::MUSIC, musicPath);
        
        std::vector<std::string> soundPaths = {
            std::string(soundFXPath),
            std::string(musicPath)
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

    std::string GetLevelPackDataBinPath() const
    {
        return SANDBOX_LEVELS_DIR_BIN;
    }

    std::string GetLevelPackDataJSONPath() const
    {
        return SANDBOX_LEVELS_DIR_RAW;
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
    
private:
    r2::mem::MemoryArea::Handle memoryAreaHandle;
    r2::mem::MemoryArea::SubArea::Handle subMemoryAreaHandle;
    r2::cam::PerspectiveController mPersController;
    r2::asset::AssetCache* assetCache;
    bool reload;
    r2::mem::utils::MemBoundary assetCacheBoundary;
    r2::SArray<r2::asset::AssetCacheRecord>* assetsBuffers;
    r2::mem::LinearArena* linearArenaPtr;


    //r2::SArray<r2::draw::vb::GPUModelRefHandle>* mAnimModelRefs;

    r2::draw::vb::GPUModelRefHandle mMicroBatModelRefHandle;
    r2::draw::vb::GPUModelRefHandle mSkeletonModelRefHandle;
    r2::draw::vb::GPUModelRefHandle mEllenModelRefHandle;

    r2::SArray<r2::draw::vb::GPUModelRefHandle>* mStaticModelRefs;
    r2::SArray<r2::draw::MaterialHandle>* mStaticModelMaterialHandles;
    r2::draw::vb::GPUModelRefHandle mSkyboxModelRef;
    r2::SArray<r2::draw::MaterialHandle>* mSkyboxMaterialHandles;

    r2::SArray<r2::draw::DrawFlags>* mStaticModelDrawFlags;
    r2::SArray<glm::mat4>* modelMats;
    r2::SArray<glm::mat4>* animModelMats;

    r2::draw::vb::GPUModelRefHandle mTransparentWindowModelRefHandle;
    r2::SArray<glm::mat4>* mTransparentWindowMats;
    r2::SArray<r2::draw::MaterialHandle>* mTransparentWindowMaterialHandles;
    r2::SArray<r2::draw::DrawFlags>* mTransparentWindowDrawFlags;

    r2::SArray<r2::draw::ShaderBoneTransform>* mBatBoneTransforms;
    r2::SArray<r2::draw::ShaderBoneTransform>* mBat2BoneTransforms;
    r2::SArray<r2::draw::DebugBone>* mBatDebugBones;
    r2::SArray<r2::draw::DebugBone>* mBat2DebugBones;

	r2::SArray<r2::draw::ShaderBoneTransform>* mSkeletonBoneTransforms;
	r2::SArray<r2::draw::DebugBone>* mSkeletonDebugBones;

	r2::SArray<r2::draw::ShaderBoneTransform>* mEllenBoneTransforms;
	r2::SArray<r2::draw::DebugBone>* mEllenDebugBones;

    r2::SArray<r2::draw::AnimationHandle>* mAnimationsHandles;

    r2::draw::vb::GPUModelRefHandle mStaticCubeModelRefHandle;
    r2::SArray<glm::mat4>* mStaticCubeModelMats;
    r2::SArray<r2::draw::MaterialHandle>* mStaticCubeMaterials;
    r2::SArray<r2::draw::DrawFlags>* mStaticCubesDrawFlags;

    r2::draw::ModelSystem* mModelSystem = nullptr;
    r2::draw::AnimationCache* mAnimationCache = nullptr;
    r2::draw::MaterialSystem* mMaterialSystem = nullptr;
    const r2::draw::AnimModel* mMicroBatModel = nullptr;
    const r2::draw::AnimModel* mSkeletonModel = nullptr;
    const r2::draw::AnimModel* mEllenModel = nullptr;
    const r2::draw::AnimModel* mSelectedAnimModel = nullptr;
    const r2::draw::Model* mSponzaModel = nullptr;
    r2::draw::vb::GPUModelRefHandle mSponzaModelRefHandle;

    s32 mSelectedAnimationID = 0;
    bool mDrawDebugBones = false;

    float mExposure = 1.0f;
    s32 mResolution = 3;
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
                r2::util::PathCpy(subpath, "assets/sound/sound_definitions");
                break;
            case r2::fs::utils::SOUND_FX:
                r2::util::PathCpy(subpath, "assets/sound/sound_fx");
                break;
            case r2::fs::utils::MUSIC:
                r2::util::PathCpy(subpath, "assets/sound/music");
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
