//
//  FileWatcher.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-07.
//

#ifndef FileWatcher_h
#define FileWatcher_h

#include "filesystem"
#include <string>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <vector>

namespace r2::asset::pln
{
    using Milliseconds = std::chrono::duration<int, std::milli>;
    using ModifiedFunc = std::function<void (const std::string& path)>;
    using CreatedFunc  = std::function<void (const std::string& path)>;
    using RemovedFunc  = std::function<void (const std::string& path)>;
    
    class FileWatcher
    {
    public:
        FileWatcher();
        ~FileWatcher();
        void Init(Milliseconds delay, const std::string& pathToWatch);
        void Run();
        
        void AddModifyListener(ModifiedFunc modFunc);
        void AddCreatedListener(CreatedFunc creatFunc);
        void AddRemovedListener(RemovedFunc removeFunc);
        
    private:
        
        bool Contains(const std::string &key);
        
        Milliseconds mDelay;
        std::string mPathToWatch;
        std::chrono::steady_clock::time_point mLastTime;
        std::unordered_map<std::string, std::filesystem::file_time_type> mPaths;
        std::vector<ModifiedFunc> mModifierListeners;
        std::vector<CreatedFunc> mCreatedListeners;
        std::vector<RemovedFunc> mRemovedListeners;
    };
}

#endif /* FileWatcher_h */
