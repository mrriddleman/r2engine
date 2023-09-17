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

		std::shared_ptr<InspectorPanelTransformDataSource> transformDataSource = std::make_shared<InspectorPanelTransformDataSource>(coordinator);
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



		//inspectorPanel.RegisterComponentType(
		//	"Transform Component",
		//	sortOrder++,
		//	coordinator->GetComponentType<r2::ecs::TransformComponent>(),
		//	coordinator->GetComponentTypeHash<r2::ecs::TransformComponent>(),
		//	InspectorPanelTransformComponent,
		//	[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		//This should probably be an action?
		//		coordinator->RemoveComponent<r2::ecs::TransformComponent>(theEntity);

		//		if (coordinator->HasComponent<ecs::InstanceComponentT<ecs::TransformComponent>>(theEntity))
		//		{
		//			coordinator->RemoveComponent<ecs::InstanceComponentT<ecs::TransformComponent>>(theEntity);
		//		}
		//	},
		//	[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		ecs::TransformComponent newTransformComponent;

		//		//This should probably be an action?
		//		coordinator->AddComponent<ecs::TransformComponent>(theEntity, newTransformComponent);
		//	}, InspectorPanelAddTransformInstanceComponent);

		//inspectorPanel.RegisterComponentType(
		//	"Hierarchy Component",
		//	sortOrder++,
		//	coordinator->GetComponentType<r2::ecs::HierarchyComponent>(),
		//	coordinator->GetComponentTypeHash<r2::ecs::HierarchyComponent>(),
		//	InspectorPanelHierarchyComponent,
		//	[editor](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		const ecs::HierarchyComponent& hierarchyComponent = coordinator->GetComponent<ecs::HierarchyComponent>(theEntity);

		//		editor->PostNewAction(std::make_unique<edit::DetachEntityEditorAction>(editor, theEntity, hierarchyComponent.parent));

		//		//This should probably be an action?
		//		coordinator->RemoveComponent<ecs::HierarchyComponent>(theEntity);
		//	},
		//	[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		ecs::HierarchyComponent newHierarchyComponent;
		//		newHierarchyComponent.parent = ecs::INVALID_ENTITY;

		//		//This should probably be an action?
		//		coordinator->AddComponent<ecs::HierarchyComponent>(theEntity, newHierarchyComponent);
		//	}, nullptr);

		//inspectorPanel.RegisterComponentType(
		//	"Render Component",
		//	sortOrder++,
		//	coordinator->GetComponentType<r2::ecs::RenderComponent>(),
		//	coordinator->GetComponentTypeHash<r2::ecs::RenderComponent>(),
		//	InspectorPanelRenderComponent,
		//	[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		ecs::RenderComponent& renderComponent = coordinator->GetComponent<ecs::RenderComponent>(theEntity);
		//		
		//		bool isAnimated = renderComponent.isAnimated;

		//		coordinator->RemoveComponent<ecs::RenderComponent>(theEntity);

		//		if (isAnimated)
		//		{
		//			bool hasAnimationComponent = coordinator->HasComponent<r2::ecs::SkeletalAnimationComponent>(theEntity);
		//			if (hasAnimationComponent)
		//			{
		//				coordinator->RemoveComponent<ecs::SkeletalAnimationComponent>(theEntity);
		//			}

		//			bool hasDebugBoneComponent = coordinator->HasComponent<ecs::DebugBoneComponent>(theEntity);
		//			if (hasDebugBoneComponent)
		//			{
		//				coordinator->RemoveComponent<ecs::DebugBoneComponent>(theEntity);
		//			}

		//			bool hasInstancedSkeletalAnimationComponent = coordinator->HasComponent<r2::ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(theEntity);
		//			if (hasInstancedSkeletalAnimationComponent)
		//			{
		//				coordinator->RemoveComponent<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(theEntity);
		//			}

		//			bool hasInstancedDebugBoneComponent = coordinator->HasComponent<r2::ecs::InstanceComponentT<ecs::DebugBoneComponent>>(theEntity);
		//			if (hasInstancedDebugBoneComponent)
		//			{
		//				coordinator->RemoveComponent<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(theEntity);
		//			}
		//		}
		//	},
		//	[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		ecs::RenderComponent newRenderComponent;
		//		newRenderComponent.assetModelName = r2::draw::renderer::GetDefaultModel(draw::CUBE)->assetName;
		//		newRenderComponent.gpuModelRefHandle = r2::draw::renderer::GetModelRefHandleForModelAssetName(newRenderComponent.assetModelName);
		//		newRenderComponent.isAnimated = false;
		//		newRenderComponent.primitiveType = static_cast<u32>( r2::draw::PrimitiveType::TRIANGLES );
		//		newRenderComponent.drawParameters.layer = draw::DL_WORLD;
		//		newRenderComponent.drawParameters.flags.Clear();
		//		newRenderComponent.drawParameters.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);
		//		newRenderComponent.optrMaterialOverrideNames = nullptr;

		//		r2::draw::renderer::SetDefaultDrawParameters(newRenderComponent.drawParameters);

		//		coordinator->AddComponent<ecs::RenderComponent>(theEntity, newRenderComponent);
		//	}, nullptr);

		//inspectorPanel.RegisterComponentType(
		//	"Animation Component",
		//	sortOrder++,
		//	coordinator->GetComponentType<r2::ecs::SkeletalAnimationComponent>(),
		//	coordinator->GetComponentTypeHash<r2::ecs::SkeletalAnimationComponent>(),
		//	InspectorPanelSkeletalAnimationComponent,
		//	nullptr, //@NOTE(Serge): We don't enable this since this is controlled by the render component
		//	nullptr,
		//	nullptr);//@NOTE(Serge): We don't enable this since this is controlled by the render component

		//inspectorPanel.RegisterComponentType(
		//	"Audio Listener Component",
		//	sortOrder++,
		//	coordinator->GetComponentType<r2::ecs::AudioListenerComponent>(),
		//	coordinator->GetComponentTypeHash<r2::ecs::AudioListenerComponent>(),
		//	InspectorPanelAudioListenerComponent,
		//	nullptr, //@NOTE(Serge): we probably shouldn't be able to remove the audio listener component either
		//	nullptr,
		//	nullptr); //@NOTE(Serge): we probably shouldn't be able to add audio listener components this easily since typically we only have 1 + we have a limited amount of them (8 due to FMOD)

		//inspectorPanel.RegisterComponentType(
		//	"Audio Emitter Component",
		//	sortOrder++,
		//	coordinator->GetComponentType<r2::ecs::AudioEmitterComponent>(),
		//	coordinator->GetComponentTypeHash<r2::ecs::AudioEmitterComponent>(),
		//	InspectorPanelAudioEmitterComponent,
		//	[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		coordinator->RemoveComponent<r2::ecs::AudioEmitterComponent>(theEntity);
		//	},
		//	[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		ecs::AudioEmitterComponent audioEmitterComponent;

		//		audioEmitterComponent.eventInstanceHandle = r2::audio::AudioEngine::InvalidEventInstanceHandle;
		//		r2::util::PathCpy(audioEmitterComponent.eventName, "No Event");
		//		audioEmitterComponent.numParameters = 0;
		//		audioEmitterComponent.startCondition = ecs::PLAY_ON_EVENT;
		//		audioEmitterComponent.allowFadeoutWhenStopping = false;
		//		audioEmitterComponent.releaseAfterPlay = true;

		//		coordinator->AddComponent<ecs::AudioEmitterComponent>(theEntity, audioEmitterComponent);
		//	}, nullptr);

		//inspectorPanel.RegisterComponentType(
		//	"Debug Render Component",
		//	sortOrder++,
		//	coordinator->GetComponentType<r2::ecs::DebugRenderComponent>(),
		//	coordinator->GetComponentTypeHash<r2::ecs::DebugRenderComponent>(),
		//	InspectorPanelDebugRenderComponent,
		//	[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		coordinator->RemoveComponent<r2::ecs::DebugRenderComponent>(theEntity);

		//		if (coordinator->HasComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugRenderComponent>>(theEntity))
		//		{
		//			coordinator->RemoveComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugRenderComponent>>(theEntity);
		//		}
		//	},
		//	[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		ecs::DebugRenderComponent newDebugRenderComponent;
		//		newDebugRenderComponent.debugModelType = draw::DEBUG_CUBE;
		//		newDebugRenderComponent.radius = 1.0f;
		//		newDebugRenderComponent.scale = glm::vec3(1.0f);
		//		newDebugRenderComponent.direction = glm::vec3(0.0f, 0.0f, 1.0f);
		//		newDebugRenderComponent.offset = glm::vec3(0);
		//		newDebugRenderComponent.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		//		newDebugRenderComponent.filled = true;
		//		newDebugRenderComponent.depthTest = true;

		//		coordinator->AddComponent<ecs::DebugRenderComponent>(theEntity, newDebugRenderComponent);
		//	}, nullptr);

		//inspectorPanel.RegisterComponentType(
		//	"Debug Bone Component",
		//	sortOrder++,
		//	coordinator->GetComponentType<r2::ecs::DebugBoneComponent>(),
		//	coordinator->GetComponentTypeHash<r2::ecs::DebugBoneComponent>(),
		//	InspectorPanelDebugBoneComponent,
		//	[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		if (coordinator->HasComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugBoneComponent>>(theEntity))
		//		{
		//			coordinator->RemoveComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugBoneComponent>>(theEntity);
		//		}

		//		if (coordinator->HasComponent<r2::ecs::DebugBoneComponent>(theEntity))
		//		{
		//			coordinator->RemoveComponent<r2::ecs::DebugBoneComponent>(theEntity);
		//		}
		//	},
		//	[&ecsWorld](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
		//	{
		//		ecs::DebugBoneComponent debugBoneComponent;
		//		debugBoneComponent.color = glm::vec4(1, 1, 0, 1);

		//		const ecs::SkeletalAnimationComponent& animationComponent = coordinator->GetComponent<ecs::SkeletalAnimationComponent>(theEntity);
		//		debugBoneComponent.debugBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::DebugBone, r2::sarr::Size(*animationComponent.animModel->optrBoneInfo));
		//		coordinator->AddComponent<r2::ecs::DebugBoneComponent>(theEntity, debugBoneComponent);
		//	}, nullptr);


		//@TODO(Serge): we probably need a way for the app to register new components to the inspector 
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
		Editor* editor = inspectorPanel.GetEditor();
		R2_CHECK(editor != nullptr, "Should never be nullptr");
		r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();
		R2_CHECK(coordinator != nullptr, "Should never be nullptr");
		bool open = ImGui::CollapsingHeader(mComponentDataSource->GetComponentName().c_str(), ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth);

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

			if (mComponentDataSource->InstancesEnabled())
			{
				const u32 numInstances = mComponentDataSource->GetNumInstances(coordinator, theEntity);

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowItemOverlap;

				const r2::ecs::EditorComponent* editorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(theEntity);

				R2_CHECK(editorComponent != nullptr, "Should always exist");

				std::string nodeName = editorComponent->editorName + " - " + mComponentDataSource->GetComponentName() + " - Instance";				

				for (u32 i = 0; i < numInstances; ++i)
				{
					ImGui::PushID(i);
					std::string instanceNodeNode = nodeName + ": " + std::to_string(i);
					
					bool isInstanceOpen = ImGui::TreeNodeEx(instanceNodeNode.c_str(), flags, "%s", instanceNodeNode.c_str());

					ImVec2 size = ImGui::GetContentRegionAvail();
					//std::string instancedDeleteButtonID = nodeName + std::string(" delete - ") + std::to_string(i);
					//ImGui::PushID(instancedDeleteButtonID.c_str());
					ImGui::PushItemWidth(50);
					ImGui::SameLine(size.x - 50);

					if (ImGui::SmallButton("Delete"))
					{
						mComponentDataSource->DeleteInstance(i, coordinator, theEntity);
						isInstanceOpen = false;
					}

					ImGui::PopItemWidth();
			//		ImGui::PopID();

					if (isInstanceOpen)
					{
						void* componentInstanceData = mComponentDataSource->GetInstancedComponentData(i, coordinator, theEntity);

						mComponentDataSource->DrawComponentData(componentInstanceData, coordinator, theEntity);

						ImGui::TreePop();
					}

					ImGui::PopID();
				}

				ImGui::Indent();
				if (ImGui::Button("Add Instance"))
				{
					mComponentDataSource->AddNewInstance(coordinator, theEntity);
				}
				ImGui::Unindent();
			}
		}

		ImGui::PopID();
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

	void InspectorPanelComponentWidget::AddInstancedComponentToEntity(ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		mComponentDataSource->AddNewInstance(coordinator, theEntity);
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