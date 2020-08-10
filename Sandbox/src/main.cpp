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
#include "r2/Core/Assets/RawAssetFile.h"
#include "r2/Core/Assets/ZipAssetFile.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/Commands.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Model/Model.h"
#include "r2/Utils/Hash.h"
#include "r2/Render/Camera/PerspectiveCameraController.h"
#include "glm/gtc/type_ptr.hpp"
#include "r2/Core/Memory/InternalEngineMemory.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetManifest.h"
#endif

namespace
{
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
    
    enum ConstantHandles
    {
        VP_MATRICES = 0,
        MODEL_MATRICES,
        SUB_COMMANDS,
        MODEL_MATERIALS,
    };
    
    const u64 NUM_DRAWS = 7;
    
    virtual bool Init() override
    {
        memoryAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("SandboxArea");

        R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "Invalid memory area");
        
        r2::mem::MemoryArea* sandBoxMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
        R2_CHECK(sandBoxMemoryArea != nullptr, "Feailed to get the memory area!");
        
        auto result = sandBoxMemoryArea->Init(Megabytes(1), 0);
        R2_CHECK(result == true, "Failed to initialize memory area");
        
        subMemoryAreaHandle = sandBoxMemoryArea->AddSubArea(Megabytes(1));
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
        //make the buffer layouts
        layouts = MAKE_SARRAY(*linearArenaPtr, r2::draw::BufferLayoutConfiguration, 10);
        constantLayouts = MAKE_SARRAY(*linearArenaPtr, r2::draw::ConstantBufferLayoutConfiguration, 10);
        mPersController.Init(2.5f, 45.0f, static_cast<float>(CENG.DisplaySize().width) / static_cast<float>(CENG.DisplaySize().height), 0.1f, 100.f, glm::vec3(0.0f, 0.0f, 3.0f));
        modelMats = MAKE_SARRAY(*linearArenaPtr, glm::mat4, NUM_DRAWS);
        
        
        glm::mat4 quadMat = glm::mat4(1.0f);
        
        quadMat = glm::rotate(quadMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        quadMat = glm::scale(quadMat, glm::vec3(10.0f));
        r2::sarr::Push(*modelMats, quadMat);

        glm::mat4 sphereMat = glm::mat4(1.0f);
        sphereMat = glm::translate(sphereMat, glm::vec3(4, 1.1, 0));
        r2::sarr::Push(*modelMats, sphereMat);

        glm::mat4 cubeMat = glm::mat4(1.0f);
        cubeMat = glm::translate(cubeMat, glm::vec3(1.5, 0.6, 0));
        r2::sarr::Push(*modelMats, cubeMat);

        glm::mat4 cylinderMat = glm::mat4(1.0f);
        
        cylinderMat = glm::translate(cylinderMat, glm::vec3(-4, 0.6, 0));
        cylinderMat = glm::rotate(cylinderMat, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        r2::sarr::Push(*modelMats, cylinderMat);

        glm::mat4 coneMat = glm::mat4(1.0f);
        coneMat = glm::translate(coneMat, glm::vec3(-1, 0.6, 0));
        coneMat = glm::rotate(coneMat, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
		r2::sarr::Push(*modelMats, coneMat);


        subCommandsToDraw = MAKE_SARRAY(*linearArenaPtr, r2::draw::cmd::DrawBatchSubCommand, NUM_DRAWS);
        modelMaterials = MAKE_SARRAY(*linearArenaPtr, r2::draw::MaterialHandle, NUM_DRAWS);

        r2::draw::BufferLayoutConfiguration layoutConfig{
            {
                {{r2::draw::ShaderDataType::Float3, "aPos"},
                 {r2::draw::ShaderDataType::Float3, "aNormal"},
                 {r2::draw::ShaderDataType::Float2, "aTexCoord"}}
            },
            {
                Megabytes(1),
                r2::draw::VertexDrawTypeStatic
            },
            {
                Megabytes(1),
                r2::draw::VertexDrawTypeStatic
            },
            true, NUM_DRAWS //use draw ids
        };

        //For Matrices
        r2::draw::ConstantBufferLayoutConfiguration constantLayout{
            {
                r2::draw::ConstantBufferLayout::Small, 0,0,//create flags
                {
                    {r2::draw::ShaderDataType::Mat4, "projection"},
                    {r2::draw::ShaderDataType::Mat4, "view"}
                }
            },
            r2::draw::VertexDrawTypeDynamic
        };

        //for Models
        r2::draw::ConstantBufferLayoutConfiguration constantLayout2
        {
            //layout
            {
                r2::draw::ConstantBufferLayout::Big,
                r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT,
                r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE,
                {
                    {r2::draw::ShaderDataType::Mat4, "models", NUM_DRAWS}
                }
            },
            //drawType
            r2::draw::VertexDrawTypeDynamic
        };



        //for Sub Commands
        r2::draw::ConstantBufferLayoutConfiguration subCommands
        {
            //layout
            {},
            r2::draw::VertexDrawTypeDynamic
        };

        subCommands.layout.InitForSubCommands(r2::draw::CB_FLAG_WRITE | r2::draw::CB_FLAG_MAP_PERSISTENT, r2::draw::CB_CREATE_FLAG_DYNAMIC_STORAGE, NUM_DRAWS);


		r2::draw::ConstantBufferLayoutConfiguration materials
		{
			//layout
			{

			},
			//drawType
			r2::draw::VertexDrawTypeDynamic
		};

		materials.layout.InitForMaterials(0, 0, NUM_DRAWS);

        r2::sarr::Push(*layouts, layoutConfig);
        r2::sarr::Push(*constantLayouts, constantLayout);
        r2::sarr::Push(*constantLayouts, constantLayout2);
        r2::sarr::Push(*constantLayouts, subCommands);
        r2::sarr::Push(*constantLayouts, materials);


        r2::draw::renderer::SetDepthTest(true);
        bool success = r2::draw::renderer::GenerateBufferLayouts(layouts);
        R2_CHECK(success, "We couldn't create the buffer layouts!");

        success = r2::draw::renderer::GenerateConstantBuffers(constantLayouts);
        R2_CHECK(success, "We couldn't create the constant buffers");

        r2::draw::BufferHandles& handles = r2::draw::renderer::GetBufferHandles();
        const r2::SArray<r2::draw::ConstantBufferHandle>* constantBufferHandles = r2::draw::renderer::GetConstantBufferHandles();

        //fill the buffers with data
        r2::draw::Model* quadModel = r2::draw::renderer::GetDefaultModel(r2::draw::QUAD);
        r2::draw::Model* sphereModel = r2::draw::renderer::GetDefaultModel(r2::draw::SPHERE);
        r2::draw::Model* cubeModel = r2::draw::renderer::GetDefaultModel(r2::draw::CUBE);
        r2::draw::Model* cylinderModel = r2::draw::renderer::GetDefaultModel(r2::draw::CYLINDER);
        r2::draw::Model* coneModel = r2::draw::renderer::GetDefaultModel(r2::draw::CONE);

        u64 vertexOffset = r2::draw::renderer::AddFillVertexCommandsForModel(quadModel, r2::sarr::At(*handles.vertexBufferHandles, 0));
        u64 indexOffset = r2::draw::renderer::AddFillIndexCommandsForModel(quadModel, r2::sarr::At(*handles.indexBufferHandles, 0));
        
        vertexOffset = r2::draw::renderer::AddFillVertexCommandsForModel(sphereModel, r2::sarr::At(*handles.vertexBufferHandles, 0), vertexOffset);
        indexOffset = r2::draw::renderer::AddFillIndexCommandsForModel(sphereModel, r2::sarr::At(*handles.indexBufferHandles, 0), indexOffset);

		vertexOffset = r2::draw::renderer::AddFillVertexCommandsForModel(cubeModel, r2::sarr::At(*handles.vertexBufferHandles, 0), vertexOffset);
		indexOffset = r2::draw::renderer::AddFillIndexCommandsForModel(cubeModel, r2::sarr::At(*handles.indexBufferHandles, 0), indexOffset);

		vertexOffset = r2::draw::renderer::AddFillVertexCommandsForModel(cylinderModel, r2::sarr::At(*handles.vertexBufferHandles, 0), vertexOffset);
		indexOffset = r2::draw::renderer::AddFillIndexCommandsForModel(cylinderModel, r2::sarr::At(*handles.indexBufferHandles, 0), indexOffset);

		vertexOffset = r2::draw::renderer::AddFillVertexCommandsForModel(coneModel, r2::sarr::At(*handles.vertexBufferHandles, 0), vertexOffset);
		indexOffset = r2::draw::renderer::AddFillIndexCommandsForModel(coneModel, r2::sarr::At(*handles.indexBufferHandles, 0), indexOffset);


        r2::draw::renderer::AddFillConstantBufferCommandForData(r2::sarr::At(*constantBufferHandles, 0), constantLayout.layout.GetType(), constantLayout.layout.GetFlags().IsSet(r2::draw::CB_FLAG_MAP_PERSISTENT), glm::value_ptr(mPersController.GetCameraPtr()->proj), constantLayout.layout.GetElements().at(0).size, constantLayout.layout.GetElements().at(0).offset);
        
        //@NOTE: these need to be in the order that we submit the fill commands
        r2::SArray<r2::draw::Model*>* modelsToDraw = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::Model*, NUM_DRAWS);
        r2::sarr::Push(*modelsToDraw, quadModel);
        r2::sarr::Push(*modelsToDraw, sphereModel);
        r2::sarr::Push(*modelsToDraw, cubeModel);
        r2::sarr::Push(*modelsToDraw, cylinderModel);
        r2::sarr::Push(*modelsToDraw, coneModel);

        r2::draw::renderer::FillSubCommandsFromModels(*subCommandsToDraw, *modelsToDraw);
        
        const u64 numModels = r2::sarr::Size(*modelsToDraw);
        for (u64 i = 0; i < numModels; ++i)
        {
            r2::draw::Model* model = r2::sarr::At(*modelsToDraw, i);
			const r2::draw::Mesh& mesh = r2::sarr::At(*model->optrMeshes, 0);
			r2::draw::MaterialHandle materialHandle = r2::sarr::At(*mesh.optrMaterials, 0);
            r2::sarr::Push(*modelMaterials, materialHandle);
        }


        FREE(modelsToDraw, *MEM_ENG_SCRATCH_PTR);

        r2::draw::renderer::SetClearColor(glm::vec4(0.5, 0.5, 0.5, 1.0));

        r2::draw::renderer::LoadEngineTexturesFromDisk();
        r2::draw::renderer::UploadMaterialTexturesToGPUFromMaterialName(STRING_ID("Basic"));


        return assetCache != nullptr;
    }

    virtual void OnEvent(r2::evt::Event& e) override
    {
		r2::evt::EventDispatcher dispatcher(e);

		dispatcher.Dispatch<r2::evt::WindowResizeEvent>([this](const r2::evt::WindowResizeEvent& e)
			{
				mPersController.SetAspect(static_cast<float>(e.Width()) / static_cast<float>(e.Height()));

				r2::draw::renderer::WindowResized(e.Width(), e.Height());
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
				//    r2::draw::OpenGLPrevAnimation();
				return true;
			}
			else if (e.KeyCode() == r2::io::KEY_RIGHT)
			{
				//    r2::draw::OpenGLNextAnimation();
				return true;
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
            
            r2::asset::Asset levelsAsset("breakout_level_pack.breakout_level");
            
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
            
            r2::asset::Asset powerupsAsset("breakout_powerups.powerup");
            
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
            
            r2::asset::Asset highScoresAsset("breakout_high_scores.scores");
            
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
            
            r2::asset::Asset playerSettingsAsset("breakout_player_save.player");
            
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
    }

    virtual void Render(float alpha) override
    {
        //add my commands here

        r2::draw::Model* quadModel = r2::draw::renderer::GetDefaultModel(r2::draw::QUAD);
        const r2::draw::Mesh& mesh = r2::sarr::At(*quadModel->optrMeshes, 0);
        r2::draw::MaterialHandle materialHandle = r2::sarr::At(*mesh.optrMaterials, 0);

        r2::draw::key::Basic drawElemKey = r2::draw::key::GenerateKey(0, 0, 0, 0, 0, materialHandle);

        r2::draw::BufferHandles& handles = r2::draw::renderer::GetBufferHandles();
       // 
        //r2::draw::cmd::DrawIndexed* drawIndexedCMD = r2::draw::renderer::AddDrawIndexedCommand(drawElemKey);
        //drawIndexedCMD->indexCount = static_cast<u32>(r2::sarr::Size(*r2::sarr::At(*quadModel->optrMeshes, 0).optrIndices));
        //drawIndexedCMD->startIndex = 0;
        //drawIndexedCMD->bufferLayoutHandle = r2::sarr::At(*handles.bufferLayoutHandles, 0);
        //drawIndexedCMD->vertexBufferHandle = r2::sarr::At(*handles.vertexBufferHandles, 0);
        //drawIndexedCMD->indexBufferHandle = r2::sarr::At(*handles.indexBufferHandles, 0);

        const r2::SArray<r2::draw::ConstantBufferHandle>* constHandles = r2::draw::renderer::GetConstantBufferHandles();

        //@TODO(Serge): draw batch here
		r2::draw::BatchConfig batch;
		batch.key = drawElemKey;
		batch.layoutHandle = r2::sarr::At(*handles.bufferLayoutHandles, 0);
		batch.subcommands = subCommandsToDraw;
		batch.models = modelMats;
        batch.materials = modelMaterials;
		batch.modelsHandle = r2::sarr::At(*constHandles, MODEL_MATRICES);
		batch.subCommandsHandle = r2::sarr::At(*constHandles, SUB_COMMANDS);
        batch.materialsHandle = r2::sarr::At(*constHandles, MODEL_MATERIALS);

		r2::draw::renderer::AddDrawBatch(batch);

       // r2::draw::key::Basic clearKey;

       // r2::draw::cmd::Clear* clearCMD = r2::draw::renderer::AddClearCommand(clearKey);
       // clearCMD->flags = r2::draw::cmd::CLEAR_COLOR_BUFFER | r2::draw::cmd::CLEAR_DEPTH_BUFFER;

		//update the camera
        const r2::draw::ConstantBufferLayoutConfiguration& constantLayout = r2::sarr::At(*constantLayouts, 0);
        r2::draw::renderer::AddFillConstantBufferCommandForData(
            r2::sarr::At(*constHandles, 0),
            constantLayout.layout.GetType(),
            constantLayout.layout.GetFlags().IsSet(r2::draw::CB_FLAG_MAP_PERSISTENT),
            glm::value_ptr(mPersController.GetCameraPtr()->view),
            constantLayout.layout.GetElements().at(1).size,
            constantLayout.layout.GetElements().at(1).offset);
    }
    
    virtual void Shutdown() override
    {
        FREE(layouts, *linearArenaPtr);
        FREE(constantLayouts, *linearArenaPtr);
        FREE(modelMats, *linearArenaPtr);
        FREE(subCommandsToDraw, *linearArenaPtr);
        FREE(modelMaterials, *linearArenaPtr);

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
        return {};
    }

    virtual std::vector<std::string> GetMaterialPacksWatchPaths() const override
    {
        return {};
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
    
private:
    r2::mem::MemoryArea::Handle memoryAreaHandle;
    r2::mem::MemoryArea::SubArea::Handle subMemoryAreaHandle;
    r2::cam::PerspectiveController mPersController;
    r2::asset::AssetCache* assetCache;
    bool reload;
    r2::mem::utils::MemBoundary assetCacheBoundary;
    r2::SArray<r2::asset::AssetCacheRecord>* assetsBuffers;
    r2::mem::LinearArena* linearArenaPtr;
    r2::SArray<r2::draw::BufferLayoutConfiguration>* layouts;
    r2::SArray<r2::draw::ConstantBufferLayoutConfiguration>* constantLayouts;
    r2::SArray<r2::draw::cmd::DrawBatchSubCommand>* subCommandsToDraw;
    r2::SArray<r2::draw::MaterialHandle>* modelMaterials;
    r2::SArray<glm::mat4>* modelMats;
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
                r2::util::PathCpy(subpath, "assets/textures");
                break;
            case r2::fs::utils::SHADERS_BIN:
                r2::util::PathCpy(subpath, "assets/shaders/bin");
                break;
            case r2::fs::utils::SHADERS_RAW:
                r2::util::PathCpy(subpath, "assets/shaders/raw");
                break;
            case r2::fs::utils::SHADERS_MANIFEST:
                r2::util::PathCpy(subpath, "assets/shaders/manifests");
                break;
            case r2::fs::utils::MODELS:
                r2::util::PathCpy(subpath, "assets/models");
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
