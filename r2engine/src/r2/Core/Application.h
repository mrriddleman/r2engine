//
//  Application.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-02.
//  Copyright © 2019 Serge Lansiquot. All rights reserved.
//

#ifndef Application_hpp
#define Application_hpp


#include "r2/Utils/Utils.h"
#include "r2/Core/Assets/Asset.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Game/ECS/Entity.h"
#include "r2/Core/File/PathUtils.h"

#include <string>

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/MaterialPackManifestUtils.h"
#include "r2/Core/Assets/Pipeline/AssetCommands/ShaderHotReloadCommand.h"
#include "r2/Core/Assets/AssetReference.h"
#endif

#ifdef R2_EDITOR


namespace r2::ecs
{
    class ECSCoordinator;
}

namespace r2::edit
{
    class InspectorPanel;
}

#endif // R2_EDITOR


namespace r2::asset 
{
    class AssetFile;
}

namespace r2::ecs
{
    class ECSWorld;
}


namespace r2::io
{
    struct InputType;
}

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
        virtual char* GetApplicationName() const;

        virtual draw::RendererBackend GetRendererBackend() const;
        void ChangeRendererBackend(r2::draw::RendererBackend newBackend);

        virtual util::Size GetAppResolution() const;
        virtual bool ShouldKeepAspectRatio() const;
        virtual bool ShouldScaleResolution() const;
        virtual bool IsWindowResizable() const;
        virtual bool WindowShouldBeMaximized() const;

        virtual std::string GetAppLogPath() const;
        virtual r2::asset::PathResolver GetPathResolver() const;
        virtual std::string GetSoundDefinitionPath() const;
        virtual void GetSubPathForDirectory(r2::fs::utils::Directory directory, char* subpath) const;
        
        virtual std::string GetShaderManifestsPath() const;
        virtual std::vector<std::string> GetTexturePackManifestsBinaryPaths() const;
        virtual std::vector<std::string> GetTexturePackManifestsRawPaths() const;
        virtual std::vector<std::string> GetMaterialPackManifestsBinaryPaths() const;
        virtual std::vector<std::string> GetMaterialPackManifestsRawPaths() const;
        
        virtual std::vector<std::string> GetModelBinaryPaths() const;
        virtual std::vector<std::string> GetModelRawPaths() const;
        virtual std::vector<std::string> GetModelManifestRawPaths() const;
        virtual std::vector<std::string> GetModelManifestBinaryPaths() const;


        virtual std::vector<std::string> GetAnimationBinaryPaths() const;
        virtual std::vector<std::string> GetAnimationRawPaths() const;

        virtual std::vector<std::string> GetInternalShaderManifestsBinaryPaths() const;
        virtual std::vector<std::string> GetInternalShaderManifestsRawPaths() const;

        virtual std::vector<std::string> GetMaterialPacksManifestsBinaryPaths() const;
       
        virtual u32 GetAssetMemorySize() const = 0;
        virtual u32 GetTextureMemorySize() const = 0;
        virtual u32 GetMaxNumECSSystems() const = 0;
        virtual u32 GetMaxNumECSEntities() const = 0;
        virtual u32 GetMaxNumComponents() const = 0;
        virtual u64 GetECSWorldAuxMemory() const = 0;

		virtual std::string GetLevelPackDataBinPath() const = 0;
		virtual std::string GetLevelPackDataJSONPath() const = 0;

        virtual std::string GetLevelPackDataManifestBinPath() const = 0;
        virtual std::string GetLevelPackDataManifestJSONPath() const = 0;


        virtual void RegisterECSData(r2::ecs::ECSWorld& ecsWorld) = 0;
        virtual void UnRegisterECSData(r2::ecs::ECSWorld& ecsWorld) = 0;


        virtual const r2::io::InputType& GetInputTypeForPlayer(s32 player) const = 0;

#ifdef R2_ASSET_PIPELINE

        virtual std::vector<std::string> GetAssetWatchPaths() const = 0;
        virtual std::string GetAssetManifestPath() const = 0;
        virtual std::string GetAssetCompilerTempPath() const = 0;
        virtual std::vector<std::string> GetSoundDirectoryWatchPaths() const = 0;
        virtual std::vector<std::string> GetTexturePacksWatchPaths() const = 0;
        virtual std::vector<std::string> GetTexturePacksBinaryDirectoryPaths() const = 0;
        virtual std::vector<std::string> GetMaterialPacksWatchPaths() const = 0;
        virtual std::vector<std::string> GetMaterialPacksBinaryPaths() const = 0;
        virtual std::string GetRawSoundDefinitionsPath() const = 0;
        virtual std::vector<r2::asset::pln::FindMaterialPackManifestFileFunc> GetFindMaterialManifestsFuncs() const = 0;
        virtual std::vector<r2::asset::pln::GenerateMaterialPackManifestFromDirectoriesFunc> GetGenerateMaterialManifestsFromDirectoriesFuncs() const = 0;

        virtual r2::asset::pln::InternalShaderPassesBuildFunc GetInternalShaderPassesBuildFunc() const = 0;
#endif

#ifdef R2_EDITOR
        virtual void RegisterComponentImGuiWidgets(r2::edit::InspectorPanel& inspectorPanel) const = 0;
#endif

    protected:
  //      r2::io::InputState mInputState;
       
    };
    
    std::unique_ptr<Application> CreateApplication();
}

#endif /* Application_hpp */
