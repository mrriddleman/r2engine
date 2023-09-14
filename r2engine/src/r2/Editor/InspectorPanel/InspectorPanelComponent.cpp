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

#include "imgui.h"

namespace r2::edit
{
	void RegisterAllEngineComponentWidgets(InspectorPanel& inspectorPanel)
	{
		Editor* editor = inspectorPanel.GetEditor();
		ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

		r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();

		u32 sortOrder = 0;

		inspectorPanel.RegisterComponentType(
			"Transform Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::TransformComponent>(),
			coordinator->GetComponentTypeHash<r2::ecs::TransformComponent>(),
			InspectorPanelTransformComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				//This should probably be an action?
				coordinator->RemoveComponent<r2::ecs::TransformComponent>(theEntity);
			},
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				ecs::TransformComponent newTransformComponent;

				//This should probably be an action?
				coordinator->AddComponent<ecs::TransformComponent>(theEntity, newTransformComponent);
			});

		inspectorPanel.RegisterComponentType(
			"Hierarchy Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::HierarchyComponent>(),
			coordinator->GetComponentTypeHash<r2::ecs::HierarchyComponent>(),
			InspectorPanelHierarchyComponent,
			[editor](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				const ecs::HierarchyComponent& hierarchyComponent = coordinator->GetComponent<ecs::HierarchyComponent>(theEntity);

				editor->PostNewAction(std::make_unique<edit::DetachEntityEditorAction>(editor, theEntity, hierarchyComponent.parent));

				//This should probably be an action?
				coordinator->RemoveComponent<ecs::HierarchyComponent>(theEntity);
			},
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				ecs::HierarchyComponent newHierarchyComponent;
				newHierarchyComponent.parent = ecs::INVALID_ENTITY;

				//This should probably be an action?
				coordinator->AddComponent<ecs::HierarchyComponent>(theEntity, newHierarchyComponent);
			});

		inspectorPanel.RegisterComponentType(
			"Render Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::RenderComponent>(),
			coordinator->GetComponentTypeHash<r2::ecs::RenderComponent>(),
			InspectorPanelRenderComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				ecs::RenderComponent& renderComponent = coordinator->GetComponent<ecs::RenderComponent>(theEntity);
				
				bool isAnimated = renderComponent.isAnimated;

				coordinator->RemoveComponent<ecs::RenderComponent>(theEntity);

				if (isAnimated)
				{
					bool hasAnimationComponent = coordinator->HasComponent<r2::ecs::SkeletalAnimationComponent>(theEntity);
					if (hasAnimationComponent)
					{
						coordinator->RemoveComponent<ecs::SkeletalAnimationComponent>(theEntity);
					}

					bool hasDebugBoneComponent = coordinator->HasComponent<ecs::DebugBoneComponent>(theEntity);
					if (hasDebugBoneComponent)
					{
						coordinator->RemoveComponent<ecs::DebugBoneComponent>(theEntity);
					}

					bool hasInstancedSkeletalAnimationComponent = coordinator->HasComponent<r2::ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(theEntity);
					if (hasInstancedSkeletalAnimationComponent)
					{
						coordinator->RemoveComponent<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(theEntity);
					}

					bool hasInstancedDebugBoneComponent = coordinator->HasComponent<r2::ecs::InstanceComponentT<ecs::DebugBoneComponent>>(theEntity);
					if (hasInstancedDebugBoneComponent)
					{
						coordinator->RemoveComponent<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(theEntity);
					}
				}
			},
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				ecs::RenderComponent newRenderComponent;
				newRenderComponent.assetModelName = r2::draw::renderer::GetDefaultModel(draw::CUBE)->assetName;
				newRenderComponent.gpuModelRefHandle = r2::draw::renderer::GetModelRefHandleForModelAssetName(newRenderComponent.assetModelName);
				newRenderComponent.isAnimated = false;
				newRenderComponent.primitiveType = static_cast<u32>( r2::draw::PrimitiveType::TRIANGLES );
				newRenderComponent.drawParameters.layer = draw::DL_WORLD;
				newRenderComponent.drawParameters.flags.Clear();
				newRenderComponent.drawParameters.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);
				
				r2::draw::renderer::SetDefaultDrawParameters(newRenderComponent.drawParameters);

				coordinator->AddComponent<ecs::RenderComponent>(theEntity, newRenderComponent);
			});

		inspectorPanel.RegisterComponentType(
			"Animation Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::SkeletalAnimationComponent>(),
			coordinator->GetComponentTypeHash<r2::ecs::SkeletalAnimationComponent>(),
			InspectorPanelSkeletalAnimationComponent,
			nullptr, //@NOTE(Serge): We don't enable this since this is controlled by the render component
			nullptr);//@NOTE(Serge): We don't enable this since this is controlled by the render component

		inspectorPanel.RegisterComponentType(
			"Audio Listener Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::AudioListenerComponent>(),
			coordinator->GetComponentTypeHash<r2::ecs::AudioListenerComponent>(),
			InspectorPanelAudioListenerComponent,
			nullptr, //@NOTE(Serge): we probably shouldn't be able to remove the audio listener component either
			nullptr); //@NOTE(Serge): we probably shouldn't be able to add audio listener components this easily since typically we only have 1 + we have a limited amount of them (8 due to FMOD)

		inspectorPanel.RegisterComponentType(
			"Audio Emitter Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::AudioEmitterComponent>(),
			coordinator->GetComponentTypeHash<r2::ecs::AudioEmitterComponent>(),
			InspectorPanelAudioEmitterComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				coordinator->RemoveComponent<r2::ecs::AudioEmitterComponent>(theEntity);
			},
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				/*
					r2::audio::AudioEngine::EventInstanceHandle eventInstanceHandle;
					char eventName[r2::fs::FILE_PATH_LENGTH];
					AudioEmitterParameter parameters[MAX_AUDIO_EMITTER_PARAMETERS];
					u32 numParameters;
					AudioEmitterStartCondition startCondition;
					b32 allowFadeoutWhenStopping;
					b32 releaseAfterPlay;
				*/

				ecs::AudioEmitterComponent audioEmitterComponent;

				audioEmitterComponent.eventInstanceHandle = r2::audio::AudioEngine::InvalidEventInstanceHandle;
				r2::util::PathCpy(audioEmitterComponent.eventName, "No Event");
				audioEmitterComponent.numParameters = 0;
				audioEmitterComponent.startCondition = ecs::PLAY_ON_EVENT;
				audioEmitterComponent.allowFadeoutWhenStopping = false;
				audioEmitterComponent.releaseAfterPlay = true;

				coordinator->AddComponent<ecs::AudioEmitterComponent>(theEntity, audioEmitterComponent);
			});

		inspectorPanel.RegisterComponentType(
			"Debug Render Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::DebugRenderComponent>(),
			coordinator->GetComponentTypeHash<r2::ecs::DebugRenderComponent>(),
			InspectorPanelDebugRenderComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				coordinator->RemoveComponent<r2::ecs::DebugRenderComponent>(theEntity);

				if (coordinator->HasComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugRenderComponent>>(theEntity))
				{
					coordinator->RemoveComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugRenderComponent>>(theEntity);
				}
			},
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				/*
					r2::draw::DebugModelType debugModelType;

					//for Arrow this is headBaseRadius
					float radius;

					//for Arrow this is length
					//for Cone and Cylinder - this is height
					//for a line - this is length of the line
					glm::vec3 scale;

					//For Lines, this is used for the direction of p1
					//we use the scale to determine the p1 of the line
					//for Quads, this will be the normal
					glm::vec3 direction;

					//some offset to give the position of the render debug component
					glm::vec3 offset;

					glm::vec4 color;
					b32 filled;
					b32 depthTest;
				*/

				ecs::DebugRenderComponent newDebugRenderComponent;
				newDebugRenderComponent.debugModelType = draw::DEBUG_CUBE;
				newDebugRenderComponent.radius = 1.0f;
				newDebugRenderComponent.scale = glm::vec3(1.0f);
				newDebugRenderComponent.direction = glm::vec3(0.0f, 0.0f, 1.0f);
				newDebugRenderComponent.offset = glm::vec3(0);
				newDebugRenderComponent.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
				newDebugRenderComponent.filled = true;
				newDebugRenderComponent.depthTest = true;

				coordinator->AddComponent<ecs::DebugRenderComponent>(theEntity, newDebugRenderComponent);
			});

		inspectorPanel.RegisterComponentType(
			"Debug Bone Component",
			sortOrder++,
			coordinator->GetComponentType<r2::ecs::DebugBoneComponent>(),
			coordinator->GetComponentTypeHash<r2::ecs::DebugBoneComponent>(),
			InspectorPanelDebugBoneComponent,
			[](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				if (coordinator->HasComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugBoneComponent>>(theEntity))
				{
					coordinator->RemoveComponent<r2::ecs::InstanceComponentT<r2::ecs::DebugBoneComponent>>(theEntity);
				}

				if (coordinator->HasComponent<r2::ecs::DebugBoneComponent>(theEntity))
				{
					coordinator->RemoveComponent<r2::ecs::DebugBoneComponent>(theEntity);
				}
			},
			[&ecsWorld](r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
			{
				ecs::DebugBoneComponent debugBoneComponent;
				debugBoneComponent.color = glm::vec4(1, 1, 0, 1);

				const ecs::SkeletalAnimationComponent& animationComponent = coordinator->GetComponent<ecs::SkeletalAnimationComponent>(theEntity);
				debugBoneComponent.debugBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::DebugBone, r2::sarr::Size(*animationComponent.animModel->optrBoneInfo));
				coordinator->AddComponent<r2::ecs::DebugBoneComponent>(theEntity, debugBoneComponent);
			});


		//@TODO(Serge): we probably need a way for the app to register new components to the inspector 
	}

	InspectorPanelComponentWidget::InspectorPanelComponentWidget()
		:mComponentName("EmptyComponent")
		,mComponentType(0)
		,mComponentTypeHash(0)
		,mComponentWidgetFunc(nullptr)
		,mRemoveComponentFunc(nullptr)
		,mAddComponentFunc(nullptr)
		,mSortOrder(0)
	{

	}

	InspectorPanelComponentWidget::InspectorPanelComponentWidget(
		const std::string& componentName,
		r2::ecs::ComponentType componentType,
		u64 componentTypeHash,
		InspectorPanelComponentWidgetFunc widgetFunction,
		InspectorPanelRemoveComponentFunc removeComponentFunc,
		InspectorPanelAddComponentFunc addComponentFunc)
		:mComponentName(componentName)
		,mComponentType(componentType)
		,mComponentTypeHash(componentTypeHash)
		,mComponentWidgetFunc(widgetFunction)
		,mRemoveComponentFunc(removeComponentFunc)
		,mAddComponentFunc(addComponentFunc)
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

		if (!mRemoveComponentFunc)
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
			mRemoveComponentFunc(theEntity, coordinator);
			open = false;
		}

		ImGui::PopItemWidth();
		ImGui::PopID();

		if (!mRemoveComponentFunc)
		{
			ImGui::PopStyleVar();
			ImGui::EndDisabled();
		}
		
		if (open)
		{
			mComponentWidgetFunc(editor, theEntity, coordinator);
		}
	}

	void InspectorPanelComponentWidget::SetSortOrder(u32 sortOrder)
	{
		mSortOrder = sortOrder;
	}

	void InspectorPanelComponentWidget::AddComponentToEntity(ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{ 
		if (mAddComponentFunc)
		{
			mAddComponentFunc(theEntity, coordinator);
		}
	}

}
#endif