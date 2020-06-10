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
#include "r2/Audio/AudioEngine.h"
#include "r2/Render/Renderer/ShaderSystem.h"

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
    static std::vector<ShaderManifest> s_shaderManifests;
    
    static std::string s_flatBufferCompilerPath;
    
    static std::string s_soundDefinitionsDirectory;
    
    AssetCommand s_assetCommand;
    
    static SoundDefinitionCommand s_soundDefinitionCommand;
    static bool s_reloadManifests = false;
    static bool s_callRebuiltSoundDefinitions = false;
    
    //Shader data
    ShaderManifestCommand s_shaderCommand;
    //FileWatcher s_shaderManifestFileWatcher;
    FileWatcher s_shadersFileWatcher;
    //bool s_reloadShaderManifests = false;
    
    TexturePackManifestCommand s_texturePackCommand;
    MaterialPackManifestCommand s_materialPackCommand;

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


    void GenerateMaterialPackManifestsIfNeeded();

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

        

        LoadSoundDefinitions();
        
        //Shaders
        {
            ReloadShaderManifests();
            s_shadersFileWatcher.Init(s_delay, s_shaderCommand.shaderWatchPath);
            s_shadersFileWatcher.AddModifyListener(ShaderChangedRequest);
            s_shadersFileWatcher.AddCreatedListener(ShaderChangedRequest);
        }
        
        GenerateTexturePackManifestsIfNeeded();
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
            s_shadersFileWatcher.Run();
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
        s_shaderManifests = LoadAllShaderManifests(s_shaderCommand.manifestFilePath);
        bool success = BuildShaderManifestsIfNeeded(s_shaderManifests, s_shaderCommand.manifestFilePath, s_shaderCommand.shaderWatchPath);
        
        if (!success)
        {
            R2_LOGE("Failed to build shader manifests");
        }
        else
        {
            r2::draw::shadersystem::ReloadManifestFile(s_shaderCommand.manifestFilePath);
        }
    }

    void ShaderChangedRequest(std::string changedPath)
    {
        bool success = BuildShaderManifestsIfNeeded(s_shaderManifests, s_shaderCommand.manifestFilePath, s_shaderCommand.shaderWatchPath);
        
        if (!success)
        {
            R2_LOGE("Failed to build shader manifests");
        }
        
        //look through the manifests and find which need to be re-compiled and linked
        for (const auto& shaderManifest : s_shaderManifests)
        {
            if (changedPath == shaderManifest.vertexShaderPath ||
                changedPath == shaderManifest.fragmentShaderPath)
            {
                r2::draw::shadersystem::ReloadShader(shaderManifest);
            }
        }
    }

	//Textures
    void GenerateTexturePackManifestsIfNeeded()
    {
        R2_CHECK(s_texturePackCommand.manifestFilePaths.size() == s_texturePackCommand.texturePacksWatchDirectories.size(),
            "these should be the same size!");

        for (size_t i = 0; i < s_texturePackCommand.manifestFilePaths.size(); ++i)
        {
            std::string manifestPath = s_texturePackCommand.manifestFilePaths[i];
            std::string texturePackDir = s_texturePackCommand.texturePacksWatchDirectories[i];

            std::filesystem::path p = manifestPath;
            std::string manifestDir = p.parent_path().string();

            std::string texturePackManifestFile;
			r2::asset::pln::tex::FindTexturePacksManifestFile(manifestDir, p.stem().string(), texturePackManifestFile, true);

			if (texturePackManifestFile.empty())
			{
				if (r2::asset::pln::tex::FindTexturePacksManifestFile(manifestDir, p.stem().string(), texturePackManifestFile, false))
				{
					if (!r2::asset::pln::tex::GenerateTexturePacksManifestFromJson(texturePackManifestFile))
					{
						R2_CHECK(false, "Failed to generate texture pack manifest file from json!");
						return;
					}
				}
				else
				{
					if (!r2::asset::pln::tex::GenerateTexturePacksManifestFromDirectories(manifestPath, texturePackDir))
					{
						R2_CHECK(false, "Failed to generate texture pack manifest file from directories!");
						return;
					}
				}
			}
        }
    }
    
    //materials
    void GenerateMaterialPackManifestsIfNeeded()
    {
		R2_CHECK(s_materialPackCommand.manifestFilePaths.size() == s_materialPackCommand.materialPacksWatchDirectories.size(),
			"these should be the same size!");

		for (size_t i = 0; i < s_materialPackCommand.manifestFilePaths.size(); ++i)
		{
			std::string manifestPath = s_materialPackCommand.manifestFilePaths[i];
			std::string materialPackDir = s_materialPackCommand.materialPacksWatchDirectories[i];

			std::filesystem::path p = manifestPath;
			std::string manifestDir = p.parent_path().string();

			std::string materialPackManifestFile;
			r2::asset::pln::FindMaterialPackManifestFile(manifestDir, p.stem().string(), materialPackManifestFile, true);

			if (materialPackManifestFile.empty())
			{
				if (r2::asset::pln::FindMaterialPackManifestFile(manifestDir, p.stem().string(), materialPackManifestFile, false))
				{
					if (!r2::asset::pln::GenerateMaterialPackManifestFromJson(materialPackManifestFile))
					{
						R2_CHECK(false, "Failed to generate texture pack manifest file from json!");
						return;
					}
				}
				else
				{
					if (!r2::asset::pln::GenerateMaterialPackManifestFromDirectories(manifestPath, materialPackDir))
					{
						R2_CHECK(false, "Failed to generate texture pack manifest file from directories!");
						return;
					}
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
}

#endif
