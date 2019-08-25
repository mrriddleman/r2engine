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

#ifdef R2_DEBUG
#include "r2/Core/Assets/Pipeline/AssetManifest.h"
#endif

namespace
{
    void TestAssetCache()
    {
        r2::asset::AssetCache assetCache;
        char filePath[r2::fs::FILE_PATH_LENGTH];
        
        r2::fs::utils::AppendSubPath(CPLAT.RootPath().c_str(), filePath, "AllBreakoutData.zip");
        
        r2::asset::ZipAssetFile* zipFile = assetCache.MakeZipAssetFile(filePath);
        
//        r2::fs::utils::AppendSubPath(CPLAT.RootPath().c_str(), filePath, "breakout_level_pack.breakout_level");
//
//        r2::asset::RawAssetFile* levelsFile = (r2::asset::RawAssetFile*)assetCache.MakeRawAssetFile(filePath);
//
//        r2::fs::utils::AppendSubPath(CPLAT.RootPath().c_str(), filePath, "breakout_powerups.powerup");
//
//        r2::asset::RawAssetFile* powerupsFile = (r2::asset::RawAssetFile*)assetCache.MakeRawAssetFile(filePath);
//
//        r2::fs::utils::AppendSubPath(CPLAT.RootPath().c_str(), filePath, "breakout_high_scores.scores");
//
//        r2::asset::RawAssetFile* highscoresFile = (r2::asset::RawAssetFile*)assetCache.MakeRawAssetFile(filePath);
//
//        r2::fs::utils::AppendSubPath(CPLAT.RootPath().c_str(), filePath, "breakout_player_save.player");
//
//        r2::asset::RawAssetFile* settingsFile = (r2::asset::RawAssetFile*)assetCache.MakeRawAssetFile(filePath);
//
//
//
//        r2::sarr::Push(*files, (r2::asset::AssetFile*)levelsFile);
//        r2::sarr::Push(*files, (r2::asset::AssetFile*)powerupsFile);
//        r2::sarr::Push(*files, (r2::asset::AssetFile*)highscoresFile);
//        r2::sarr::Push(*files, (r2::asset::AssetFile*)settingsFile);
        
        r2::asset::FileList files = assetCache.MakeFileList(10);
        r2::sarr::Push(*files, (r2::asset::AssetFile*)zipFile);
        
        bool assetCacheInitialized = assetCache.Init(r2::mem::utils::MemBoundary(), files);
        
        R2_CHECK(assetCacheInitialized, "Asset cache didn't initialize");
        
        r2::asset::Asset levelsAsset("breakout_level_pack.breakout_level");
        
        r2::asset::AssetBuffer* assetBuffer = assetCache.GetAssetBuffer(levelsAsset);
        
        R2_CHECK(assetBuffer != nullptr, "Asset buffer is nullptr");
        
        const byte* data = assetBuffer->Data();
        
        const auto levelPack = Breakout::GetLevelPack(data);
        
        const auto levels = levelPack->levels();
        
        printf("======================Levels==========================\n");
        
        for (u32 i = 0; i < levels->Length(); ++i)
        {
            printf("Level: %s\n", levels->Get(i)->name()->c_str());
            
            printf("Hash: %llu\n", levels->Get(i)->hashName());
            
            const auto blocks = levels->Get(i)->blocks();
            
            for (u32 j = 0; j < blocks->Length(); ++j)
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
        
        r2::asset::AssetBuffer* powerupAssetBuffer = assetCache.GetAssetBuffer(powerupsAsset);
        
        R2_CHECK(powerupAssetBuffer != nullptr, "Asset buffer is nullptr");
        
        const byte* powerupsData = powerupAssetBuffer->Data();
        
        const auto powerupsfbb = Breakout::GetPowerups(powerupsData);
        
        const auto powerups = powerupsfbb->powerups();
        
        for (u32 i = 0; i < powerups->Length(); ++i)
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
        
        r2::asset::AssetBuffer* highScoreAssetBuffer = assetCache.GetAssetBuffer(highScoresAsset);
        
        R2_CHECK(highScoreAssetBuffer != nullptr, "Asset buffer is nullptr");
        
        const auto scores = Breakout::GetHighScores(highScoreAssetBuffer->Data())->scores();
        
        printf("======================Scores==========================\n");
        
        for (u32 i = 0; i < scores->Length(); ++i)
        {
            auto score = scores->Get(i);
            
            printf("Player: %s\t\tScore: %u\n", score->name()->c_str(), score->points());
        }
        
        printf("======================================================\n");
        
        
        
        r2::asset::Asset playerSettingsAsset("breakout_player_save.player");
        
        r2::asset::AssetBuffer* settingsAssetBuffer = assetCache.GetAssetBuffer(playerSettingsAsset);
        
        R2_CHECK(settingsAssetBuffer != nullptr, "Asset buffer is nullptr");
        
        const auto settings = Breakout::GetPlayerSettings(settingsAssetBuffer->Data());
        
        printf("======================Settings==========================\n");
        
        printf("Player Starting Level: %u\n", settings->startingLevel());
        
        printf("Player Starting Lives: %u\n", settings->lives());
        
        printf("Player points: %u\n", settings->points());
        
        printf("========================================================\n");
        
        
        auto oneMoreLevelRef = assetCache.GetAssetBuffer(levelsAsset);
        
        auto oneMoreScoreRef = assetCache.GetAssetBuffer(highScoresAsset);
        
        
        assetCache.ReturnAssetBuffer(assetBuffer);
        
        assetCache.ReturnAssetBuffer(powerupAssetBuffer);
        
        assetCache.ReturnAssetBuffer(highScoreAssetBuffer);
        
        assetCache.ReturnAssetBuffer(settingsAssetBuffer);
        
        assetCache.ReturnAssetBuffer(oneMoreLevelRef);
        
        assetCache.ReturnAssetBuffer(oneMoreScoreRef);
        
        
        assetCache.Shutdown();
    }
    
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
#ifdef R2_DEBUG
        std::string assetBinDir = ASSET_BIN_DIR;
        std::string assetRawDir = ASSET_RAW_DIR;
        std::string assetFBSSchemaDir = ASSET_FBS_SCHEMA_DIR;
        
        r2::asset::pln::AssetManifest manifest;
        manifest.outputType = r2::asset::pln::AssetType::Zip;
        manifest.assetOutputPath = assetBinDir + r2::fs::utils::PATH_SEPARATOR + "AllBreakoutData.zip";
        
        r2::asset::pln::AssetFileCommand inputFile;
        inputFile.command.compile = r2::asset::pln::AssetCompileCommand::FlatBufferCompile;
        inputFile.command.schemaPath = assetFBSSchemaDir + r2::fs::utils::PATH_SEPARATOR + "BreakoutHighScoresSchema.fbs";
        inputFile.inputFile = assetRawDir + r2::fs::utils::PATH_SEPARATOR + "breakout_high_scores.json";
        inputFile.outputFile = assetBinDir + r2::fs::utils::PATH_SEPARATOR + "breakout_high_scores.scores";
        manifest.fileCommands.push_back(inputFile);
        
        inputFile.command.compile = r2::asset::pln::AssetCompileCommand::FlatBufferCompile;
        inputFile.command.schemaPath = assetFBSSchemaDir + r2::fs::utils::PATH_SEPARATOR + "BreakoutLevelSchema.fbs";
        inputFile.inputFile = assetRawDir + r2::fs::utils::PATH_SEPARATOR + "breakout_level_pack.json";
        inputFile.outputFile = assetBinDir + r2::fs::utils::PATH_SEPARATOR + "breakout_level_pack.breakout_level";
        manifest.fileCommands.push_back(inputFile);
        
        inputFile.command.compile = r2::asset::pln::AssetCompileCommand::FlatBufferCompile;
        inputFile.command.schemaPath = assetFBSSchemaDir + r2::fs::utils::PATH_SEPARATOR + "BreakoutPlayerSettings.fbs";
        inputFile.inputFile = assetRawDir + r2::fs::utils::PATH_SEPARATOR + "breakout_player_save.json";
        inputFile.outputFile = assetBinDir + r2::fs::utils::PATH_SEPARATOR + "breakout_player_save.player";
        manifest.fileCommands.push_back(inputFile);
        
        inputFile.command.compile = r2::asset::pln::AssetCompileCommand::FlatBufferCompile;
        inputFile.command.schemaPath = assetFBSSchemaDir + r2::fs::utils::PATH_SEPARATOR + "BreakoutPowerupSchema.fbs";
        inputFile.inputFile = assetRawDir + r2::fs::utils::PATH_SEPARATOR + "breakout_powerups.json";
        inputFile.outputFile = assetBinDir + r2::fs::utils::PATH_SEPARATOR + "breakout_powerups.powerup";
        manifest.fileCommands.push_back(inputFile);
        
        r2::asset::pln::WriteAssetManifest(manifest, "BreakoutData.aman", manifestPath);
#endif
    }
}

class Sandbox: public r2::Application
{
    virtual bool Init() override
    {
        MakeAssetManifest(GetAssetManifestPath());
        
        //TestAssetCache();
        //MakeAllData();
        return true;
    }
    
#ifdef R2_DEBUG
    virtual std::string GetAssetManifestPath() const override
    {
        return MANIFESTS_DIR;
    }
#endif
};


std::unique_ptr<r2::Application> r2::CreateApplication()
{
    return std::make_unique<Sandbox>();
}
