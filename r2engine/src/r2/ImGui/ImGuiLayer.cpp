//
//  ImGuiLayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-16.
//

#include "ImGuiLayer.h"
#include "r2/Platform/IO.h"
#include "r2/Platform/Platform.h"
#include "imgui.h"
#include "r2/ImGui/ImGuiOpenGLRenderer.h"
#include "r2/Core/Engine.h"
#include "glad/glad.h"

namespace r2
{
    ImGuiLayer::ImGuiLayer(): Layer("ImGui", true)
    {
        
    }
    
    ImGuiLayer::~ImGuiLayer()
    {
        
    }
    
    void ImGuiLayer::Init()
    {
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        
        ImGuiIO& io = ImGui::GetIO();
        
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
        io.BackendPlatformName = "imgui_impl_r2";
        
        io.KeyMap[ImGuiKey_Tab] = r2::io::KEY_TAB;
        io.KeyMap[ImGuiKey_LeftArrow] = r2::io::KEY_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = r2::io::KEY_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = r2::io::KEY_UP;
        io.KeyMap[ImGuiKey_DownArrow] = r2::io::KEY_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = r2::io::KEY_PAGEUP;
        io.KeyMap[ImGuiKey_PageDown] = r2::io::KEY_PAGEDOWN;
        io.KeyMap[ImGuiKey_Home] = r2::io::KEY_HOME;
        io.KeyMap[ImGuiKey_End] = r2::io::KEY_END;
        io.KeyMap[ImGuiKey_Insert] = r2::io::KEY_INSERT;
        io.KeyMap[ImGuiKey_Delete] = r2::io::KEY_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = r2::io::KEY_BACKSPACE;
        io.KeyMap[ImGuiKey_Space] = r2::io::KEY_SPACE;
        io.KeyMap[ImGuiKey_Enter] = r2::io::KEY_RETURN;
        io.KeyMap[ImGuiKey_Escape] = r2::io::KEY_ESCAPE;
        io.KeyMap[ImGuiKey_A] = r2::io::KEY_a;
        io.KeyMap[ImGuiKey_C] = r2::io::KEY_c;
        io.KeyMap[ImGuiKey_V] = r2::io::KEY_v;
        io.KeyMap[ImGuiKey_X] = r2::io::KEY_x;
        io.KeyMap[ImGuiKey_Y] = r2::io::KEY_y;
        io.KeyMap[ImGuiKey_Z] = r2::io::KEY_z;
        
        io.SetClipboardTextFn = CENG.mSetClipboardTextFunc;
        io.GetClipboardTextFn = CENG.mGetClipboardTextFunc;
     
        ImGui_ImplOpenGL3_Init("#version 410");
    }
    
    void ImGuiLayer::Shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::GetIO().Fonts->TexID = NULL;
        ImGui::DestroyContext();
    }
    
    void ImGuiLayer::Update()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(CENG.DisplaySize().width, CENG.DisplaySize().height);

        static u64 frequency = CENG.GetPerformanceFrequency();
        u64 currentTime = CENG.GetPerformanceCounter();
        io.DeltaTime = mTime > 0 ? static_cast<float>(static_cast<double>(currentTime - mTime)/static_cast<double>(frequency)) : 1.0f/60.0f;
        mTime = currentTime;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        
        static bool show = true;
        ImGui::ShowDemoWindow(&show);
        
        ImGui::Render();
    }
    
    void ImGuiLayer::Render(float alpha)
    {
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    
    void ImGuiLayer::OnEvent(evt::Event& event)
    {
        evt::EventDispatcher dispatcher(event);
        dispatcher.Dispatch<evt::MouseButtonPressedEvent>(R2_BIND_EVENT_FN(ImGuiLayer::OnMouseButtonPressedEvent));
        dispatcher.Dispatch<evt::MouseButtonReleasedEvent>(R2_BIND_EVENT_FN(ImGuiLayer::OnMouseButtonReleasedEvent));
        dispatcher.Dispatch<evt::MouseMovedEvent>(R2_BIND_EVENT_FN(ImGuiLayer::OnMouseMovedEvent));
        dispatcher.Dispatch<evt::MouseWheelEvent>(R2_BIND_EVENT_FN(ImGuiLayer::OnMouseWheelEvent));
        dispatcher.Dispatch<evt::KeyPressedEvent>(R2_BIND_EVENT_FN(ImGuiLayer::OnKeyPressedEvent));
        dispatcher.Dispatch<evt::KeyTypedEvent>(R2_BIND_EVENT_FN(ImGuiLayer::OnKeyTypedEvent));
        dispatcher.Dispatch<evt::KeyReleasedEvent>(R2_BIND_EVENT_FN(ImGuiLayer::OnKeyReleasedEvent));
        dispatcher.Dispatch<evt::WindowResizeEvent>(R2_BIND_EVENT_FN(ImGuiLayer::OnWindowResizeEvent));
    }
    
    bool ImGuiLayer::OnMouseButtonPressedEvent(evt::MouseButtonPressedEvent & e)
    {
        ImGuiIO& io = ImGui::GetIO();
        const u8 mouseButton = e.MouseButton();
        
        if(mouseButton == r2::io::MOUSE_BUTTON_LEFT)
        {
            io.MouseDown[0] = true;
        }
        else if(mouseButton == r2::io::MOUSE_BUTTON_RIGHT)
        {
            io.MouseDown[1] = true;
        }
        else if(mouseButton == r2::io::MOUSE_BUTTON_MIDDLE)
        {
            io.MouseDown[2] = true;
        }
        
        return false;
    }
    
    bool ImGuiLayer::OnMouseButtonReleasedEvent(evt::MouseButtonReleasedEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        const u8 mouseButton = e.MouseButton();
        
        if(mouseButton == r2::io::MOUSE_BUTTON_LEFT)
        {
            io.MouseDown[0] = false;
        }
        else if(mouseButton == r2::io::MOUSE_BUTTON_RIGHT)
        {
            io.MouseDown[1] = false;
        }
        else if(mouseButton == r2::io::MOUSE_BUTTON_MIDDLE)
        {
            io.MouseDown[2] = false;
        }
        
        return false;
    }
    
    bool ImGuiLayer::OnMouseMovedEvent(evt::MouseMovedEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = ImVec2(e.X(), e.Y());
        
        return false;
    }
    
    bool ImGuiLayer::OnMouseWheelEvent(evt::MouseWheelEvent& e)
    {
        
        ImGuiIO& io = ImGui::GetIO();
        
        io.MouseWheelH += e.XAmount();
        io.MouseWheel += e.YAmount();
        
         return false;
    }
    
    bool ImGuiLayer::OnKeyPressedEvent(evt::KeyPressedEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        s32 key = e.KeyCode();
        
        if(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown))
        {
            io.KeysDown[key] = true;
            io.KeyCtrl = (e.Modifiers() & r2::io::Key::CONTROL_PRESSED) != 0;
            io.KeyAlt = (e.Modifiers() & r2::io::Key::ALT_PRESSED) != 0;
            io.KeyShift = (e.Modifiers() & r2::io::Key::SHIFT_PRESSED) != 0;
        }
    
        return false;
    }
    
    bool ImGuiLayer::OnKeyReleasedEvent(evt::KeyReleasedEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        s32 key = e.KeyCode();
        if(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown))
        {
            io.KeysDown[key] = false;
        
            io.KeyCtrl = (e.Modifiers() & r2::io::Key::CONTROL_PRESSED) != 0;
            io.KeyAlt = (e.Modifiers() & r2::io::Key::ALT_PRESSED) != 0;
            io.KeyShift = (e.Modifiers() & r2::io::Key::SHIFT_PRESSED) != 0;
        }
        
         return false;
    }
    
    bool ImGuiLayer::OnKeyTypedEvent(evt::KeyTypedEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddInputCharactersUTF8(e.Text());
        return false;
    }
    
    bool ImGuiLayer::OnWindowResizeEvent(evt::WindowResizeEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(e.Width(), e.Height());
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        glViewport(0, 0, e.Width(), e.Height());
        
        return false;
    }
}
