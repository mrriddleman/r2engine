//
//  AssetCompiler.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-25.
//
#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetCompiler.h"
#include "r2/Core/Assets/Pipeline/AssetManifest.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Core/File/PathUtils.h"
#include <filesystem>
#include <stdio.h>
#include "miniz.h"
#include <deque>

namespace r2::asset::pln::cmp
{
    static std::string s_tempBuildPath;
    static std::deque<r2::asset::pln::AssetManifest> s_manifestWorkQueue;
    static r2::asset::pln::AssetsBuiltFunc s_assetsBuiltFunc;
    
    std::string BuildTempOutputPath(const std::string& inputFileName)
    {
        std::string filename = inputFileName;
        
        if (std::filesystem::path(filename).has_filename())
        {
            filename = std::filesystem::path(filename).filename().string();
        }
        
        std::string outputPath = s_tempBuildPath + r2::fs::utils::PATH_SEPARATOR + filename;
        
        if (s_tempBuildPath[s_tempBuildPath.size()-1] == r2::fs::utils::PATH_SEPARATOR)
        {
            outputPath = s_tempBuildPath + filename;
        }
        
        return outputPath;
    }
    
    bool Output(const r2::asset::pln::AssetManifest& manifest, const std::vector<std::string>& files)
    {
        std::string tempOutputPath = BuildTempOutputPath(manifest.assetOutputPath);
        std::string outputFileName = std::filesystem::path(manifest.assetOutputPath).filename().string();
        switch (manifest.outputType)
        {
            case r2::asset::pln::AssetType::Zip:
            {
                mz_zip_archive archive;
                memset(&archive, 0, sizeof(archive));
                
                mz_bool initialized = mz_zip_writer_init_file(&archive, tempOutputPath.c_str(), 0);
                bool fail = !initialized;
                
                for (const std::string& path: files)
                {
                    mz_bool result = mz_zip_writer_add_file(&archive, std::filesystem::path(path).filename().string().c_str(), path.c_str(), "", 0, MZ_DEFAULT_COMPRESSION);
                    
                    if (!result)
                    {
                        fail = true;
                        break;
                    }
                }
                
                mz_bool finalizeResult = mz_zip_writer_finalize_archive(&archive);
                mz_bool endResult = mz_zip_writer_end(&archive);
                
                if (!finalizeResult || !endResult)
                {
                    fail = true;
                }
                
                if (!fail)
                {
                    //copy the file over to the output dir
                    std::filesystem::rename(tempOutputPath, manifest.assetOutputPath);
                }
                else
                {
                    std::filesystem::remove(tempOutputPath);
                }
                return !fail;
            }
            break;
            case r2::asset::pln::AssetType::Raw:
            {
                R2_CHECK(files.size() == 1, "There should only be 1 file if it's a raw output file");
                
                std::filesystem::rename(tempOutputPath, manifest.assetOutputPath);
                return false;
            }
            break;
            default:
                break;
        }
        
        return false;
    }
    
    bool Init(const std::string& tempBuildPath, r2::asset::pln::AssetsBuiltFunc assetsBuiltFunc)
    {
        s_tempBuildPath = tempBuildPath;
        s_assetsBuiltFunc = assetsBuiltFunc;
        return true;
    }
    
    void PushBuildRequest(const r2::asset::pln::AssetManifest& manifest)
    {
        //Should ignore dups
        for (const auto& man : s_manifestWorkQueue)
        {
            if (man.assetOutputPath == manifest.assetOutputPath)
            {
                return;
            }
        }
        
        s_manifestWorkQueue.push_back(manifest);
    }

    bool CompileAsset(const r2::asset::pln::AssetManifest& manifest)
    {
        bool fail = false;

        std::vector<std::string> inputFiles;

        for (u32 i = 0; i < manifest.fileCommands.size() && !fail; ++i)
        {
            const auto& inputFile = manifest.fileCommands[i];

            std::string outputPath = BuildTempOutputPath(inputFile.outputFileName);

            inputFiles.push_back(outputPath);

            switch (inputFile.command.compile) {
            case r2::asset::pln::AssetCompileCommand::FlatBufferCompile:
            {
                if (!r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(s_tempBuildPath, inputFile.command.schemaPath, inputFile.inputPath))
                {
                    fail = true;
                }
            }
            break;
            case r2::asset::pln::AssetCompileCommand::None:
            {
                if (!std::filesystem::copy_file(inputFile.inputPath, outputPath))
                {
                    fail = true;
                }
            }
            break;
            default:
                R2_CHECK(false, "Unknown asset compile command!");
                break;
            }
        }

        if (!fail)
        {
            if (!Output(manifest, inputFiles))
            {
                R2_LOGE("Failed to output asset: %s\n", manifest.assetOutputPath.c_str());
            }
            else
            {
                return true;
            }
        }
        else
        {
            R2_LOGE("Failed to compile asset: %s\n", manifest.assetOutputPath.c_str());
        }

        return false;
    }
    
    void Update()
    {
        if (!std::filesystem::exists(s_tempBuildPath) && s_manifestWorkQueue.size() > 0)
        {
            bool result = std::filesystem::create_directory(s_tempBuildPath);
            R2_CHECK(result, "Failed to create temporary directory: %s\n", s_tempBuildPath.c_str());
        }
        
        std::vector<std::string> outputFiles;
        while (s_manifestWorkQueue.size() > 0)
        {
            const r2::asset::pln::AssetManifest& nextAsset = s_manifestWorkQueue.front();
            
            if (CompileAsset(nextAsset))
            {
                outputFiles.push_back(nextAsset.assetOutputPath);
            }
            
            s_manifestWorkQueue.pop_front();
        }
        
        std::filesystem::remove_all(s_tempBuildPath);
        
        if (outputFiles.size() > 0)
        {
            s_assetsBuiltFunc(outputFiles);
        }
    }
}
#endif
