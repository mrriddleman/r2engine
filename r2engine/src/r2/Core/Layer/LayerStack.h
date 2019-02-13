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
        
        inline std::vector<std::unique_ptr<Layer>>::iterator LayerBegin() {return mLayers.begin();}
        inline ConstLayerIt CLayerBegin() const {return mLayers.cbegin();}

        inline std::vector<std::unique_ptr<Layer>>::iterator LayerEnd() {return mLayers.end();}
        inline ConstLayerIt CLayerEnd() const {return mLayers.cend();}
        
        inline std::vector<std::unique_ptr<Layer>>::iterator OverlayBegin() {return mOverlays.begin();}
        inline ConstLayerIt COverlayBegin() const {return mOverlays.cbegin();}
        
        inline std::vector<std::unique_ptr<Layer>>::iterator OverlayEnd() {return mOverlays.end();}
        inline ConstLayerIt COverlayEnd() const {return mOverlays.cend();}
        
    private:
        LayerStackContainer mLayers;
        LayerStackContainer mOverlays;
        
        static Layer * Release(LayerStackContainer& layers, Layer * layer);
        static void MoveBefore(LayerStackContainer& layers, LayerIt layerToMove, Layer* beforeLayer);
        static void MoveAfter(LayerStackContainer& layers, LayerIt layerToMove, Layer* afterLater);
    };
}

#endif /* LayerStack_hpp */
