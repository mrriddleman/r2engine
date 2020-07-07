//
//  RenderLayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-04.
//

#include "r2pch.h"
#include "RenderLayer.h"
#include "r2/Core/Events/Events.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Platform/Platform.h"
//temp
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Render/Renderer/RenderKey.h"
//#include "r2/Render/Backends/RendererBackend.h"



namespace r2
{
    RenderLayer::RenderLayer(const char* shaderManifestPath)
        : Layer("Render Layer", true)
        , mShaderManifestPath(shaderManifestPath)
    {
        
    }
    
    void RenderLayer::Init()
    {
        r2::mem::InternalEngineMemory& engineMem = r2::mem::GlobalMemory::EngineMemory();
        r2::draw::renderer::Init(engineMem.internalEngineMemoryHandle, mShaderManifestPath);
    }
    
    void RenderLayer::Update()
    {
        r2::draw::renderer::Update();
    }
    
    void RenderLayer::Render(float alpha)
    {
        r2::draw::renderer::Render(alpha);
    }
    
    void RenderLayer::OnEvent(evt::Event& event)
    {
       
    }
    
    void RenderLayer::Shutdown()
    {
        r2::draw::renderer::Shutdown();
        //@Temp
        //r2::draw::OpenGLShutdown();
    }
}
