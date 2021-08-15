//
//  AppLayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-18.
//

#include "r2pch.h"
#include "AppLayer.h"

namespace r2
{
    AppLayer::AppLayer(std::unique_ptr<Application> app): Layer("App Layer", true), mApp(std::move(app))
    {
    }
    
    void AppLayer::Init()
    {
        mApp->Init();
    }
    
    void AppLayer::Shutdown()
    {
        mApp->Shutdown();
    }
    
    void AppLayer::Update()
    {
        mApp->Update();
    }
    
    void AppLayer::Render(float alpha)
    {
        mApp->Render(alpha);
    }
    
    void AppLayer::OnEvent(evt::Event& event)
    {
        mApp->OnEvent(event);
    }

    const Application& AppLayer::GetApplication() const
    {
        return *mApp.get();
    }
}
