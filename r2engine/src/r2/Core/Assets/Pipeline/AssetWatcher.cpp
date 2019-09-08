//
//  AssetWatcher.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-08.
//

#ifdef R2_DEBUG
#include "AssetWatcher.h"
#include "r2/Core/Assets/Pipeline/AssetManifest.h"
#include "r2/Core/Assets/Pipeline/AssetCompiler.h"


namespace r2::asset::pln
{
    static std::vector<FileWatcher> s_fileWatchers;
    static std::vector<AssetManifest> s_manifests;
    static std::vector<AssetBuiltFunc> s_assetsBuiltListeners;
    
    static FileWatcher s_manifestFileWatcher;
    static std::string s_flatBufferCompilerPath;
    static std::string s_manifestsPath;
    
    static bool s_reloadManifests = false;
    
    void ModifiedWatchPathDispatch(const std::string& path);
    void CreatedWatchPathDispatch(const std::string& path);
    void RemovedWatchPathDispatch(const std::string& path);
    
    void SetReloadManifests(const std::string& changedPath);
    void ReloadManifests();
    void BuildManifests();
    void NotifyAssetChanged();
    void PushBuildRequest(const std::string& changedPath);
    
    void Init(const std::string& assetManifestsPath,
              const std::string& assetTempPath,
              const std::string& flatbufferCompilerLocation)
    {
        s_flatBufferCompilerPath = flatbufferCompilerLocation;
        s_manifestsPath = assetManifestsPath;
        
        ReloadManifests();
        
        s_manifestFileWatcher.Init(std::chrono::milliseconds(1000), assetManifestsPath);
        //@TODO(Serge): modify these to be their own functions
        s_manifestFileWatcher.AddCreatedListener(SetReloadManifests);
        s_manifestFileWatcher.AddModifyListener(SetReloadManifests);
        s_manifestFileWatcher.AddRemovedListener(SetReloadManifests);
        
        r2::asset::pln::cmp::Init(assetTempPath);
    }
    
    void Update()
    {
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
    
    void ModifiedWatchPathDispatch(const std::string& path)
    {
        //R2_LOGI("Asset watcher - Modified path: %s\n", path.c_str());
        //Push some sort of command to a queue here
        PushBuildRequest(path);
    }
    
    void CreatedWatchPathDispatch(const std::string& path)
    {
        //R2_LOGI("Asset watcher - Created path: %s\n", path.c_str());
        //Push some sort of command to a queue here
        PushBuildRequest(path);
    }
    
    void RemovedWatchPathDispatch(const std::string& path)
    {
        R2_LOGI("Asset watcher - Removed path: %s\n", path.c_str());
        //Push some sort of command to a queue here
    }
    
    void SetReloadManifests(const std::string& changedPath)
    {
        s_reloadManifests = true;
    }
    
    void ReloadManifests()
    {
        BuildManifests();
        
        LoadAssetManifests(s_manifests, s_manifestsPath);
        s_reloadManifests = false;
    }
    
    void BuildManifests()
    {
        BuildAssetManifestsFromJson(s_manifestsPath);
    }
    
    void PushBuildRequest(const std::string& changedPath)
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
    
    void PushNewlyBuiltAssets(std::vector<std::string> paths)
    {
        for (AssetBuiltFunc& func : s_assetsBuiltListeners)
        {
            func(paths);
        }
    }
    
    void AddAssetBuiltFunction(AssetBuiltFunc func)
    {
        s_assetsBuiltListeners.push_back(func);
    }
}

#endif
