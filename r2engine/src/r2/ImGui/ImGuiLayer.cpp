//
//  ImGuiLayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-16.
//
#include "r2pch.h"
#ifdef R2_IMGUI
#include "ImGuiLayer.h"
#include "r2/Platform/IO.h"
#include "r2/Platform/Platform.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include "r2/Core/Engine.h"
#include "glad/glad.h"
#include "r2/Render/Renderer/RendererImpl.h"
#include "ImGuiFileDialog.h"
#include "CustomFont.h"
#include "CustomFont.cpp"
#include <glad/glad.h>
#include "ImGuizmo.h"
#include "r2/Render/Renderer/Renderer.h"
#include <glm/gtc/type_ptr.hpp>
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
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;
        
        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        
        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        
        // Setup Platform/Renderer bindings
        ImGui_ImplSDL2_InitForOpenGL((SDL_Window*)r2::draw::rendererimpl::GetWindowHandle(), r2::draw::rendererimpl::GetRenderContext());
        ImGui_ImplOpenGL3_Init("#version 410");

		// Create thumbnails texture
		ImGuiFileDialog::Instance()->SetCreateThumbnailCallback([](IGFD_Thumbnail_Info* vThumbnail_Info) -> void
			{
				if (vThumbnail_Info &&
					vThumbnail_Info->isReadyToUpload &&
					vThumbnail_Info->textureFileDatas)
				{
					GLuint textureId = 0;
					glGenTextures(1, &textureId);
					vThumbnail_Info->textureID = (void*)(size_t)textureId;

					glBindTexture(GL_TEXTURE_2D, textureId);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
						(GLsizei)vThumbnail_Info->textureWidth, (GLsizei)vThumbnail_Info->textureHeight,
						0, GL_RGBA, GL_UNSIGNED_BYTE, vThumbnail_Info->textureFileDatas);
					glFinish();
					glBindTexture(GL_TEXTURE_2D, 0);

					delete[] vThumbnail_Info->textureFileDatas;
					vThumbnail_Info->textureFileDatas = nullptr;

					vThumbnail_Info->isReadyToUpload = false;
					vThumbnail_Info->isReadyToDisplay = true;
				}
			});
		
		ImGuiFileDialog::Instance()->SetDestroyThumbnailCallback([](IGFD_Thumbnail_Info* vThumbnail_Info)
			{
				if (vThumbnail_Info)
				{
					GLuint texID = (GLuint)(size_t)vThumbnail_Info->textureID;
					glDeleteTextures(1, &texID);
					glFinish();
				}
			});


		ImGui::GetIO().Fonts->AddFontDefault();
		static const ImWchar icons_ranges[] = { ICON_MIN_IGFD, ICON_MAX_IGFD, 0 };
		ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
		ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_IGFD, 15.0f, &icons_config, icons_ranges);

		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByFullName, "(Custom.+[.]h)", ImVec4(0.1f, 0.9f, 0.1f, 0.9f)); // use a regex
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".cpp", ImVec4(1.0f, 1.0f, 0.0f, 0.9f));
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".hpp", ImVec4(0.0f, 0.0f, 1.0f, 0.9f));
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".md", ImVec4(1.0f, 0.0f, 1.0f, 0.9f));
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".png", ImVec4(0.0f, 1.0f, 1.0f, 0.9f), ICON_IGFD_FILE_PIC); // add an icon for the filter type
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".gif", ImVec4(0.0f, 1.0f, 0.5f, 0.9f), "[GIF]"); // add an text for a filter type
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeDir, nullptr, ImVec4(0.5f, 1.0f, 0.9f, 0.9f), ICON_IGFD_FOLDER); // for all dirs
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeFile, "CMakeLists.txt", ImVec4(0.1f, 0.5f, 0.5f, 0.9f), ICON_IGFD_ADD);
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByFullName, "doc", ImVec4(0.9f, 0.2f, 0.0f, 0.9f), ICON_IGFD_FILE_PIC);
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeFile, nullptr, ImVec4(0.2f, 0.9f, 0.2f, 0.9f), ICON_IGFD_FILE); // for all link files
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeDir | IGFD_FileStyleByTypeLink, nullptr, ImVec4(0.8f, 0.8f, 0.8f, 0.8f), ICON_IGFD_FOLDER); // for all link dirs
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeFile | IGFD_FileStyleByTypeLink, nullptr, ImVec4(0.8f, 0.8f, 0.8f, 0.8f), ICON_IGFD_FILE); // for all link files
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeDir | IGFD_FileStyleByContainedInFullName, ".git", ImVec4(0.9f, 0.2f, 0.0f, 0.9f), ICON_IGFD_BOOKMARK);
		ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeFile | IGFD_FileStyleByContainedInFullName, ".git", ImVec4(0.5f, 0.8f, 0.5f, 0.9f), ICON_IGFD_SAVE);
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
		ImGui_ImplSDL2_NewFrame((SDL_Window*)r2::draw::rendererimpl::GetWindowHandle());
		
		ImGui::NewFrame();
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::BeginFrame();
		ImGuiIO& io = ImGui::GetIO();
		
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			mDockingSpace = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), dockspace_flags); 
		}

		ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(ImGui::GetMainViewport()));
		ImGuizmo::SetRect(ImGui::GetMainViewport()->Pos.x, ImGui::GetMainViewport()->Pos.y, io.DisplaySize.x, io.DisplaySize.y);
    }
    
    void ImGuiLayer::End()
    {
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<f32>(CENG.DisplaySize().width), static_cast<f32>(CENG.DisplaySize().height));

		
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
		    ImGui::UpdatePlatformWindows();
		    ImGui::RenderPlatformWindowsDefault();
	    	r2::draw::rendererimpl::MakeCurrent();
    	}

		ImGuiFileDialog::Instance()->ManageGPUThumbnails();
    }

    u32 ImGuiLayer::GetDockingSpace()
    {
        return mDockingSpace;
    }
    
    void ImGuiLayer::ImGuiRender(u32 dockingSpaceID)
    {
		
    }
   
}
#endif