//
//  r2Catch.cpp
//  r2Tests
//
//  Created by Serge Lansiquot on 2019-03-11.
//

#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"
#include "r2.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"
#include "r2/Core/Memory/Allocators/RingBufferAllocator.h"
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SQueue.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Core/File/PathUtils.h"
#include <cstring>

TEST_CASE("TEST GLOBAL MEMORY")
{
    r2::mem::GlobalMemory::Init(1);
    
    SECTION("Test Create Memory Area")
    {
        int test[50];
        
        r2::mem::utils::MemBoundary boundary = STACK_BOUNDARY(test);
    
        REQUIRE(boundary.location == &test);
        
        int* test2 = (int*)boundary.location;
        
        REQUIRE(test2[45] == test[45]);
        
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1));
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(5));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subArea1Handle = testMemoryArea->AddSubArea(Kilobytes(512));
        
        REQUIRE(subArea1Handle != r2::mem::MemoryArea::SubArea::Invalid);
        
        auto subArea2Handle = testMemoryArea->AddSubArea(Kilobytes(512));
        
        REQUIRE(subArea2Handle != r2::mem::MemoryArea::SubArea::Invalid);
        
        auto subArea3Handle = testMemoryArea->AddSubArea(Kilobytes(512));
        
        REQUIRE(subArea3Handle == r2::mem::MemoryArea::SubArea::Invalid);
        
        REQUIRE(testMemoryArea->SubAreas().size() == 3);
        
        r2::mem::utils::MemBoundary subBoundary1 = testMemoryArea->SubAreaBoundary(subArea1Handle);
        
        r2::mem::utils::MemBoundary subBoundary2 = testMemoryArea->SubAreaBoundary(subArea2Handle);
        
        REQUIRE(subBoundary1.location != nullptr);
        REQUIRE(subBoundary1.size == Kilobytes(512));
        REQUIRE(subBoundary2.location == r2::mem::utils::PointerAdd(subBoundary1.location, subBoundary1.size));
        REQUIRE(subBoundary2.size == Kilobytes(512));
    }

    r2::mem::GlobalMemory::Shutdown();
}

TEST_CASE("TEST LINEAR ALLOCATOR")
{
    r2::mem::GlobalMemory::Init(1);
    SECTION("Test Linear Allocator")
    {
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1));
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(5));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
        
        r2::mem::LinearAllocator linearAllocator(testMemoryArea->SubAreaBoundary(subAreaHandle));
        
        REQUIRE(linearAllocator.GetTotalBytesAllocated() == 0);
        
        const u64 totalMem = linearAllocator.GetTotalMemory();
        
        REQUIRE(totalMem == Megabytes(1));
        
        int* intArray = (int*)linearAllocator.Allocate(512 * sizeof(int), alignof(int), 0);
        
        REQUIRE(intArray != nullptr);
        
        u32 allocationSize = linearAllocator.GetAllocationSize(intArray);
        
        REQUIRE(allocationSize == 512 * sizeof(int));
        
        memset(intArray, 0, sizeof(int) * 512);
        for (int i = 1; i < 511; ++i)
        {
            intArray[i] = i;
        }
        
        REQUIRE(intArray[0] == 0);
        REQUIRE(intArray[1] == 1);
        
        REQUIRE(linearAllocator.GetTotalBytesAllocated() >= 512 * sizeof(int));
        
        char* stringPtr = (char*)linearAllocator.Allocate(Kilobytes(1), 1, 0);
        
        memset(stringPtr, '\0', Kilobytes(1));
        
        strcpy(stringPtr, "My String");

        memset(intArray, 0, sizeof(int) * 512);

        size_t lengthOfString = strlen(stringPtr);
        REQUIRE(lengthOfString == 9);
        REQUIRE(strcmp(stringPtr, "My String") == 0);
        
        REQUIRE(linearAllocator.GetAllocationSize(stringPtr) == Kilobytes(1));
        
        linearAllocator.Free(intArray);
        
        REQUIRE(linearAllocator.GetTotalBytesAllocated() >= (512 * sizeof(int) + Kilobytes(1)));
        
        linearAllocator.Reset();
        
        REQUIRE(linearAllocator.GetTotalBytesAllocated() == 0);
    }
    r2::mem::GlobalMemory::Shutdown();
}

class TestClass
{
public:
    TestClass():x(0){}
    u64 Square() const {return x * x;}
    u64 x;
};

class TestClass2
{
public:
    TestClass2(u64 x):mX(x){}
    inline u64 X() const {return mX;}
private:
    u64 mX;
};

class PointClass
{
public:
    PointClass():x(0), y(0){}
    PointClass(u64 _x, u64 _y):x(_x), y(_y){}
    u64 Square() const {return x*x;}
    inline u64 X() const {return x;}
    inline u64 Y() const {return y;}
    
    u64 x, y;
};

TEST_CASE("Test Linear Memory Arena No Checking")
{
    r2::mem::GlobalMemory::Init(1);
    SECTION("Test Linear Allocator Arena")
    {
       //
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1));
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(5));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
        
        r2::mem::LinearArena linearArena(*testMemoryArea->GetSubArea(subAreaHandle));
        
        TestClass* tc = ALLOC(TestClass, linearArena);
        
        tc->x = 5;
        
        REQUIRE(tc->x == 5);
        
        FREE(tc, linearArena);
        
        TestClass2* tc2 = ALLOC_PARAMS(TestClass2, linearArena, 10);
        
        REQUIRE(tc2->X() == 10);
        
        FREE(tc2, linearArena);
        
        TestClass* testArray = ALLOC_ARRAY(TestClass[10], linearArena);
        
        for (size_t i = 0; i < 10; ++i)
        {
            testArray[i].x = i;
        }
        
        REQUIRE(testArray[9].Square() == 81);
        
        FREE_ARRAY(testArray, linearArena);
        
    }
    r2::mem::GlobalMemory::Shutdown();
}

TEST_CASE("Test Stack Allocator")
{
    r2::mem::GlobalMemory::Init(1);
    SECTION("Test Stack Allocator")
    {
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1));
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(5));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
        
        r2::mem::StackAllocator stackAllocator(testMemoryArea->SubAreaBoundary(subAreaHandle));
        
        //Test basic stack allocator methods
        REQUIRE(stackAllocator.GetTotalBytesAllocated() == 0);
        
        const u64 subareaSize = testMemoryArea->SubAreaBoundary(subAreaHandle).size;
        const u64 totalMemoryForAllocator = stackAllocator.GetTotalMemory();
        
        REQUIRE(totalMemoryForAllocator == subareaSize);
        
        REQUIRE(stackAllocator.StartPtr() == testMemoryArea->SubAreaBoundary(subAreaHandle).location);
        
        //Test Alocations
        
        void* firstStackAllocation = stackAllocator.Allocate(Kilobytes(1), alignof(byte), 0);
        
        REQUIRE(firstStackAllocation != nullptr);
        
        const u64 allocSize = stackAllocator.GetAllocationSize(firstStackAllocation);
        
        REQUIRE(allocSize == Kilobytes(1));
        
        char* stringPtr = static_cast<char*>(firstStackAllocation);
        
        memset(stringPtr, '\0', Kilobytes(1));

        strcpy(stringPtr, "My String");
        
        size_t lengthOfString = strlen(stringPtr);
        
        REQUIRE(lengthOfString == 9);
        
        REQUIRE(strcmp(stringPtr, "My String") == 0);
        
        const u64 u64ArraySize = 512 * sizeof(u64);
        
        u64* u64Array = (u64*)stackAllocator.Allocate(u64ArraySize, alignof(u64), 0);
        
        REQUIRE(u64Array != nullptr);
        
        REQUIRE(stackAllocator.GetAllocationSize(u64Array) == u64ArraySize);
        
        REQUIRE(stackAllocator.GetTotalBytesAllocated() >= (u64ArraySize + Kilobytes(1)));
        
        for (u64 i = 0; i < 512; ++i)
        {
            u64Array[i] = i;
        }
        
        REQUIRE(u64Array[0] == 0);
        REQUIRE(u64Array[511] == 511);
        
        stackAllocator.Free(u64Array);
        
        REQUIRE(stackAllocator.GetAllocationSize(firstStackAllocation) == Kilobytes(1));
        
        REQUIRE(stackAllocator.GetTotalBytesAllocated() >= Kilobytes(1));
        
        stackAllocator.Free(firstStackAllocation);
        
        REQUIRE(stackAllocator.GetTotalBytesAllocated() == 0);
    }
    
    r2::mem::GlobalMemory::Shutdown();
}

TEST_CASE("Test Stack Memory Arena No Checking")
{
    r2::mem::GlobalMemory::Init(1);
    SECTION("Test Stack Allocator Arena")
    {
        //
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1));
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(5));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
        
        r2::mem::StackArena stackArena(*testMemoryArea->GetSubArea(subAreaHandle));
        
        TestClass* tc = ALLOC(TestClass, stackArena);
        
        tc->x = 5;
        
        REQUIRE(tc->x == 5);
        
        FREE(tc, stackArena);
        
        TestClass2* tc2 = ALLOC_PARAMS(TestClass2, stackArena, 10);
        
        REQUIRE(tc2->X() == 10);
        
        FREE(tc2, stackArena);
        
        TestClass* testArray = ALLOC_ARRAY(TestClass[10], stackArena);
        
        for (size_t i = 0; i < 10; ++i)
        {
            testArray[i].x = i;
        }
        
        REQUIRE(testArray[9].Square() == 81);
        
        FREE_ARRAY(testArray, stackArena);
    }
    r2::mem::GlobalMemory::Shutdown();
}

TEST_CASE("Test Pool Allocator")
{
    r2::mem::GlobalMemory::Init(1);
    SECTION("Test Pool Allocator")
    {
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1));
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(5));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
        
        r2::mem::utils::MemBoundary poolBoundary = testMemoryArea->SubAreaBoundary(subAreaHandle);
        
        poolBoundary.elementSize = 32;
        poolBoundary.offset = 0;
        poolBoundary.alignment = alignof(u64);
        
        r2::mem::PoolAllocator poolAllocator(poolBoundary);
        
        REQUIRE(poolAllocator.StartPtr() == poolBoundary.location);
        REQUIRE(poolAllocator.GetTotalMemory() == Megabytes(1));
        REQUIRE(poolAllocator.GetTotalBytesAllocated() == 0);
        REQUIRE(poolAllocator.NumElementsAllocated() == 0);
        
        const u64 TOTAL_NUM_ELEMENTS = poolAllocator.TotalElements();
        
        void* poolElement = poolAllocator.Allocate(poolBoundary.elementSize, alignof(u64), 0);
        
        REQUIRE(poolAllocator.GetAllocationSize(poolElement) == poolBoundary.elementSize);
        REQUIRE(poolAllocator.NumElementsAllocated() == 1);
        REQUIRE(poolAllocator.GetTotalBytesAllocated() == poolBoundary.elementSize);
        
        char* stringPtr = static_cast<char*>(poolElement);
        
        memset(stringPtr, '\0', poolBoundary.elementSize);
        
        strcpy(stringPtr, "My String");
        
        size_t lengthOfString = strlen(stringPtr);
        
        REQUIRE(lengthOfString == 9);
        
        REQUIRE(strcmp(stringPtr, "My String") == 0);
        
        poolAllocator.Free(poolElement);
        
        REQUIRE(poolAllocator.NumElementsAllocated() == 0);
        REQUIRE(poolAllocator.GetTotalBytesAllocated() == 0);
        
        std::vector<void*> poolElements;
        
        poolElements.reserve(TOTAL_NUM_ELEMENTS);
        
        //Max out the pool
        for (u64 i = 0; i < TOTAL_NUM_ELEMENTS; ++i)
        {
            void* pointer = poolAllocator.Allocate(poolBoundary.elementSize, poolBoundary.alignment, poolBoundary.offset);
            REQUIRE(pointer != nullptr);
            REQUIRE(poolAllocator.GetAllocationSize(pointer) == poolBoundary.elementSize);
            REQUIRE(poolAllocator.NumElementsAllocated() == i+1);
            
            poolElements.push_back(pointer);
        }
        
        REQUIRE(poolAllocator.Allocate(poolBoundary.elementSize, poolBoundary.alignment, poolBoundary.offset) == nullptr);
        
        REQUIRE(poolAllocator.NumElementsAllocated() == TOTAL_NUM_ELEMENTS);
        REQUIRE(poolAllocator.GetTotalBytesAllocated() == TOTAL_NUM_ELEMENTS*poolBoundary.elementSize);
        
        for (u64 i = TOTAL_NUM_ELEMENTS; i > 0; --i)
        {
            poolAllocator.Free(poolElements[i-1]);
            REQUIRE(poolAllocator.NumElementsAllocated() == i-1);
            REQUIRE(poolAllocator.GetTotalBytesAllocated() == poolAllocator.NumElementsAllocated()*poolBoundary.elementSize);
        }
        poolElements.clear();
        
        REQUIRE(poolAllocator.NumElementsAllocated() == 0);
        REQUIRE(poolAllocator.GetTotalBytesAllocated() == 0);
    }
    r2::mem::GlobalMemory::Shutdown();
}

TEST_CASE("Test Pool Memory Arena No Checking")
{
    r2::mem::GlobalMemory::Init(1);
    SECTION("Test Pool Allocator Arena")
    {
        //
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1));
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(5));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
        
        r2::mem::utils::MemBoundary* boundary = testMemoryArea->SubAreaBoundaryPtr(subAreaHandle);
        
        REQUIRE(boundary != nullptr);
        boundary->elementSize = 11 * sizeof(TestClass);
        boundary->alignment = alignof(TestClass);
        boundary->offset = 0;
        
        r2::mem::NoCheckPoolArena poolArena(*testMemoryArea->GetSubArea(subAreaHandle));
        
        TestClass* tc = ALLOC(TestClass, poolArena);
        
        tc->x = 5;
        
        REQUIRE(tc->x == 5);
        
        FREE(tc, poolArena);
        
        TestClass2* tc2 = ALLOC_PARAMS(TestClass2, poolArena, 10);
        
        REQUIRE(tc2->X() == 10);
        
        FREE(tc2, poolArena);
        
        TestClass* testArray = ALLOC_ARRAY(TestClass[10], poolArena);
        
        for (size_t i = 0; i < 10; ++i)
        {
            testArray[i].x = i;
        }
        
        REQUIRE(testArray[9].Square() == 81);
        
        FREE_ARRAY(testArray, poolArena);
    }
    r2::mem::GlobalMemory::Shutdown();
}


TEST_CASE("Test Malloc Allocator")
{
    SECTION("Test Malloc Allocator")
    {
        r2::mem::MallocAllocator mallocAllocator;
        
        //Test basic stack allocator methods
        REQUIRE(mallocAllocator.GetTotalBytesAllocated() == 0);
        
        //Test Alocations
        
        void* firstAllocation = mallocAllocator.Allocate(Kilobytes(1), alignof(byte), 0);
        
        REQUIRE(firstAllocation != nullptr);
        
        const u64 allocSize = mallocAllocator.GetAllocationSize(firstAllocation);
        
        REQUIRE(allocSize == Kilobytes(1));
        
        char* stringPtr = static_cast<char*>(firstAllocation);
        
        memset(stringPtr, '\0', Kilobytes(1));
        
        strcpy(stringPtr, "My String");
        
        size_t lengthOfString = strlen(stringPtr);
        
        REQUIRE(lengthOfString == 9);
        
        REQUIRE(strcmp(stringPtr, "My String") == 0);
        
        const u64 u64ArraySize = 512 * sizeof(u64);
        
        u64* u64Array = (u64*)mallocAllocator.Allocate(u64ArraySize, alignof(u64), 0);
        
        REQUIRE(u64Array != nullptr);
        
        REQUIRE(mallocAllocator.GetAllocationSize(u64Array) == u64ArraySize);
        
        REQUIRE(mallocAllocator.GetTotalBytesAllocated() >= (u64ArraySize + Kilobytes(1)));
        
        for (u64 i = 0; i < 512; ++i)
        {
            u64Array[i] = i;
        }
        
        REQUIRE(u64Array[0] == 0);
        REQUIRE(u64Array[511] == 511);
        
        mallocAllocator.Free(u64Array);
        
        REQUIRE(mallocAllocator.GetAllocationSize(firstAllocation) == Kilobytes(1));
        
        REQUIRE(mallocAllocator.GetTotalBytesAllocated() >= Kilobytes(1));
        
        mallocAllocator.Free(firstAllocation);
        
        REQUIRE(mallocAllocator.GetTotalBytesAllocated() == 0);
    }
}

TEST_CASE("Test Malloc Memory Arena No Checking")
{
    SECTION("Test Malloc Allocator Arena")
    {
        r2::mem::utils::MemBoundary boundary;
        r2::mem::MallocArena mallocArena(boundary);
        
        TestClass* tc = ALLOC(TestClass, mallocArena);
        
        tc->x = 5;
        
        REQUIRE(tc->x == 5);
        
        FREE(tc, mallocArena);
        
        TestClass2* tc2 = ALLOC_PARAMS(TestClass2, mallocArena, 10);
        
        REQUIRE(tc2->X() == 10);
        
        FREE(tc2, mallocArena);
        
        TestClass* testArray = ALLOC_ARRAY(TestClass[10], mallocArena);
        
        for (size_t i = 0; i < 10; ++i)
        {
            testArray[i].x = i;
        }
        
        REQUIRE(testArray[9].Square() == 81);
        
        FREE_ARRAY(testArray, mallocArena);
    }
}

TEST_CASE("Test Free List")
{
    r2::mem::GlobalMemory::Init(1);
    SECTION("Test Free List Allocator")
    {
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1), 0);
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(1));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
        
        r2::mem::FreeListAllocator freeListAllocator(testMemoryArea->SubAreaBoundary(subAreaHandle));
        
        //Test basic free list allocator methods
        REQUIRE(freeListAllocator.GetTotalBytesAllocated() == 0);
        REQUIRE(freeListAllocator.UnallocatedBytes() == Megabytes(1));
        
        const u64 subareaSize = testMemoryArea->SubAreaBoundary(subAreaHandle).size;
        const u64 totalMemoryForAllocator = freeListAllocator.GetTotalMemory();
        
        REQUIRE(totalMemoryForAllocator == subareaSize);
        
        REQUIRE(freeListAllocator.StartPtr() == testMemoryArea->SubAreaBoundary(subAreaHandle).location);
        
        REQUIRE(freeListAllocator.HeaderSize() == sizeof(r2::mem::FreeListAllocator::AllocationHeader));
        
        //Do some allocations
        //@Note(Serge): this could change depending on 32 bit platforms
        const u64 overhead = alignof(byte*) + sizeof(size_t);
        const u64 max1KBAllocations = std::floor((float)freeListAllocator.GetTotalMemory() / (float)(Kilobytes(1) + overhead));
        const u64 max1KBAllocationSize = max1KBAllocations * (Kilobytes(1) + overhead);
        const u64 leftoverMemory = freeListAllocator.GetTotalMemory() - max1KBAllocationSize;
        
        std::vector<byte*> pointers;
        byte* firstAllocation = (byte*)freeListAllocator.Allocate(Kilobytes(1), alignof(byte*), 0);
        pointers.push_back(firstAllocation);
        
        REQUIRE(firstAllocation != nullptr);
        
        const u64 allocSize = freeListAllocator.GetAllocationSize(firstAllocation);
        
        REQUIRE(allocSize == Kilobytes(1));
        
        
        pointers.reserve(max1KBAllocations-1);
        
        for (u64 i = 0; i < (max1KBAllocations-1); ++i)
        {
            byte* nextPointer = (byte*)freeListAllocator.Allocate(Kilobytes(1), alignof(byte*), 0);
            
            REQUIRE(nextPointer != nullptr);
            
            pointers.push_back(nextPointer);
        }
        
        REQUIRE(freeListAllocator.GetTotalBytesAllocated() == max1KBAllocationSize);
        REQUIRE(freeListAllocator.UnallocatedBytes() == leftoverMemory);
        
        void* shouldBeNull = freeListAllocator.Allocate(Kilobytes(1), alignof(byte*), 0);
        
        REQUIRE(shouldBeNull == nullptr);
        
        for (u64 i = 0; i < max1KBAllocations; ++i)
        {
            if (i % 2 == 1)
            {
                freeListAllocator.Free(pointers[i]);
                pointers[i] = nullptr;
            }
        }
        
        byte* tooBigOfAnAllocation = (byte*)freeListAllocator.Allocate(Kilobytes(2), alignof(byte*), 0);
        
        REQUIRE(tooBigOfAnAllocation == nullptr);
        
        REQUIRE(pointers[0] != nullptr);
        freeListAllocator.Free(pointers[0]);
        
        tooBigOfAnAllocation = (byte*)freeListAllocator.Allocate(Kilobytes(2), alignof(byte*), 0);
        
        REQUIRE(tooBigOfAnAllocation != nullptr);
        
        for (u64 i = 0; i < max1KBAllocations; ++i)
        {
            if(pointers[i] != nullptr)
            {
                freeListAllocator.Free(pointers[i]);
                pointers[i] = nullptr;
            }
        }
        
        pointers.clear();
        
        REQUIRE(freeListAllocator.GetTotalBytesAllocated() == 0);
       
        byte* shouldBeAbleToAllocate2KB = (byte*)freeListAllocator.Allocate(Kilobytes(2), alignof(byte*), 0);
        
        REQUIRE(shouldBeAbleToAllocate2KB != nullptr);
        
        freeListAllocator.Free(shouldBeAbleToAllocate2KB);
        
        const u64 maxAllocationSize = freeListAllocator.GetTotalMemory() - overhead;
        
        byte* maxAllocation = (byte*)freeListAllocator.Allocate(maxAllocationSize, alignof(byte*), 0);
        
        REQUIRE(maxAllocation != nullptr);
        REQUIRE(freeListAllocator.GetTotalBytesAllocated() == freeListAllocator.GetTotalMemory());
        
        byte* noRoom = (byte*)freeListAllocator.Allocate(16, alignof(byte*), 0);
        
        REQUIRE(noRoom == nullptr);
        
        freeListAllocator.Reset();
        
        REQUIRE(freeListAllocator.GetTotalBytesAllocated() == 0);
        REQUIRE(freeListAllocator.UnallocatedBytes() == Megabytes(1));
    }
    
    r2::mem::GlobalMemory::Shutdown();
}

TEST_CASE("Test Free List Memory Arena No Checking")
{
    r2::mem::GlobalMemory::Init(1);
    SECTION("Test Free List Arena")
    {
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1), 0);
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(1));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
        
        r2::mem::FreeListArena freeListArena(*testMemoryArea->GetSubArea(subAreaHandle));
        
        PointClass* tc = ALLOC(PointClass, freeListArena);
        
        tc->x = 5;
        
        REQUIRE(tc->x == 5);
        
        FREE(tc, freeListArena);
        
        PointClass* tc2 = ALLOC_PARAMS(PointClass, freeListArena, 10, 6);
        
        REQUIRE(tc2->X() == 10);
        REQUIRE(tc2->Y() == 6);
        
        FREE(tc2, freeListArena);
        
        PointClass* testArray = ALLOC_ARRAY(PointClass[10], freeListArena);
        
        for (size_t i = 0; i < 10; ++i)
        {
            testArray[i].x = i;
            testArray[i].y = 10 - i;
        }
        
        REQUIRE(testArray[9].Square() == 81);
        REQUIRE(testArray[5].Y() * testArray[5].Y() == 25);
        FREE_ARRAY(testArray, freeListArena);
    }
    r2::mem::GlobalMemory::Shutdown();
}


TEST_CASE("Test Basic SArray")
{
    r2::mem::GlobalMemory::Init(1);
    
    auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
    REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
    r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
    REQUIRE(testMemoryArea != nullptr);
    REQUIRE(testMemoryArea->Name() == "TestArea");
    auto result = testMemoryArea->Init(Megabytes(1));
    REQUIRE(result);
    REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(5));
    REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
    auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
    REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
    
    SECTION("Test Basic SArray operations linear allocator")
    {
        r2::mem::LinearArena linearArena(*testMemoryArea->GetSubArea(subAreaHandle));
        r2::SArray<int>* intArray = MAKE_SARRAY(linearArena, int, 100);
        
        r2::sarr::Push(*intArray, 10);
        
        for (int i = 1; i < 100; i++) {
            r2::sarr::Push(*intArray, 55);
        }
        
        REQUIRE(r2::sarr::Size(*intArray) == 100);
        REQUIRE(r2::sarr::Capacity(*intArray) == 100);
        REQUIRE(r2::sarr::IsEmpty(*intArray) == false);
        REQUIRE(r2::sarr::At(*intArray, 99) == 55);
        REQUIRE(r2::sarr::At(*intArray, 0) == 10);
        REQUIRE(r2::sarr::At(*intArray, 50) == 55);
        
        REQUIRE(r2::sarr::First(*intArray) == 10);
        REQUIRE(r2::sarr::Last(*intArray) == 55);
        
        REQUIRE(r2::sarr::Begin(*intArray) == r2::mem::utils::PointerAdd(intArray, sizeof(r2::SArray<int>)));
        REQUIRE(r2::sarr::End(*intArray) == r2::mem::utils::PointerAdd(intArray, sizeof(r2::SArray<int>) + sizeof(int) * r2::sarr::Size(*intArray)));
        
        r2::sarr::Pop(*intArray);
        
        REQUIRE(r2::sarr::Size(*intArray) == 99);
        REQUIRE(r2::sarr::Capacity(*intArray) == 100);
        REQUIRE(r2::sarr::IsEmpty(*intArray) == false);
        
        
        for (int i = 0; i < 99; ++i) {
            r2::sarr::Pop(*intArray);
        }
        
        REQUIRE(r2::sarr::Size(*intArray) == 0);
        REQUIRE(r2::sarr::Capacity(*intArray) == 100);
        REQUIRE(r2::sarr::IsEmpty(*intArray) == true);
        
        REQUIRE(r2::sarr::Begin(*intArray) == r2::mem::utils::PointerAdd(intArray, sizeof(r2::SArray<int>)));
        REQUIRE(r2::sarr::End(*intArray) == r2::mem::utils::PointerAdd(intArray, sizeof(r2::SArray<int>) + sizeof(int) * r2::sarr::Size(*intArray)));
        
        FREE(intArray, linearArena);
    }
    
    SECTION("Test Basic SArray operations stack allocator")
    {
        r2::mem::StackArena stackArena(*testMemoryArea->GetSubArea(subAreaHandle));
        r2::SArray<int>* intArray = MAKE_SARRAY(stackArena, int, 100);
        
        r2::sarr::Push(*intArray, 10);
        
        for (int i = 1; i < 100; i++) {
            r2::sarr::Push(*intArray, 55);
        }
        
        REQUIRE(r2::sarr::Size(*intArray) == 100);
        REQUIRE(r2::sarr::Capacity(*intArray) == 100);
        REQUIRE(r2::sarr::IsEmpty(*intArray) == false);
        REQUIRE(r2::sarr::At(*intArray, 99) == 55);
        REQUIRE(r2::sarr::At(*intArray, 0) == 10);
        REQUIRE(r2::sarr::At(*intArray, 50) == 55);
        
        REQUIRE(r2::sarr::First(*intArray) == 10);
        REQUIRE(r2::sarr::Last(*intArray) == 55);
        
        REQUIRE(r2::sarr::Begin(*intArray) == r2::mem::utils::PointerAdd(intArray, sizeof(r2::SArray<int>)));
        REQUIRE(r2::sarr::End(*intArray) == r2::mem::utils::PointerAdd(intArray, sizeof(r2::SArray<int>) + sizeof(int) * r2::sarr::Size(*intArray)));
        
        r2::sarr::Pop(*intArray);
        
        REQUIRE(r2::sarr::Size(*intArray) == 99);
        REQUIRE(r2::sarr::Capacity(*intArray) == 100);
        REQUIRE(r2::sarr::IsEmpty(*intArray) == false);
        
        
        for (int i = 0; i < 99; ++i) {
            r2::sarr::Pop(*intArray);
        }
        
        REQUIRE(r2::sarr::Size(*intArray) == 0);
        REQUIRE(r2::sarr::Capacity(*intArray) == 100);
        REQUIRE(r2::sarr::IsEmpty(*intArray) == true);
        
        REQUIRE(r2::sarr::Begin(*intArray) == r2::mem::utils::PointerAdd(intArray, sizeof(r2::SArray<int>)));
        REQUIRE(r2::sarr::End(*intArray) == r2::mem::utils::PointerAdd(intArray, sizeof(r2::SArray<int>) + sizeof(int) * r2::sarr::Size(*intArray)));
        
        FREE(intArray, stackArena);
    }
    r2::mem::GlobalMemory::Shutdown();
}


bool cmp(const int& a, const int& b)
{
    return a < b;
}

bool cmp2(const int& a, const int& b)
{
    return a > b;
}

TEST_CASE("Test SQueue")
{
    r2::mem::GlobalMemory::Init(1);
    
    auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
    REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
    r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
    REQUIRE(testMemoryArea != nullptr);
    REQUIRE(testMemoryArea->Name() == "TestArea");
    auto result = testMemoryArea->Init(Megabytes(1));
    REQUIRE(result);
    REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(5));
    REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
    auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
    REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
    
    SECTION("Basic SQueue Functionality")
    {
        r2::mem::LinearArena linearArena(*testMemoryArea->GetSubArea(subAreaHandle));
        r2::SQueue<int>* intQueue = MAKE_SQUEUE(linearArena, int, 100);
        
        REQUIRE(r2::squeue::Space(*intQueue) == 100);
        REQUIRE(r2::squeue::Size(*intQueue) == 0);
        
        r2::squeue::PushBack(*intQueue, 31);
        
        REQUIRE(r2::squeue::First(*intQueue) == 31);
        REQUIRE(r2::squeue::Last(*intQueue) == 31);
        
        REQUIRE(r2::squeue::Size(*intQueue) == 1);
        REQUIRE(r2::squeue::Space(*intQueue) == 99);
        
        r2::squeue::PushFront(*intQueue, 20);
        
        REQUIRE(r2::squeue::First(*intQueue) == 20);
        REQUIRE(r2::squeue::Last(*intQueue) == 31);
        
        REQUIRE(r2::squeue::Size(*intQueue) == 2);
        REQUIRE(r2::squeue::Space(*intQueue) == 98);
        
        for (int i = 2; i < 100; i++) {
            r2::squeue::PushBack(*intQueue, 55);
        }
        
        REQUIRE(r2::squeue::Size(*intQueue) == 100);
        REQUIRE(r2::squeue::Space(*intQueue) == 0);
        
        REQUIRE(r2::squeue::First(*intQueue) == 20);
        REQUIRE(r2::squeue::Last(*intQueue) == 55);
        REQUIRE((*intQueue)[1] == 31);
        
        for (int i = 0 ; i < 100; ++i)
        {
            r2::squeue::PopBack(*intQueue);
        }
        
        REQUIRE(r2::squeue::Size(*intQueue) == 0);
        REQUIRE(r2::squeue::Space(*intQueue) == 100);
        
        for(int i = 0; i < 5; ++i)
        {
            r2::squeue::PushBack(*intQueue, i + 1);
        }
        
        for (int i = 0; i < 5; ++i)
        {
            REQUIRE((*intQueue)[i] == i + 1);
        }
        
        r2::squeue::PopBack(*intQueue);
        r2::squeue::PopFront(*intQueue);
        
        REQUIRE(r2::squeue::First(*intQueue) == 2);
        REQUIRE(r2::squeue::Last(*intQueue) == 4);
        
        
        
        r2::squeue::ConsumeAll(*intQueue);
        
        REQUIRE(r2::squeue::Size(*intQueue) == 0);
        REQUIRE(r2::squeue::Space(*intQueue) == 100);
        
        
        for (int i = 0; i < 5; ++i)
        {
            r2::squeue::PushFront(*intQueue, i);
        }
        
        r2::squeue::Sort(*intQueue, cmp);
        
        
        for (int i = 0; i < 5; ++i)
        {
            REQUIRE((*intQueue)[i] == i);
        }
        
        r2::squeue::Sort(*intQueue, cmp2);
        
        for (int i = 0; i < 5; ++i)
        {
            REQUIRE((*intQueue)[i] == (4-i));
        }
        
        FREE(intQueue, linearArena);
    }
    
    r2::mem::GlobalMemory::Shutdown();
}


TEST_CASE("Test SHashMap")
{
    r2::mem::GlobalMemory::Init(1);
    
    auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
    REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
    r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
    REQUIRE(testMemoryArea != nullptr);
    REQUIRE(testMemoryArea->Name() == "TestArea");
    auto result = testMemoryArea->Init(Megabytes(1));
    REQUIRE(result);
    REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(5));
    REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
    auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
    REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
 
    SECTION("Test shashmap functionality")
    {
        r2::mem::StackArena stackArena(*testMemoryArea->GetSubArea(subAreaHandle));
        r2::SHashMap<char>* hashMap = MAKE_SHASHMAP(stackArena, char, 255);
        
        REQUIRE(hashMap != nullptr);
        
        size_t key = 88;
        const char& defaultChar = '~';
        char valInHashMap = 'c';
        
        //88 is a key here
        REQUIRE(!r2::shashmap::Has(*hashMap, 88));
        
        const char& shouldBeDefault = r2::shashmap::Get(*hashMap, key, defaultChar);
        
        REQUIRE(shouldBeDefault == defaultChar);
        
        //add something new
        r2::shashmap::Set(*hashMap, key, valInHashMap);
        
        REQUIRE(r2::shashmap::Has(*hashMap, key));
        
        const char& val = r2::shashmap::Get(*hashMap, key, defaultChar);
        
        REQUIRE(val == valInHashMap);
        
        r2::shashmap::Remove(*hashMap, key);

        REQUIRE(!r2::shashmap::Has(*hashMap, key));
        
        r2::shashmap::Set(*hashMap, key, valInHashMap);
        
        r2::shashmap::Clear(*hashMap);
        
        REQUIRE(!r2::shashmap::Has(*hashMap, key));
        
        //add a bunch
        for (size_t i = 32; i < 127; ++i)
        {
            r2::shashmap::Set(*hashMap, i, (char)i);
            REQUIRE(r2::shashmap::Has(*hashMap, i));
        }
        
        size_t keyToRemove = (size_t)'F';
        
        r2::shashmap::Remove(*hashMap, keyToRemove);
        
        REQUIRE(!r2::shashmap::Has(*hashMap, keyToRemove));
        
        for (size_t i = 32; i < 127; ++i)
        {
            if (i != keyToRemove)
            {
                REQUIRE(r2::shashmap::Has(*hashMap, i));
                
                const char& val = r2::shashmap::Get(*hashMap, i, defaultChar);
                
                REQUIRE(val == (char)i);
            }
            else
            {
                const char& val = r2::shashmap::Get(*hashMap, i, defaultChar);

                REQUIRE(val == defaultChar);
            }
        }
        
        FREE(hashMap, stackArena);
    }
    
    SECTION("Test smultihash functionality")
    {
        r2::mem::StackArena stackArena(*testMemoryArea->GetSubArea(subAreaHandle));
        
        r2::SHashMap<float>* hashMap = MAKE_SHASHMAP(stackArena, float, 255);
        
        r2::SArray<float>* results = MAKE_SARRAY(stackArena, float, 255);
        
        REQUIRE(hashMap != nullptr);
        REQUIRE(results != nullptr);
        
        size_t key = 100;
        float testVal1 = 100.1;
        float testVal2 = 100.2;
        
        //basic empty tests
        REQUIRE(r2::smultihash::Count(*hashMap, key) == 0);
        
        r2::smultihash::Get(*hashMap, key, *results);
        
        REQUIRE(r2::sarr::Size(*results) == 0);
        
        auto firstResult = r2::smultihash::FindFirst(*hashMap, key);
        
        REQUIRE(firstResult == nullptr);
        
        //test 1 element
        r2::smultihash::Insert(*hashMap, key, testVal1);
        
        REQUIRE(r2::smultihash::Count(*hashMap, key) == 1);
        
        r2::smultihash::Get(*hashMap, key, *results);
        
        REQUIRE(r2::sarr::Size(*results) == 1);
        
        REQUIRE( r2::sarr::At(*results, 0) == testVal1 );
        
        r2::sarr::Clear(*results);
        
        firstResult = r2::smultihash::FindFirst(*hashMap, key);
        
        r2::smultihash::Remove(*hashMap, firstResult);
        
        REQUIRE(r2::smultihash::Count(*hashMap, key) == 0);
        
        r2::smultihash::Get(*hashMap, key, *results);
        
        REQUIRE(r2::sarr::Size(*results) == 0);
        
        
        //test 2 elements with the same key
        
        r2::smultihash::Insert(*hashMap, key, testVal1);
        r2::smultihash::Insert(*hashMap, key, testVal2);
        
        REQUIRE(r2::smultihash::Count(*hashMap, key) == 2);
        
        r2::smultihash::Get(*hashMap, key, *results);
        
        REQUIRE(r2::sarr::Size(*results) == 2);
        
        
        r2::sarr::Clear(*results);
        
        firstResult = r2::smultihash::FindFirst(*hashMap, key);
        
        REQUIRE( firstResult->value == testVal2);
        
        auto nextResult = r2::smultihash::FindNext(*hashMap, firstResult);
        
        REQUIRE(nextResult->value == testVal1);
        
        
        //Remove first one
        r2::smultihash::Remove(*hashMap, firstResult);
        
        REQUIRE(r2::smultihash::Count(*hashMap, key) == 1);
        
        r2::smultihash::Get(*hashMap, key, *results);
        
        REQUIRE(r2::sarr::Size(*results) == 1);
        
        REQUIRE(r2::sarr::At(*results, 0) == testVal1);
        
        r2::sarr::Clear(*results);
        
        firstResult = r2::smultihash::FindFirst(*hashMap, key);
        
        REQUIRE( firstResult->value == testVal1 );
        
        nextResult = r2::smultihash::FindNext(*hashMap, firstResult);
        
        REQUIRE(nextResult == nullptr);
        
        
        //Add first one back
        r2::smultihash::Insert(*hashMap, key, testVal1);
        
        REQUIRE(r2::smultihash::Count(*hashMap, key) == 2);
        
        r2::smultihash::Get(*hashMap, key, *results);
        
        REQUIRE(r2::sarr::Size(*results) == 2);
        
        
        
        r2::sarr::Clear(*results);
        
        
        //Test remove all
        r2::smultihash::RemoveAll(*hashMap, key);
        
        REQUIRE(r2::smultihash::Count(*hashMap, key) == 0);
        
        r2::smultihash::Get(*hashMap, key, *results);
        
        REQUIRE(r2::sarr::Size(*results) == 0);
        
        firstResult = r2::smultihash::FindFirst(*hashMap, key);
        
        REQUIRE(firstResult == nullptr);
        
        
        //Test to see if things still work
        r2::smultihash::Insert(*hashMap, key, testVal1);
        
        REQUIRE(r2::smultihash::Count(*hashMap, key) == 1);
        
        
        FREE(results, stackArena);
        FREE(hashMap, stackArena);
    }
    
    r2::mem::GlobalMemory::Shutdown();
}

TEST_CASE("Test Ring Buffer")
{
    r2::mem::GlobalMemory::Init(1);
    
    auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
    REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
    r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
    REQUIRE(testMemoryArea != nullptr);
    REQUIRE(testMemoryArea->Name() == "TestArea");
    auto result = testMemoryArea->Init(Megabytes(1));
    REQUIRE(result);
    REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(5));
    REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
    auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
    REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
    
    SECTION("Test Ring Buffer with Temp Memory")
    {
        r2::mem::utils::MemBoundary scratchBoundary = testMemoryArea->ScratchBoundary();
        
        r2::mem::RingBufferAllocator ringAllocator(scratchBoundary);
        
        REQUIRE(ringAllocator.GetTotalBytesAllocated() == 0);
        
        const u64 totalMem = ringAllocator.GetTotalMemory();
        
        REQUIRE(totalMem == Megabytes(4));
        
        int* intArray = (int*)ringAllocator.Allocate(512 * sizeof(int), alignof(int), 0);
        
        REQUIRE(intArray != nullptr);
        REQUIRE(ringAllocator.InUse(intArray));
        
        u32 allocationSize = ringAllocator.GetAllocationSize(intArray);
        
        REQUIRE(allocationSize == 513 * sizeof(int)); //1 more for the header
        
        memset(intArray, 0, sizeof(int) * 512);
        for (int i = 1; i < 511; ++i)
        {
            intArray[i] = i;
        }
        
        REQUIRE(intArray[0] == 0);
        REQUIRE(intArray[1] == 1);
        
        REQUIRE(ringAllocator.GetTotalBytesAllocated() >= 512 * sizeof(int));

        ringAllocator.Free(intArray);
        REQUIRE(!ringAllocator.InUse(intArray));
        
        REQUIRE(ringAllocator.GetTotalBytesAllocated() == 0);
        
    }
    
    r2::mem::GlobalMemory::Shutdown();
}


TEST_CASE("Test Ring Arena")
{
    r2::mem::GlobalMemory::Init(1);
    auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
    REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
    r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
    REQUIRE(testMemoryArea != nullptr);
    REQUIRE(testMemoryArea->Name() == "TestArea");
    auto result = testMemoryArea->Init(Megabytes(1), 0);
    REQUIRE(result);
    REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(1));
    REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
    auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
    REQUIRE(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid);
    
    SECTION("Test Ring Arena allocator")
    {
        const int NUM_TEST_CLASSES = 87381;
        r2::mem::RingArena ringArena(*testMemoryArea->GetSubArea(subAreaHandle));
        
        u64 sizeOfTestClass = sizeof(TestClass);
        
        u64 numTestClassesWeCanAllocate = testMemoryArea->GetSubArea(subAreaHandle)->mBoundary.size/(sizeOfTestClass + sizeof(r2::mem::utils::Header));
        
        REQUIRE(numTestClassesWeCanAllocate == NUM_TEST_CLASSES);
        
        TestClass* testClassObjects[NUM_TEST_CLASSES];
        
        for (u64 i = 0; i < numTestClassesWeCanAllocate; ++i)
        {
            testClassObjects[i] = ALLOC(TestClass, ringArena);
            REQUIRE(testClassObjects[i] != nullptr);
        }

        TestClass* t = ALLOC(TestClass, ringArena);
        
        REQUIRE(t == nullptr);
        
        TestClass* t2 = ALLOC(TestClass, ringArena);
        
        REQUIRE(t2 == nullptr);
        
        for (u64 i = 0; i < numTestClassesWeCanAllocate; ++i)
        {
            FREE(testClassObjects[i], ringArena);
            testClassObjects[i] = nullptr;
        }
        
        byte* bytes = ALLOC_BYTESN(ringArena, Megabytes(1)-2*ringArena.HeaderSize(), 4);
        
        REQUIRE(bytes == nullptr);
        
        
        
    }
    
    r2::mem::GlobalMemory::Shutdown();
}

TEST_CASE("Path Utils")
{
    SECTION("Test GetNextSubPath() - Good Case")
    {
        char path[] = "/subpath1/subpath2/subpath3/file.txt";
        char subPath[Kilobytes(1)] = "";
        
        char* resultPath = r2::fs::utils::GetNextSubPath(path, subPath, '/');
        
        REQUIRE(strcmp(path, "/subpath1/subpath2/subpath3/file.txt") == 0);
        REQUIRE(strcmp(resultPath, "/subpath2/subpath3/file.txt") == 0);
        REQUIRE(strcmp(subPath, "subpath1") == 0);
        
        resultPath = r2::fs::utils::GetNextSubPath(resultPath, subPath, '/');
        
        REQUIRE(strcmp(path, "/subpath1/subpath2/subpath3/file.txt") == 0);
        REQUIRE(strcmp(resultPath, "/subpath3/file.txt") == 0);
        REQUIRE(strcmp(subPath, "subpath2") == 0);
        
        resultPath = r2::fs::utils::GetNextSubPath(resultPath, subPath, '/');
        
        REQUIRE(strcmp(path, "/subpath1/subpath2/subpath3/file.txt") == 0);
        REQUIRE(strcmp(resultPath, "/file.txt") == 0);
        REQUIRE(strcmp(subPath, "subpath3") == 0);
        
        resultPath = r2::fs::utils::GetNextSubPath(resultPath, subPath, '/');
        
        REQUIRE(strcmp(path, "/subpath1/subpath2/subpath3/file.txt") == 0);
        REQUIRE(strcmp(resultPath, "") == 0);
        REQUIRE(strcmp(subPath, "file.txt") == 0);
        
        resultPath = r2::fs::utils::GetNextSubPath(resultPath, subPath, '/');
        
        REQUIRE(strcmp(path, "/subpath1/subpath2/subpath3/file.txt") == 0);
        REQUIRE(strcmp(resultPath, "") == 0);
        REQUIRE(strcmp(subPath, "") == 0);
    }
    
    SECTION("Test GetNextSubPath() - Bad Case 1")
    {
        char path[] = "/////////subpath1/subpath2/subpath3/file.txt";
        char subPath[Kilobytes(1)] = "";
        
        char* resultPath = r2::fs::utils::GetNextSubPath(path, subPath, '/');
        
        REQUIRE(strcmp(path, "/////////subpath1/subpath2/subpath3/file.txt") == 0);
        REQUIRE(strcmp(resultPath, "/subpath2/subpath3/file.txt") == 0);
        REQUIRE(strcmp(subPath, "subpath1") == 0);
        
        resultPath = r2::fs::utils::GetNextSubPath(path, subPath, '.');
        
        REQUIRE(strcmp(path, "/////////subpath1/subpath2/subpath3/file.txt") == 0);
        REQUIRE(strcmp(resultPath, ".txt") == 0);
        REQUIRE(strcmp(subPath, "/////////subpath1/subpath2/subpath3/file") == 0);
    }
    
    SECTION("Test GetNextSubPath() - Bad Case 2")
    {
        char path[] = "";
        char subPath[Kilobytes(1)] = "Hello";
        
        char* resultPath = r2::fs::utils::GetNextSubPath(path, subPath, '/');
        
        REQUIRE(strcmp(resultPath, "") == 0);
        REQUIRE(strcmp(subPath, "") == 0);
        
        resultPath = r2::fs::utils::GetNextSubPath(nullptr, subPath, '/');
        
        REQUIRE(resultPath == nullptr);
    }
    
    SECTION("Test NumMatchingSubPaths")
    {
        char path[] = "/subpath1/subpath2/subpath3/file.txt";
        char path2[] = "/subpath1/subpath4";
        char path3[] = "/";
        char path4[] = "/subpath1/subpath2/subpath3/";
        char path5[] = "/subpath1/subpath4/";
        
        char path6[] = "subpath1/subpath2/subpath3";
        
        REQUIRE(r2::fs::utils::NumMatchingSubPaths(path, path2, '/') == 1);
        REQUIRE(r2::fs::utils::NumMatchingSubPaths(path, path3, '/') == 0);
        REQUIRE(r2::fs::utils::NumMatchingSubPaths(path2, path3, '/') == 0);
        REQUIRE(r2::fs::utils::NumMatchingSubPaths(path, path4, '/') == 3);
        REQUIRE(r2::fs::utils::NumMatchingSubPaths(path2, path5, '/') == 2);
        REQUIRE(r2::fs::utils::NumMatchingSubPaths(path, path6, '/') == 3);
    }
    
    SECTION("Test GetLastSubPath")
    {
        char path[] = "/subpath1/subpath2/subpath3/file.txt";
        char subPath[Kilobytes(1)] = "";
        
        char* resultPath = r2::fs::utils::GetLastSubPath(path, subPath, '/');
        
        REQUIRE(strcmp(path, "/subpath1/subpath2/subpath3/file.txt") == 0);
        REQUIRE(strcmp(subPath, "file.txt") == 0);
        REQUIRE(strcmp(resultPath, "/file.txt") == 0);
    }
    
    SECTION("Test File Nam functions")
    {
        char path[] = "/subpath1/subpath2/subpath3/file.txt";
        char tempBuf[Kilobytes(1)] = "";
        
        REQUIRE(r2::fs::utils::CopyFileNameWithExtension(path, tempBuf));
        
        REQUIRE(strcmp("file.txt", tempBuf) == 0);
        
        REQUIRE(r2::fs::utils::CopyFileName(path, tempBuf));
        
        REQUIRE(strcmp("file", tempBuf) == 0);
        
        REQUIRE(r2::fs::utils::CopyFileExtension(path, tempBuf));
        
        REQUIRE(strcmp(".txt", tempBuf) == 0);
    }
    
    
    SECTION("Test CopyDirectoryOfFile")
    {
        char path[] = "/subpath1/subpath2/subpath3/file.txt";
        char tempBuf[Kilobytes(1)] = "";
        
        REQUIRE(r2::fs::utils::CopyDirectoryOfFile(path, tempBuf));
        
        REQUIRE(strcmp(tempBuf, "/subpath1/subpath2/subpath3/") == 0);
    }
    
    SECTION("Test Append Path")
    {
        char path[] = "/subpath1/subpath2/";
        char tempBuf[Kilobytes(1)] = "";
        
        REQUIRE(r2::fs::utils::AppendSubPath(path, tempBuf, "subpath3", '/'));
        REQUIRE(strcmp(tempBuf, "/subpath1/subpath2/subpath3") == 0);
    }
    
    SECTION("Test Append Path no separator")
    {
        char path[] = "/subpath1/subpath2";
        char tempBuf[Kilobytes(1)] = "";
        
        REQUIRE(r2::fs::utils::AppendSubPath(path, tempBuf, "subpath3", '/'));
        REQUIRE(strcmp(tempBuf, "/subpath1/subpath2/subpath3") == 0);
    }
}
