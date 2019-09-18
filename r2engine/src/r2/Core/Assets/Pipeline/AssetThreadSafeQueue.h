//
//  AssetThreadSafeQueue.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-09-17.
//

#ifndef AssetThreadSafeQueue_h
#define AssetThreadSafeQueue_h

#include <queue>
#include <mutex>
#include <condition_variable>

namespace r2::asset::pln
{
    template<typename T>
    class AssetThreadSafeQueue
    {
    private:
        mutable std::mutex mMut;
        std::queue<T> mDataQ;
        std::condition_variable mDataCond;
        
    public:
        AssetThreadSafeQueue() {}
        
        AssetThreadSafeQueue(const AssetThreadSafeQueue& other)
        {
            std::lock_guard<std::mutex> lk(other.mMut);
            mDataQ = other.mDataQ;
        }
        
        AssetThreadSafeQueue& operator=(const AssetThreadSafeQueue& queue) = delete;
        
        void Push(T newValue)
        {
            std::lock_guard<std::mutex> lk(mMut);
            mDataQ.push(newValue);
            mDataCond.notify_one();
        }
        
        bool TryPop(T& value)
        {
            std::lock_guard<std::mutex> lk(mMut);
            if(mDataQ.empty())
                return false;
            value = mDataQ.front();
            mDataQ.pop();
            return true;
        }
        
        void WaitAndPop(T& value)
        {
            std::unique_lock<std::mutex> lk(mMut);
            mDataCond.wait(lk, [this]{return !mDataQ.empty();});
            value = mDataQ.front();
            mDataQ.pop();
        }
        
        bool Empty() const
        {
            std::lock_guard<std::mutex> lk(mMut);
            return mDataQ.empty();
        }
    };
}

#endif /* AssetThreadSafeQueue_h */
