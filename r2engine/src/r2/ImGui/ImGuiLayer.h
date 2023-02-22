//
//  ImGuiLayer.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-16.
//

#ifdef R2_IMGUI
#ifndef ImGuiLayer_hpp
#define ImGuiLayer_hpp

#include "r2/Core/Layer/Layer.h"
#include "r2/Core/Events/Events.h"

namespace r2
{
    class R2_API ImGuiLayer: public Layer
    {
    public:
        
        ImGuiLayer();
        virtual ~ImGuiLayer();
        
        virtual void Init() override;
        virtual void Shutdown() override;

        virtual void ImGuiRender() override;
        
        void Begin();
        void End();
        
    private:
        
        u64 mTime = 0;
    };
}

#endif /* ImGuiLayer_hpp */
#endif