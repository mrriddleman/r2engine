//
//  SQueue.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-20.
//

#ifndef SQueue_h
#define SQueue_h

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"

#define MAKE_SQUEUE(arena, T, capacity) r2::squeue::CreateSQueue<T>(arena, capacity, __FILE__, __LINE__, "")

namespace r2
{
    template <typename T> struct SQueue
    {
        SQueue(): mData(nullptr), mSize(0), mOffset(0){}
        ~SQueue();
        
        void Create(SArray<T>* arrayStart, T* dataStart, u64 capacity);
        
        SQueue(const SQueue& other) = delete;
        SQueue& operator=(const SQueue& other);
        
        SQueue(SQueue && other) = delete;
        SQueue& operator=(SQueue && other) = delete;
        
        T& operator[](u64 index);
        const T& operator[](u64 index) const;
        
        u64 mSize;
        u64 mOffset;
        SArray<T>* mData;
    };
    
    namespace squeue
    {
        template <typename T> inline u64 Size(const SQueue<T> & q);
        
        template <typename T> inline u64 Space(const SQueue<T>& q);
        
        template <typename T> inline void PushBack(SQueue<T> & q, const T& item);
        
        template <typename T> inline void PopBack(SQueue<T>& q);
        
        template <typename T> inline void PushFront(SQueue<T>& q, const T& item);
        
        template <typename T> inline void PopFront(SQueue<T>&q);
        
        template <typename T> inline void Consume(SQueue<T>& q, u64 n);
        
        template <typename T> inline void Push(SQueue<T>& q, const T* items, u64 n);
        
        template <typename T> inline T* BeginFront(SQueue<T> & q);
        
        template <typename T> inline const T* BeginFront(const SQueue<T>& q);
        
        template <typename T> inline T* EndFront(SQueue<T>& q);
        
        template <typename T> inline const T* EndFront(const SQueue<T>&q);
        
        template<typename T, class ARENA> inline SQueue<T>* CreateSQueue(ARENA& a, u64 capacity, const char* file, s32 line, const char* description);
        
        template<typename T> inline T& First(SQueue<T>& q);
        template<typename T> inline const T& First(const SQueue<T>& q) ;
        
        template<typename T> inline T& Last(SQueue<T>&q);
        template<typename T> inline const T& Last(const SQueue<T>& q);
        template<typename T> inline void ConsumeAll(SQueue<T>& q);
        
        template<typename T> using SQueueCompareFunction = bool (*)(const T & a, const T& b);
        template<typename T> void Sort(SQueue<T>& q, SQueueCompareFunction<T> cmp);
        template<typename T> void MoveToFront(SQueue<T>& q, u64 index);
    }
    
    namespace squeue
    {
        template<typename T> inline u64 Size(const SQueue<T> & q)
        {
            return q.mSize;
        }
        
        template<typename T> inline u64 Space(const SQueue<T>& q)
        {
            return r2::sarr::Capacity(*q.mData) - q.mSize;
        }
        
        template<typename T> inline void PushBack(SQueue<T> & q, const T&item)
        {
            R2_CHECK(Space(q) > 0, "There isn't enough space to PushBack");
            
            q[q.mSize++] = item;
        }
        
        template<typename T> inline void PopBack(SQueue<T>& q)
        {
            --q.mSize;
        }
        
        template<typename T> inline void PushFront(SQueue<T> & q, const T& item)
        {
            R2_CHECK(Space(q) > 0, "Not enough space to PushFront");
            
            q.mOffset = (q.mOffset - 1 + r2::sarr::Capacity(*q.mData)) % r2::sarr::Capacity(*q.mData);
            ++q.mSize;
            q[0] = item;
        }
        
        template<typename T> inline void PopFront(SQueue<T>& q)
        {
            q.mOffset = (q.mOffset + 1) % r2::sarr::Capacity(*q.mData);
            --q.mSize;
        }
        
        template<typename T> inline void Consume(SQueue<T>& q, u64 n)
        {
            R2_CHECK(n <= q.mSize, "n must be less than or equal to q.mSize");
            
            q.mOffset = (q.mOffset + n) % r2::sarr::Capacity(*q.mData);
            q.mSize -= n;
        }
        
        template <typename T> void Push(SQueue<T>& q, const T* items, u64 n)
        {
            R2_CHECK(Space(q) >= n, "Space(q) must be >= n");
            
            const u64 size = r2::sarr::Capacity(*q.mData);
            const u64 insert = (q.mOffset + q.mSize) % size;
            u64 toInsert = n;
            
            if (insert + toInsert > size)
            {
                toInsert = size - insert;
            }
            
            memcpy(r2::sarr::Begin(*q.mData) + insert, items, toInsert + sizeof(T));
            q.mSize += toInsert;
            items += toInsert;
            n -= toInsert;
            memcpy(r2::sarr::Begin(*q.mData), items, n * sizeof(T));
            q.mSize += n;
        }
        
        template <typename T> inline T* BeginFront(SQueue<T> & q)
        {
            return r2::sarr::Begin(*q.mData) + q.mOffset;
        }
        
        template <typename T> inline const T* BeginFront(const SQueue<T>& q)
        {
            return r2::sarr::Begin(*q.mData) + q.mOffset;
        }
        
        template<typename T> inline T* EndFront(SQueue<T>& q)
        {
            u64 end = q.mOffset + q.mSize;
            return end > r2::sarr::Capacity(*q.mData) ? r2::sarr::End(*q.mData) : r2::sarr::Begin(*q.mData) + end;
        }
        
        template <typename T> inline const T* EndFront(const SQueue<T>& q)
        {
            u64 end = q.mOffset + q.mSize;
            return end > r2::sarr::Capacity(*q.mData) ? r2::sarr::End(*q.mData) : r2::sarr::Begin(*q.mData) + end;
        }
        
        template<typename T> T& First(SQueue<T>& q)
        {
            R2_CHECK(q.mSize > 0, "Queue size must be greater than 0 in order to get the first element");
            return q[0];
        }
        
        template<typename T> const T& First(const SQueue<T>& q)
        {
            R2_CHECK(q.mSize > 0, "Queue size must be greater than 0 in order to get the first element");
            return q[0];
        }
        
        template<typename T> T& Last(SQueue<T>&q)
        {
            R2_CHECK(q.mSize > 0, "Queue size must be greater than 0 in order to get the last element");
            return q[q.mSize-1];
        }
        
        template<typename T> const T& Last(const SQueue<T>& q)
        {
            R2_CHECK(q.mSize > 0, "Queue size must be greater than 0 in order to get the last element");
            return q[q.mSize-1];
        }
        
        template<typename T, class ARENA> SQueue<T>* CreateSQueue(ARENA& a, u64 capacity, const char* file, int line, const char* description)
        {
            SQueue<T>* q = new (ALLOC_BYTES(a, sizeof(SQueue<T>) + sizeof(SArray<T>) + sizeof(T)*capacity, alignof(T), file, line, description)) SQueue<T>;
            
            SArray<T>* startOfArray = new (r2::mem::utils::PointerAdd(q, sizeof(SQueue<T>))) SArray<T>();
            
            T* dataStart = (T*)r2::mem::utils::PointerAdd(startOfArray, sizeof(SArray<T>));
            
            q->Create(startOfArray, dataStart, capacity);

            return q;
        }

        template<typename T> void ConsumeAll(SQueue<T>& q)
        {
            q.mOffset = 0;
            q.mSize = 0;
        }
        
        template<typename T> void Sort(SQueue<T>& q, SQueueCompareFunction<T> cmp)
        {
            std::sort(BeginFront(q), EndFront(q), cmp);
        }
        
        template<typename T> void MoveToFront(SQueue<T>& q, u64 index)
        {
            s64 prevIndex = index -1;
            while (prevIndex >= 0)
            {
                std::swap(q[prevIndex], q[prevIndex+1]);
                --prevIndex;
            }
        }
    }
    
    template<typename T>
    SQueue<T>::~SQueue()
    {
        mData->~SArray<T>();
        mData = nullptr;
        mSize = 0;
        mOffset = 0;
    }
    
    template<typename T>
    void SQueue<T>::Create(SArray<T>* arrayStart, T* dataStart, u64 capacity)
    {
        R2_CHECK(arrayStart != nullptr, "arrayStart should never be nullptr");
        R2_CHECK(dataStart != nullptr, "dataStart should never be nullptr");
        
        mData = arrayStart;
        mData->Create(dataStart, capacity);
        mSize = 0;
        mOffset = 0;
    }
    
    template<typename T>
    SQueue<T>& SQueue<T>::operator=(const SQueue& other)
    {
        if (this == &other)
        {
            return *this;
        }
        
        R2_CHECK(r2::sarr::Capacity(*other.mData) == r2::sarr::Capacity(*mData), "We must have the same capacith in order to copy these!");
        
        *mData = *other.mData;
        mOffset = other.mOffset;
        mSize = other.mSize;
    }
    
    template<typename T>
    T& SQueue<T>::operator[](u64 index)
    {
        R2_CHECK(mData && index >= 0 && index < r2::sarr::Capacity(*mData), "i: %llu is out of bounds in this array", index);
        return (*mData)[(index + mOffset) % r2::sarr::Capacity(*mData)];
    }
    
    template<typename T>
    const T& SQueue<T>::operator[](u64 index) const
    {
        R2_CHECK(mData && index >= 0 && index < r2::sarr::Capacity(*mData), "i: %llu is out of bounds in this array", index);
        return (*mData)[(index + mOffset) % r2::sarr::Capacity(*mData)];
    }
}

#endif /* SQueue_h */
