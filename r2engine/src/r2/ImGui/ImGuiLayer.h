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

        virtual void ImGuiRender(u32 dockingSpaceID) override;
        
        void Begin();
        void End();
        u32 GetDockingSpace();
    private:
        
        u64 mTime = 0;
        u32 mDockingSpace;
    };
}

#endif /* ImGuiLayer_hpp */
#endif