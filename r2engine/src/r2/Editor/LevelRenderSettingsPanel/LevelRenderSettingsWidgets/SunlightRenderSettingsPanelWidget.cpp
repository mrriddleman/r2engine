#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsWidgets/SunlightRenderSettingsPanelWidget.h"
#include "r2/Render/Renderer/Renderer.h"

#include "r2/Core/Events/Event.h"
#include "r2/Core/Events/KeyEvent.h"

#include "r2/Editor/TransformWidgetInput.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "imgui.h"
#include "ImGuizmo.h"

namespace r2::edit
{

	SunLightRenderSettingsPanelWidget::SunLightRenderSettingsPanelWidget()
		:LevelRenderSettingsDataSource("Sunlight Settings")
		, mDirectionLightHandle({})
		, mDirection({0, 0, -1})
		, mPosition({0, 0, 5})
		, mImGuizmoOperation(ImGuizmo::OPERATION::ROTATE)
	{

	}

	SunLightRenderSettingsPanelWidget::~SunLightRenderSettingsPanelWidget()
	{

	}

	void SunLightRenderSettingsPanelWidget::Init()
	{
		r2::draw::DirectionLight dirLight;
		mDirectionLightLightProperties.color = glm::vec4(1);
		mDirectionLightLightProperties.castsShadowsUseSoftShadows = glm::uvec4(1, 0, 0, 0);
		mDirectionLightLightProperties.fallOff = 0;
		mDirectionLightLightProperties.intensity = 1.0f;

		dirLight.direction = glm::vec4(mDirection, 1);

		mDirectionLightHandle = r2::draw::renderer::AddDirectionLight(dirLight);
		mDirectionLightLightProperties = r2::draw::renderer::GetDirectionLightConstPtr(mDirectionLightHandle)->lightProperties;
	}

	void SunLightRenderSettingsPanelWidget::Update()
	{

	}

	void SunLightRenderSettingsPanelWidget::Render()
	{
		bool needsUpdate = false;

		if (ImGui::ColorEdit4("Light Color", glm::value_ptr(mDirectionLightLightProperties.color)))
		{
			needsUpdate = true;
		}

		ImGui::Text("Fall Off: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label falloff", &mDirectionLightLightProperties.fallOff, 0.01, 0.0f, 1.0f))
		{
			needsUpdate = true;
		}

		ImGui::Text("Intensity: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label intensity", &mDirectionLightLightProperties.intensity, 0.1, 0.0f, 1000.0f))
		{
			needsUpdate = true;
		}

		bool castsShadows = mDirectionLightLightProperties.castsShadowsUseSoftShadows.x > 0u;
		if (ImGui::Checkbox("Cast Shadows", &castsShadows))
		{
			mDirectionLightLightProperties.castsShadowsUseSoftShadows.x = castsShadows ? 1u : 0u;
			needsUpdate = true;
		}

		bool softShadows = mDirectionLightLightProperties.castsShadowsUseSoftShadows.y > 0u;
		if (ImGui::Checkbox("Soft Shadows", &softShadows))
		{
			mDirectionLightLightProperties.castsShadowsUseSoftShadows.y = softShadows ? 1u : 0u;
			needsUpdate = true;
		}

		ImGui::Text("Direction: ");
		ImGui::SameLine();
		if (ImGui::DragFloat3("##label direction", glm::value_ptr(mDirection), 0.01, -1.0, 1.0))
		{
			mDirection = glm::normalize(mDirection);
			needsUpdate = true;
		}

		glm::mat4 localMat = glm::mat4(1);

		const r2::Camera* camera = r2::draw::renderer::GetRenderCamera();

		static const glm::vec3 s_baseDirection = glm::vec3(0, 0, -1);
		
		glm::vec3 axis = glm::cross(s_baseDirection, mDirection);

		if (axis == glm::vec3(0))
		{
			axis = glm::vec3(0, -1, 0);
		}

		axis = glm::normalize(axis);
		float angleRadians = glm::orientedAngle(s_baseDirection, mDirection,axis);

		glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(glm::angleAxis(angleRadians, axis)) );
		glm::vec3 scale = glm::vec3(1);

		ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(mPosition), glm::value_ptr(eulerAngles), glm::value_ptr(scale), glm::value_ptr(localMat));

		if (ImGuizmo::Manipulate(glm::value_ptr(camera->view), glm::value_ptr(camera->proj), static_cast<ImGuizmo::OPERATION>(mImGuizmoOperation), ImGuizmo::MODE::WORLD, glm::value_ptr(localMat)))
		{
			glm::vec3 translation;
			glm::vec3 resultAngles;
			glm::vec3 scale;

			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localMat), glm::value_ptr(translation), glm::value_ptr(resultAngles), glm::value_ptr(scale));

		 	mDirection = glm::rotate(glm::quat(glm::radians(resultAngles)), glm::vec3(0, 0, -1));

			mPosition = translation;

			needsUpdate = true;
		}

		if (needsUpdate)
		{
			auto* directionLight = r2::draw::renderer::GetDirectionLightPtr(mDirectionLightHandle);

			directionLight->lightProperties = mDirectionLightLightProperties;
			directionLight->direction = glm::vec4(mDirection, 0);
		}
		
		r2::draw::renderer::DrawArrow(mPosition, mDirection, 2, 0.3f, mDirectionLightLightProperties.color, true);
	}

	bool SunLightRenderSettingsPanelWidget::OnEvent(r2::evt::Event& e)
	{
		r2::evt::EventDispatcher dispatcher(e);
		dispatcher.Dispatch<r2::evt::KeyPressedEvent>([this](const r2::evt::KeyPressedEvent& keyEvent)
			{
				return DoImGuizmoOperationKeyInput(keyEvent, mImGuizmoOperation);
			});

		return false;
	}

	void SunLightRenderSettingsPanelWidget::Shutdown()
	{
		if (mDirectionLightHandle.handle >= 0)
		{
			r2::draw::renderer::RemoveDirectionLight(mDirectionLightHandle);
		}
		mDirectionLightHandle = {};
	}
}

#endif