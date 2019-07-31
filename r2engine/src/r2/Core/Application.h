//
//  Application.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-02.
//  Copyright © 2019 Serge Lansiquot. All rights reserved.
//

#ifndef Application_hpp
#define Application_hpp

#include "r2/Platform/MacroDefines.h"
#include "r2/Utils/Utils.h"
#include "r2/Core/Assets/Asset.h"

#include <memory>
#include <string>

namespace r2
{
    namespace evt
    {
        class Event;
    }
    
    
    class R2_API Application
    {
    public:
        Application()
        {
            
        }
        
        virtual ~Application()
        {
            
        }
        
        virtual bool Init();
        virtual void Update();
        virtual void Shutdown();
        virtual void Render(float alpha);
        virtual void OnEvent(evt::Event& e);
        virtual std::string GetApplicationName() const;
        virtual util::Size GetPreferredResolution() const;
        virtual std::string GetAppLogPath() const;
        virtual r2::asset::PathResolver GetPathResolver() const;
        
    };
    
    std::unique_ptr<Application> CreateApplication();
}

#endif /* Application_hpp */
