//
//  Application.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-02.
//  Copyright © 2019 Serge Lansiquot. All rights reserved.
//

#include "Application.h"
#include "r2/Platform/Platform.h"

namespace
{
    bool ResolveCategoryPath(u32 category, char* path)
    {
        
        //@TODO(Serge): implement some basic categories for testing later
        
        strcpy(path, CPLAT.RootPath().c_str());
        return true;
    }
}

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
    
    r2::asset::PathResolver Application::GetPathResolver() const
    {
        return ResolveCategoryPath;
    }
    
#ifdef R2_DEBUG
    std::vector<std::string> Application::GetAssetWatchPaths() const
    {
        
        std::vector<std::string> paths = {
            //Add watch paths here
            R2_ENGINE_ASSETS
        };
        return paths;
    }
    
    std::string Application::GetAssetManifestPath() const
    {
        return "";
    }
#endif
}
