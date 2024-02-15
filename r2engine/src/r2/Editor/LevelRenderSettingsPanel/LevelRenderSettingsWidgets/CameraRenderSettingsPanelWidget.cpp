#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsWidgets/CameraRenderSettingsPanelWidget.h"
#include "imgui.h"
#include "r2/Editor/Editor.h"
#include "r2/Game/Level/Level.h"
#include "r2/Game/Level/LevelRenderSettings.h"
#include "r2/Render/Renderer/Renderer.h"

namespace r2::edit
{

	CameraRenderSettingsPanelWidget::CameraRenderSettingsPanelWidget(r2::Editor* noptrEditor)
		:LevelRenderSettingsDataSource(noptrEditor, "Camera Settings")
	{
		strcpy(newCameraText, "");
	}

	CameraRenderSettingsPanelWidget::~CameraRenderSettingsPanelWidget()
	{

	}

	void CameraRenderSettingsPanelWidget::Init()
	{

	}

	void CameraRenderSettingsPanelWidget::Update()
	{

	}

	void CameraRenderSettingsPanelWidget::Render()
	{
		Level& editorLevel = mnoptrEditor->GetEditorLevelRef();

		LevelRenderSettings& levelRenderSettings = editorLevel.GetLevelRenderSettings();

		r2::SArray<Camera>* levelCameras = levelRenderSettings.GetLevelCameras();

		const u32 numLevelCameras = r2::sarr::Size(*levelCameras);


		std::string perspectiveCameraControlCamera = "?";
		if (mnoptrEditor->IsEditorCameraInControl())
		{
			perspectiveCameraControlCamera = "Editor Camera";
		}
		else
		{
			perspectiveCameraControlCamera = levelRenderSettings.GetCameraName(levelRenderSettings.GetCurrentCameraIndex());
		}

		ImGui::Text("Camera In Use: %s", perspectiveCameraControlCamera.c_str());


		std::string currentLevelCamera = "None";

		if (levelRenderSettings.GetCurrentCameraIndex()>= 0)
		{
			currentLevelCamera = levelRenderSettings.GetCameraName(levelRenderSettings.GetCurrentCameraIndex());
		}

		ImGui::PushItemWidth(100.0f);
		if (ImGui::BeginCombo("##label currentcamera", currentLevelCamera.c_str()))
		{
			for (u32 i = 0; i < numLevelCameras; ++i)
			{
				std::string cameraName = levelRenderSettings.GetCameraName(i);

				if (ImGui::Selectable(cameraName.c_str(), i == levelRenderSettings.GetCurrentCameraIndex()))
				{
					levelRenderSettings.SetCurrentCamera(i);
				}
			}

			ImGui::EndCombo();
		}
		

		ImGui::SameLine();

		if (ImGui::Button("Preview Camera View"))
		{
			mnoptrEditor->SwitchPerspectiveControllerCamera(levelRenderSettings.GetCurrentCamera());
			r2::draw::renderer::SetRenderCamera(levelRenderSettings.GetCurrentCamera());
		}
		ImGui::SameLine();
		if (ImGui::Button("Editor Camera"))
		{
			mnoptrEditor->SetDefaultEditorPerspectiveCameraController();
			mnoptrEditor->SetRenderToEditorCamera();
		}

		ImGui::PopItemWidth();

		s32 indexToDelete = -1;

		for (u32 i = 0; i < numLevelCameras; ++i)
		{
			std::string cameraName = levelRenderSettings.GetCameraName(i);

			ImGui::PushID(cameraName.c_str());
			bool open = ImGui::TreeNodeEx(cameraName.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
			
			ImGui::SameLine();

			bool deleted = false;

			if (numLevelCameras == 1)
			{
				ImGui::BeginDisabled(true);
				ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
			}

			if (ImGui::SmallButton("Delete"))
			{
				deleted = true;

				indexToDelete = i;

				if (open)
				{
					ImGui::TreePop();
				}

				ImGui::PopID();
				break;
			}

			if (numLevelCameras == 1)
			{
				ImGui::PopStyleVar();
				ImGui::EndDisabled();
			}

			if ( open)
			{
				Camera& camera = r2::sarr::At(*levelCameras, i);

				bool needsUpdate = false;

				if (camera.isPerspectiveCam)
				{
					float fovDegrees = glm::degrees(camera.fov);

					if (ImGui::DragFloat("##label fov", &fovDegrees, 1.0f, 30.0f, 120.0f))
					{
						needsUpdate = true;
					}

					if (ImGui::DragFloat("##label nearplane", &camera.nearPlane, 0.01, 0.01, camera.farPlane))
					{
						needsUpdate = true;
					}

					if (ImGui::DragFloat("##label farplane", &camera.farPlane, 0.01, camera.nearPlane, 1000.0f))
					{
						needsUpdate = true;
					}

					if (ImGui::DragFloat("##label exposure", &camera.exposure, 0.01, 0.0, 10.0)) //no clue for upper limit
					{
						needsUpdate = true;
					}

					if (needsUpdate)
					{
						cam::SetPerspectiveCam(camera, glm::radians(fovDegrees), camera.aspectRatio, camera.nearPlane, camera.farPlane);
					}
				}
				else
				{
					TODO;
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
		}


		if (indexToDelete >= 0)
		{
			Camera& camera = r2::sarr::At(*levelCameras, indexToDelete);

			if (&camera == r2::draw::renderer::GetRenderCamera())
			{
				//switch back to the editor camera
				mnoptrEditor->SetRenderToEditorCamera();
			}

			//now remove it
			levelRenderSettings.RemoveCamera(indexToDelete);
		}


		ImGui::Text("Add new Camera: ");
		ImGui::SameLine();
		ImGui::PushItemWidth(200);
		ImGui::InputText("##label newcameraname", newCameraText, 2048);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Add New Camera"))
		{
			const Camera* editorCamera = mnoptrEditor->GetEditorCamera();

			Camera newCamera;
			const auto displaySize = CENG.DisplaySize();
			auto aspect = static_cast<float>(displaySize.width) / static_cast<float>(displaySize.height);
			r2::cam::InitPerspectiveCam(newCamera, editorCamera->fov, editorCamera->aspectRatio, editorCamera->nearPlane, editorCamera->farPlane, editorCamera->position, editorCamera->facing);

			levelRenderSettings.AddCamera(newCamera, std::string(newCameraText));

			strcpy(newCameraText, "");
		}
	}

	bool CameraRenderSettingsPanelWidget::OnEvent(r2::evt::Event& e)
	{
		return nullptr;
	}

	void CameraRenderSettingsPanelWidget::Shutdown()
	{

	}

}


#endif