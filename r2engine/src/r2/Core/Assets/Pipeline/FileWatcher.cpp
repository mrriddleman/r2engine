//
//  FileWatcher.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-07.
//

#ifdef R2_ASSET_PIPELINE
#include "FileWatcher.h"

namespace r2::asset::pln
{
    FileWatcher::FileWatcher()
    {
        
    }
    
    FileWatcher::~FileWatcher()
    {
        
    }
    
    void FileWatcher::Init(Milliseconds delay, const std::string& pathToWatch)
    {
        bool isDirectory = std::filesystem::is_directory(pathToWatch);
        R2_CHECK(isDirectory, "Path should be a directory!");
        
        mDelay = delay;
        mPathToWatch = pathToWatch;
        
        std::filesystem::path aPath(pathToWatch);
        
        for (auto& file : std::filesystem::recursive_directory_iterator(aPath))
        {
            mPaths[file.path().string()] = std::filesystem::last_write_time(file);
        }
        
        mLastTime = std::chrono::steady_clock::now();
    }
    
    void FileWatcher::Run()
    {
        auto now = std::chrono::steady_clock::now();
        auto dt = std::chrono::duration_cast<Milliseconds>(now - mLastTime);
        //if(dt >= mDelay)
        {
            mLastTime = now;
            auto it = mPaths.begin();
            while(it != mPaths.end())
            {
                if (!std::filesystem::is_directory(it->first) && !std::filesystem::exists(it->first))
                {
                    for (auto listener : mRemovedListeners)
                    {
                        listener(it->first);
                    }
                    
                    it = mPaths.erase(it);
                }
                else
                {
                    it++;
                }
            }
            
            for (auto& file: std::filesystem::recursive_directory_iterator(mPathToWatch))
            {
                if(std::filesystem::is_directory(file.path().string()))
                {
                    continue;
                }
                
                auto currentFileLastWriteTime = std::filesystem::last_write_time(file);
                
                if (!Contains(file.path().string()))
                {
                    mPaths[file.path().string()] = currentFileLastWriteTime;
                    
                    for (auto listener : mCreatedListeners)
                    {
                        listener(file.path().string());
                    }
                }
                else
                {
                    if (mPaths[file.path().string()] != currentFileLastWriteTime)
                    {
                        mPaths[file.path().string()] = currentFileLastWriteTime;
                        for (auto listener : mModifierListeners)
                        {
                           listener(file.path().string());
                        }
                    }
                }
            }
        }
    }
    
    void FileWatcher::AddModifyListener(ModifiedFunc modFunc)
    {
        mModifierListeners.push_back(modFunc);
    }
    
    void FileWatcher::AddCreatedListener(CreatedFunc creatFunc)
    {
        mCreatedListeners.push_back(creatFunc);
    }
    
    void FileWatcher::AddRemovedListener(RemovedFunc removeFunc)
    {
        mRemovedListeners.push_back(removeFunc);
    }
    
    bool FileWatcher::Contains(const std::string &key)
    {
        auto timeEntry = mPaths.find(key);
        return timeEntry != mPaths.end();
    }
}
#endif
