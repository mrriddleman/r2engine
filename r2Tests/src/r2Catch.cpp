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
