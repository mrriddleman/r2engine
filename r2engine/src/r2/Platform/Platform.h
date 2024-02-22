//
//  Platform.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-03.
//

#ifndef Platform_hpp
#define Platform_hpp

#include "r2/Core/Engine.h"
#include "r2/Utils/Utils.h"

#define CPLAT r2::Platform::GetConst()
#define MPLAT r2::Platform::Get()

#define CENG r2::Platform::Get().EngineConst()
#define MENG r2::Platform::Get().EngineMutable()

#define CAPP r2::Platform::Get().EngineConst().GetApplication();

namespace r2
{
    class R2_API Platform
    {
    public:
        Platform(){}
        virtual ~Platform(){}
        
        static const r2::Platform& GetConst();
        static r2::Platform& Get();
        
        Platform(const Platform&) = delete;
        Platform& operator=(const Platform&) = delete;
        Platform(Platform && ) = delete;
        Platform& operator=(Platform &&) = delete;
        
        //Pure virtual methods
        virtual bool Init(std::unique_ptr<r2::Application> app) = 0;
        virtual void Run() = 0;
        virtual void Shutdown() = 0;
        
        virtual const f64 TickRate() const = 0;
        virtual const s32 NumLogicalCPUCores() const = 0;
        virtual const s32 SystemRAM() const = 0;
        virtual const s32 CPUCacheLineSize() const = 0;
        virtual const std::string RootPath() const = 0;
        virtual const std::string AppPath() const = 0;
        virtual const std::string SoundDefinitionsPath() const = 0;
        
        inline r2::asset::PathResolver GetPathResolver() const {return mAssetPathResolver;}
        
        const Engine& EngineConst() { return mEngine; }
        Engine& EngineMutable() {return mEngine;}
        
        virtual void ResetAccumulator() = 0;


        virtual u32 GetNumberOfAttachedGameControllers() const = 0;
        virtual void CloseGameController(io::ControllerID index) = 0;
        virtual void CloseAllGameControllers() = 0;
        virtual bool IsGameControllerConnected(io::ControllerID index) const = 0;
		virtual const char* GetGameControllerMapping(r2::io::ControllerID controllerID) const  = 0;
		virtual const char* GetGameControllerButtonName(r2::io::ControllerID controllerID, r2::io::ControllerButtonName buttonName) const  = 0;
		virtual const char* GetGameControllerAxisName(r2::io::ControllerID controllerID, r2::io::ControllerAxisName axisName) const = 0;
		virtual const char* GetGameControllerName(r2::io::ControllerID controllerID) const = 0;

		virtual s32 GetPlayerIndex(r2::io::ControllerID controllerID) const = 0;
		virtual void SetPlayerIndex(r2::io::ControllerID controllerID, s32 playerIndex) = 0;
		virtual io::ControllerType GetControllerType(r2::io::ControllerID controllerID)const = 0;


    protected:
        
        Engine mEngine;
        r2::asset::PathResolver mAssetPathResolver;
    };
}


#endif /* Platform_hpp */
