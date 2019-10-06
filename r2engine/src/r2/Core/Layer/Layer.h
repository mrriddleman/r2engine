//
//  Layer.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-12.
//

#ifndef Layer_h
#define Layer_h

#include "r2/Core/Events/Event.h"

namespace r2
{
    class R2_API Layer
    {
    public:
        Layer(const std::string& name, b32 renderable);
        virtual ~Layer(){}
        
        virtual void Init(){}
        virtual void Shutdown(){}
        virtual void Update(){}
        virtual void ImGuiRender() {}
        virtual void Render(float alpha){}
        virtual void OnEvent(evt::Event& event){}
        inline const std::string& DebugName() const {return mDebugName;}
        inline b32 IsDisabled() const {return mDisabled;}
        inline void SetDisabled(bool disabled) {mDisabled = disabled;}
        inline b32 Renderable() const { return mRenderable;}
        
    private:
        std::string mDebugName;
        b32 mDisabled;
        b32 mRenderable;
    };
}

#endif /* Layer_hpp */
