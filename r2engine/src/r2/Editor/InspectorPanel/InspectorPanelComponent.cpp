#include "r2pch.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponent.h"
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/Editor.h"

#include "r2/Game/ECS/ECSCoordinator.h"

//Game Components
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/HierarchyComponent.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"

//ImGui Components
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelEditorComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelTransformComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelHierarchyComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelDebugBoneComponent.h"

#include "imgui.h"

namespace r2::edit
{
	void RegisterAllEngineComponentWidgets(InspectorPanel& inspectorPanel)
	{
		Editor* editor = inspectorPanel.GetEditor();

		r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();

		inspectorPanel.RegisterComponentType(
			"Transform Component",
			0,
			coordinator->GetComponentType<r2::ecs::TransformComponent>(),
			InspectorPanelTransformComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				//coordinator->RemoveComponent<r2::ecs::TransformComponent>(theEntity);
			});
		inspectorPanel.RegisterComponentType(
			"Hierarchy Component",
			1,
			coordinator->GetComponentType<r2::ecs::HierarchyComponent>(),
			InspectorPanelHierarchyComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				//coordinator->RemoveComponent<r2::ecs::HierarchyComponent>(theEntity);
			});
		inspectorPanel.RegisterComponentType(
			"Debug Bone Component",
			2,
			coordinator->GetComponentType<r2::ecs::DebugBoneComponent>(),
			InspectorPanelDebugBoneComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				//if (coordinator->HasComponent<r2::ecs::DebugBoneComponent>(theEntity))
				//{
				//	coordinator->RemoveComponent<r2::ecs::DebugBoneComponent>(theEntity);
				//}
				//
				//if (coordinator->HasComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugBoneComponent>>(theEntity))
				//{
				//	coordinator->RemoveComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugBoneComponent>>(theEntity);
				//}
			});
	}

	InspectorPanelComponentWidget::InspectorPanelComponentWidget()
		:mComponentName("EmptyComponent")
		,mComponentType(0)
		,mComponentWidgetFunc(nullptr)
		,mRemoveComponentFunc(nullptr)
		,mSortOrder(0)
	{

	}

	InspectorPanelComponentWidget::InspectorPanelComponentWidget(
		const std::string& componentName,
		r2::ecs::ComponentType componentType,
		InspectorPanelComponentWidgetFunc widgetFunction,
		InspectorPanelRemoveComponentFunc removeComponentFunc)
		:mComponentName(componentName)
		,mComponentType(componentType)
		,mComponentWidgetFunc(widgetFunction)
		,mRemoveComponentFunc(removeComponentFunc)
		,mSortOrder(0)
	{

	}

	void InspectorPanelComponentWidget::ImGuiDraw(InspectorPanel& inspectorPanel, ecs::Entity theEntity)
	{
		Editor* editor = inspectorPanel.GetEditor();

		r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();

		bool open = ImGui::CollapsingHeader(mComponentName.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth);

		ImVec2 size = ImGui::GetContentRegionAvail();
		ImGui::PushID(mSortOrder);
		ImGui::PushItemWidth(50);
		ImGui::SameLine(size.x - 50);
		if (ImGui::SmallButton("Delete"))
		{
			if (mRemoveComponentFunc)
			{
				mRemoveComponentFunc(theEntity, coordinator);
				open = false;
			}
		}
		ImGui::PopItemWidth();
		ImGui::PopID();

		if (open)
		{
			mComponentWidgetFunc(editor, theEntity, coordinator);
		}
	}

	void InspectorPanelComponentWidget::SetSortOrder(u32 sortOrder)
	{
		mSortOrder = sortOrder;
	}

}