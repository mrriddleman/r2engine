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
#include "r2/Core/Layer/LayerStack.h"

namespace r2
{
    namespace evt
    {
        class Event;
    }
    using SetVSyncFunc = std::function<bool (bool)>;
    using SetFullScreenFunc = std::function<bool (u32 flags)>;
    using SetWindowSizeFunc = std::function<void (s32 width, s32 height)>;
    typedef const char* (*GetClipboardTextFunc)(void* user_data);
    typedef void (*SetClipboardTextFunc)(void* user_data, const char* text);
    using GetPerformanceFrequencyFunc = std::function<u64 (void)>;
    using GetPerformanceCounterFunc = std::function<u64 (void)>;
    
    class R2_API Engine
    {
    public:
        Engine();
        ~Engine();
        
        bool Init(std::unique_ptr<Application> app);
        void Update();
        void Shutdown();
        void Render(float alpha);
        util::Size GetInitialResolution() const;
        inline util::Size DisplaySize() const {return mDisplaySize;}
        
        const std::string& OrganizationName() const;
        
        
        u64 GetPerformanceFrequency() const;
        u64 GetPerformanceCounter() const;
        
        
        //Layers
        void PushLayer(std::unique_ptr<Layer> layer);
        void PushOverlay(std::unique_ptr<Layer> overlay);
        
        //Platform callbacks
        inline void SetVSyncCallback(SetVSyncFunc vsync) { mSetVSyncFunc = vsync; }
        inline void SetFullscreenCallback(SetFullScreenFunc fullscreen) {mFullScreenFunc = fullscreen;}
        inline void SetScreenSizeCallback(SetWindowSizeFunc windowSize) {mWindowSizeFunc = windowSize;}
        SetClipboardTextFunc mSetClipboardTextFunc;
        GetClipboardTextFunc mGetClipboardTextFunc;
        inline void SetGetPerformanceFrequencyCallback(GetPerformanceFrequencyFunc func) {mGetPerformanceFrequencyFunc = func;}
        inline void SetGetPerformanceCounterCallback(GetPerformanceCounterFunc func) {mGetPerformanceCounterFunc = func;}
        
        //Memory
        
        
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
        void TextEvent(const char* text);
        
    private:
        
        void OnEvent(evt::Event& e);
    
        util::Size mDisplaySize;
        SetVSyncFunc mSetVSyncFunc = nullptr;
        SetFullScreenFunc mFullScreenFunc = nullptr;
        SetWindowSizeFunc mWindowSizeFunc = nullptr;
        GetPerformanceFrequencyFunc mGetPerformanceFrequencyFunc = nullptr;
        GetPerformanceCounterFunc mGetPerformanceCounterFunc = nullptr;
        
        LayerStack mLayerStack;
    };
}


#endif /* Engine_hpp */
