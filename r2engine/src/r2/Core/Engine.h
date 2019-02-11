//
//  Engine.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-08.
//

#ifndef Engine_hpp
#define Engine_hpp

#include "r2/Core/Application.h"
#include "r2/Platform/IO.h"



namespace r2
{
    namespace evt
    {
        class Event;
    }
    using SetVSyncFunc = std::function<bool (bool)>;
    using SetFullScreenFunc = std::function<bool (u32 flags)>;
    using SetWindowSizeFunc = std::function<void (s32 width, s32 height)>;
    
    class R2_API Engine
    {
    public:
        Engine();
        ~Engine();
        bool Init(std::unique_ptr<Application> app);
        void Update();
        void Shutdown();
        void Render(float alpha);
        utils::Size GetInitialResolution() const;
        const std::string& OrganizationName() const;
        
        inline void SetVSyncCallback(SetVSyncFunc vsync) { mSetVSyncFunc = vsync; }
        inline void SetFullscreenCallback(SetFullScreenFunc fullscreen) {mFullScreenFunc = fullscreen;}
        inline void SetScreenSize(SetWindowSizeFunc windowSize) {mWindowSizeFunc = windowSize;}
        
        //Events
        //@TODO(Serge): have different windows?
        void QuitTriggered();
        
        //Window events
        void WindowResizedEvent(u32 width, u32 height);
        void WindowSizeChangedEvent(u32 width, u32 height);
        
        //Mouse events
        void MouseButtonEvent(io::MouseData mouseData);
        void MouseMovedEvent(io::MouseData mouseData);
        void MouseWheelEvent(io::MouseData mouseData);
        
        //KeyEvents
        void KeyEvent(io::Key keyData);
        
    private:
        
        void OnEvent(evt::Event& e);
        
        std::unique_ptr<Application> mApp;
        SetVSyncFunc mSetVSyncFunc;
        SetFullScreenFunc mFullScreenFunc;
        SetWindowSizeFunc mWindowSizeFunc;
    };
}


#endif /* Engine_hpp */
