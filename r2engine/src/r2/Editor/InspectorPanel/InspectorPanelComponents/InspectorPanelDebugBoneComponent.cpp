#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelDebugBoneComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "imgui.h"

namespace r2::edit
{
	void DebugBoneComponentInstance(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity entity, int id, r2::ecs::DebugBoneComponent& debugBoneComponent)
	{
		const r2::ecs::EditorComponent* editorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(entity);

		std::string nodeName = std::string("Entity - ") + std::to_string(entity) + std::string(" - Debug Bone Instance - ") + std::to_string(id);
		if (editorComponent)
		{
			nodeName = editorComponent->editorName + std::string(" - Debug Bone Instance - ") + std::to_string(id);
		}

		if (ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth, "%s", nodeName.c_str()))
		{
			float color[3] = { debugBoneComponent.color.x, debugBoneComponent.color.y, debugBoneComponent.color.z };

			ImGui::ColorEdit3("Debug Bone Color", color);

			debugBoneComponent.color.x = color[0];
			debugBoneComponent.color.y = color[1];
			debugBoneComponent.color.z = color[2];

			ImGui::TreePop();
		}
	}

	void InspectorPanelDebugBoneComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	{
		r2::ecs::DebugBoneComponent& debugBoneComponent = coordinator->GetComponent<r2::ecs::DebugBoneComponent>(theEntity);

		if (ImGui::CollapsingHeader("Debug Bone Component"))
		{
			DebugBoneComponentInstance(coordinator, theEntity, 0, debugBoneComponent);

			r2::ecs::InstanceComponentT<r2::ecs::DebugBoneComponent>* instancedDebugBones = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT< r2::ecs::DebugBoneComponent>>(theEntity);

			if (instancedDebugBones)
			{
				const auto numInstances = instancedDebugBones->numInstances;

				for (u32 i = 0; i < numInstances; ++i)
				{
					r2::ecs::DebugBoneComponent& nextDebugBoneComponent = r2::sarr::At(*instancedDebugBones->instances, i);
					DebugBoneComponentInstance(coordinator, theEntity, i + 1, nextDebugBoneComponent);
				}
			}
		}
	}
}

#endif