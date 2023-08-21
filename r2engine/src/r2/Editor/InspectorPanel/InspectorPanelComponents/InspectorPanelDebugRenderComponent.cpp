#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelDebugRenderComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/DebugRenderComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "imgui.h"

namespace r2::edit
{
	std::string GetDebugModelTypeName(r2::draw::DebugModelType type)
	{
		switch (type)
		{
		case r2::draw::DEBUG_QUAD:
			return "DEBUG_QUAD";
			break;
		case r2::draw::DEBUG_CUBE:
			return "DEBUG_CUBE";
			break;
		case r2::draw::DEBUG_SPHERE:
			return "DEBUG_SPHERE";
			break;
		case r2::draw::DEBUG_CONE:
			return "DEBUG_CONE";
			break;
		case r2::draw::DEBUG_CYLINDER:
			return "DEBUG_CYLINDER";
			break;
		case r2::draw::DEBUG_ARROW:
			return "DEBUG_ARROW";
			break;
		case r2::draw::DEBUG_LINE:
			return "DEBUG_LINE";
			break;
		case r2::draw::NUM_DEBUG_MODELS:
			R2_CHECK(false, "Invalid debug model type");
			break;
		default:
			R2_CHECK(false, "Invalid debug model type");
			break;
		}
		return "";
	}

	void DebugRenderComponentInstance(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity entity, int id, r2::ecs::DebugRenderComponent& debugRenderComponent)
	{
		const r2::ecs::EditorComponent* editorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(entity);

		std::string nodeName = std::string("Entity - ") + std::to_string(entity) + std::string(" - Debug Bone Instance - ") + std::to_string(id);
		if (editorComponent)
		{
			nodeName = editorComponent->editorName + std::string(" - Debug Render Instance - ") + std::to_string(id);
		}

		if (ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth, "%s", nodeName.c_str()))
		{
			if (ImGui::BeginCombo("Debug Model Type", GetDebugModelTypeName(debugRenderComponent.debugModelType).c_str()))
			{
				for (u32 i = 0; i < r2::draw::NUM_DEBUG_MODELS; ++i)
				{
					if (ImGui::Selectable(GetDebugModelTypeName(static_cast<r2::draw::DebugModelType>(i)).c_str(), static_cast<r2::draw::DebugModelType>(i) == debugRenderComponent.debugModelType))
					{
						debugRenderComponent.debugModelType = static_cast<r2::draw::DebugModelType>(i);
					}
				}

				ImGui::EndCombo();
			}

			ImGui::Text("X Offset: ");
			ImGui::SameLine();
			ImGui::DragFloat("##label xoffset", &debugRenderComponent.offset.x, 0.1f);

			ImGui::Text("Y Offset: ");
			ImGui::SameLine();
			ImGui::DragFloat("##label yoffset", &debugRenderComponent.offset.y, 0.1f);

			ImGui::Text("Z Offset: ");
			ImGui::SameLine();
			ImGui::DragFloat("##label zoffset", &debugRenderComponent.offset.z, 0.1f);

			if (debugRenderComponent.debugModelType != draw::DEBUG_QUAD &&
				debugRenderComponent.debugModelType != draw::DEBUG_LINE &&
				debugRenderComponent.debugModelType != draw::DEBUG_CUBE)
			{
				if (r2::math::LessThanOrEq(debugRenderComponent.radius, 0.0f))
				{
					debugRenderComponent.radius = 1.0f;
				}

				ImGui::Text("Radius: ");
				ImGui::SameLine();
				ImGui::DragFloat("##label radius", &debugRenderComponent.radius, 0.01, 0.01);
			}

			if (debugRenderComponent.debugModelType != draw::DEBUG_SPHERE)
			{
				ImGui::Text("Scale X: ");
				ImGui::SameLine();
				ImGui::DragFloat("##label scaleX", &debugRenderComponent.scale.x, 0.01, 0.01);

				ImGui::Text("Scale Y: ");
				ImGui::SameLine();
				ImGui::DragFloat("##label scaleY", &debugRenderComponent.scale.y, 0.01, 0.01);

				if (debugRenderComponent.debugModelType != draw::DEBUG_QUAD)
				{
					ImGui::Text("Scale Z: ");
					ImGui::SameLine();
					ImGui::DragFloat("##label scaleZ", &debugRenderComponent.scale.z, 0.01, 0.01);
				}
			}

			if (debugRenderComponent.debugModelType != draw::DEBUG_CUBE &&
				debugRenderComponent.debugModelType != draw::DEBUG_SPHERE)
			{
				glm::vec3 direction = debugRenderComponent.direction;

				ImGui::Text("Direction X: ");
				ImGui::SameLine();
				ImGui::DragFloat("##label dirX", &direction.x, 0.01);

				ImGui::Text("Direction Y: ");
				ImGui::SameLine();
				ImGui::DragFloat("##label dirY", &direction.y, 0.01);

				ImGui::Text("Direction Z: ");
				ImGui::SameLine();
				ImGui::DragFloat("##label dirZ", &direction.z, 0.01);

				debugRenderComponent.direction = glm::normalize(direction);
			}

			float color[] = { debugRenderComponent.color.r, debugRenderComponent.color.g, debugRenderComponent.color.b, debugRenderComponent.color.a };
			if (ImGui::ColorEdit4("Color", color))
			{
				debugRenderComponent.color = glm::vec4(color[0], color[1], color[2], color[3]);
			}

			bool filled = debugRenderComponent.filled;
			if (ImGui::Checkbox("Fill", &filled))
			{
				debugRenderComponent.filled = filled;
			}

			bool depthTest = debugRenderComponent.depthTest;
			if (ImGui::Checkbox("Depth Test", &depthTest))
			{
				debugRenderComponent.depthTest = depthTest;
			}

			ImGui::TreePop();
		}
	}

	void InspectorPanelDebugRenderComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	{
		r2::ecs::DebugRenderComponent& debugRenderComponent = coordinator->GetComponent<r2::ecs::DebugRenderComponent>(theEntity);

		DebugRenderComponentInstance(coordinator, theEntity, 0, debugRenderComponent);

		r2::ecs::InstanceComponentT<r2::ecs::DebugRenderComponent>* instancedDebugRenderComponents = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT< r2::ecs::DebugRenderComponent>>(theEntity);

		if (instancedDebugRenderComponents)
		{
			const auto numInstances = instancedDebugRenderComponents->numInstances;

			for (u32 i = 0; i < numInstances; ++i)
			{
				r2::ecs::DebugRenderComponent& nextDebugRenderComponent = r2::sarr::At(*instancedDebugRenderComponents->instances, i);
				DebugRenderComponentInstance(coordinator, theEntity, i + 1, nextDebugRenderComponent);
			}
		}
	}
}

#endif