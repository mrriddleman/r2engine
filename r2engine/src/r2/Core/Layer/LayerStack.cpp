//
//  LayerStack.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-12.
//

#include "r2pch.h"
#include "LayerStack.h"
#include "AppLayer.h"


namespace r2
{
    LayerStack::LayerStack(u32 maxLength)
    {
        mLayers.reserve(maxLength);
        mOverlays.reserve(maxLength);
    }
    
    LayerStack::~LayerStack()
    {
        
    }
    
    void LayerStack::ShutdownAll()
    {
        for(auto layer = mLayers.rbegin(); layer != mLayers.rend(); ++layer)
        {
            layer->get()->Shutdown();
        }
        
        for(auto layer = mOverlays.rbegin(); layer != mOverlays.rend(); ++layer)
        {
            layer->get()->Shutdown();
        }
    }
    
    void LayerStack::Update()
    {
        for(auto layer = mOverlays.rbegin(); layer != mOverlays.rend(); ++layer)
        {
            if(!layer->get()->IsDisabled())
                layer->get()->Update();
        }
        
        for(auto layer = mLayers.rbegin(); layer != mLayers.rend(); ++layer)
        {
            if(!layer->get()->IsDisabled())
                layer->get()->Update();
        }
    }
    
    void LayerStack::Render(float alpha)
    {
        for(auto layer = mLayers.rbegin(); layer != mLayers.rend(); ++layer)
        {
            if(!layer->get()->IsDisabled() && layer->get()->Renderable())
                layer->get()->Render(alpha);
        }
        
        for(auto layer = mOverlays.rbegin(); layer != mOverlays.rend(); ++layer)
        {
            if(!layer->get()->IsDisabled() && layer->get()->Renderable())
                layer->get()->Render(alpha);
        }
    }
    
    void LayerStack::ImGuiRender(u32 dockingSpaceID)
    {
        //@TODO(Serge): we may want to re-order these OR iterate backwards in the layer stack
//              essentially we want to make sure that the render layer is called last!
        for(auto layer = mLayers.rbegin(); layer != mLayers.rend(); ++layer)
        {
            if(!layer->get()->IsDisabled())
                layer->get()->ImGuiRender(dockingSpaceID);
        }
        
        for(auto layer = mOverlays.rbegin(); layer != mOverlays.rend(); ++layer)
        {
            if(!layer->get()->IsDisabled())
                layer->get()->ImGuiRender(dockingSpaceID);
        }
    }
    
    void LayerStack::OnEvent(evt::Event& e)
    {
        for(auto layer = mOverlays.rbegin(); layer != mOverlays.rend(); ++layer)
        {
            if(!layer->get()->IsDisabled())
            {
                layer->get()->OnEvent(e);
                
                if(e.handled && !e.OverrideEventHandled())
                    break;
            }
        }
        
        if(!e.handled || (e.handled && !e.OverrideEventHandled()))
        {
            for(auto layer = mLayers.rbegin(); layer != mLayers.rend(); ++layer)
            {
                if(!layer->get()->IsDisabled())
                {
                    layer->get()->OnEvent(e);
                    
                    if(e.handled && !e.OverrideEventHandled())
                        break;
                }
            }
        }
    }

    const Application& LayerStack::GetApplication() const
    {
        return GetAppLayer().GetApplication();
    }

    const AppLayer& LayerStack::GetAppLayer() const
    {
        return *mAppLayer.get();
    }
    
    void LayerStack::PushLayer(std::unique_ptr<Layer> layer)
    {
        mLayers.emplace_back(std::move(layer));
    }

    void LayerStack::PushLayer(std::unique_ptr<AppLayer> layer)
    {
        mLayers.emplace_back(std::move(layer));
        mAppLayer = std::static_pointer_cast<AppLayer>(mLayers.back());
    }
    
    Layer* LayerStack::ReleaseLayer(Layer* layer)
    {
        return Release(mLayers, layer);
    }
    
    void LayerStack::PushOverlay(std::unique_ptr<Layer> overlay)
    {
        mOverlays.emplace_back(std::move(overlay));
    }
    
    Layer* LayerStack::ReleaseOverlay(Layer * overlay)
    {
        return Release(mOverlays, overlay);
    }

    void LayerStack::LayerMoveBefore(LayerIt layerToMove, Layer* beforeLayer)
    {
        MoveBefore(mLayers, layerToMove, beforeLayer);
    }
    
    void LayerStack::LayerMoveAfter(LayerIt layerToMove, Layer* afterLayer)
    {
        MoveAfter(mLayers, layerToMove, afterLayer);
    }
    
    void LayerStack::OverlayMoveBefore(LayerIt layerToMove, Layer* beforeLayer)
    {
        MoveBefore(mOverlays, layerToMove, beforeLayer);
    }
    
    void LayerStack::OverlayMoveAfter(LayerIt layerToMove, Layer* afterLayer)
    {
        MoveAfter(mOverlays, layerToMove, afterLayer);
    }
    
    Layer * LayerStack::Release(LayerStackContainer& layers, Layer * layer)
    {
        auto it = layers.begin();
        for(; it != layers.end(); it++)
        {
            if(it->get() == layer)
            {
                break;
            }
        }
        
        Layer* theLayer = nullptr;
        
        if(it != layers.end())
        {
        //    theLayer = it->release();
            layers.erase(it);
        }
        
        return theLayer;
    }
    
    void LayerStack::MoveBefore(LayerStackContainer& layers, LayerIt layerToMove, Layer* beforeLayer)
    {
        auto it = std::find(layers.begin(), layers.end(), *layerToMove);
        if(it == layers.end())
            return;
        
        if(it->get() == beforeLayer)
            return;
        
        std::shared_ptr<Layer> movedLayer = std::move(*layerToMove);
        
        layers.erase(layerToMove);
        
        auto beforeLayerIt = layers.end();
        for (auto itr = layers.begin(); itr != layers.end(); ++itr)
        {
            if(itr->get() == beforeLayer)
            {
                beforeLayerIt = ++itr;
                break;
            }
        }
        
        if(beforeLayerIt == layers.end())
        {
            layers.emplace_back(std::move(movedLayer));
        }
        else
        {
            layers.emplace(beforeLayerIt, std::move(movedLayer));
        }
    }

    void LayerStack::MoveAfter(LayerStackContainer& layers, LayerIt layerToMove, Layer* afterLayer)
    {
        auto it = std::find(layers.begin(), layers.end(), *layerToMove);
        if(it == layers.end())
            return;
        
        if(it->get() == afterLayer)
            return;

        std::shared_ptr<Layer> movedLayer = std::move(*layerToMove);

        layers.erase(layerToMove);

        auto afterLayerIt = layers.begin();
        for (auto itr = layers.begin(); itr != layers.end(); ++itr)
        {
            if(itr->get() == afterLayer)
            {
                afterLayerIt = itr;
            }
        }

        layers.emplace(afterLayerIt, std::move(movedLayer));
    }
    
    void LayerStack::PrintLayerStack(b32 topToBottom)
    {
        R2_LOGI( "====================================\n" );
        
        if(topToBottom)
        {
            for(auto layer = mLayers.end(); layer != mLayers.begin(); )
            {
                --layer;
                R2_LOGI("%s", layer->get()->DebugName().c_str());
            }
        }
        else
        {
            for(auto& layer : mLayers)
            {
                R2_LOGI("%s", layer->DebugName().c_str());
            }
        }
        
        R2_LOGI( "====================================\n\n" );
    }
}


