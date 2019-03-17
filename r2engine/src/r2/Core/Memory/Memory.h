//
//  Memory.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-23.
//

#ifndef Memory_h
#define Memory_h

#if defined(_MSC_VER)
#define _ALLOW_KEYWORD_MACROS
#endif

#if !defined(alignof)
#define alignof(x) __alignof(x)
#endif

#define ALLOC(T, ARENA) r2::mem::utils::Alloc<T>(ARENA, __FILE__, __LINE__, "")
#define ALLOC_PARAMS(type, ARENA, ...) r2::mem::utils::AllocParams<type>(ARENA, __FILE__, __LINE__, "", __VA_ARGS__)
#define FREE(objPtr, ARENA) r2::mem::utils::Dealloc(objPtr, ARENA, __FILE__, __LINE__, "")
#define ALLOC_ARRAY(type, ARENA) r2::mem::utils::AllocArray<r2::mem::utils::TypeAndCount<type>::Type>(ARENA, r2::mem::utils::TypeAndCount<type>::Count, __FILE__, __LINE__, "", r2::mem::utils::IntToType<r2::mem::utils::IsPOD<r2::mem::utils::TypeAndCount<type>::Type>::Value>())
#define FREE_ARRAY(objPtr, ARENA) r2::mem::utils::DeallocArray(objPtr, ARENA, __FILE__, __LINE__, "")

#ifndef R2_CHECK_ALLOCATIONS_ON_DESTRUCTION
    #define R2_CHECK_ALLOCATIONS_ON_DESTRUCTION 0
#endif

namespace r2
{
    namespace mem
    {
        namespace utils
        {
            struct MemBoundary
            {
                u64 size = 0;
                void* location = nullptr;
            };
            
            struct Header
            {
                u32 size;
            };
            
            struct MemoryTag
            {
                char fileName[Kilobytes(1)];
                u64 alignment = 0;
                u64 size = 0;
                s32 line = -1;
                void* memPtr = nullptr;
                
                MemoryTag(){}
                MemoryTag(void* memPtr, const char* fileName, u64 alignment, u64 size, s32 line);
                MemoryTag(const MemoryTag& tag);
                MemoryTag& operator=(const MemoryTag& tag);
                ~MemoryTag() = default;
                MemoryTag(MemoryTag&& tag) = default;
                MemoryTag& operator=(MemoryTag&& tag) = default;
            };
            
            const u32 HEADER_PAD_VALUE = 0xffffffffu;
            static const u8 DEFAULT_ALIGN = 8;
            inline bool IsAligned(void* p, u64 align);
            inline void* AlignForward(void *p, u64 align);
            inline void* PointerAdd(void* p, u64 bytes);
            inline const void* PointerAdd(const void *p, u64 bytes);
            inline void* PointerSubtract(void *p, u64 bytes);
            inline const void* PointerSubtract(const void *p, u64 bytes);
            inline u64 PointerOffset(void* p1, void* p2);
            inline void* DataPointer(Header *header, u64 align);
            // Given a pointer to the data, returns a pointer to the header before it.
            inline Header* GetHeader(void* data);
            
            // Stores the size in the header and pads with HEADER_PAD_VALUE up to the
            // data pointer.
            inline void Fill(Header* header, void* data, u64 size);
            static inline u64 SizeWithPadding(u64 size, u64 align);
        }
        
        //From https://blog.molecular-matters.com/2011/08/03/memory-system-part-5/
        
        class R2_API SingleThreadPolicy
        {
        public:
            inline void Enter() const {}
            inline void Leave() const {}
        };
        
        class MemoryArenaBase
        {
        public:
            virtual const u64 TotalSize() const = 0;
            virtual const void* StartPtr() const = 0;
            virtual const u64 NumAllocations() const = 0;
            virtual const std::vector<utils::MemoryTag> Tags() const = 0;
        };
        
        class R2_API MemoryArea
        {
        public:
            using Handle = s64;
            static const Handle Invalid = -1;
            
            struct R2_API MemorySubArea
            {
                using Handle = s64;
                static const Handle Invalid = -1;
                //@TODO(Serge): add in a debug name?
                utils::MemBoundary mBoundary;
                MemoryArenaBase* mnoptrArena = nullptr;
            };
            
            MemoryArea(const char* debugName);
            ~MemoryArea();
            bool Init(u64 sizeInBytes);
            MemorySubArea::Handle AddSubArea(u64 sizeInBytes);
            utils::MemBoundary SubAreaBoundary(MemorySubArea::Handle) const;
            MemorySubArea* GetSubArea(MemorySubArea::Handle);
            void Shutdown();
            
            inline std::string Name() const {return &mDebugName[0];}
            inline const utils::MemBoundary& AreaBoundary() const {return mBoundary;}
            inline const std::vector<MemorySubArea>& SubAreas() const {return mSubAreas;}
        private:
            //The entire boundary of this region
            utils::MemBoundary mBoundary;
            //@Temporary - should be static?
            std::vector<MemorySubArea> mSubAreas;
            //#if defined(R2_DEBUG) || defined(R2_RELEASE)
            std::array<char, Kilobytes(1)> mDebugName;
            void* mCurrentNext = nullptr;
            //char mDebugName[Kilobytes(1)];
            //#endif
            bool mInitialized;
        };
        
        
        template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
        class R2_API MemoryArena: public MemoryArenaBase
        {
        public:
            explicit MemoryArena(MemoryArea::MemorySubArea& subArea):mAllocator(subArea.mBoundary)
            {
                subArea.mnoptrArena = this;
            }
            
            void* Allocate(u64 size, u64 alignment, const char* file, s32 line, const char* description)
            {
                mThreadGuard.Enter();
                
                const u64 originalSize = size;
                const u64 newSize = size + BoundsCheckingPolicy::SIZE_FRONT + BoundsCheckingPolicy::SIZE_BACK;
                
                byte* plainMemory = static_cast<byte*>(mAllocator.Allocate(newSize, alignment, BoundsCheckingPolicy::SIZE_FRONT));
                
                mBoundsChecker.GuardFront(plainMemory);
                mMemoryTagger.TagAllocation(plainMemory + BoundsCheckingPolicy::SIZE_FRONT, originalSize);
                mBoundsChecker.GuardBack(plainMemory + BoundsCheckingPolicy::SIZE_FRONT + originalSize);
                
                mMemoryTracker.OnAllocation(plainMemory, newSize, alignment, file, line, description);
                
                mThreadGuard.Leave();
                
                return plainMemory + BoundsCheckingPolicy::SIZE_FRONT;
            }
            
            void Free(void* ptr, const char* file, s32 line, const char* description)
            {
                mThreadGuard.Enter();
                
                byte* originalMemory = static_cast<byte*>(ptr) - BoundsCheckingPolicy::SIZE_FRONT;
                const u64 allocationSize = mAllocator.GetAllocationSize(originalMemory);
                
                mBoundsChecker.CheckFront(originalMemory);
                mBoundsChecker.CheckBack(originalMemory + allocationSize - BoundsCheckingPolicy::SIZE_BACK);
                
                mMemoryTracker.OnDeallocation(originalMemory, file, line, description);
                
                mMemoryTagger.TagDeallocation(originalMemory, allocationSize);
                
                mAllocator.Free(originalMemory);
                
                mThreadGuard.Leave();
            }
            
            virtual const u64 TotalSize() const override
            {
                return mAllocator.GetTotalBytesAllocated();
            }
            
            virtual const void* StartPtr() const override
            {
                return utils::PointerSubtract(mAllocator.StartPtr(), BoundsCheckingPolicy::SIZE_FRONT);
            }
            
            virtual const u64 NumAllocations() const override
            {
                return mMemoryTracker.NumAllocations();
            }
            
            virtual const std::vector<utils::MemoryTag> Tags() const override
            {
                return mMemoryTracker.Tags();
            }
            
        private:
            AllocationPolicy mAllocator;
            ThreadPolicy mThreadGuard;
            BoundsCheckingPolicy mBoundsChecker;
            //@Note: Maybe we want this to be a reference or pointer? I think this will be tied to a per system
            //thing
            MemoryTrackingPolicy mMemoryTracker;
            MemoryTaggingPolicy mMemoryTagger;
        };

        class R2_API GlobalMemory
        {
        public:
            //Internal to r2
            template<u64 N>
            constexpr static bool Init()
            {
                mMemoryAreas.reserve(static_cast<size_t>(N));
                return true;
            }
            static void Shutdown();
            
            //Can be used outside of r2
            static MemoryArea::Handle AddMemoryArea(const char* debugName);
            static MemoryArea* GetMemoryArea(MemoryArea::Handle handle);
            static inline const std::vector<MemoryArea>& MemoryAreas() {return mMemoryAreas;}
        private:
            //@TODO(Serge): make into static array
            static std::vector<MemoryArea> mMemoryAreas;
        };

        namespace utils
        {
            // ---------------------------------------------------------------
            // Inline function implementations
            // ---------------------------------------------------------------
            
            inline bool IsAligned(void* p, u64 align)
            {
                uptr pi = uptr(p);
                const u64 mod = pi % align;
                
                return mod == 0;
            }
            
            // Aligns p to the specified alignment by moving it forward if necessary
            // and returns the result.
            inline void* AlignForward(void *p, u64 align)
            {
                uptr pi = uptr(p);
                const u64 mod = pi % align;
                if (mod)
                    pi += (align - mod);
                return (void *)pi;
            }
            
            /// Returns the result of advancing p by the specified number of bytes
            inline void* PointerAdd(void *p, u64 bytes)
            {
                return (void*)((uptr*)p + bytes);
            }
            
            inline const void* PointerAdd(const void *p, u64 bytes)
            {
                return (const void*)((uptr*)p + bytes);
            }
            
            /// Returns the result of moving p back by the specified number of bytes
            inline void* PointerSubtract(void *p, u64 bytes)
            {
                return (void*)((uptr*)p - bytes);
            }
            
            inline const void* PointerSubtract(const void *p, u64 bytes)
            {
                return (const void*)((uptr*)p - bytes);
            }
            
            inline u64 PointerOffset(void* p1, void* p2)
            {
                return (uptr*)p2 - (uptr*)p1;
            }
            
            inline void * DataPointer(Header *header, u64 align) {
                void *p = header + 1;
                return AlignForward(p, align);
            }
            
            // Given a pointer to the data, returns a pointer to the header before it.
            inline Header * GetHeader(void *data)
            {
                u32* p = (u32*)data;
                
                while (p[-1] == HEADER_PAD_VALUE)
                    --p;
                
                return (Header*)p - 1;
            }
            
            // Stores the size in the header and pads with HEADER_PAD_VALUE up to the
            // data pointer.
            inline void Fill(Header *header, void *data, u64 size)
            {
                header->size = (u32)size; //this sets the total size including the header
                u32* p = (u32*)(header + 1);
                while (p < data)
                    *p++ = HEADER_PAD_VALUE;
            }
            
            static inline u64 SizeWithPadding(u64 size, u64 align)
            {
                return size + (u64)align + sizeof(Header);
            }
            
            template <typename T, class ARENA>
            T* Alloc(ARENA& arena, const char* file, s32 line, const char* description)
            {
                return new (arena.Allocate(sizeof(T), alignof(T), file, line, description)) T();
            }
            
            template <typename T, class ARENA, class... Args>
            T* AllocParams(ARENA& arena,const char* file, s32 line, const char* description, Args&&... args)
            {
                return new (arena.Allocate(sizeof(T), alignof(T), file, line, description)) T(std::forward<Args>(args)...);
            }
            
            //we must call the destructor manually now since we used the placement new call for Alloc
            template <typename T, class ARENA>
            void Dealloc(T *p, ARENA& arena, const char* file = "", s32 line = -1, const char* description = "")
            {
                if (p)
                {
                    p->~T();
                    arena.Free(p, file, line, description);
                }
            }
            
            template <class T>
            struct TypeAndCount
            {
            };
            
            template <class T, u64 N>
            struct TypeAndCount<T[N]>
            {
                typedef T Type;
                static const u64 Count = N;
            };
            
            template <typename T>
            struct IsPOD
            {
                static const bool Value = std::is_pod<T>::value;
            };
            
            template <bool I>
            struct IntToType
            {
            };
            
            typedef IntToType<false> NonPODType;
            typedef IntToType<true> PODType;
            
            template <typename T, class ARENA>
            T* AllocArray(ARENA& arena, u64 length, const char* file, s32 line, const char* description, NonPODType)
            {
                R2_CHECK(length != 0, "Can't make a 0 length Array");
                u8 headerSize = sizeof(u64) /sizeof(T);
                
                if(sizeof(u64)%sizeof(T) > 0) headerSize += 1;
                
                //Allocate extra space to store array length in the bytes before the array
                T* p =  ((T*) arena.Allocate(sizeof(T)*(length + headerSize), alignof(T), file, line, description)) + headerSize;
                
                *(((u64*)p) - 1) = length;
                
                for(u64 i = 0; i < length; i++)
                {
                    new (&p[i]) T;
                }
                
                return p;
            }
            
            template <typename T, class ARENA>
            T* AllocArray(ARENA& arena, u64 length, const char* file, s32 line, const char* description, PODType)
            {
                R2_CHECK(length != 0, "Can't make a 0 length Array");
                return (T*)arena.Allocate(sizeof(T)*length, alignof(T), file, line, description);
            }
            
            template<typename T, class ARENA>
            void DeallocArray(T* array, ARENA& arena, const char* file, s32 line, const char* description, NonPODType)
            {
                assert(array != nullptr);
                
                u64 length = *(((u64*)array) - 1);
                
                for(u64 i = length; i > 0; --i)
                    array[i-1].~T();
                
                u8 headerSize = sizeof(u64) / sizeof(T);
                
                if(sizeof(u64)%sizeof(T) > 0)
                {
                    headerSize += 1;
                }
                
                arena.Free(array - headerSize, file, line, description);
            }
            
            template<typename T, class ARENA>
            void DeallocArray(T* array, ARENA& arena, const char* file, s32 line, const char* description, PODType)
            {
                arena.Free(array, file, line, description);
            }
            
            template<typename T, class ARENA>
            void DeallocArray(T* array, ARENA& arena, const char* file, s32 line, const char* description)
            {
                DeallocArray(array, arena, file, line, description, IntToType<IsPOD<T>::Value>());
            }
        }
    }
}

#endif /* Memory_h */
