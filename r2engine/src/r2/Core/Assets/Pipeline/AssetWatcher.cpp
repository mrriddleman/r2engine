//
//  AssetWatcher.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-08.
//

#include "AssetWatcher.h"
#include "r2/Core/Assets/Pipeline/AssetManifest.h"

namespace r2::asset::pln
{
    static std::vector<FileWatcher> s_fileWatchers;
    static std::vector<AssetManifest> s_manifests;
    static std::vector<AssetChangedFunc> s_assetChangedListeners;
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
    
    void Init(const std::string& assetManifestsPath,
              const std::string& flatbufferCompilerLocation)
    {
        s_flatBufferCompilerPath = flatbufferCompilerLocation;
        s_manifestsPath = assetManifestsPath;
        
        BuildManifests();
        ReloadManifests();
        
        s_manifestFileWatcher.Init(std::chrono::milliseconds(1000), assetManifestsPath);
        //@TODO(Serge): modify these to be their own functions
        s_manifestFileWatcher.AddCreatedListener(SetReloadManifests);
        s_manifestFileWatcher.AddModifyListener(SetReloadManifests);
        s_manifestFileWatcher.AddRemovedListener(SetReloadManifests);
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
        R2_LOGI("Asset watcher - Modified path: %s\n", path.c_str());
        //Push some sort of command to a queue here
    }
    
    void CreatedWatchPathDispatch(const std::string& path)
    {
        R2_LOGI("Asset watcher - Created path: %s\n", path.c_str());
        //Push some sort of command to a queue here
    }
    
    void RemovedWatchPathDispatch(const std::string& path)
    {
        R2_LOGI("Asset watcher - Removed path: %s\n", path.c_str());
        //Push some sort of command to a queue here
    }
    
    void AddAssetChangedListener(AssetChangedFunc func)
    {
        s_assetChangedListeners.push_back(func);
    }
    
    void SetReloadManifests(const std::string& changedPath)
    {
        s_reloadManifests = true;
    }
    
    void ReloadManifests()
    {
        LoadAssetManifests(s_manifests, s_manifestsPath);
        s_reloadManifests = false;
    }
    
    void BuildManifests()
    {
        BuildAssetManifestsFromJson(s_manifestsPath);
    }
}
