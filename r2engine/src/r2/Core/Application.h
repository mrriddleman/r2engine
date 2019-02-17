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
#include <memory>
#include <string>

namespace r2
{
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
        virtual std::string GetApplicationName() const;
        virtual util::Size GetPreferredResolution() const;
    };
    
    std::unique_ptr<Application> CreateApplication();
}

#endif /* Application_hpp */
