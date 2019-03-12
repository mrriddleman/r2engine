//
//  r2Catch.cpp
//  r2Tests
//
//  Created by Serge Lansiquot on 2019-03-11.
//

#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"
#include "r2.h"

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
