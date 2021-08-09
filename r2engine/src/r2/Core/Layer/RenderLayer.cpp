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
    RenderLayer::RenderLayer(const char* shaderManifestPath, const char* internalShaderManifestPath)
        : Layer("Render Layer", true)
		
	{
        r2::util::PathCpy(mShaderManifestPath, shaderManifestPath);
        r2::util::PathCpy(mInternalShaderManifestPath, internalShaderManifestPath);
    }
    
    void RenderLayer::Init()
    {
        r2::mem::InternalEngineMemory& engineMem = r2::mem::GlobalMemory::EngineMemory();
        r2::draw::renderer::Init(engineMem.internalEngineMemoryHandle, mShaderManifestPath, mInternalShaderManifestPath);
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
		r2::evt::EventDispatcher dispatcher(event);

		dispatcher.Dispatch<r2::evt::WindowResizeEvent>([this](const r2::evt::WindowResizeEvent& e)
		{
           //printf("Render Layer: Window resized to width: %d, height: %d\n", e.Width(), e.Height());
		    r2::draw::renderer::WindowResized(e.Width(), e.Height());
			return true;
		});
    }
    
    void RenderLayer::Shutdown()
    {
        r2::draw::renderer::Shutdown();
        //@Temp
        //r2::draw::OpenGLShutdown();
    }
}
