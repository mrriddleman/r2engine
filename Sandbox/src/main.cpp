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
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/File.h"
#include "r2/Core/File/PathUtils.h"

class Sandbox: public r2::Application
{
    virtual bool Init() override
    {
        
//        flatbuffers::FlatBufferBuilder builder;
//
//        Breakout::color4 redColor(1.0f, 0.0f, 0.0f, 1.0f);
//        Breakout::color4 blueColor(0.0f, 0.0f, 1.0f, 1.0f);
//        Breakout::color4 greenColor(0.0f, 1.0f, 0.0f, 1.0f);
//
//        auto redBlock = Breakout::CreateBlock(builder, 'r', &redColor, 1);
//        auto blueBlock = Breakout::CreateBlock(builder, 'b', &blueColor, 1);
//        auto greenBlock = Breakout::CreateBlock(builder, 'g', &greenColor, 1);
//        std::vector<flatbuffers::Offset<Breakout::Block>> blocksVec = {redBlock, blueBlock, greenBlock};
//
//        auto layout1 = Breakout::CreateLayout(builder, 10, 3, builder.CreateString("rrrrrrrrrr\nbbbbbbbbbb\ngggggggggg"));
//        auto layout2 = Breakout::CreateLayout(builder, 10, 3, builder.CreateString("bbbbbbbbbb\nrrrrrrrrrr\ngggggggggg"));
//        auto layout3 = Breakout::CreateLayout(builder, 10, 3, builder.CreateString("gggggggggg\nbbbbbbbbbb\n\rrrrrrrrrr"));
//
//        auto level1 = Breakout::CreateLevel(builder, builder.CreateString("Level1"), 1, builder.CreateVector(blocksVec), layout1);
//        auto level2 = Breakout::CreateLevel(builder, builder.CreateString("Level2"), 2, builder.CreateVector(blocksVec), layout2);
//        auto level3 = Breakout::CreateLevel(builder, builder.CreateString("Level3"), 3, builder.CreateVector(blocksVec), layout3);
//
//        std::vector<flatbuffers::Offset<Breakout::Level>> levelsVec = {level1, level2, level3};
//
//        auto levelPack = Breakout::CreateLevelPack(builder, builder.CreateVector(levelsVec));
//
//        builder.Finish(levelPack);
//
//        byte* buf = builder.GetBufferPointer();
//        u32 size = builder.GetSize();
//
//        r2::fs::FileMode mode;
//        mode = r2::fs::Mode::Write;
//        mode |= r2::fs::Mode::Binary;
//
//        char pathToWrite[r2::fs::FILE_PATH_LENGTH];
//
//        r2::fs::utils::AppendSubPath(CPLAT.RootPath().c_str(), pathToWrite, "breakout_level_pack.bin");
//
//        r2::fs::File* file = r2::fs::FileSystem::Open(r2::fs::DeviceConfig(), pathToWrite, mode);
//        auto result = file->Write(buf, size);
//
//
//        R2_LOGI("Wrote out %llu bytes", result);
//
//        r2::fs::FileSystem::Close(file);
        
        return true;
    }
};


std::unique_ptr<r2::Application> r2::CreateApplication()
{
    return std::make_unique<Sandbox>();
}
