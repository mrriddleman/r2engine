//
//  ImGuiLayer.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-16.
//

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
        virtual void Update() override;
        virtual void Render(float alpha) override;
        virtual void OnEvent(evt::Event& event) override;
        
    private:
        bool OnMouseButtonPressedEvent(evt::MouseButtonPressedEvent & e);
        bool OnMouseButtonReleasedEvent(evt::MouseButtonReleasedEvent& e);
        bool OnMouseMovedEvent(evt::MouseMovedEvent& e);
        bool OnMouseWheelEvent(evt::MouseWheelEvent& e);
        bool OnKeyPressedEvent(evt::KeyPressedEvent& e);
        bool OnKeyReleasedEvent(evt::KeyReleasedEvent& e);
        bool OnKeyTypedEvent(evt::KeyTypedEvent& e);
        bool OnWindowResizeEvent(evt::WindowResizeEvent& e);
        
        u64 mTime = 0;
    };
}

#endif /* ImGuiLayer_hpp */
