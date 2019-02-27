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

#define PTRALLOC(ptrAllocator, size, align) ptrAllocator->Allocate(size, align, __FILE__, __LINE__)
#define REFALLOC(allocator, size, align) allocator.Allocate(size, align, __FILE__, __LINE__)

#define ALLOC(allocator, T1) r2::mem::utils::Alloc<T1>(allocator, __FILE__, __LINE__)
#define DEALLOC(allocator, objPtr) r2::mem::utils::Dealloc(allocator, objPtr, __FILE__, __LINE__)
#define ALLOC_ARRAY(allocator, T1, length) r2::mem::utils::AllocArray<T1>(allocator, length, __FILE__, __LINE__)
#define DEALLOC_ARRAY(allocator, T1, array) r2::mem::utils::DeallocArray<T1>(allocator, array, __FILE__, __LINE__)

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
                char name[Kilobytes(1)];
            };
            
            struct Header
            {
                u32 size;
            };
            
            const u32 HEADER_PAD_VALUE = 0xffffffffu;
            static const u8 DEFAULT_ALIGN = 8;
            inline bool IsAligned(const void* p, u8 align);
            inline void* AlignForward(void *p, u8 align);
            inline void* AlignForward(const void *p, u8 align);
            inline u8 AlignForwardAdjustment(const void* address, u8 alignment);
            inline void* PointerAdd(void* p, u64 bytes);
            inline const void* PointerAdd(const void *p, u64 bytes);
            inline void* PointerSubtract(void *p, u64 bytes);
            inline const void* PointerSubtract(const void *p, u64 bytes);
            inline u64 PointerOffset(void* p1, void* p2);
            inline void* DataPointer(Header *header, u8 align);
            // Given a pointer to the data, returns a pointer to the header before it.
            inline Header* GetHeader(void* data);
            
            // Stores the size in the header and pads with HEADER_PAD_VALUE up to the
            // data pointer.
            inline void Fill(Header* header, void* data, u64 size);
            static inline u64 SizeWithPadding(u64 size, u8 align);
            
            template<typename T>
            inline u8 AlignForwardAdjustmentWithHeader(const void* address, u8 alignment);
        }
        
        class Allocator
        {
        public:

            static const u32 SIZE_NOT_TRACKED = 0xffffffffu;
            
            Allocator(const char* name = "");
            virtual ~Allocator() {}
            
            Allocator(const Allocator& other) = delete;
            Allocator& operator=(const Allocator& other) = delete;
            
            virtual void* Allocate(u64 size, u8 align = utils::DEFAULT_ALIGN, const char* file = "", s32 line = -1, const char* description = "") = 0;
            /// Frees an allocation previously made with allocate().
            //returns true if it deallocated something
            virtual bool Deallocate(void* ptr, const char* file = "", s32 line = -1, const char* description = "" ) = 0;
            
            /// Returns the amount of usable memory allocated at p. p must be a pointer
            /// returned by allocate() that has not yet been deallocated. The value returned
            /// will be at least the size specified to allocate(), but it can be bigger.
            /// (The allocator may round up the allocation to fit into a set of predefined
            /// slot sizes.)
            ///
            /// Not all allocators support tracking the size of individual allocations.
            /// An allocator that doesn't suppor it will return SIZE_NOT_TRACKED.
            virtual u64 AllocatedSize(void* p) = 0;
            
            /// Returns the total amount of memory allocated by this allocator. Note that the
            /// size returned can be bigger than the size of all individual allocations made,
            /// because the allocator may keep additional structures.
            ///
            /// If the allocator doesn't track memory, this function returns SIZE_NOT_TRACKED.
            virtual u64 TotalAllocated() = 0;
            
            virtual bool HasHeader() const = 0;
            
            inline const std::string Name() const {return mName;}
            inline const std::string BoundaryName() const {return mBoundary.name;}
            
        protected:
            utils::MemBoundary mBoundary;
            char mName[Kilobytes(1)];
        };
        
        namespace utils
        {
            // ---------------------------------------------------------------
            // Inline function implementations
            // ---------------------------------------------------------------
            
            inline bool IsAligned(const void* p, u8 align)
            {
                return AlignForwardAdjustment(p, align);
            }
            
            // Aligns p to the specified alignment by moving it forward if necessary
            // and returns the result.
            inline void* AlignForward(void *p, u8 align)
            {
                return (void*)((reinterpret_cast<uptr>(p)+static_cast<uptr>(align - 1)) & static_cast<uptr>(~(align - 1)));
            }
            
            inline void* AlignForward(const void *p, u8 align)
            {
                return (void*)((reinterpret_cast<uptr>(p)+static_cast<uptr>(align - 1)) & static_cast<uptr>(~(align - 1)));
            }
            
            inline u8 AlignForwardAdjustment(const void* address, u8 alignment)
            {
                u8 adjustment =  alignment - ( reinterpret_cast<uptr>(address) & static_cast<uptr>(alignment-1) );
                
                if(adjustment == alignment)
                    return 0; //already aligned
                
                return adjustment;
            }
            
            template<typename T>
            inline u8 AlignForwardAdjustmentWithHeader(const void* address, u8 alignment)
            {
                if(alignof(T) > alignment)
                    alignment = alignof(T);
                
                u8 adjustment = sizeof(T) + AlignForwardAdjustment(PointerAdd(address, sizeof(T)), alignment);
                
                return adjustment;
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
            
            inline void * DataPointer(Header *header, u8 align) {
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
            
            static inline u64 SizeWithPadding(u64 size, u8 align)
            {
                return size + (u64)align + sizeof(Header);
            }
            
            template <class T> T* Alloc(Allocator& allocator, const char* file = "", s32 line = -1, const char* description = "")
            {
                return new (allocator.Allocate(sizeof(T), alignof(T), file, line, description)) T();
            }
            
            template <class T, class... Args>
            T* Alloc(Allocator& allocator,const char* file, s32 line, const char* description, Args&&... args)
            {
                return new (allocator.Allocate(sizeof(T), alignof(T), file, line, description)) T(std::forward<Args>(args)...);
            }
            
            //we must call the destructor manually now since we used the placement new call for MakeNew
            template <class T> void Dealloc(Allocator& allocator, T *p, const char* file = "", s32 line = -1, const char* description = "")
            {
                if (p)
                {
                    p->~T();
                    allocator.Deallocate(p, file, line, description);
                }
            }
            
            //Can only be used with MallocAllocator
            template <class T> T* AllocArray(Allocator& allocator, u64 length, const char* file = "", s32 line = -1, const char* description = "")
            {
                assert(length != 0);
                u8 headerSize = sizeof(u64) /sizeof(T);
                
                if(sizeof(u64)%sizeof(T) > 0) headerSize += 1;
                
                //Allocate extra space to store array length in the bytes before the array
                T* p =  ((T*) allocator.Allocate(sizeof(T)*(length + headerSize), alignof(T), file, line, description)) + headerSize;
                
                *(((u64*)p) - 1) = length;
                
                for(u64 i = 0; i < length; i++)
                {
                    new (&p[i]) T;
                }
                
                return p;
            }
            
            //Can only be used with MallocAllocator
            template<class T> void DeallocArray(Allocator& allocator, T* array, const char* file = "", s32 line = -1, const char* description = "")
            {
                assert(array != nullptr);
                
                u64 length = *(((u64*)array) - 1);
                
                for(u64 i = 0; i < length; i++)
                    array[i].~T();
                
                u8 headerSize = sizeof(u64) / sizeof(T);
                
                if(sizeof(u64)%sizeof(T) > 0)
                {
                    headerSize += 1;
                }
                
                allocator.Deallocate(array - headerSize, file, line, description);
            }
        }
    }
}

#endif /* Memory_h */
