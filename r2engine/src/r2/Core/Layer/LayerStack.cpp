//
//  LayerStack.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-12.
//

#include "LayerStack.h"
#include <cassert> //@TODO(Serge): replace with our own assert

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
        for(auto& layer : mLayers)
        {
            layer->Shutdown();
        }
        
        for(auto& overlay : mOverlays)
        {
            overlay->Shutdown();
        }
    }
    
    void LayerStack::Update()
    {
        for(auto& layer : mOverlays)
        {
            if(!layer->IsDisabled())
                layer->Update();
        }
        
        for(auto& layer : mLayers)
        {
            if(!layer->IsDisabled())
                layer->Update();
        }
    }
    
    void LayerStack::Render(float alpha)
    {
        for(auto& layer : mLayers)
        {
            if(!layer->IsDisabled() && layer->Renderable())
                layer->Render(alpha);
        }
        
        for(auto& layer : mOverlays)
        {
            if(!layer->IsDisabled() && layer->Renderable())
                layer->Render(alpha);
        }
    }
    
    void LayerStack::OnEvent(evt::Event& e)
    {
        for(auto layer = mOverlays.end(); layer != mOverlays.begin(); )
        {
            --layer;
            if(!layer->get()->IsDisabled())
            {
                layer->get()->OnEvent(e);
                
                if(e.handled)
                    break;
            }
        }
        
        if(!e.handled)
        {
            for(auto layer = mLayers.end(); layer != mLayers.begin(); )
            {
                --layer;
                if(!layer->get()->IsDisabled())
                {
                    layer->get()->OnEvent(e);
                    
                    if(e.handled)
                        break;
                }
            }
        }
    }
    
    void LayerStack::PushLayer(std::unique_ptr<Layer> layer)
    {
        mLayers.emplace_back(std::move(layer));
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
            theLayer = it->release();
            layers.erase(it);
        }
        
        return theLayer;
    }
    
    void LayerStack::MoveBefore(LayerStackContainer& layers, LayerIt layerToMove, Layer* beforeLayer)
    {
        auto it = std::find(layers.begin(), layers.end(), *layerToMove);
        if(it == layers.end())
            return;
        
        std::unique_ptr<Layer> movedLayer = std::move(*layerToMove);
        
        layers.erase(layerToMove);
        
        auto beforeLayerIt = layers.end();
        for (auto itr = layers.begin(); itr != layers.end(); ++itr)
        {
            if(itr->get() == beforeLayer)
            {
                beforeLayerIt = itr;
            }
        }
        
        layers.insert(beforeLayerIt, std::move(movedLayer));
    }

    void LayerStack::MoveAfter(LayerStackContainer& layers, LayerIt layerToMove, Layer* afterLayer)
    {
        auto it = std::find(layers.begin(), layers.end(), *layerToMove);
        if(it == layers.end())
            return;

        std::unique_ptr<Layer> movedLayer = std::move(*layerToMove);

        layers.erase(layerToMove);

        auto afterLayerIt = layers.begin();
        for (auto itr = layers.begin(); itr != layers.end(); ++itr)
        {
            if(itr->get() == afterLayer)
            {
                afterLayerIt = itr;
            }
        }

        if(afterLayerIt == layers.end())
        {
            layers.insert(afterLayerIt, std::move(movedLayer));
        }
        else
        {
            layers.insert(++afterLayerIt, std::move(movedLayer));
        }
    }
}


