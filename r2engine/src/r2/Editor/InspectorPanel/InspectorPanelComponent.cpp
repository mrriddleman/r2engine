#include "r2pch.h"
#ifdef R2_EDITOR
#include "r2/Editor/InspectorPanel/InspectorPanelComponent.h"
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/Editor.h"

#include "r2/Game/ECS/ECSCoordinator.h"

//Game Components
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/HierarchyComponent.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#include "r2/Game/ECS/Components/DebugRenderComponent.h"
#include "r2/Game/ECS/Components/AudioListenerComponent.h"
#include "r2/Game/ECS/Components/AudioEmitterComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"

//ImGui Components
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelEditorComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelTransformComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelHierarchyComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelDebugBoneComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelDebugRenderComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelAudioListenerComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelAudioEmitterComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelSkeletalAnimationComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelRenderComponent.h"

#include "imgui.h"

namespace r2::edit
{
	void RegisterAllEngineComponentWidgets(InspectorPanel& inspectorPanel)
	{
		Editor* editor = inspectorPanel.GetEditor();

		r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();

		u32 sortOrder = 0;

		inspectorPanel.RegisterComponentType(
			"Transform Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::TransformComponent>(),
			InspectorPanelTransformComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				//coordinator->RemoveComponent<r2::ecs::TransformComponent>(theEntity);
			});
		inspectorPanel.RegisterComponentType(
			"Hierarchy Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::HierarchyComponent>(),
			InspectorPanelHierarchyComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				//coordinator->RemoveComponent<r2::ecs::HierarchyComponent>(theEntity);
			});

		inspectorPanel.RegisterComponentType(
			"Render Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::RenderComponent>(),
			InspectorPanelRenderComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				//@TODO(Serge): implement
			});

		inspectorPanel.RegisterComponentType(
			"Animation Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::SkeletalAnimationComponent>(),
			InspectorPanelSkeletalAnimationComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				//@TODO(Serge): implement
			});

		inspectorPanel.RegisterComponentType(
			"Audio Listener Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::AudioListenerComponent>(),
			InspectorPanelAudioListenerComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				coordinator->RemoveComponent<r2::ecs::AudioListenerComponent>(theEntity);
			});

		inspectorPanel.RegisterComponentType(
			"Audio Emitter Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::AudioEmitterComponent>(),
			InspectorPanelAudioEmitterComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				coordinator->RemoveComponent<r2::ecs::AudioEmitterComponent>(theEntity);
			});

		inspectorPanel.RegisterComponentType(
			"Debug Render Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::DebugRenderComponent>(),
			InspectorPanelDebugRenderComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				//@TODO(Serge): implement
			});

		inspectorPanel.RegisterComponentType(
			"Debug Bone Component",
			sortOrder++,
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
		R2_CHECK(editor != nullptr, "Should never be nullptr");
		r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();
		R2_CHECK(coordinator != nullptr, "Should never be nullptr");
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
#endif