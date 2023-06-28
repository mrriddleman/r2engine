//
//  SHashMap.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-25.
//

#ifndef SHashMap_h
#define SHashMap_h

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"
#include <limits>

#define MAKE_SHASHMAP(arena, T, capacity) r2::shashmap::CreateSHashMap<T>(arena, capacity, __FILE__, __LINE__, "")
#define MAKE_SHASHMAP_IN_PLACE(T, placement, capacity) r2::shashmap::CreateHashMapInPlace<T>(placement, capacity)
namespace
{
    const f64 MAX_LOAD_FACTOR = 0.75;
    const f64 LOAD_FACTOR_MULT = 1.5;
}

namespace r2
{
    template<typename T>
    struct SHashMap
    {
        struct HashMapEntry
        {
            u64 key = 0;
            u64 next = 0;
            T value{};
        };
        
        SHashMap(): mCapacity(0), mHash(nullptr), mData(nullptr){}
        ~SHashMap();
        void Create(SArray<u64>* hashStart, u64* hashDataStart, SArray<HashMapEntry>* dataArrayStart, HashMapEntry* dataStart, u64 capacity);
        
        SHashMap(const SHashMap& other) = delete;
        SHashMap& operator=(const SHashMap& other) = delete;
        
        SHashMap(SHashMap && other) = delete;
        SHashMap& operator=(SHashMap && other) = delete;
        
        static u64 MemorySize(u64 capacity);
        static u64 FullAtCapacity(u64 capcity);
        static f64 LoadFactor()
        {
            return MAX_LOAD_FACTOR;
        }
        static f64 LoadFactorMultiplier()
        {
            return LOAD_FACTOR_MULT;
        }


        u64 mCapacity;
        SArray<u64>* mHash;
        SArray<HashMapEntry>* mData;
    };
    
    namespace shashmap
    {
        template<typename T> bool Has(const SHashMap<T>& h, u64 key);
        
        template<typename T> T& Get(SHashMap<T>& h, u64 key, T& theDefault);

        template<typename T> T& Get(SHashMap<T>& h, u64 key, T& theDefault, bool& success); //success == true if it's not default
        
        template<typename T> const T& Get(const SHashMap<T>& h, u64 key, const T& theDefault);
        
        template<typename T> void Set(SHashMap<T>& h, u64 key, const T& value);
        
        template<typename T> void Remove(SHashMap<T>& h, u64 key);
        
        template<typename T> void Clear(SHashMap<T>& h);
        
        template<typename T> const typename SHashMap<T>::HashMapEntry* Begin(const SHashMap<T>& h);
        
        template<typename T> const typename SHashMap<T>::HashMapEntry* End(const SHashMap<T>& h);

		template<typename T> typename SHashMap<T>::HashMapEntry* Begin(SHashMap<T>& h);

		template<typename T> typename SHashMap<T>::HashMapEntry* End(SHashMap<T>& h);

        template<typename T, class ARENA> SHashMap<T>* CreateSHashMap(ARENA& a, u64 capacity, const char* file, s32 line, const char* description);
    
        template<typename T> SHashMap<T>* CreateHashMapInPlace(void* hashMapPlacement, u64 capacity);

        template<typename T> bool IsFull(SHashMap<T>& h);
    
    }
    
    namespace smultihash
    {
        template<typename T> const typename SHashMap<T>::HashMapEntry* FindFirst(const SHashMap<T>& h, u64 key);
        
        template<typename T> const typename SHashMap<T>::HashMapEntry* FindNext(const SHashMap<T>& h, const typename SHashMap<T>::HashMapEntry* e);
        
        template<typename T> u64 Count(const SHashMap<T>& h, u64 key);
        
        template<typename T> void Get(const SHashMap<T>& h, u64 key, SArray<T>& items);
        
        template<typename T> void Insert(SHashMap<T>& h, u64 key, const T& value);
        
        template<typename T> void Remove(SHashMap<T>& h, const typename SHashMap<T>::HashMapEntry* e);
        
        template<typename T> void RemoveAll(SHashMap<T>& h, u64 key);
    }
    
    namespace hashmap_internal
    {
        const u64 END_OF_LIST = std::numeric_limits<std::size_t>::max();
        
        struct FindResult
        {
            u64 hash_i;
            u64 data_prev;
            u64 data_i;
        };
        
        template<typename T> u64 AddEntry(SHashMap<T>& h, u64 key)
        {
            typename SHashMap<T>::HashMapEntry e;
            
            e.key = key;
            e.next = END_OF_LIST;
            u64 ei = r2::sarr::Size(*h.mData);
            r2::sarr::Push(*h.mData, e);
            return ei;
        }
        
        template<typename T> FindResult Find(const SHashMap<T>& h, u64 key)
        {
            FindResult fr;
            
            fr.hash_i = END_OF_LIST;
            fr.data_prev = END_OF_LIST;
            fr.data_i = END_OF_LIST;
            
            u64 size = h.mHash->mSize;
            if (size == 0)
            {
                R2_CHECK(false, "");
            }
            
            fr.hash_i = key % size;
            fr.data_i = (*h.mHash)[fr.hash_i];
            
            while (fr.data_i != END_OF_LIST)
            {
                if (((*h.mData)[fr.data_i]).key == key)
                {
                    return fr;
                }
                
                fr.data_prev = fr.data_i;
                fr.data_i = ((*h.mData)[fr.data_i]).next;
            }
            return fr;
            
        }
        
        template<typename T> FindResult Find(const SHashMap<T>& h, const typename SHashMap<T>::HashMapEntry * e)
        {
            FindResult fr;
            
            fr.hash_i = END_OF_LIST;
            fr.data_prev = END_OF_LIST;
            fr.data_i = END_OF_LIST;

            u64 size = h.mHash->mSize;
            if (size == 0)
            {
                R2_CHECK(false, "");
            }
            
            fr.hash_i = e->key % size;
            fr.data_i = (*h.mHash)[fr.hash_i];
            
            while (fr.data_i != END_OF_LIST)
            {
                if (&((*h.mData)[fr.data_i]) == e)
                {
                    return fr;
                }
                
                fr.data_prev = fr.data_i;
                fr.data_i = ((*h.mData)[fr.data_i]).next;
            }
            
            return fr;
        }
        
        template<typename T> void Erase(SHashMap<T>& h, const FindResult& fr)
        {
            if (fr.data_prev == END_OF_LIST)
            {
                (*h.mHash)[fr.hash_i] = ((*h.mData)[fr.data_i]).next;
            }
            else
            {
                ((*h.mData)[fr.data_prev]).next = ((*h.mData)[fr.data_i]).next;
            }

            s64 size = static_cast<s64>(h.mData->mSize);
            if (fr.data_i == size - 1)
            { 
                r2::sarr::Pop(*h.mData);

                return;
            }
            
            (*h.mData)[fr.data_i] = (*h.mData)[size-1];
            
            FindResult last = Find(h, h.mData->mData[fr.data_i].key);
            
            if (last.data_prev != END_OF_LIST)
            {
                ((*h.mData)[last.data_prev]).next = last.data_i;
            }
            else
            {
                (*h.mHash)[last.hash_i] = last.data_i;
            }
        }
        
        template<typename T> u64 FindOrFail(const SHashMap<T>& h, u64 key)
        {
            return Find(h, key).data_i;
        }
        
        template<typename T> u64 FindOrMake(SHashMap<T>& h, u64 key)
        {
            const FindResult fr = Find(h, key);
            
            if (fr.data_i != END_OF_LIST)
            {
                return fr.data_i;
            }
            
            u64 i = AddEntry(h, key);
            
            if (fr.data_prev == END_OF_LIST)
            {
                (*h.mHash)[fr.hash_i] = i;
            }
            else
            {
                ((*h.mData)[fr.data_prev]).next = i;
            }

            return i;
        }
        
        template<typename T> u64 Make(SHashMap<T>& h, u64 key)
        {
            const FindResult fr = Find(h, key);
            
            const u64 i = AddEntry(h, key);
            
            if (fr.data_prev == END_OF_LIST)
            {
                ((*h.mHash)[fr.hash_i]) = i;
            }
            else
            {
                ((*h.mData)[fr.data_prev]).next = i;
            }
             
            ((*h.mData)[i]).next = fr.data_i;

            return i;
        }
        
        template<typename T> void FindAndErase(SHashMap<T>& h, size_t key)
        {
            const FindResult fr = Find(h, key);
            
            if (fr.data_i != END_OF_LIST)
            {
                Erase(h, fr);
            }
        }
        
        template<typename T> bool IsFull(const SHashMap<T>& h)
        {
            const float maxLoadFactor = MAX_LOAD_FACTOR;
            
            return r2::sarr::Size(*h.mData) >= h.mCapacity * maxLoadFactor;
        }
        
    }
    
    namespace shashmap
    {
        template<typename T> bool Has(const SHashMap<T>& h, u64 key)
        {
            return hashmap_internal::FindOrFail(h, key) != hashmap_internal::END_OF_LIST;
        }
        
        template<typename T> T& Get(SHashMap<T>& h, u64 key, T& theDefault)
        {
            const u64 i = hashmap_internal::FindOrFail(h, key);
            return i == hashmap_internal::END_OF_LIST ? theDefault : (*h.mData)[i].value;
        }


        template<typename T> T& Get(SHashMap<T>& h, u64 key, T& theDefault, bool& success) //success == true if it's not default
        {
            success = false;

			const u64 i = hashmap_internal::FindOrFail(h, key);

            if (i == hashmap_internal::END_OF_LIST)
            {
                return theDefault;
            }

            success = true;

            return (*h.mData)[i].value;
		}
        
        template<typename T> const T& Get(const SHashMap<T>& h, u64 key, const T& theDefault)
        {
            const u64 i = hashmap_internal::FindOrFail(h, key);
            return i == hashmap_internal::END_OF_LIST ? theDefault : (*h.mData)[i].value;
        }
        
        template<typename T> void Set(SHashMap<T>& h, u64 key, const T & value)
        {
            R2_CHECK(!hashmap_internal::IsFull(h), "The HashMap should not be full!");
            
            const u64 i = hashmap_internal::FindOrMake(h, key);
            ((*h.mData)[i]).value = value;
        }
        
        template<typename T> void Remove(SHashMap<T>& h, u64 key)
        {
            hashmap_internal::FindAndErase(h, key);
        }
        
        template<typename T> void Clear(SHashMap<T>& h)
        {
            r2::sarr::Clear(*h.mData);
            
            h.mHash->mSize = h.mCapacity;
            
            for (u64 i = 0; i < h.mCapacity; ++i)
            {
                (*h.mHash)[i] = hashmap_internal::END_OF_LIST;
            }
        }
        
        template<typename T> const typename SHashMap<T>::HashMapEntry * Begin(const SHashMap<T>& h)
        {
            return r2::sarr::Begin(*h.mData);
        }
        
        template<typename T> const typename SHashMap<T>::HashMapEntry * End(const SHashMap<T>& h)
        {
            return r2::sarr::End(*h.mData);
        }

		template<typename T> typename SHashMap<T>::HashMapEntry* Begin(SHashMap<T>& h)
		{
			return r2::sarr::Begin(*h.mData);
		}

		template<typename T> typename SHashMap<T>::HashMapEntry* End(SHashMap<T>& h)
		{
			return r2::sarr::End(*h.mData);
		}

        template<typename T, class ARENA> SHashMap<T>* CreateSHashMap(ARENA& a, u64 capacity, const char* file, s32 line, const char* description)
        {
            const auto memSize = SHashMap<T>::MemorySize(capacity);
            void* hashMapPlacement = ALLOC_BYTES(a, memSize, alignof(T), file, line, description);

            return CreateHashMapInPlace<T>(hashMapPlacement, capacity);
        }

        template<typename T> SHashMap<T>* CreateHashMapInPlace(void* hashMapPlacement, u64 capacity)
        {
            SHashMap<T>* h = new (hashMapPlacement) SHashMap<T>();

			SArray<u64>* startOfHashArray = new(r2::mem::utils::PointerAdd(h, sizeof(SHashMap<T>))) SArray<u64>();

			u64* hashArrayDataStart = (u64*)r2::mem::utils::PointerAdd(startOfHashArray, sizeof(SArray<u64>));

			SArray<typename SHashMap<T>::HashMapEntry>* startOfDataArray = new(r2::mem::utils::PointerAdd(hashArrayDataStart, sizeof(u64) * capacity)) SArray<typename SHashMap<T>::HashMapEntry>();

			typename SHashMap<T>::HashMapEntry* dataStart = (typename SHashMap<T>::HashMapEntry*)r2::mem::utils::PointerAdd(startOfDataArray, sizeof(SArray<typename SHashMap<T>::HashMapEntry>));

			h->Create(startOfHashArray, hashArrayDataStart, startOfDataArray, dataStart, capacity);

			return h;
        }

        template<typename T> bool IsFull(SHashMap<T>& h)
        {
            return hashmap_internal::IsFull(h);
        }
    }
    
    namespace smultihash
    {
        template<typename T> const typename SHashMap<T>::HashMapEntry * FindFirst(const SHashMap<T>& h, u64 key)
        {
            const u64 i = hashmap_internal::FindOrFail(h, key);
            return i == hashmap_internal::END_OF_LIST ? nullptr : &((*h.mData)[i]);
        }
        
        template<typename T> const typename SHashMap<T>::HashMapEntry * FindNext(const SHashMap<T>& h, const typename SHashMap<T>::HashMapEntry * e)
        {
            u64 i = e->next;
            
            while (i != hashmap_internal::END_OF_LIST)
            {
                if ((*h.mData)[i].key == e->key)
                {
                    return &((*h.mData)[i]);
                }
                
                i = ((*h.mData)[i]).next;
            }
            
            return nullptr;
        }
        
        template<typename T> u64 Count(const SHashMap<T>& h, u64 key)
        {
            u64 i = 0;
            
            const typename SHashMap<T>::HashMapEntry * e = FindFirst(h, key);
            
            while (e)
            {
                ++i;
                e = FindNext(h, e);
            }
            
            return i;
        }
        
        template<typename T> void Get(const SHashMap<T>& h, u64 key, SArray<T>& items)
        {
            const typename SHashMap<T>::HashMapEntry * e = FindFirst(h, key);
            
            while (e)
            {
                r2::sarr::Push(items, e->value);
                e = FindNext(h, e);
            }
        }
        
        template<typename T> void Insert(SHashMap<T>& h, u64 key, const T& value)
        {
            R2_CHECK(!hashmap_internal::IsFull(h), "Hash Map should not be full!");
            
            const u64 i = hashmap_internal::Make(h, key);
            ((*h.mData)[i]).value = value;
        }
        
        template<typename T> void Remove(SHashMap<T>& h, const typename SHashMap<T>::HashMapEntry* e)
        {
            const hashmap_internal::FindResult fr = hashmap_internal::Find(h, e);
            
            if (fr.data_i != hashmap_internal::END_OF_LIST)
            {
                hashmap_internal::Erase(h, fr);
            }
        }
        
        template<typename T> void RemoveAll(SHashMap<T>& h, u64 key)
        {
            while (shashmap::Has(h, key))
            {
                shashmap::Remove(h, key);
            }
        }
    }
    
    template<typename T>
    SHashMap<T>::~SHashMap()
    {
        mHash->~SArray();
        mHash = nullptr;
        mData->~SArray();
        mData = nullptr;
    }

    template <typename T>
    u64 SHashMap<T>::FullAtCapacity(u64 capacity)
    {
        return static_cast<u64>(round(static_cast<f64>(capacity) * MAX_LOAD_FACTOR));
    }
    
    template <typename T>
    u64 SHashMap<T>::MemorySize(u64 capacity)
    {
        return SArray<SHashMap<T>::HashMapEntry>::MemorySize(capacity) + SArray<u64>::MemorySize(capacity) + sizeof(SHashMap<T>);
    }
    
    template <typename T> void SHashMap<T>::Create(SArray<u64>* hashStart, u64* hashDataStart, SArray<HashMapEntry>* dataArrayStart, HashMapEntry* dataStart, u64 capacity)
    {
        mCapacity = capacity;
        mHash = hashStart;
        mHash->Create(hashDataStart, capacity);
        mData = dataArrayStart;
        mData->Create(dataStart, capacity);
        
        mHash->mSize = capacity;
        
        for (u64 i = 0; i < capacity; ++i)
        {
            (*mHash)[i] = hashmap_internal::END_OF_LIST;
        }
    }
}

#endif /* SHashMap_h */
