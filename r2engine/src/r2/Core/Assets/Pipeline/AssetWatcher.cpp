//
//  AssetWatcher.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-08.
//

#ifdef R2_ASSET_PIPELINE
#include "AssetWatcher.h"
#include "r2/Core/Assets/Pipeline/AssetManifest.h"
#include "r2/Core/Assets/Pipeline/AssetCompiler.h"
#include "r2/Core/Assets/Pipeline/SoundDefinitionUtils.h"
#include "r2/Audio/AudioEngine.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"
#include <string>
#include <thread>

namespace r2::asset::pln
{
    
    void Update();
    
    static std::vector<FileWatcher> s_fileWatchers;
    static std::vector<AssetManifest> s_manifests;
    Milliseconds s_delay;
    
    static FileWatcher s_manifestFileWatcher;
    static std::vector<FileWatcher> s_soundDefinitionFileWatchers;
    static std::vector<r2::audio::AudioEngine::SoundDefinition> s_soundDefinitions;
    static std::string s_flatBufferCompilerPath;
    static std::string s_manifestsPath;
    static std::string s_soundDefinitionsDirectory;
    
    static SoundDefinitionCommand s_soundDefinitionCommand;
    static bool s_reloadManifests = false;
    static bool s_callRebuiltSoundDefinitions = false;
    
    std::thread s_assetWatcherThread;
    std::atomic_bool s_end;
    
    void ModifiedWatchPathDispatch(std::string path);
    void CreatedWatchPathDispatch(std::string path);
    void RemovedWatchPathDispatch(std::string path);
    
    void SetReloadManifests(std::string changedPath);
    void ReloadManifests();
    
    void AddNewSoundDefinitionFromFile(std::string path);
    void SoundDefinitionsChanged(std::string path);
    void RemovedSoundDefinitionFromFile(std::string path);
    
    void LoadSoundDefinitions();
    void BuildManifests();
    void NotifyAssetChanged();
    void PushBuildRequest(std::string changedPath);
    void AddWatchPaths(Milliseconds delay, const std::vector<std::string>& paths);
    
    void Init(const std::string& assetManifestsPath,
              const std::string& assetTempPath,
              const std::string& flatbufferCompilerLocation,
              Milliseconds delay,
              const std::vector<std::string>& paths,
              AssetsBuiltFunc builtFunc,
              const SoundDefinitionCommand& soundDefinitionCommand)
    {
        s_flatBufferCompilerPath = flatbufferCompilerLocation;
        s_manifestsPath = assetManifestsPath;
        s_delay = delay;
        s_soundDefinitionCommand = soundDefinitionCommand;
        std::filesystem::path p = soundDefinitionCommand.soundDefinitionFilePath;
        s_soundDefinitionsDirectory = p.parent_path().string();
        ReloadManifests();
        LoadSoundDefinitions();
        
        s_manifestFileWatcher.Init(std::chrono::milliseconds(delay), assetManifestsPath);
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
        
        r2::asset::pln::cmp::Init(assetTempPath, builtFunc);
        
        AddWatchPaths(delay, paths);
        s_end = false;
        
        std::thread t(r2::asset::pln::Update);
        
        s_assetWatcherThread = std::move(t);
    }
    
    void Update()
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
        
        LoadAssetManifests(s_manifests, s_manifestsPath);
        s_reloadManifests = false;
    }
    
    void AddNewSoundDefinitionFromFile(std::string path)
    {
        r2::audio::AudioEngine::SoundDefinition soundDefinition;
        strcpy( soundDefinition.soundName, path.c_str() );
        
        std::string fileName = std::filesystem::path(path).filename();

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
        
        if (!soundDefinitionFile.empty())
        {
            s_soundDefinitions = r2::asset::pln::audio::LoadSoundDefinitions(soundDefinitionFile);
            
        }
    }
    
    void BuildManifests()
    {
        BuildAssetManifestsFromJson(s_manifestsPath);
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
