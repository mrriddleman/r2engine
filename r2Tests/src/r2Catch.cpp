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
#include <cstring>

TEST_CASE("TEST GLOBAL MEMORY")
{
    r2::mem::GlobalMemory::Init<1>();
    
    SECTION("Test Create Memory Area")
    {
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1));
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(1));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subArea1Handle = testMemoryArea->AddSubArea(Kilobytes(512));
        
        REQUIRE(subArea1Handle != r2::mem::MemoryArea::MemorySubArea::Invalid);
        
        auto subArea2Handle = testMemoryArea->AddSubArea(Kilobytes(512));
        
        REQUIRE(subArea2Handle != r2::mem::MemoryArea::MemorySubArea::Invalid);
        
        auto subArea3Handle = testMemoryArea->AddSubArea(Kilobytes(512));
        
        REQUIRE(subArea3Handle == r2::mem::MemoryArea::MemorySubArea::Invalid);
        
        REQUIRE(testMemoryArea->SubAreas().size() == 2);
        
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
    r2::mem::GlobalMemory::Init<1>();
    SECTION("Test Linear Allocator")
    {
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1));
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(1));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::MemorySubArea::Invalid);
        
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
    int Square() const {return x * x;}
    int x;
};

class TestClass2
{
public:
    TestClass2(int x):mX(x){}
    inline int X() const {return mX;}
private:
    int mX;
};

TEST_CASE("Test Linear Memory Arena No Checking")
{
    r2::mem::GlobalMemory::Init<1>();
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
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(1));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::MemorySubArea::Invalid);
        
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
    r2::mem::GlobalMemory::Init<1>();
    SECTION("Test Stack Allocator")
    {
        auto testAreaHandle = r2::mem::GlobalMemory::AddMemoryArea("TestArea");
        
        REQUIRE(testAreaHandle != r2::mem::MemoryArea::Invalid);
        
        r2::mem::MemoryArea* testMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(testAreaHandle);
        
        REQUIRE(testMemoryArea != nullptr);
        
        REQUIRE(testMemoryArea->Name() == "TestArea");
        
        auto result = testMemoryArea->Init(Megabytes(1));
        
        REQUIRE(result);
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(1));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::MemorySubArea::Invalid);
        
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
        
        memset(stringPtr, '\0', Kilobytes(0));

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
    r2::mem::GlobalMemory::Init<1>();
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
        
        REQUIRE(testMemoryArea->AreaBoundary().size == Megabytes(1));
        
        REQUIRE(testMemoryArea->AreaBoundary().location != nullptr);
        
        auto subAreaHandle = testMemoryArea->AddSubArea(Megabytes(1));
        
        REQUIRE(subAreaHandle != r2::mem::MemoryArea::MemorySubArea::Invalid);
        
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
