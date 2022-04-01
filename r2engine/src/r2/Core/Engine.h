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

#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommandHandler.h"

#endif

namespace r2::draw
{
    struct Renderer;
}

namespace r2
{
    class ImGuiLayer;
    
    extern const s32 FULL_SCREEN_WINDOW;
    extern const s32 FULL_SCREEN_DESKTOP;

    namespace evt
    {
        class Event;
    }
    
    class R2_API Engine
    {
    public:

        

        Engine();
        ~Engine();
        
        bool Init(std::unique_ptr<Application> app);
        void Update();
        void Shutdown();
        void Render(float alpha);
        inline util::Size DisplaySize() const {return mDisplaySize;}
        
        const std::string& OrganizationName() const;
        
        u64 GetPerformanceFrequency() const;
        u64 GetPerformanceCounter() const;
        f64 GetTicks() const;
        
        //Game Controllers functions
        r2::io::ControllerID OpenGameController(r2::io::ControllerID controllerID);
        u32 NumberOfGameControllers() const;
        bool IsGameController(r2::io::ControllerID controllerID);
        void CloseGameController(r2::io::ControllerID controllerID);
        bool IsGameControllerAttached(r2::io::ControllerID controllerID);
        const char* GetGameControllerMapping(r2::io::ControllerID controllerID);
        u8 GetGameControllerButtonState(r2::io::ControllerID controllerID, r2::io::ControllerButtonName buttonName);
        s16 GetGameControllerAxisValue(r2::io::ControllerID controllerID, r2::io::ControllerAxisName axisName);
        const char* GetGameControllerButtonName(r2::io::ControllerButtonName buttonName);
        const char* GetGameControllerAxisName(r2::io::ControllerAxisName axisName);
        const char* GetGameControllerName(r2::io::ControllerID controllerID);

        void SetResolution(util::Size previousResolution, util::Size newResolution);
        void SetFullScreen();
        void SetRendererBackend(r2::draw::RendererBackend newBackend);

        r2::draw::Renderer* GetCurrentRendererPtr();
        r2::draw::Renderer& GetCurrentRendererRef();

        //Layers
        void PushLayer(std::unique_ptr<Layer> layer);
        void PushAppLayer(std::unique_ptr<AppLayer> appLayer);
        void PushOverlay(std::unique_ptr<Layer> overlay);

        //GetApplication
        const Application& GetApplication() const;
        
    private:
        static const u32 NUM_PLATFORM_CONTROLLERS = 8;
        friend class SDL2Platform;
        
        //Platform callbacks
        using SetVSyncFunc = std::function<bool (bool)>;
        using SetFullScreenFunc = std::function<bool (u32 flags)>;
        using SetWindowSizeFunc = std::function<void (s32 width, s32 height)>;
        using SetWindowPositionFunc = std::function<void(s32 xPos, s32 yPos)>;
        using CenterWindowFunc = std::function<void()>;

        typedef const char* (*GetClipboardTextFunc)(void* user_data);
        typedef void (*SetClipboardTextFunc)(void* user_data, const char* text);
        using GetPerformanceFrequencyFunc = std::function<u64 (void)>;
        using GetPerformanceCounterFunc = std::function<u64 (void)>;
        using GetTicksFunc = std::function<f64 (void)>;
        
        //Game Controller callbacks from the platform
        using OpenGameControllerFunc = std::function<void* (r2::io::ControllerID controllerID)>;
        using IsGameControllerFunc = std::function<bool (r2::io::ControllerID controllerID)>;
        using NumberOfGameControllersFunc = std::function<u32 (void)>;
        using CloseGameControllerFunc = std::function<void (void*)>;
        using IsGameControllerAttachedFunc = std::function<bool (void*)>;
        using GetGameControllerMappingFunc = std::function<const char* (void*)>;
        using GetGameControllerButtonStateFunc = std::function<u8 (void*, r2::io::ControllerButtonName)>;
        using GetGameControllerAxisValueFunc = std::function<s16 (void*, r2::io::ControllerAxisName)>;
        using GetStringForButtonFunc = std::function<const char* (r2::io::ControllerButtonName)>;
        using GetStringForAxisFunc = std::function<const char* (r2::io::ControllerAxisName)>;
        using GetGameControllerNameFunc = std::function<const char* (void*)>;
        
        //Platform callbacks
        inline void SetVSyncCallback(SetVSyncFunc vsync) { mSetVSyncFunc = vsync; }
        inline void SetFullscreenCallback(SetFullScreenFunc fullscreen) {mFullScreenFunc = fullscreen;}
        inline void SetScreenSizeCallback(SetWindowSizeFunc windowSize) {mWindowSizeFunc = windowSize;}
        inline void SetWindowPositionCallback(SetWindowPositionFunc windowPos) { mWindowPosFunc = windowPos; }
        inline void CenterWindowCallback(CenterWindowFunc centerWindow) { mCenterWindowFunc = centerWindow; }
        SetClipboardTextFunc mSetClipboardTextFunc;
        GetClipboardTextFunc mGetClipboardTextFunc;
        inline void SetGetPerformanceFrequencyCallback(GetPerformanceFrequencyFunc func) {mGetPerformanceFrequencyFunc = func;}
        inline void SetGetPerformanceCounterCallback(GetPerformanceCounterFunc func) {mGetPerformanceCounterFunc = func;}
        inline void SetGetTicksCallback(GetTicksFunc func)
        {
            mGetTicksFunc = func;
        }
        
        //Platform Game Controller callbacks
        inline void SetOpenGameControllerCallback(OpenGameControllerFunc func)
        {
            mOpenGameControllerFunc = func;
        }
        
        inline void SetNumberOfGameControllersCallback(NumberOfGameControllersFunc func)
        {
            mNumberOfGameControllersFunc = func;
        }
        
        inline void SetIsGameControllerCallback(IsGameControllerFunc func)
        {
            mIsGameControllerFunc = func;
        }
        
        inline void SetCloseGameControllerCallback(CloseGameControllerFunc func)
        {
            mCloseGameControllerFunc = func;
        }
        
        inline void SetIsGameControllerAttchedCallback(IsGameControllerAttachedFunc func)
        {
            mIsGameControllerAttchedFunc = func;
        }
        
        inline void SetGetGameControllerMappingCallback(GetGameControllerMappingFunc func)
        {
            mGetGameControllerMappingFunc = func;
        }
        
        inline void SetGetGameControllerButtonStateCallback(GetGameControllerButtonStateFunc func)
        {
            mGetGameControllerButtonStateFunc = func;
        }
        
        inline void SetGetGameControllerAxisValueCallback(GetGameControllerAxisValueFunc func)
        {
            mGetGameControllerAxisValueFunc = func;
        }
        
        inline void SetGetStringForButtonCallback(GetStringForButtonFunc func)
        {
            mGetStringForButtonFunc = func;
        }
        
        inline void SetGetStringForAxisCallback(GetStringForAxisFunc func)
        {
            mGetStringForAxisFunc = func;
        }
        
        inline void SetGetGameControllerNameFunc(GetGameControllerNameFunc func)
        {
            mGetGameControllerNameFunc = func;
        }
        
        void OnEvent(evt::Event& e);
        void DetectGameControllers();
        
        //Events
        //@TODO(Serge): have different windows?
        void QuitTriggered();
        
        //Window events


        void WindowResizedEvent(u32 width, u32 height);
        void WindowSizeChangedEvent(u32 width, u32 height);
        void WindowSizeEventInternal(u32 width, u32 height, u32 resX, u32 resY);
        
        void WindowMinimizedEvent();
        void WindowUnMinimizedEvent();
        
        //Mouse events
        void MouseButtonEvent(io::MouseData mouseData);
        void MouseMovedEvent(io::MouseData mouseData);
        void MouseWheelEvent(io::MouseData mouseData);
        
        //KeyEvents
        void KeyEvent(io::Key keyData);
        void TextEvent(const char* text);
        
        //ControllerEvents
        void ControllerDetectedEvent(io::ControllerID controllerID);
        void ControllerDisonnectedEvent(io::ControllerID controllerID);
        void ControllerAxisEvent(io::ControllerID controllerID, io::ControllerAxisName axis, s16 value);
        void ControllerButtonEvent(io::ControllerID controllerID, io::ControllerButtonName buttonName, u8 state);
        void ControllerRemappedEvent(io::ControllerID controllerID);
        
        SetVSyncFunc mSetVSyncFunc = nullptr;
        SetFullScreenFunc mFullScreenFunc = nullptr;
        SetWindowSizeFunc mWindowSizeFunc = nullptr;
        SetWindowPositionFunc mWindowPosFunc = nullptr;
        CenterWindowFunc mCenterWindowFunc = nullptr;
        GetPerformanceFrequencyFunc mGetPerformanceFrequencyFunc = nullptr;
        GetPerformanceCounterFunc mGetPerformanceCounterFunc = nullptr;
        GetTicksFunc mGetTicksFunc = nullptr;
        OpenGameControllerFunc mOpenGameControllerFunc = nullptr;
        IsGameControllerFunc mIsGameControllerFunc = nullptr;
        NumberOfGameControllersFunc mNumberOfGameControllersFunc = nullptr;
        CloseGameControllerFunc mCloseGameControllerFunc = nullptr;
        IsGameControllerAttachedFunc mIsGameControllerAttchedFunc = nullptr;
        GetGameControllerMappingFunc mGetGameControllerMappingFunc = nullptr;
        GetGameControllerButtonStateFunc mGetGameControllerButtonStateFunc = nullptr;
        GetGameControllerAxisValueFunc mGetGameControllerAxisValueFunc = nullptr;
        GetStringForButtonFunc mGetStringForButtonFunc = nullptr;
        GetStringForAxisFunc mGetStringForAxisFunc = nullptr;
        GetGameControllerNameFunc mGetGameControllerNameFunc = nullptr;
        
        util::Size mDisplaySize;
        LayerStack mLayerStack;
        ImGuiLayer* mImGuiLayer;
        
        draw::Renderer* mRendererBackends[draw::RendererBackend::NUM_RENDERER_BACKEND_TYPES];
        
        draw::RendererBackend mCurrentRendererBackend;

        void* mPlatformControllers[NUM_PLATFORM_CONTROLLERS];
        b32 mMinimized;
        b32 mFullScreen;
        b32 mNeedsResolutionChange;
        util::Size mResolution;
        r2::mem::utils::MemBoundary mAssetLibMemBoundary;
        
#ifdef R2_ASSET_PIPELINE
        r2::asset::pln::AssetCommandHandler mAssetCommandHandler;
#endif
    };
}


#endif /* Engine_hpp */
