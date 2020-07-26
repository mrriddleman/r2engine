//
//  Application.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-02.
//  Copyright © 2019 Serge Lansiquot. All rights reserved.
//

#include "r2pch.h"
#include "Application.h"
#include "r2/Platform/Platform.h"
#include "r2/Core/Containers/SinglyLinkedList.h"

namespace
{
    bool ResolveCategoryPath(u32 category, char* path)
    {
        return false;
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
    
    char* Application::GetApplicationName() const
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
    
    std::string Application::GetSoundDefinitionPath() const
    {
        std::string soundDefitionPath = "";
        return soundDefitionPath;
    }
    
    std::string Application::GetShaderManifestsPath() const
    {
        std::string shaderManifestsPath = "";
        return shaderManifestsPath;
    }

    std::vector<std::string> Application::GetTexturePackManifestsBinaryPaths() const
    {
        return {};
    }

    std::vector<std::string> Application::GetTexturePackManifestsRawPaths() const
    {
        return {};
    }

    std::vector<std::string> Application::GetMaterialPackManifestsBinaryPaths() const
    {
        return {};
    }
    
	std::vector<std::string> Application::GetMaterialPackManifestsRawPaths() const
	{
		return {};
	}

#ifdef R2_ASSET_PIPELINE
    std::vector<std::string> Application::GetAssetWatchPaths() const
    {
        std::vector<std::string> paths;
        return paths;
    }
    
    std::string Application::GetAssetManifestPath() const
    {
        return "";
    }

    std::string Application::GetAssetCompilerTempPath() const
    {
        return "";
    }
    
    std::vector<std::string> Application::GetSoundDirectoryWatchPaths() const
    {
        std::vector<std::string> paths;
        return paths;
    }

    std::vector<std::string> Application::GetTexturePacksWatchPaths() const
    {
        return {};
    }

    std::vector<std::string> Application::GetMaterialPacksWatchPaths() const
    {
        return {};
    }
#endif
}
