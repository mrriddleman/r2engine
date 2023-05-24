//
//  RenderLayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-04.
//

#include "r2pch.h"
#include "RenderLayer.h"
#include "r2/Core/Events/Events.h"
//#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Platform/Platform.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Core/Application.h"



namespace r2
{
    RenderLayer::RenderLayer()
        : Layer("Render Layer", true)
	{

    }
    
    void RenderLayer::Init()
    {
    }
    
    void RenderLayer::Update()
    {
        r2::draw::renderer::Update(MENG.GetCurrentRendererRef());
    }
    
    void RenderLayer::Render(float alpha)
    {
        r2::draw::renderer::Render(MENG.GetCurrentRendererRef(), alpha);
    }

	void UpdateRendererResolution(u32 windowWidth, u32 windowHeight, u32 resolutionX, u32 resolutionY)
	{
		R2_CHECK(windowWidth >= resolutionX, "This should never be the case.");
		R2_CHECK(windowHeight >= resolutionY, "This should never be the case.");

		const bool shouldKeepAspectRatio = CENG.GetApplication().ShouldKeepAspectRatio();
		const bool shouldScale = CENG.GetApplication().ShouldScaleResolution();

		float xScale = 1.0f;
		float yScale = 1.0f;
		float xOffset = 0.0f, yOffset = 0.0f;
		float scale = 1.0;

		if (shouldKeepAspectRatio)
		{
			if (shouldScale)
			{
				xScale = static_cast<float>(windowWidth) / static_cast<float>(resolutionX);
				yScale = static_cast<float>(windowHeight) / static_cast<float>(resolutionY);
				scale = std::min(xScale, yScale);

				xOffset = (windowWidth - round(scale * resolutionX)) / 2.0f;
				yOffset = (windowHeight - round(scale * resolutionY)) / 2.0f;

				xScale = scale;
				yScale = scale;
			}
			else
			{
				xScale = 1.0f;
				yScale = 1.0f;
				xOffset = (windowWidth - resolutionX) / 2.0f;
				yOffset = (windowHeight - resolutionY) / 2.0f;
			}
		}
		else
		{
			resolutionX = windowWidth;
			resolutionY = windowHeight;
		}

		//printf("Render Layer: Window resized to width: %d, height: %d\n", e.Width(), e.Height());
		r2::draw::renderer::WindowResized(MENG.GetCurrentRendererRef(), windowWidth, windowHeight, resolutionX, resolutionY, xScale, yScale, xOffset, yOffset);

	}
    
    void RenderLayer::OnEvent(evt::Event& event)
    {
		r2::evt::EventDispatcher dispatcher(event);

		dispatcher.Dispatch<r2::evt::WindowResizeEvent>([this](const r2::evt::WindowResizeEvent& e)
		{
			const util::Size appResolution = CENG.GetApplication().GetAppResolution();
			UpdateRendererResolution(e.NewWidth(), e.NewHeight(), appResolution.width, appResolution.height);

			return true;
		});

		dispatcher.Dispatch<r2::evt::ResolutionChangedEvent>([this](const evt::ResolutionChangedEvent& e)
		{
			UpdateRendererResolution(CENG.DisplaySize().width, CENG.DisplaySize().height, e.ResolutionX(), e.ResolutionY());
			return true;
		});
    }
    
    void RenderLayer::Shutdown()
    {
      //  r2::draw::renderer::Shutdown();
        //@Temp
        //r2::draw::OpenGLShutdown();
    }
}
