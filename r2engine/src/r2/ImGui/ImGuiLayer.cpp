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
#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"
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
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;
        
        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();
        
        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        
        // Setup Platform/Renderer bindings
        ImGui_ImplSDL2_InitForOpenGL((SDL_Window*)MPLAT.GetNativeWindowPtr(), MPLAT.GetRenderingContext());
        ImGui_ImplOpenGL3_Init("#version 410");
    }
    
    void ImGuiLayer::Shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::Begin()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame((SDL_Window*)MPLAT.GetNativeWindowPtr());
        ImGui::NewFrame();
    }
    
    void ImGuiLayer::End()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(CENG.DisplaySize().width, CENG.DisplaySize().height);
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            MPLAT.MakeCurrent();
        }
    }
    
    void ImGuiLayer::ImGuiRender()
    {
        static bool show = true;
        ImGui::ShowDemoWindow(&show);
    }
   
}
