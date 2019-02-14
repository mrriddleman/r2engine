//
//  LayerStack.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-12.
//

#ifndef LayerStack_hpp
#define LayerStack_hpp

#include "r2/Core/Layer/Layer.h"
#include "r2/Core/Events/Event.h"

namespace r2
{
    class R2_API LayerStack
    {
    public:
        
        using LayerStackContainer = std::vector<std::unique_ptr<Layer > >;
        using LayerIt = std::vector<std::unique_ptr<Layer > >::iterator;
        using ConstLayerIt = std::vector<std::unique_ptr<Layer > >::const_iterator;
        
        LayerStack(u32 maxLength = 100);
        ~LayerStack();
        
        void ShutdownAll();
        
        void Update();
        void Render(float alpha);
        void OnEvent(evt::Event& e);
        
        void PushLayer(std::unique_ptr<Layer> layer);
        //release ownership of the layer and remove it from the stack
        Layer* ReleaseLayer(Layer* layer);
        
        void PushOverlay(std::unique_ptr<Layer> overlay);
        //release ownership of the layer and remove it from the stack
        Layer* ReleaseOverlay(Layer* layer);
        
        void LayerMoveBefore(LayerIt layerToMove, Layer* beforeLayer);
        void LayerMoveAfter(LayerIt layerToMove, Layer* afterLayer);
        void OverlayMoveBefore(LayerIt layerToMove, Layer* beforeLayer);
        void OverlayMoveAfter(LayerIt layerToMove, Layer* afterLayer);
        
        inline LayerIt BottomLayer() {return mLayers.begin();}
        inline ConstLayerIt CBottomLayer() const {return mLayers.cbegin();}

        inline LayerIt TopLayer()
        {
            if(mLayers.size() > 0)
                return --mLayers.end();
            return mLayers.end();
        }
        
        inline ConstLayerIt CTopLayer() const
        {
            if(mLayers.size() > 0)
                return --mLayers.cend();
            return mLayers.cend();
        }
        
        inline LayerIt BottomOverlay() {return mOverlays.begin();}
        inline ConstLayerIt CBottomOverlay() const {return mOverlays.cbegin();}
        
        inline LayerIt TopOverlay()
        {
            if(mOverlays.size() > 0)
                return --mOverlays.end();
            return mOverlays.end();
        }
        
        inline ConstLayerIt CTopOverlay() const
        {
            if(mOverlays.size() > 0)
                return --mOverlays.cend();
            return mOverlays.cend();
        }
        
        //DEBUG
        
        void PrintLayerStack(b32 topToBottom = true);
        
    private:
        LayerStackContainer mLayers;
        LayerStackContainer mOverlays;
        
        static Layer * Release(LayerStackContainer& layers, Layer * layer);
        static void MoveBefore(LayerStackContainer& layers, LayerIt layerToMove, Layer* beforeLayer);
        static void MoveAfter(LayerStackContainer& layers, LayerIt layerToMove, Layer* afterLater);
    };
}

#endif /* LayerStack_hpp */
