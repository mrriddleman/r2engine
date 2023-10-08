#include "r2pch.h"
#ifdef R2_EDITOR
#include "r2/Editor/InspectorPanel/InspectorPanelComponent.h"
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/Editor.h"

#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Utils.h"

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

//Editor Actions
#include "r2/Editor/EditorActions/DetachEntityEditorAction.h"

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

#include "r2/Editor/InspectorPanel/InspectorPanelComponentDataSource.h"

#include "imgui.h"

namespace r2::edit
{
	void RegisterAllEngineComponentWidgets(InspectorPanel& inspectorPanel)
	{
		Editor* editor = inspectorPanel.GetEditor();
		ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

		r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();

		u32 sortOrder = 0;

		std::shared_ptr<InspectorPanelTransformDataSource> transformDataSource = std::make_shared<InspectorPanelTransformDataSource>(editor,coordinator);
		std::shared_ptr<InspectorPanelAudioListenerComponentDataSource> audioListenerDataSource = std::make_shared<InspectorPanelAudioListenerComponentDataSource>(coordinator);
		std::shared_ptr<InspectorPanelAudioEmitterComponentDataSource> audioEmitterDataSource = std::make_shared<InspectorPanelAudioEmitterComponentDataSource>(coordinator);
		std::shared_ptr<InspectorPanelDebugBoneComponentDataSource> debugBonesDataSource = std::make_shared<InspectorPanelDebugBoneComponentDataSource>(coordinator);
		std::shared_ptr<InspectorPanelDebugRenderDataSource> debugRenderDataSource = std::make_shared<InspectorPanelDebugRenderDataSource>(coordinator);
		std::shared_ptr<InspectorPanelHierarchyComponentDataSource> hierarchyDataSource = std::make_shared<InspectorPanelHierarchyComponentDataSource>(editor, coordinator);
		std::shared_ptr<InspectorPanelRenderComponentDataSource> renderDataSource = std::make_shared<InspectorPanelRenderComponentDataSource>(coordinator);
		std::shared_ptr<InspectorPanelSkeletonAnimationComponentDataSource> skeletalAnimationDataSource = std::make_shared<InspectorPanelSkeletonAnimationComponentDataSource>(coordinator);

		InspectorPanelComponentWidget transformComponentWidget = InspectorPanelComponentWidget(sortOrder++, transformDataSource );
		inspectorPanel.RegisterComponentWidget(transformComponentWidget);

		InspectorPanelComponentWidget hierarchyComponentWidget = InspectorPanelComponentWidget(sortOrder++, hierarchyDataSource);
		inspectorPanel.RegisterComponentWidget(hierarchyComponentWidget);

		InspectorPanelComponentWidget renderComponentWidget = InspectorPanelComponentWidget(sortOrder++, renderDataSource);
		inspectorPanel.RegisterComponentWidget(renderComponentWidget);

		InspectorPanelComponentWidget skeletalAnimationComponentWidget = InspectorPanelComponentWidget(sortOrder++, skeletalAnimationDataSource);
		inspectorPanel.RegisterComponentWidget(skeletalAnimationComponentWidget);

		InspectorPanelComponentWidget audioListenerComponentWidget = InspectorPanelComponentWidget(sortOrder++, audioListenerDataSource);
		inspectorPanel.RegisterComponentWidget(audioListenerComponentWidget);

		InspectorPanelComponentWidget audioEmitterComponentWidget = InspectorPanelComponentWidget(sortOrder++, audioEmitterDataSource);
		inspectorPanel.RegisterComponentWidget(audioEmitterComponentWidget);

		InspectorPanelComponentWidget debugRenderComponentWidget = InspectorPanelComponentWidget(sortOrder++, debugRenderDataSource);
		inspectorPanel.RegisterComponentWidget(debugRenderComponentWidget);

		InspectorPanelComponentWidget debugBonesComponentWidget = InspectorPanelComponentWidget(sortOrder++, debugBonesDataSource);
		inspectorPanel.RegisterComponentWidget(debugBonesComponentWidget); 
	}

	InspectorPanelComponentWidget::InspectorPanelComponentWidget()
		:mSortOrder(0)
		,mComponentDataSource(nullptr)
	{
	}

	InspectorPanelComponentWidget::InspectorPanelComponentWidget(s32 sortOrder, std::shared_ptr<InspectorPanelComponentDataSource> componentDataSource)
		:mSortOrder(sortOrder)
		,mComponentDataSource(componentDataSource)
	{
	}

	void InspectorPanelComponentWidget::ImGuiDraw(InspectorPanel& inspectorPanel, ecs::Entity theEntity)
	{
		if (theEntity == ecs::INVALID_ENTITY)
		{
			return;
		}

		Editor* editor = inspectorPanel.GetEditor();
		R2_CHECK(editor != nullptr, "Should never be nullptr");
		r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();
		R2_CHECK(coordinator != nullptr, "Should never be nullptr");

		std::string nodeName = mComponentDataSource->GetComponentName();
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowItemOverlap;

		bool open = ImGui::TreeNodeEx(nodeName.c_str(), flags, "%s", nodeName.c_str());


		const bool shouldDisableComponentDelete = mComponentDataSource->ShouldDisableRemoveComponentButton();
		if (shouldDisableComponentDelete)
		{
			ImGui::BeginDisabled(true);
			ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
		}

		ImVec2 size = ImGui::GetContentRegionAvail();
		ImGui::PushID(mSortOrder);
		ImGui::PushItemWidth(50);
		ImGui::SameLine(size.x - 50);

		if (ImGui::SmallButton("Delete"))
		{
			mComponentDataSource->DeleteComponent(coordinator, theEntity);
			open = false;
		}

		ImGui::PopItemWidth();
		

		if (shouldDisableComponentDelete)
		{
			ImGui::PopStyleVar();
			ImGui::EndDisabled();
		}
		
		if (open)
		{
			void* componentData = mComponentDataSource->GetComponentData(coordinator, theEntity);
			mComponentDataSource->DrawComponentData(componentData, coordinator, theEntity);

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void InspectorPanelComponentWidget::ImGuiDrawInstance(InspectorPanel& inspectorPanel, ecs::Entity theEntity, u32 instance)
	{
		if (mComponentDataSource->InstancesEnabled())
		{
			Editor* editor = inspectorPanel.GetEditor();
			R2_CHECK(editor != nullptr, "Should never be nullptr");
			r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();
			R2_CHECK(coordinator != nullptr, "Should never be nullptr");

			const u32 numInstances = mComponentDataSource->GetNumInstances(coordinator, theEntity);

			if (numInstances == 0)
			{
				return;
			}

			R2_CHECK(instance < numInstances, "This should never happen");


			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowItemOverlap;
			std::string nodeName = mComponentDataSource->GetComponentName() + " - Instance: " + std::to_string(instance);
			ImGui::PushID(instance);

			bool isInstanceOpen = ImGui::TreeNodeEx(nodeName.c_str(), flags, "%s", nodeName.c_str());

			if (isInstanceOpen)
			{
				void* componentInstanceData = mComponentDataSource->GetInstancedComponentData(instance, coordinator, theEntity);

				mComponentDataSource->DrawComponentData(componentInstanceData, coordinator, theEntity);

				ImGui::TreePop();
			}

			ImGui::PopID();
		}
	}

	r2::ecs::ComponentType InspectorPanelComponentWidget::GetComponentType() const
	{
		return mComponentDataSource->GetComponentType();
	}

	u64 InspectorPanelComponentWidget::GetComponentTypeHash() const
	{
		return mComponentDataSource->GetComponentTypeHash();
	}

	bool InspectorPanelComponentWidget::CanAddComponent(ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		return mComponentDataSource->CanAddComponent(coordinator, theEntity);
	}

	bool InspectorPanelComponentWidget::CanAddInstancedComponent() const
	{
		return mComponentDataSource->InstancesEnabled();
	}

	u32 InspectorPanelComponentWidget::GetNumInstances(ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		if (!mComponentDataSource->InstancesEnabled())
		{
			return 0;
		}

		return mComponentDataSource->GetNumInstances(coordinator, theEntity);
	}

	void InspectorPanelComponentWidget::AddInstancedComponentsToEntity(ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances)
	{
		mComponentDataSource->AddNewInstances(coordinator, theEntity, numInstances);
	}

	void InspectorPanelComponentWidget::AddComponentToEntity(ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{ 
		if (mComponentDataSource->CanAddComponent(coordinator, theEntity))
		{
			mComponentDataSource->AddComponent(coordinator, theEntity);
		}
	}

}
#endif