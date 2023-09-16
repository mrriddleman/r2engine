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
	//void DebugBoneComponentInstance(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity entity, int id, r2::ecs::DebugBoneComponent& debugBoneComponent)
	//{
	//	const r2::ecs::EditorComponent* editorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(entity);

	//	std::string nodeName = std::string("Entity - ") + std::to_string(entity) + std::string(" - Debug Bone Instance - ") + std::to_string(id);
	//	if (editorComponent)
	//	{
	//		nodeName = editorComponent->editorName + std::string(" - Debug Bone Instance - ") + std::to_string(id);
	//	}

	//	if (ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth, "%s", nodeName.c_str()))
	//	{
	//		float color[3] = { debugBoneComponent.color.x, debugBoneComponent.color.y, debugBoneComponent.color.z };

	//		ImGui::ColorEdit3("Debug Bone Color", color);

	//		debugBoneComponent.color.x = color[0];
	//		debugBoneComponent.color.y = color[1];
	//		debugBoneComponent.color.z = color[2];

	//		ImGui::TreePop();
	//	}
	//}

	//void InspectorPanelDebugBoneComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	//{
	//	r2::ecs::DebugBoneComponent& debugBoneComponent = coordinator->GetComponent<r2::ecs::DebugBoneComponent>(theEntity);

	////	if (ImGui::CollapsingHeader("Debug Bone Component"))
	//	{
	//		DebugBoneComponentInstance(coordinator, theEntity, 0, debugBoneComponent);

	//		r2::ecs::InstanceComponentT<r2::ecs::DebugBoneComponent>* instancedDebugBones = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT< r2::ecs::DebugBoneComponent>>(theEntity);

	//		if (instancedDebugBones)
	//		{
	//			const auto numInstances = instancedDebugBones->numInstances;

	//			for (u32 i = 0; i < numInstances; ++i)
	//			{
	//				r2::ecs::DebugBoneComponent& nextDebugBoneComponent = r2::sarr::At(*instancedDebugBones->instances, i);
	//				DebugBoneComponentInstance(coordinator, theEntity, i + 1, nextDebugBoneComponent);
	//			}
	//		}
	//	}
	//}

	InspectorPanelDebugBoneComponentDataSource::InspectorPanelDebugBoneComponentDataSource()
		:InspectorPanelComponentDataSource("Debug Bone Component", 0, 0)
	{

	}

	InspectorPanelDebugBoneComponentDataSource::InspectorPanelDebugBoneComponentDataSource(r2::ecs::ECSCoordinator* coordinator)
		: InspectorPanelComponentDataSource("Debug Bone Component", coordinator->GetComponentType<ecs::DebugBoneComponent>(), coordinator->GetComponentTypeHash<ecs::DebugBoneComponent>())
	{

	}

	void InspectorPanelDebugBoneComponentDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::DebugBoneComponent* debugBoneComponentPtr = static_cast<ecs::DebugBoneComponent*>(componentData);
		ecs::DebugBoneComponent& debugBoneComponent = *debugBoneComponentPtr;


		float color[3] = { debugBoneComponent.color.x, debugBoneComponent.color.y, debugBoneComponent.color.z };

		ImGui::ColorEdit3("Debug Bone Color", color);

		debugBoneComponent.color.x = color[0];
		debugBoneComponent.color.y = color[1];
		debugBoneComponent.color.z = color[2];
	}

	bool InspectorPanelDebugBoneComponentDataSource::InstancesEnabled() const
	{
		return true;
	}

	u32 InspectorPanelDebugBoneComponentDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		ecs::InstanceComponentT<ecs::DebugBoneComponent>* instancedDebugBoneComponent = coordinator->GetComponentPtr<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(theEntity);

		if (instancedDebugBoneComponent)
		{
			return instancedDebugBoneComponent->numInstances;
		}

		return 0;
	}

	void* InspectorPanelDebugBoneComponentDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return &coordinator->GetComponent<ecs::DebugBoneComponent>(theEntity);
	}

	void* InspectorPanelDebugBoneComponentDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::InstanceComponentT<ecs::DebugBoneComponent>* instancedDebugBoneComponent = coordinator->GetComponentPtr<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(theEntity);

		if (instancedDebugBoneComponent)
		{
			R2_CHECK(i < instancedDebugBoneComponent->numInstances, "Trying to access instance: %u but only have %u instances", i, instancedDebugBoneComponent->numInstances);

			return &r2::sarr::At(*instancedDebugBoneComponent->instances, i);
		}

		return nullptr;
	}

	void InspectorPanelDebugBoneComponentDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		if (coordinator->HasComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugBoneComponent>>(theEntity))
		{
			coordinator->RemoveComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugBoneComponent>>(theEntity);
		}

		coordinator->RemoveComponent<r2::ecs::DebugBoneComponent>(theEntity);
	}

	void InspectorPanelDebugBoneComponentDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::InstanceComponentT<ecs::DebugBoneComponent>* instancedDebugBoneComponent = coordinator->GetComponentPtr<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(theEntity);

		if (instancedDebugBoneComponent)
		{
			R2_CHECK(i < instancedDebugBoneComponent->numInstances, "Trying to access instance: %u but only have %u instances", i, instancedDebugBoneComponent->numInstances);

			r2::sarr::RemoveElementAtIndexShiftLeft(*instancedDebugBoneComponent->instances, i);
			instancedDebugBoneComponent->numInstances--;
		}
	}

	void InspectorPanelDebugBoneComponentDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

		ecs::DebugBoneComponent debugBoneComponent;
		debugBoneComponent.color = glm::vec4(1, 1, 0, 1);

		const ecs::SkeletalAnimationComponent& animationComponent = coordinator->GetComponent<ecs::SkeletalAnimationComponent>(theEntity);
		debugBoneComponent.debugBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::DebugBone, r2::sarr::Size(*animationComponent.animModel->optrBoneInfo));
		coordinator->AddComponent<r2::ecs::DebugBoneComponent>(theEntity, debugBoneComponent);
	}

	void InspectorPanelDebugBoneComponentDataSource::AddNewInstance(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

		r2::ecs::InstanceComponentT<ecs::DebugBoneComponent>* instancedDebugBoneComponentToUse = AddNewInstanceCapacity<ecs::DebugBoneComponent>(coordinator, theEntity);

		ecs::DebugBoneComponent newDebugBoneComponent;

		newDebugBoneComponent.color = glm::vec4(1, 1, 0, 1);
		const ecs::SkeletalAnimationComponent& animationComponent = coordinator->GetComponent<ecs::SkeletalAnimationComponent>(theEntity);
		newDebugBoneComponent.debugBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::DebugBone, r2::sarr::Size(*animationComponent.animModel->optrBoneInfo));

		r2::sarr::Push(*instancedDebugBoneComponentToUse->instances, newDebugBoneComponent);

		instancedDebugBoneComponentToUse->numInstances++;
	}

}

#endif