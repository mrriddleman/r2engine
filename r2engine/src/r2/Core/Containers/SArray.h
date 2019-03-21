//
//  Array.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-18.
//

#ifndef Array_h
#define Array_h

#include "r2/Core/Memory/Memory.h"

#define MAKE_SARRAY(arena, T, capacity) r2::sarr::CreateSArray<T>(arena, capacity, __FILE__, __LINE__, "")

namespace r2
{
    template <typename T>
    struct SArray
    {
        SArray();
        ~SArray();
    
        void Create(T* start, u64 capacity);
        
        //Copyable
        SArray(const SArray& other) = delete;
        SArray& operator=(const SArray& other);
        
        //Not moveable?
        SArray(SArray && other) = delete;
        SArray& operator=(SArray && other) = delete;
        
        T& operator[](u64 i);
        const T& operator[](u64 i) const;
        
        u64 mSize = 0;
        u64 mCapacity = 0;
        T* mData = nullptr;
    };
    
    namespace sarr
    {
        template<typename T> inline u64 Size(const SArray<T>& arr);
        template<typename T> inline bool IsEmpty(const SArray<T>& arr);
        template<typename T> inline u64 Capacity(const SArray<T>& arr);
        
        template<typename T> inline T* Begin(SArray<T>& arr);
        template<typename T> inline const T* Begin(const SArray<T>& arr);
        template<typename T> inline T* End(SArray<T>& arr);
        template<typename T> inline const T* End(const SArray<T>& arr);
        
        template<typename T> inline T& First(SArray<T>& arr);
        template<typename T> inline const T& First(const SArray<T>& arr);
        template<typename T> inline T& Last(SArray<T>& arr);
        template<typename T> inline const T& Last(const SArray<T>& arr);
        
        template<typename T> inline void Push(SArray<T>& arr, const T& item);
        template<typename T> inline void Pop(SArray<T>& arr);
        
        template<typename T> inline T& At(SArray<T>& arr, u64 index);
        template<typename T> inline const T& At(const SArray<T>& arr, u64 index);
        
        template<typename T> inline void Clear(SArray<T>& arr);
        
        template<typename T, class ARENA> r2::SArray<T>* CreateSArray(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description);
    }
    
    namespace sarr
    {
        template<typename T> inline u64 Size(const SArray<T>& arr)
        {
            return arr.mSize;
        }
        
        template<typename T> inline bool IsEmpty(const SArray<T>& arr)
        {
            return arr.mSize == 0;
        }
        
        template<typename T> inline u64 Capacity(const SArray<T>& arr)
        {
            return arr.mCapacity;
        }
        
        template<typename T> inline T* Begin(SArray<T>& arr)
        {
            return arr.mData;
        }
        
        template<typename T> inline const T* Begin(const SArray<T>& arr)
        {
            return arr.mData;
        }
        
        template<typename T> inline T* End(SArray<T>& arr)
        {
            return arr.mData + arr.mSize;
        }
        
        template<typename T> inline const T* End(const SArray<T>& arr)
        {
            return arr.mData + arr.mSize;
        }
        
        template<typename T> inline T& First(SArray<T>& arr)
        {
            R2_CHECK(arr.mSize > 0, "The SArray is empty thus we don't have a first element!");
            return arr.mData[0];
        }
        
        template<typename T> inline const T& First(const SArray<T>& arr)
        {
            R2_CHECK(arr.mSize > 0, "The SArray is empty thus we don't have a first element!");
            return arr.mData[0];
        }
        
        template<typename T> inline T& Last(SArray<T>& arr)
        {
            R2_CHECK(arr.mSize > 0, "The SArray is empty thus we don't have a last element!");
            return arr.mData[arr.mSize-1];
        }
        
        template<typename T> inline const T& Last(const SArray<T>& arr)
        {
            R2_CHECK(arr.mSize > 0, "The SArray is empty thus we don't have a last element!");
            return arr.mData[arr.mSize-1];
        }
        
        template<typename T> inline void Push(SArray<T>& arr, const T& item)
        {
            R2_CHECK(arr.mSize + 1 <= arr.mCapacity, "We're at capacity!");
            arr.mData[arr.mSize++] = item;
        }
        
        template<typename T> inline void Pop(SArray<T>& arr)
        {
            if(arr.mSize > 0)
            {
                --arr.mSize;
            }
        }
        
        template<typename T> inline T& At(SArray<T>& arr, u64 index)
        {
            return arr[index];
        }
        
        template<typename T> inline const T& At(const SArray<T>& arr, u64 index)
        {
            return arr[index];
        }
        
        template<typename T> inline void Clear(SArray<T>& arr)
        {
            arr.mSize = 0;
        }
        
        template<typename T, class ARENA> SArray<T>* CreateSArray(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description)
        {
            SArray<T>* array = new (ALLOC_BYTES(arena, sizeof(SArray<T>) + sizeof(T)*capacity, alignof(T), file, line, description)) SArray<T>();
            
            array->Create((T*)mem::utils::PointerAdd(array, sizeof(SArray<T>)), capacity);
            
            return array;
        }
    }

    template <typename T>
    SArray<T>::SArray()
    : mSize(0)
    , mCapacity(0)
    , mData(nullptr)
    {
        
    }
    
    template <typename T>
    SArray<T>::~SArray()
    {
        if(!std::is_pod<T>::value)
        {
            for(u64 i = mCapacity; i > 0; --i)
                mData[i-1].~T();
        }
    }
    
    template<typename T>
    SArray<T>& SArray<T>::operator=(const SArray<T>& other)
    {
        if(this == &other)
        {
            return *this;
        }
        
        R2_CHECK(this->mCapacity == other.mCapacity, "Can't copy SArray's that have different capcities");
        
        for (u64 i = 0; i < other.mSize; ++i)
        {
            mData[i] = other.mData[i];
        }
        
        mSize = other.mSize;
        
        return *this;
    }
    
    template <typename T>
    void SArray<T>::Create(T* start, u64 capacity)
    {
        mData = start;
        mSize = 0;
        mCapacity = capacity;
        
        if(!std::is_pod<T>::value)
        {
            for(u64 i = 0; i < mCapacity; i++)
            {
                new (&mData[i]) T;
            }
        }
    }
    
    template <typename T>
    T& SArray<T>::operator[](u64 i)
    {
        R2_CHECK(i >= 0 && i < mCapacity, "i: %llu is out of bounds in this array", i);
        return mData[i];
    }
    
    template <typename T>
    const T& SArray<T>::operator[](u64 i) const
    {
        R2_CHECK(i >= 0 && i < mCapacity, "i: %llu is out of bounds in this array", i);
        return mData[i];
    }
    
}

#endif /* Array_h */
