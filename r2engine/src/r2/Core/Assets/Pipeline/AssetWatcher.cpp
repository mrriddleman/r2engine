//
//  AssetWatcher.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-08.
//
#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "AssetWatcher.h"
#include "r2/Core/Assets/Pipeline/AssetManifest.h"
#include "r2/Core/Assets/Pipeline/ShaderManifest.h"
#include "r2/Core/Assets/Pipeline/AssetCompiler.h"
#include "r2/Core/Assets/Pipeline/SoundDefinitionUtils.h"
#include "r2/Core/Assets/Pipeline/TexturePackManifestUtils.h"
#include "r2/Core/Assets/Pipeline/MaterialPackManifestUtils.h"
#include "r2/Core/Assets/Pipeline/MakeEngineModels.h"
#include "r2/Audio/AudioEngine.h"
#include "r2/Render/Renderer/ShaderSystem.h"
#include "r2/Render/Model/Material.h"
#include "r2/Core/Assets//Pipeline/FileSystemHelpers.h"

#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"
#include <string>
#include <thread>
#include <atomic>

namespace r2::asset::pln
{
    
    void ThreadProc();
    
    static std::vector<FileWatcher> s_fileWatchers;
    static std::vector<AssetManifest> s_manifests;
    Milliseconds s_delay;
    std::chrono::steady_clock::time_point s_lastTime;
    
    static FileWatcher s_manifestFileWatcher;
    static std::vector<FileWatcher> s_soundDefinitionFileWatchers;
    static std::vector<r2::audio::AudioEngine::SoundDefinition> s_soundDefinitions;
    static std::vector<std::vector<ShaderManifest>> s_shaderManifests;
    
    static std::string s_flatBufferCompilerPath;
    
    static std::string s_soundDefinitionsDirectory;
    
    AssetCommand s_assetCommand;
    
    static SoundDefinitionCommand s_soundDefinitionCommand;
    static bool s_reloadManifests = false;
    static bool s_callRebuiltSoundDefinitions = false;
    
    //Shader data
    ShaderManifestCommand s_shaderCommand;
    //FileWatcher s_shaderManifestFileWatcher;
    std::vector<FileWatcher> s_shadersFileWatchers;
    //bool s_reloadShaderManifests = false;
    
    TexturePackManifestCommand s_texturePackCommand;
    std::vector<FileWatcher> s_texturesFileWatchers;

    MaterialPackManifestCommand s_materialPackCommand;
    FileWatcher s_materialsFileWatcher;

    std::thread s_assetWatcherThread;
    std::atomic_bool s_end;
    
    void ModifiedWatchPathDispatch(std::string path);
    void CreatedWatchPathDispatch(std::string path);
    void RemovedWatchPathDispatch(std::string path);
    
    //Assets
    void SetReloadManifests(std::string changedPath);
    void ReloadManifests();
    
    void BuildManifests();
    void NotifyAssetChanged();
    void PushBuildRequest(std::string changedPath);
    void AddWatchPaths(Milliseconds delay, const std::vector<std::string>& paths);
    
    //Sound Definitions
    void AddNewSoundDefinitionFromFile(std::string path);
    void SoundDefinitionsChanged(std::string path);
    void RemovedSoundDefinitionFromFile(std::string path);
    
    void LoadSoundDefinitions();


    //Shaders
    void ReloadShaderManifests();
  //  void SetReloadShaderManifests(std::string changedPath);
    void ShaderChangedRequest(std::string changedPath);
    

    //Textures
    void GenerateTexturePackManifestsIfNeeded();
    void TextureChangedRequest(std::string chagedPath);
    void TextureAddedRequest(std::string newPath);
    void ReloadTexturePackManifests();

    void GenerateMaterialPackManifestsIfNeeded();

    void MakeEngineBinaryAssetFolders();
    void MakeGameBinaryAssetFolders();

    void Init(  const std::string& flatbufferCompilerLocation,
              Milliseconds delay,
              const AssetCommand& assetCommand,
              const SoundDefinitionCommand& soundDefinitionCommand,
              const ShaderManifestCommand& shaderCommand,
              const TexturePackManifestCommand& texturePackCommand,
              const MaterialPackManifestCommand& materialPackCommand)
    {
        
        s_flatBufferCompilerPath = flatbufferCompilerLocation;
        s_delay = delay;
        s_lastTime = std::chrono::steady_clock::now();
        s_assetCommand = assetCommand;
        s_soundDefinitionCommand = soundDefinitionCommand;
        s_shaderCommand = shaderCommand;
        s_texturePackCommand = texturePackCommand;
        s_materialPackCommand = materialPackCommand;

        std::filesystem::path p = soundDefinitionCommand.soundDefinitionFilePath;
        s_soundDefinitionsDirectory = p.parent_path().string();
        ReloadManifests();


        MakeEngineBinaryAssetFolders();
        MakeGameBinaryAssetFolders();

        auto makeModels = ShouldMakeEngineModels();
        if (makeModels.size()>0)
        {
            MakeEngineModels(makeModels);
        }

        LoadSoundDefinitions();
        
        //Shaders
        {
            ReloadShaderManifests();

            for (const auto& path : s_shaderCommand.shaderWatchPaths)
            {
                FileWatcher fw;


                fw.Init(s_delay, path);
                fw.AddModifyListener(ShaderChangedRequest);
                fw.AddCreatedListener(ShaderChangedRequest);

                s_shadersFileWatchers.push_back(fw);
            }

            
        }
        
        //Textures
        GenerateTexturePackManifestsIfNeeded();

        {
            
            for (const auto& path : texturePackCommand.texturePacksWatchDirectories)
            {
                FileWatcher fw;
                fw.Init(s_delay, path);
                fw.AddModifyListener(TextureChangedRequest);
                fw.AddCreatedListener(TextureAddedRequest);
                s_texturesFileWatchers.push_back(fw);
            }
        }


        GenerateMaterialPackManifestsIfNeeded();

        s_manifestFileWatcher.Init(std::chrono::milliseconds(delay), s_assetCommand.assetManifestsPath);
        //@TODO(Serge): modify these to be their own functions
        s_manifestFileWatcher.AddCreatedListener(SetReloadManifests);
        s_manifestFileWatcher.AddModifyListener(SetReloadManifests);
        s_manifestFileWatcher.AddRemovedListener(SetReloadManifests);
        
        for (const auto& path: soundDefinitionCommand.soundDirectories)
        {
            FileWatcher fw;
            
            fw.Init(delay, path);
            fw.AddCreatedListener(AddNewSoundDefinitionFromFile);
           // fw.AddRemovedListener(RemovedSoundDefinitionFromFile);
            s_soundDefinitionFileWatchers.push_back(fw);
        }
        
        FileWatcher fw;
        fw.Init(delay, s_soundDefinitionsDirectory);
        fw.AddModifyListener(SoundDefinitionsChanged);
        s_soundDefinitionFileWatchers.push_back(fw);
        



        r2::asset::pln::cmp::Init(s_assetCommand.assetTempPath, s_assetCommand.assetsBuldFunc);
        
        for (const auto& manifest : s_manifests)
        {
            r2::asset::pln::cmp::CompileAsset(manifest);
        }

        AddWatchPaths(delay, s_assetCommand.pathsToWatch);
        s_end = false;
        
        std::thread t(r2::asset::pln::ThreadProc);
        
        s_assetWatcherThread = std::move(t);
    }
    
    void Update()
    {
        auto now = std::chrono::steady_clock::now();
        auto dt = std::chrono::duration_cast<Milliseconds>(now - s_lastTime);
        
        if (dt >= s_delay)
        {
            //@NOTE(Serge): shaders need to run on the main thread because of OpenGL context
            for (auto& shaderFileWatcher : s_shadersFileWatchers)
            {
                shaderFileWatcher.Run();
            }
        }
    }
    
    void ThreadProc()
    {
        while (!s_end)
        {
            std::this_thread::sleep_for(s_delay);
            
            s_manifestFileWatcher.Run();
            
            if (s_reloadManifests)
            {
                ReloadManifests();
            }
            
            for (auto& fileWatcher: s_fileWatchers)
            {
                fileWatcher.Run();
            }
            
            //pull commands from the queue to request asset builds
            r2::asset::pln::cmp::Update();
            
            
            for(auto& soundFileWatcher : s_soundDefinitionFileWatchers)
            {
                soundFileWatcher.Run();
            }
            
            if (s_callRebuiltSoundDefinitions && s_soundDefinitionCommand.buildFunc)
            {
                s_soundDefinitionCommand.buildFunc({s_soundDefinitionCommand.soundDefinitionFilePath});
                s_callRebuiltSoundDefinitions = false;
            }

			for (auto& fw : s_texturesFileWatchers)
			{
				fw.Run();
			}
        }
    }
    
    void Shutdown()
    {
        s_end = true;
        s_assetWatcherThread.join();
    }
    
    void AddWatchPaths(Milliseconds delay, const std::vector<std::string>& paths)
    {
        for (const auto& path: paths)
        {
            FileWatcher fw;
            fw.Init(delay, path);
            fw.AddModifyListener(ModifiedWatchPathDispatch);
            fw.AddCreatedListener(CreatedWatchPathDispatch);
            fw.AddRemovedListener(RemovedWatchPathDispatch);
            
            s_fileWatchers.push_back(fw);
        }
    }
    
    void ModifiedWatchPathDispatch(std::string path)
    {
        //R2_LOGI("Asset watcher - Modified path: %s\n", path.c_str());
        //Push some sort of command to a queue here
        PushBuildRequest(path);
    }
    
    void CreatedWatchPathDispatch(std::string path)
    {
        //R2_LOGI("Asset watcher - Created path: %s\n", path.c_str());
        //Push some sort of command to a queue here
        PushBuildRequest(path);
    }
    
    void RemovedWatchPathDispatch(std::string path)
    {
        R2_LOGI("Asset watcher - Removed path: %s\n", path.c_str());
        //Push some sort of command to a queue here
    }
    
    void SetReloadManifests(std::string changedPath)
    {
        s_reloadManifests = true;
    }
    
    void ReloadManifests()
    {
        BuildManifests();
        
        LoadAssetManifests(s_manifests, s_assetCommand.assetManifestsPath);
        s_reloadManifests = false;
    }
    
    void AddNewSoundDefinitionFromFile(std::string path)
    {
        r2::audio::AudioEngine::SoundDefinition soundDefinition;
        r2::util::PathCpy( soundDefinition.soundName, path.c_str() );
        
        std::string fileName = std::filesystem::path(path).filename().string();

        soundDefinition.soundKey = STRING_ID(fileName.c_str());
        
        r2::asset::pln::audio::AddNewSoundDefinition(s_soundDefinitions, soundDefinition);

        bool result = r2::asset::pln::audio::GenerateSoundDefinitionsFile(s_soundDefinitionCommand.soundDefinitionFilePath, s_soundDefinitions);
        
        R2_CHECK(result, "Failed to write sound definition file");
        
        s_callRebuiltSoundDefinitions = true;
    }
    
    void RemovedSoundDefinitionFromFile(std::string path)
    {
        //s_callRebuiltSoundDefinitions = true;
    }
    
    void SoundDefinitionsChanged(std::string path)
    {
        if (std::filesystem::path(path).extension() == ".json")
        {
            bool result = r2::asset::pln::audio::GenerateSoundDefinitionsFromJson(path);
            R2_CHECK(result, "Failed to generate sound definitions from: %s", path.c_str());
            
            s_soundDefinitions = r2::asset::pln::audio::LoadSoundDefinitions(s_soundDefinitionCommand.soundDefinitionFilePath);
            
            s_callRebuiltSoundDefinitions = true;
        }
    }
    
    void LoadSoundDefinitions()
    {
        //check to see if we have a .sdef file
        //if we do, then load it
        //otherwise if we have a .json file generate the .sdef file from the json and load that
        //otherwise generate the file from the directories
        std::string soundDefinitionFile = "";
        r2::asset::pln::audio::FindSoundDefinitionFile(s_soundDefinitionsDirectory, soundDefinitionFile, true);
        
        if (soundDefinitionFile.empty())
        {
            if(r2::asset::pln::audio::FindSoundDefinitionFile(s_soundDefinitionsDirectory, soundDefinitionFile, false))
            {
                if (!r2::asset::pln::audio::GenerateSoundDefinitionsFromJson(soundDefinitionFile))
                {
                    R2_CHECK(false, "Failed to generate sound definition file from json!");
                    return;
                }
            }
            else
            {
                if (!r2::asset::pln::audio::GenerateSoundDefinitionsFromDirectories(s_soundDefinitionCommand.soundDefinitionFilePath, s_soundDefinitionCommand.soundDirectories))
                {
                    R2_CHECK(false, "Failed to generate sound definition file from directories!");
                    return;
                }
                
            }
            soundDefinitionFile = s_soundDefinitionCommand.soundDefinitionFilePath;
            s_callRebuiltSoundDefinitions = true;
        }
        else
        {
            s_soundDefinitions = r2::asset::pln::audio::LoadSoundDefinitions(soundDefinitionFile);
            
        }
    }
    
    void ReloadShaderManifests()
    {
        s_shaderManifests.clear();

        size_t numManifestFilePaths = s_shaderCommand.manifestFilePaths.size();
        for (size_t i = 0; i < numManifestFilePaths; ++i)
        {
            const auto& manifestFilePath = s_shaderCommand.manifestFilePaths.at(i);
			std::vector<ShaderManifest> shaderManifests = LoadAllShaderManifests(manifestFilePath);
			bool success = BuildShaderManifestsIfNeeded(shaderManifests, manifestFilePath, s_shaderCommand.shaderWatchPaths.at(i));

			if (!success)
			{
				R2_LOGE("Failed to build shader manifests");
			}
			else
			{
				r2::draw::shadersystem::ReloadManifestFile(manifestFilePath);
			}

            s_shaderManifests.push_back(shaderManifests);
        }

    }

    void ShaderChangedRequest(std::string changedPath)
    {
        //look through the manifests and find which need to be re-compiled and linked
        size_t numShaderManifests = s_shaderManifests.size();

        for (size_t i = 0; i < numShaderManifests; ++i)
        {
            auto& shaderManifests = s_shaderManifests.at(i);
            for (const auto& shaderManifest : shaderManifests)
            {
				if (changedPath == shaderManifest.vertexShaderPath ||
					changedPath == shaderManifest.fragmentShaderPath)
				{
					bool success = BuildShaderManifestsIfNeeded(shaderManifests, s_shaderCommand.manifestFilePaths[i], s_shaderCommand.shaderWatchPaths[i]);

					if (!success)
					{
						R2_LOGE("Failed to build shader manifests");
					}

					r2::draw::shadersystem::ReloadShader(shaderManifest);
					break;
				}
            }
        }
    }

	//Textures
    void GenerateTexturePackManifestsIfNeeded()
    {
        R2_CHECK(s_texturePackCommand.manifestBinaryFilePaths.size() == s_texturePackCommand.manifestRawFilePaths.size(),
            "these should be the same size!");

        for (size_t i = 0; i < s_texturePackCommand.manifestBinaryFilePaths.size(); ++i)
        {
            std::string manifestBinaryPath = s_texturePackCommand.manifestBinaryFilePaths[i];
            std::string manifestRawPath = s_texturePackCommand.manifestRawFilePaths[i];
            std::string texturePackDir = s_texturePackCommand.texturePacksWatchDirectories[i];

            std::filesystem::path binaryPath = manifestBinaryPath;
            std::string manifestBinaryDir = binaryPath.parent_path().string();

            std::filesystem::path rawPath = manifestRawPath;
            std::string manifestRawDir = rawPath.parent_path().string();

            std::string texturePackManifestFile;
			r2::asset::pln::tex::FindTexturePacksManifestFile(manifestBinaryDir, binaryPath.stem().string(), texturePackManifestFile, true);

			if (texturePackManifestFile.empty())
			{
				if (r2::asset::pln::tex::FindTexturePacksManifestFile(manifestRawDir, rawPath.stem().string(), texturePackManifestFile, false))
				{
					if (!std::filesystem::remove(texturePackManifestFile))
					{
						R2_CHECK(false, "Failed to delete texture pack manifest file from json!");
						return;
					}
				}

				if (!r2::asset::pln::tex::GenerateTexturePacksManifestFromDirectories(binaryPath.string(), rawPath.string(), texturePackDir))
				{
					R2_CHECK(false, "Failed to generate texture pack manifest file from directories!");
					return;
				}
			}
        }
    }

    void TextureChangedRequest(std::string chagedPath)
    {
        r2::draw::matsys::TextureChanged(chagedPath);
    }

    void TextureAddedRequest(std::string newPath)
    {

    }

    //materials
    void GenerateMaterialPackManifestsIfNeeded()
    {
		R2_CHECK(s_materialPackCommand.manifestBinaryFilePaths.size() == s_materialPackCommand.manifestRawFilePaths.size(),
			"these should be the same size!");

		for (size_t i = 0; i < s_materialPackCommand.manifestBinaryFilePaths.size(); ++i)
		{
			std::string manifestPathBin = s_materialPackCommand.manifestBinaryFilePaths[i];
            std::string manifestPathRaw = s_materialPackCommand.manifestRawFilePaths[i];
			std::string materialPackDirRaw = s_materialPackCommand.materialPacksWatchDirectoriesRaw[i];
            std::string materialPackDirBin = s_materialPackCommand.materialPacksWatchDirectoriesBin[i];

			std::filesystem::path binaryPath = manifestPathBin;
			std::string binManifestDir = binaryPath.parent_path().string();

            std::filesystem::path rawPath = manifestPathRaw;
            std::string rawManifestDir = rawPath.parent_path().string();

			std::string materialPackManifestFile;
			r2::asset::pln::FindMaterialPackManifestFile(binManifestDir, binaryPath.stem().string(), materialPackManifestFile, true);

			if (materialPackManifestFile.empty())
			{
				if (r2::asset::pln::FindMaterialPackManifestFile(rawManifestDir, rawPath.stem().string(), materialPackManifestFile, false))
				{
					if (!std::filesystem::remove(materialPackManifestFile))
					{
						R2_CHECK(false, "Failed to generate texture pack manifest file from json!");
						return;
					}
				}

				if (!r2::asset::pln::GenerateMaterialPackManifestFromDirectories(binaryPath.string(), rawPath.string(), materialPackDirBin, materialPackDirRaw))
				{
					R2_CHECK(false, "Failed to generate texture pack manifest file from directories!");
					return;
				}
				
			}
		}
    }


    void BuildManifests()
    {
        BuildAssetManifestsFromJson(s_assetCommand.assetManifestsPath);
    }
    
    void PushBuildRequest(std::string changedPath)
    {
        for (const auto& manifest : s_manifests)
        {
            for (const auto& fileCommand : manifest.fileCommands)
            {
                if(std::filesystem::path(fileCommand.inputPath) == std::filesystem::path(changedPath))
                {
                    r2::asset::pln::cmp::PushBuildRequest(manifest);
                    return;
                }
            }
        }
    }

    void MakeEngineBinaryAssetFolders()
    {
        std::filesystem::path assetBinPath = R2_ENGINE_ASSET_BIN_PATH;
        if (!std::filesystem::exists(assetBinPath))
        {
            std::filesystem::create_directory(assetBinPath);
        }

        std::filesystem::path modelsBinPath = R2_ENGINE_INTERNAL_MODELS_BIN;
        if (!std::filesystem::exists(modelsBinPath))
        {
            std::filesystem::create_directory(modelsBinPath);
        }

		std::filesystem::path texturesBinPath = R2_ENGINE_INTERNAL_TEXTURES_BIN;
		if (!std::filesystem::exists(texturesBinPath))
		{
			std::filesystem::create_directory(texturesBinPath);
		}

		std::filesystem::path texturesManifestsBinPath = R2_ENGINE_INTERNAL_TEXTURES_MANIFESTS_BIN;
		if (!std::filesystem::exists(texturesManifestsBinPath))
		{
			std::filesystem::create_directory(texturesManifestsBinPath);
		}

		std::filesystem::path materialsBinPath = R2_ENGINE_INTERNAL_MATERIALS_DIR_BIN;
		if (!std::filesystem::exists(materialsBinPath))
		{
			std::filesystem::create_directory(materialsBinPath);
		}

		std::filesystem::path materialsManifestsBinPath = R2_ENGINE_INTERNAL_MATERIALS_MANIFESTS_BIN;
		if (!std::filesystem::exists(materialsManifestsBinPath))
		{
			std::filesystem::create_directory(materialsManifestsBinPath);
		}

		std::filesystem::path materialsPacksBinPath = R2_ENGINE_INTERNAL_MATERIALS_PACKS_DIR_BIN;
		if (!std::filesystem::exists(materialsPacksBinPath))
		{
			std::filesystem::create_directory(materialsPacksBinPath);
		}
        
    }

    

    void MakeGameBinaryAssetFolders()
    {
        MakeDirectoriesRecursively(s_texturePackCommand.manifestBinaryFilePaths, true, true);
        MakeDirectoriesRecursively(s_materialPackCommand.manifestBinaryFilePaths, true, true);
        MakeDirectoriesRecursively(s_materialPackCommand.materialPacksWatchDirectoriesBin, true, false);
    }
}

#endif
