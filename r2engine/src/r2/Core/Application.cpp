//
//  Application.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-02.
//  Copyright © 2019 Serge Lansiquot. All rights reserved.
//

#include "Application.h"
#include "r2/Platform/Platform.h"

namespace r2
{
    bool Application::Init()
    {
        R2_LOGI("App Init()");
        return true;
    }
    
    void Application::Update()
    {
        
    }
    
    void Application::Render(float alpha)
    {
        
    }
    
    void Application::Shutdown()
    {
        
    }
    
    void Application::OnEvent(evt::Event& e)
    {
        
    }
    
    std::string Application::GetApplicationName() const
    {
        return "Test_App";
    }
    
    util::Size Application::GetPreferredResolution() const
    {
        util::Size res;
        res.width = 1024;
        res.height = 720;
        
        return res;
    }
    
    std::string Application::GetAppLogPath() const
    {
        return CPLAT.AppPath() + "logs/";
    }
}
