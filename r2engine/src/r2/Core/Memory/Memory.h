//
//  Memory.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-23.
//

#ifndef Memory_h
#define Memory_h

#include <vector>
#include <array>

#if defined(_MSC_VER)
#define _ALLOW_KEYWORD_MACROS
#endif

#if !defined(alignof)
#define alignof(x) __alignof(x)
#endif

#define ALLOC(T, ARENA) r2::mem::utils::Alloc<T>(ARENA, __FILE__, __LINE__, "")
#define ALLOC_BYTES(ARENA, n, alignment, file, line, description) r2::mem::utils::AllocBytes(ARENA, n, alignment, file, line, description)

#define ALLOC_BYTESN(ARENA, n, alignment) r2::mem::utils::AllocBytes(ARENA, n, alignment, __FILE__, __LINE__, "")

#define ALLOC_PARAMS(type, ARENA, ...) r2::mem::utils::AllocParams<type>(ARENA, __FILE__, __LINE__, "", __VA_ARGS__)
#define FREE(objPtr, ARENA) r2::mem::utils::Dealloc(objPtr, ARENA, __FILE__, __LINE__, "")
#define ALLOC_ARRAY(type, ARENA) r2::mem::utils::AllocArray<r2::mem::utils::TypeAndCount<type>::Type>(ARENA, r2::mem::utils::TypeAndCount<type>::Count, __FILE__, __LINE__, "", r2::mem::utils::IntToType<r2::mem::utils::IsPOD<r2::mem::utils::TypeAndCount<type>::Type>::Value>())
#define FREE_ARRAY(objPtr, ARENA) r2::mem::utils::DeallocArray(objPtr, ARENA, __FILE__, __LINE__, "")

#define FREE_EMPLACED_ARENA(ptr) r2::mem::utils::FreeEmplacedArena(ptr)

#define MEM_ENG_PERMANENT_PTR r2::mem::GlobalMemory::EngineMemory().permanentStorageArena
#define MEM_ENG_SCRATCH_PTR r2::mem::GlobalMemory::EngineMemory().singleFrameArena

#define STACK_BOUNDARY(array) r2::mem::utils::GetBoundary(&array, sizeof(array))

#define MAKE_BOUNDARY(arena, size, alignment) r2::mem::utils::MakeMemBoundary(arena, size, alignment)

#ifndef R2_CHECK_ALLOCATIONS_ON_DESTRUCTION
    #define R2_CHECK_ALLOCATIONS_ON_DESTRUCTION 0
#endif

namespace r2
{
    namespace mem
    {
        using AllocateFunc = std::function<byte* (u64 size, u64 alignment)>;
        using FreeFunc = std::function<void (byte*)>;
        
        enum PlacementPolicy
        {
            FIND_FIRST,
            FIND_BEST
        };
        
        namespace utils
        {
            struct MemBoundary
            {
                u64 size = 0;
                u32 elementSize = 0;
                u32 alignment = 0;
                u32 offset = 0;
                PlacementPolicy policy = FIND_FIRST;
                void* location = nullptr;
            };
            
            struct Header
            {
                u32 size;
            };
            
            struct MemoryTag
            {
                char fileName[r2::fs::FILE_PATH_LENGTH];
                u64 alignment = 0;
                u64 size = 0;
                u64 requestedSize = 0;
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
            
            static const u8 DEFAULT_ALIGN = 8;
            inline bool IsAligned(void* p, u64 align);
            inline void* AlignForward(void *p, u64 align);
            inline void* PointerAdd(void* p, u64 bytes);
            inline const void* PointerAdd(const void *p, u64 bytes);
            inline void* PointerSubtract(void *p, u64 bytes);
            inline const void* PointerSubtract(const void *p, u64 bytes);
            inline u64 PointerOffset(void* p1, void* p2);
            template<typename ARENA>
            MemBoundary MakeMemBoundary(ARENA& arena, u64 size, u64 alignment, u32 elementSize = 0, u32 offset = 0, PlacementPolicy policy = FIND_FIRST);
            
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
            
            inline MemBoundary GetBoundary(void* type, u64 count);
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
            virtual const u32 HeaderSize() const = 0;
            virtual const u32 FooterSize() const = 0;
            virtual const u64 UnallocatedBytes() const = 0;
        };
        
        class R2_API MemoryArea
        {
        public:
            using Handle = s64;
            static const Handle Invalid = -1;
            static const u64 DefaultScratchBufferSize = Megabytes(4);
            
            struct R2_API MemorySubArea
            {
                using Handle = s64;
                static const Handle Invalid = -1;
                utils::MemBoundary mBoundary;
               // MemoryArenaBase* mnoptrArena = nullptr;
            
                std::string mSubArenaName;
            };
            
            MemoryArea(const char* debugName);
            ~MemoryArea();
            //Add in scratch buffer here?
            bool Init(u64 sizeInBytes, u64 scratchBufferSize = DefaultScratchBufferSize);
            MemorySubArea::Handle AddSubArea(u64 sizeInBytes, const std::string& subAreaName = "");
            utils::MemBoundary SubAreaBoundary(MemorySubArea::Handle) const;
            utils::MemBoundary* SubAreaBoundaryPtr(MemorySubArea::Handle);
            
            utils::MemBoundary ScratchBoundary() const;
            utils::MemBoundary* ScratchBoundaryPtr();
            MemorySubArea::Handle ScratchSubAreaHandle() const {return mScratchAreaHandle;}
            
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
            
            std::array<char, r2::fs::FILE_PATH_LENGTH> mDebugName;

            void* mCurrentNext = nullptr;
            
            MemorySubArea::Handle mScratchAreaHandle;
            
            
            //#endif
            bool mInitialized;
        };
        
        
        template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
        class R2_API MemoryArena//: public MemoryArenaBase
        {
        public:
            explicit MemoryArena(MemoryArea::MemorySubArea& subArea):mAllocator(subArea.mBoundary)
            {
            //    subArea.mnoptrArena = this;
            }
            
            MemoryArena(MemoryArea::MemorySubArea& subArea, const utils::MemBoundary& boundary):mAllocator(boundary)
            {
             //   subArea.mnoptrArena = this;
                mMemoryTracker.SetName(subArea.mSubArenaName);
            }
            
            explicit MemoryArena(const utils::MemBoundary& boundary):mAllocator(boundary)
            {
                
            }
            
            void* Allocate(u64 size, u64 alignment, const char* file, s32 line, const char* description)
            {
                mThreadGuard.Enter();
                
                const u64 originalSize = size;
                const u64 newSize = size + BoundsCheckingPolicy::SIZE_FRONT + BoundsCheckingPolicy::SIZE_BACK;
                
                byte* plainMemory = static_cast<byte*>(mAllocator.Allocate(newSize, alignment, BoundsCheckingPolicy::SIZE_FRONT));
                
                R2_CHECK(plainMemory != nullptr, "Failed to allocate memory for size: %llu, alignment: %llu", newSize, alignment);
                
                mBoundsChecker.GuardFront(plainMemory);
                mMemoryTagger.TagAllocation(plainMemory + BoundsCheckingPolicy::SIZE_FRONT, originalSize);
                mBoundsChecker.GuardBack(plainMemory + BoundsCheckingPolicy::SIZE_FRONT + originalSize);
                
                utils::MemoryTag tag(plainMemory, file, alignment, newSize, line);
                tag.requestedSize = originalSize;
                
                mMemoryTracker.OnAllocation(tag);
                
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
            
            void EnsureZeroAllocations()
            {
                mMemoryTracker.Verify();
            }
            
            const u64 TotalBytesAllocated() const
            {
                return mAllocator.GetTotalBytesAllocated();
            }
            
            const u64 MemorySize() const
            {
                return mAllocator.GetTotalMemory();
            }
            
            const void* StartPtr() const
            {
                return utils::PointerSubtract(mAllocator.StartPtr(), BoundsCheckingPolicy::SIZE_FRONT);
            }
            
            const u64 NumAllocations() const
            {
                return mMemoryTracker.NumAllocations();
            }
            
            const std::vector<utils::MemoryTag> Tags() const
            {
                return mMemoryTracker.Tags();
            }
            
            const u32 HeaderSize() const
            {
                return BoundsCheckingPolicy::SIZE_FRONT + mAllocator.HeaderSize();
            }
            
            const u32 FooterSize() const
            {
                return BoundsCheckingPolicy::SIZE_BACK;
            }
            
            const u64 UnallocatedBytes() const 
            {
                return mAllocator.UnallocatedBytes();
            }
            
            AllocationPolicy& GetPolicyRef()
            {
                return mAllocator;
            }
            
        private:
            AllocationPolicy mAllocator;
            ThreadPolicy mThreadGuard;
            BoundsCheckingPolicy mBoundsChecker;
            MemoryTrackingPolicy mMemoryTracker;
            MemoryTaggingPolicy mMemoryTagger;
        };

        
       // class LinearArena;
        

        
        struct InternalEngineMemory;
        
        class R2_API GlobalMemory
        {
        public:
            //Internal to r2
            static bool Init(u64 numMemoryAreas, u64 internalEngineMemory = 0, u64 permanentStorageSize = 0, u64 singleFrameStorageSize = 0);
            static void Shutdown();
            static InternalEngineMemory& EngineMemory() {return mEngineMemory;}
            
            //Can be used outside of r2
            static MemoryArea::Handle AddMemoryArea(const char* debugName);
            static MemoryArea* GetMemoryArea(MemoryArea::Handle handle);
            static inline const std::vector<MemoryArea>& MemoryAreas() {return mMemoryAreas;}
        private:
            //@TODO(Serge): make into static array
            static std::vector<MemoryArea> mMemoryAreas;
            static InternalEngineMemory mEngineMemory;
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
                return (void*)((byte*)p + bytes);
            }
            
            inline const void* PointerAdd(const void *p, u64 bytes)
            {
                return (const void*)((byte*)p + bytes);
            }
            
            /// Returns the result of moving p back by the specified number of bytes
            inline void* PointerSubtract(void *p, u64 bytes)
            {
                return (void*)((byte*)p - bytes);
            }
            
            inline const void* PointerSubtract(const void *p, u64 bytes)
            {
                return (const void*)((byte*)p - bytes);
            }
            
            inline u64 PointerOffset(void* p1, void* p2)
            {
                return (byte*)p2 - (byte*)p1;
            }
            
            

            template <typename T, class ARENA>
            T* Alloc(ARENA& arena, const char* file, s32 line, const char* description)
            {
                void* ptr = arena.Allocate(sizeof(T), alignof(T), file, line, description);
                
                if (ptr)
                {
                    return new (ptr) T();
                }
                
                return nullptr;
            }
            
            template <class ARENA>
            byte* AllocBytes(ARENA& arena, u64 nBytes, u64 alignment, const char* file, s32 line, const char* description)
            {
                return (byte*)arena.Allocate(nBytes, alignment, file, line, description);
            }
            
            template <typename T, class ARENA, class... Args>
            T* AllocParams(ARENA& arena,const char* file, s32 line, const char* description, Args&&... args)
            {
                void* ptr = arena.Allocate(sizeof(T), alignof(T), file, line, description);

                if (ptr)
                {
                    return new (ptr) T(std::forward<Args>(args)...);
                }

                return nullptr;
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
            
            template <class ARENA>
            void FreeEmplacedArena(ARENA* emplacedArenaPtr)
            {
                if (emplacedArenaPtr)
                {
                    emplacedArenaPtr->~ARENA();
                }
            }
            
            inline MemBoundary GetBoundary(void* p, u64 count)
            {
                MemBoundary boundary;
                boundary.location = p;
                boundary.size = count;
                return boundary;
            }
            
            template<typename ARENA>
            MemBoundary MakeMemBoundary(ARENA& arena, u64 size, u64 alignment, u32 elementSize, u32 offset, PlacementPolicy policy)
            {
                MemBoundary boundary;
                boundary.location = ALLOC_BYTESN(arena, size, alignment);
                boundary.size = size;
                boundary.elementSize = elementSize;
                boundary.alignment = alignment;
                boundary.offset = offset;
                boundary.policy = policy;
                
                return boundary;
            }
        }
    }
}

#endif /* Memory_h */
