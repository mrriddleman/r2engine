//
//  Application.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-02.
//  Copyright © 2019 Serge Lansiquot. All rights reserved.
//

#include "Application.h"

namespace r2
{
    bool Application::Init()
    {
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
        return "Test App";
    }
    
    util::Size Application::GetPreferredResolution() const
    {
        util::Size res;
        res.width = 1024;
        res.height = 720;
        
        return res;
    }
}


