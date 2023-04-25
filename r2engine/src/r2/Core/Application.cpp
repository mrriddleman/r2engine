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
#include "r2/Core/Events/AppEvent.h"

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

    draw::RendererBackend Application::GetRendererBackend() const
    {
        return draw::RendererBackend::OpenGL;
    }

    void Application::ChangeRendererBackend(r2::draw::RendererBackend newBackend)
    {
        MENG.SetRendererBackend(newBackend);
    }
    
    util::Size Application::GetAppResolution() const
    {
        util::Size res;
        res.width = r2::INITIAL_SCREEN_WIDTH;
        res.height = r2::INITIAL_SCREEN_HEIGHT;
        
        return res;
    }

    bool Application::ShouldKeepAspectRatio() const
    {
        return true;
    }

    bool Application::ShouldScaleResolution() const
    {
        return true;
    }

    bool Application::IsWindowResizable() const
    {
        return true;
    }

    bool Application::WindowShouldBeMaximized() const
    {
        return false;
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

    std::vector<std::string> Application::GetModelBinaryPaths() const
    {
        return {};
    }

    std::vector<std::string> Application::GetModelRawPaths() const
    {
        return {};
    }

    std::vector<std::string> Application::GetAnimationBinaryPaths() const
    {
        return {};
    }

    std::vector<std::string> Application::GetAnimationRawPaths() const
    {
        return {};
    }

    std::vector<std::string> Application::GetInternalShaderManifestsBinaryPaths() const
    {
        return {};
    }

    std::vector<std::string> Application::GetInternalShaderManifestsRawPaths() const
    {
        return {};
    }

    std::vector<std::string> Application::GetMaterialPacksManifestsBinaryPaths() const
    {
        return {};
    }

#ifdef R2_EDITOR

    r2::draw::ModelSystem* Application::GetEditorModelSystem() const
    {
        return nullptr;
    }

    r2::draw::MaterialSystem* Application::GetEditorMaterialSystem() const
    {
        return nullptr;
    }

    r2::draw::AnimationCache* Application::GetEditorAnimationCache() const
    {
        return nullptr;
    }

    void Application::RegisterComponents(r2::ecs::ECSCoordinator* coordinator) const
    {

    }

    void Application::UnRegisterComponents(r2::ecs::ECSCoordinator* coordinator) const
    {

    }
    void Application::AddComponentsToEntity(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity e) const
    {

    }
#endif

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

    std::vector<std::string> Application::GetTexturePacksBinaryDirectoryPaths() const
    {
        return {};
    }

    std::vector<std::string> Application::GetMaterialPacksWatchPaths() const
    {
        return {};
    }

    std::vector<std::string> Application::GetMaterialPacksBinaryPaths() const
    {
        return {};
    }

    std::vector<r2::asset::pln::FindMaterialPackManifestFileFunc> Application::GetFindMaterialManifestsFuncs() const
    {
        return {};
    }

    std::vector<r2::asset::pln::GenerateMaterialPackManifestFromDirectoriesFunc> Application::GetGenerateMaterialManifestsFromDirectoriesFuncs() const
    {
        return {};
    }

    r2::asset::pln::InternalShaderPassesBuildFunc Application::GetInternalShaderPassesBuildFunc() const
    {
        return nullptr;
    }

    std::string Application::GetLevelPackDataBinPath() const
    {
        return "";
    }

    std::string Application::GetLevelPackDataJSONPath() const
    {
        return "";
    }

#endif
}
