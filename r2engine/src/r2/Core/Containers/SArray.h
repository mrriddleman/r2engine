//
//  Array.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-18.
//

#ifndef Array_h
#define Array_h

#include "r2/Core/Memory/Memory.h"
#include <cassert>

#define MAKE_SARRAY(arena, T, capacity) r2::sarr::CreateSArray<T>(arena, capacity, __FILE__, __LINE__, "")
#define MAKE_SARRAY_VERBOSE(arena, T, capacity, file, line, desc) r2::sarr::CreateSArray<T>(arena, capacity, file, line, desc)
#define EMPLACE_SARRAY(ptr, T, capacity) r2::sarr::EmplaceSArray<T>(ptr, capacity);


namespace r2
{
    template <typename T>
    struct SArray
    {
        SArray();
        ~SArray();
    
        void DeallocArray(r2::mem::utils::NonPODType);
        
        void DeallocArray(r2::mem::utils::PODType);
        
        void DeallocArray();
        
        void Create(T* start, u64 capacity);
        
        //Copyable
        SArray(const SArray& other) = delete;
        SArray& operator=(const SArray& other);
        
        //Not moveable?
        SArray(SArray && other) = delete;
        SArray& operator=(SArray && other) = delete;
        
        T& operator[](u64 i);
        const T& operator[](u64 i) const;
        
        static u64 MemorySize(u64 capacity);
        
        u64 mSize = 0;
        u64 mCapacity = 0;
        T* mData = nullptr;
    };
    
    namespace sarr
    {
        template<typename T> inline u64 Size(const SArray<T>& arr);
        template<typename T> inline bool IsEmpty(const SArray<T>& arr);
        template<typename T> inline u64 Capacity(const SArray<T>& arr);
        template<typename T> inline bool HasRoom(const SArray<T>& arr);
        template<typename T> inline s32 RoomLeft(const SArray<T>& arr);

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
        template<typename T> inline void RemoveAndSwapWithLastElement(SArray<T>& arr, s64 index);
        
        template<typename T> inline T& At(SArray<T>& arr, u64 index);
        template<typename T> inline const T& At(const SArray<T>& arr, u64 index);
        
        template<typename T> inline void Clear(SArray<T>& arr);
        
        template<typename T> inline void Copy(SArray<T>& dst, const SArray<T>& src);
        template<typename T> inline void Append(SArray<T>& dst, const SArray<T>& newItems);
        template<typename T> inline void Fill(SArray<T>& dst, const T& val);

        template<typename T, class ARENA> r2::SArray<T>* CreateSArray(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description);
        template<typename T> r2::SArray<T>* EmplaceSArray(void* dataPtr, u64 capacity);
    
        template<typename T> inline s64 IndexOf(SArray<T>& dst, const T& val);

        template<typename T> inline void RemoveElementAtIndexShiftLeft(SArray<T>& dst, u64 index);

        template<typename T> inline bool Insert(SArray<T>& arr, s64 index, const T& val);
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

        template<typename T> inline bool HasRoom(const SArray<T>& arr)
        {
            return (static_cast<s64>(arr.mCapacity) - static_cast<s64>(arr.mSize)) > 0;
        }

        template<typename T> inline s32 RoomLeft(const SArray<T>& arr)
        {
            return static_cast<s32>(arr.mCapacity) - static_cast<s32>(arr.mSize);
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
            return arr.mData[arr.mSize - 1];
        }

        template<typename T> inline const T& Last(const SArray<T>& arr)
        {
            R2_CHECK(arr.mSize > 0, "The SArray is empty thus we don't have a last element!");
            return arr.mData[arr.mSize - 1];
        }

        template<typename T> inline void Push(SArray<T>& arr, const T& item)
        {
            R2_CHECK(arr.mSize + 1 <= arr.mCapacity, "We're at capacity!");
            arr.mData[arr.mSize++] = item;
        }

        template<typename T> inline void Pop(SArray<T>& arr)
        {
            if (arr.mSize > 0)
            {
                --arr.mSize;
            }
        }

        template<typename T> inline void RemoveAndSwapWithLastElement(SArray<T>& arr, s64 index)
        {
            R2_CHECK(index < arr.mSize, "You passed in an invalid index, you passed in: %llu but we only have %llu elements!", index, arr.mSize);

            if (arr.mSize <= 0)
                return;

            if ((arr.mSize - 1) == index)
            {
                --arr.mSize;
                return;
            }

            arr.mData[index] = arr.mData[arr.mSize - 1];
            --arr.mSize;
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

        template<typename T> inline void Copy(SArray<T>& dst, const SArray<T>& src)
        {
            R2_CHECK(dst.mCapacity >= src.mSize, "dst doesn't have the capacity needed to copy from src, dst's capacity is: %llu, src's size is: %llu", dst.mCapacity, src.mSize);
            memcpy(dst.mData, src.mData, sizeof(T) * src.mSize);
            dst.mSize = src.mSize;
        }

        template<typename T> inline void Append(SArray<T>& dst, const SArray<T>& newItems)
        {
            R2_CHECK(dst.mSize + newItems.mSize <= dst.mCapacity, "We don't have enough capacity in dst. Dst's capacity is: %llu, size: %llu, and src's size is: %llu", dst.mCapacity, dst.mSize, newItems.mSize);
            memcpy(r2::mem::utils::PointerAdd(dst.mData, dst.mSize * sizeof(T)), newItems.mData, sizeof(T) * newItems.mSize);
            dst.mSize += newItems.mSize;
        }

        template<typename T> inline void Fill(SArray<T>& dst, const T& val)
        {
            for (u32 i = 0; i < dst.mCapacity; ++i)
            {
                dst[i] = val;
            }
            dst.mSize = dst.mCapacity;
        }

        template<typename T> inline s64 IndexOf(SArray<T>& dst, const T& val)
        {
            s64 size = static_cast<s64>(r2::sarr::Size(dst));

            for (s64 i = 0; i < size; i++)
            {
                if (val == r2::sarr::At(dst, i))
                {
                    return i;
                }
            }

            return -1;
        }

        template<typename T, class ARENA> SArray<T>* CreateSArray(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description)
        {
            SArray<T>* array = new (ALLOC_BYTES(arena, SArray<T>::MemorySize(capacity), alignof(T), file, line, description)) SArray<T>();

            array->Create((T*)mem::utils::PointerAdd(array, sizeof(SArray<T>)), capacity);

            return array;
        }

        template<typename T> r2::SArray<T>* EmplaceSArray(void* dataPtr, u64 capacity)
        {
            SArray<T>* array = new (dataPtr) SArray<T>();

            array->Create((T*)mem::utils::PointerAdd(array, sizeof(SArray<T>)), capacity);

            return array;
        }

        template<typename T> inline bool Insert(SArray<T>& arr, s64 index, const T& val)
        {
            R2_CHECK(arr.mSize + 1 <= arr.mCapacity, "We're at capacity!");
            R2_CHECK(index >= 0 && index <= static_cast<s64>(arr.mSize), "Index is out of range: %ill", index);

            for (s64 i = arr.mSize - 1; i >= index; --i)
            {
                arr.mData[i + 1] = arr.mData[i];
            }

            arr.mData[index] = val;
            arr.mSize++;
            return true;
        }

        template<typename T> inline void RemoveElementAtIndexShiftLeft(SArray<T>& dst, u64 index)
        {
            for (u64 i = index + 1; i < r2::sarr::Size(dst); ++i)
            {
                dst.mData[i - 1] = dst.mData[i];
            }

            --dst.mSize;
        }
        
    }

    template <typename T>
    SArray<T>::SArray()
    : mSize(0)
    , mCapacity(0)
    , mData(nullptr)
    {
        
    }
    
    template<typename T>
    void SArray<T>::DeallocArray(r2::mem::utils::NonPODType)
    {

        for(u64 i = mCapacity; i > 0; --i)
        {
            mData[i-1].~T();
        }

    }
    
    template<typename T>
    void SArray<T>::DeallocArray(r2::mem::utils::PODType)
    {
        
    }
    
    template<typename T>
    void SArray<T>::DeallocArray()
    {
        DeallocArray(r2::mem::utils::IntToType<r2::mem::utils::IsPOD<T>::Value>());
    }
    
    
    template <typename T>
    SArray<T>::~SArray()
    {
        DeallocArray();
        mData = nullptr;
        mCapacity = 0;
        mSize = 0;
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
    
    template <typename T>
    u64 SArray<T>::MemorySize(u64 capacity)
    {
        return sizeof(SArray<T>) + capacity * sizeof(T);
    }
    
}

#endif /* Array_h */
