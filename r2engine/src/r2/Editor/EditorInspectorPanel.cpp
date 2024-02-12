#include "r2pch.h"

#if defined(R2_EDITOR) && defined(R2_IMGUI)
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Core/Events/Event.h"
#include "r2/Core/Events/KeyEvent.h"
#include "r2/Core/Events/MouseEvent.h"
#include "r2/Editor/Editor.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "r2/Core/Engine.h"
#include "r2/Core/Application.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelEditorComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponentDataSource.h"
#include "r2/Editor/EditorActions/SelectedEntityEditorAction.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "r2/Game/ECS/Components/SelectionComponent.h"
#include "r2/Editor/TransformWidgetInput.h"
//@HACK: we need this to get the camera but we should be able to get it through the editor or something
#include "r2/Render/Renderer/Renderer.h"
#include <glm/gtc/type_ptr.hpp>

namespace r2::edit
{

	InspectorPanel::InspectorPanel()
		:mSelectedEntity(ecs::INVALID_ENTITY)
		,mCurrentComponentIndexToAdd(-1)
		,mCurrentOperation(ImGuizmo::TRANSLATE)
		,mCurrentMode(ImGuizmo::LOCAL)
		,mCurrentInstance(-1)
	{

	}

	InspectorPanel::~InspectorPanel()
	{

	}

	void InspectorPanel::Init(Editor* noptrEditor)
	{
		mnoptrEditor = noptrEditor;

		RegisterAllEngineComponentWidgets(*this);
		CENG.GetApplication().RegisterComponentImGuiWidgets(*this);
	}

	void InspectorPanel::Shutdown()
	{

	}

	void InspectorPanel::OnEvent(evt::Event& e)
	{
		r2::evt::EventDispatcher dispatcher(e);

		dispatcher.Dispatch<r2::evt::EditorEntityDestroyedEvent>([this](const r2::evt::EditorEntityDestroyedEvent& e)
			{
				if (e.GetEntity() == mSelectedEntity)
				{
					mSelectedEntity = ecs::INVALID_ENTITY;
					mCurrentInstance = -1;
				}
				return e.ShouldConsume();
			});

		dispatcher.Dispatch<r2::evt::MouseButtonPressedEvent>([this](const r2::evt::MouseButtonPressedEvent& e)
			{
				if (e.MouseButton() == r2::io::MOUSE_BUTTON_LEFT)
				{
					r2::draw::renderer::EntityInstance entityInstance = r2::draw::renderer::ReadEntityInstanceAtMousePosition(e.XPos(), e.YPos());

					mnoptrEditor->PostNewAction(std::make_unique<r2::edit::SelectedEntityEditorAction>(mnoptrEditor, entityInstance.entityId, entityInstance.instanceId, mSelectedEntity, mCurrentInstance));
				}

				
				return false;
			});

		dispatcher.Dispatch<r2::evt::EditorEntitySelectedEvent>([this](const r2::evt::EditorEntitySelectedEvent& e)
			{
				ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

				ecs::ECSCoordinator* coordinator = mnoptrEditor->GetECSCoordinator();


				if (e.GetPreviouslySelectedEntity() != ecs::INVALID_ENTITY && coordinator->HasComponent<ecs::SelectionComponent>(e.GetPreviouslySelectedEntity()))
				{
					coordinator->RemoveComponent<ecs::SelectionComponent>(e.GetPreviouslySelectedEntity());
				}
				
				if (e.GetEntity() != ecs::INVALID_ENTITY  && coordinator->HasComponent<ecs::SelectionComponent>(e.GetEntity()))
				{
					coordinator->RemoveComponent<ecs::SelectionComponent>(e.GetEntity());
				}

				mSelectedEntity = e.GetEntity();
				mCurrentOperation = ImGuizmo::TRANSLATE;
				mCurrentInstance = e.GetSelectedInstance();

				if (!coordinator->HasComponent<ecs::TransformComponent>(mSelectedEntity))
				{
					mSelectedEntity = ecs::INVALID_ENTITY;
					mCurrentInstance = -1;
					return e.ShouldConsume();
				}
				else
				{
					if (coordinator->HasComponent<ecs::InstanceComponentT<ecs::TransformComponent>>(mSelectedEntity))
					{
						const ecs::InstanceComponentT<ecs::TransformComponent>& instancedTransformComponent = coordinator->GetComponent<ecs::InstanceComponentT<ecs::TransformComponent>>(mSelectedEntity);

						if (mCurrentInstance >= static_cast<s32>(instancedTransformComponent.numInstances))
						{
							mSelectedEntity = ecs::INVALID_ENTITY;
							mCurrentInstance = -1;

							return e.ShouldConsume();
						}
					}
					else
					{
						mCurrentInstance = -1;
					}
				}

				if (mSelectedEntity != ecs::INVALID_ENTITY)
				{
					ecs::SelectionComponent newSelectionComponent;
					newSelectionComponent.selectedInstances = ECS_WORLD_MAKE_SARRAY(ecsWorld, s32, 1); //@TODO(Serge): multiselect
					r2::sarr::Push(*newSelectionComponent.selectedInstances, mCurrentInstance);

					coordinator->AddComponent<ecs::SelectionComponent>(mSelectedEntity, newSelectionComponent);
				}

				return e.ShouldConsume();
			});

		dispatcher.Dispatch<r2::evt::KeyPressedEvent>([this](const r2::evt::KeyPressedEvent& keyEvent)
			{
				if (mSelectedEntity == r2::ecs::INVALID_ENTITY)
				{
					return false;
				}


				if (keyEvent.KeyCode() == r2::io::KEY_a && ((keyEvent.Modifiers() & r2::io::ALT_PRESSED) == r2::io::ALT_PRESSED))
				{
					ecs::Entity lastEntity = mSelectedEntity;
					s32 lastInstance = mCurrentInstance;

					mSelectedEntity = ecs::INVALID_ENTITY;
					mCurrentInstance = 0;

					mnoptrEditor->PostNewAction(std::make_unique<r2::edit::SelectedEntityEditorAction>(mnoptrEditor, ecs::INVALID_ENTITY, 0, lastEntity, lastInstance));
					return true;
				}
				
				ecs::ECSCoordinator* coordinator = mnoptrEditor->GetECSCoordinator();
				if (!coordinator->HasComponent<r2::ecs::TransformComponent>(mSelectedEntity))
				{
					return false;
				}

				if (coordinator->HasComponent<r2::ecs::InstanceComponentT<r2::ecs::TransformComponent>>(mSelectedEntity))
				{
					s32 numInstances = coordinator->GetComponent<r2::ecs::InstanceComponentT<r2::ecs::TransformComponent>>(mSelectedEntity).numInstances;

					if (keyEvent.KeyCode() == r2::io::KEY_LEFTBRACKET)
					{
						mCurrentInstance--;

						if (mCurrentInstance < -1 )
						{
							mCurrentInstance = numInstances - 1;
						}
					}
					else if (keyEvent.KeyCode() == r2::io::KEY_RIGHTBRACKET)
					{
						mCurrentInstance++;

						if (mCurrentInstance >= numInstances)
						{
							mCurrentInstance = -1;
						}
					}
				}

				return DoImGuizmoOperationKeyInput(keyEvent, mCurrentOperation);
			});
	}

	void InspectorPanel::Update()
	{

	}

	void InspectorPanel::ManipulateTransformComponent( r2::ecs::TransformComponent& transformComponent)
	{

		r2::ecs::ECSCoordinator* coordinator = mnoptrEditor->GetECSCoordinator();

		r2::Camera* camera = r2::draw::renderer::GetRenderCamera();

		glm::mat4 localMat = glm::mat4(1);

		math::Transform* transform = nullptr;
		ecs::eTransformDirtyFlags dirtyFlags;

		math::Transform transformToUse;
		const auto* hierarchyComponentPtr = coordinator->GetComponentPtr<ecs::HierarchyComponent>(mSelectedEntity);

		if (static_cast<ImGuizmo::MODE>(mCurrentMode) == ImGuizmo::MODE::LOCAL)
		{
			transform = &transformComponent.localTransform;
			transformToUse = *transform;
			dirtyFlags = ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY;

			if (hierarchyComponentPtr && hierarchyComponentPtr->parent != ecs::INVALID_ENTITY)
			{
				const auto& parentTransform = coordinator->GetComponent<ecs::TransformComponent>(hierarchyComponentPtr->parent);

				transformToUse = math::Combine(parentTransform.accumTransform, transformToUse);
			}
		}
		else
		{
			transform = &transformComponent.accumTransform;
			dirtyFlags = ecs::eTransformDirtyFlags::GLOBAL_TRANSFORM_DIRTY;

			transformToUse = *transform;
		}

		glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transformToUse.rotation));

		ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(transformToUse.position), glm::value_ptr(eulerAngles), glm::value_ptr(transformToUse.scale), glm::value_ptr(localMat));

		if (ImGuizmo::Manipulate(glm::value_ptr(camera->view), glm::value_ptr(camera->proj), static_cast<ImGuizmo::OPERATION>(mCurrentOperation), static_cast<ImGuizmo::MODE>(mCurrentMode), glm::value_ptr(localMat)))
		{
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localMat), glm::value_ptr(transformToUse.position), glm::value_ptr(eulerAngles), glm::value_ptr(transformToUse.scale));

			transformToUse.rotation = glm::quat(glm::radians(eulerAngles));

			if (static_cast<ImGuizmo::MODE>(mCurrentMode) == ImGuizmo::MODE::LOCAL && (hierarchyComponentPtr && hierarchyComponentPtr->parent != ecs::INVALID_ENTITY) )
			{
				const auto& parentTransform = coordinator->GetComponent<ecs::TransformComponent>(hierarchyComponentPtr->parent);

				*transform = math::Combine(math::Inverse(parentTransform.accumTransform), transformToUse);
			}
			else
			{
				*transform = transformToUse;
			}

			mnoptrEditor->GetSceneGraph().UpdateTransformForEntity(mSelectedEntity, dirtyFlags);			
		}
	}

	void InspectorPanel::Render(u32 dockingSpaceID)
	{
		ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);

		bool open = true;
		
		if (!ImGui::Begin("Inspector", &open))
		{
			ImGui::End();
			return;
		}

			ecs::ECSCoordinator* coordinator = mnoptrEditor->GetECSCoordinator();

			if (coordinator && mSelectedEntity != ecs::INVALID_ENTITY)
			{
				const ecs::InstanceComponentT<ecs::TransformComponent>* instancedTransforms = coordinator->GetComponentPtr<ecs::InstanceComponentT<ecs::TransformComponent>>(mSelectedEntity);

				const ecs::Signature& entitySignature = coordinator->GetSignature(mSelectedEntity);

				R2_CHECK(entitySignature.test(coordinator->GetComponentType<r2::ecs::EditorComponent>()), "Should always have an editor component");

				ImGui::Text("Mode: ");
				ImGui::SameLine();
				ImGui::RadioButton("LOCAL", &mCurrentMode, ImGuizmo::LOCAL);
				ImGui::SameLine();
				ImGui::RadioButton("WORLD", &mCurrentMode, ImGuizmo::WORLD);

				InspectorPanelEditorComponent(mnoptrEditor, mSelectedEntity, coordinator);

				//Add Component here
				std::vector<std::string> componentNames = coordinator->GetAllRegisteredNonInstancedComponentNames();
				std::vector<u64> componentTypeHashes = coordinator->GetAllRegisteredNonInstancedComponentTypeHashes();

				std::string currentComponentName = "";
				if (mCurrentComponentIndexToAdd >= 0)
				{
					currentComponentName = componentNames[mCurrentComponentIndexToAdd];
				}
				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6);
				if (ImGui::BeginCombo("##label components", currentComponentName.c_str()))
				{
					for (s32 i = 0; i < componentNames.size(); i++)
					{
						if (ImGui::Selectable(componentNames[i].c_str(), mCurrentComponentIndexToAdd == i))
						{
							mCurrentComponentIndexToAdd = i;
						}
					}

					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				InspectorPanelComponentWidget* inspectorPanelComponentWidget = nullptr;
				
				if (mCurrentComponentIndexToAdd >= 0 && !coordinator->HasComponent(mSelectedEntity, componentTypeHashes[mCurrentComponentIndexToAdd]))
				{
					for (u32 i = 0; i < mComponentWidgets.size(); ++i)
					{
						if (mComponentWidgets[i].GetComponentTypeHash() == componentTypeHashes[mCurrentComponentIndexToAdd])
						{
							inspectorPanelComponentWidget = &mComponentWidgets[i];
						}
					}
				}

				ImGui::SameLine();

				const bool hasAddComponentFunc = inspectorPanelComponentWidget && inspectorPanelComponentWidget->CanAddComponent(coordinator, mSelectedEntity);
				if (!hasAddComponentFunc)
				{
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}

				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.2);
				if (ImGui::Button("Add"))
				{
					inspectorPanelComponentWidget->AddComponentToEntity(coordinator, mSelectedEntity);

					if (instancedTransforms && instancedTransforms->numInstances > 0 && inspectorPanelComponentWidget->CanAddInstancedComponent())
					{
						inspectorPanelComponentWidget->AddInstancedComponentsToEntity(coordinator, mSelectedEntity, instancedTransforms->numInstances);
					}
				}
				ImGui::PopItemWidth();

				if (!hasAddComponentFunc)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}
				
				const bool hasTransformComponent = coordinator->HasComponent<ecs::TransformComponent>(mSelectedEntity);

				if ( hasTransformComponent)
				{

					//ImGuizmo stuff here since it has a position + render component
					//will need to think about how to deal with instances

					ecs::TransformComponent* transformComponent = coordinator->GetComponentPtr<ecs::TransformComponent>(mSelectedEntity);

					if (mCurrentInstance >= 0 && coordinator->HasComponent<ecs::InstanceComponentT<ecs::TransformComponent>>(mSelectedEntity))
					{
						const ecs::InstanceComponentT<ecs::TransformComponent>& instancedTransformComponent = coordinator->GetComponent<ecs::InstanceComponentT<ecs::TransformComponent>>(mSelectedEntity);

						if (mCurrentInstance < instancedTransformComponent.numInstances)
						{
							transformComponent = &r2::sarr::At(*instancedTransformComponent.instances, mCurrentInstance);
						}
					}

					ManipulateTransformComponent(*transformComponent);
				}

				const r2::ecs::EditorComponent* editorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(mSelectedEntity);
				R2_CHECK(editorComponent != nullptr, "Should always exist");

				bool open = ImGui::CollapsingHeader(editorComponent->editorName.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen);
				if (open)
				{
					for (size_t i = 0; i < mComponentWidgets.size(); ++i)
					{
						if (entitySignature.test(mComponentWidgets[i].GetComponentType()))
						{
							mComponentWidgets[i].ImGuiDraw(*this, mSelectedEntity);
						}
					}
				}
				
				
				if (instancedTransforms && instancedTransforms->numInstances > 0)
				{
					ImGui::Indent();

					for (u32 instanceIndex = 0; instanceIndex < instancedTransforms->numInstances; ++instanceIndex)
					{
						std::string instanceName = editorComponent->editorName + " - Instance: " + std::to_string(instanceIndex);

						s32 flags = ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

						if (static_cast<s32>(instanceIndex) == mCurrentInstance)
						{
							flags |= ImGuiTreeNodeFlags_DefaultOpen;
						}

						open = ImGui::CollapsingHeader(instanceName.c_str(), flags);
						
						ImVec2 size = ImGui::GetContentRegionAvail();
						ImGui::PushID( (std::string("DeleteInstance:") + std::to_string(instanceIndex)).c_str());
						ImGui::PushItemWidth(40);
						ImGui::SameLine(size.x - 40);

						if (ImGui::SmallButton("Delete"))
						{
							open = false;

							//Doing a copy to ensure nothing changes inbetween loop iterations and any event that my happen
							auto entityToDelete = mSelectedEntity;
							auto instanceToDelete = instanceIndex;

							for (size_t i = 0; i < mComponentWidgets.size(); ++i)
							{
								auto& componentWidget = mComponentWidgets[i];
								if (entitySignature.test(componentWidget.GetComponentType()) && componentWidget.CanAddInstancedComponent())
								{
									componentWidget.DeleteInstanceFromEntity(coordinator, entityToDelete, instanceToDelete);
								}
							}
							
						}

						ImGui::PopItemWidth();
						ImGui::PopID();


						if (open)
						{
							for (size_t i = 0; i < mComponentWidgets.size(); ++i)
							{
								if (entitySignature.test(mComponentWidgets[i].GetComponentType()))
								{
									mComponentWidgets[i].ImGuiDrawInstance(*this, mSelectedEntity, instanceIndex);
								}
							}
						}
						
						
					}
				}

				if (hasTransformComponent)
				{
					if (ImGui::Button("Add Instance"))
					{
						u32 numInstancesToAdd = 1;

						for (size_t i = 0; i < mComponentWidgets.size(); ++i)
						{
							auto& componentWidget = mComponentWidgets[i];
							if (entitySignature.test(componentWidget.GetComponentType()) && componentWidget.CanAddInstancedComponent())
							{
								componentWidget.AddInstancedComponentsToEntity(coordinator, mSelectedEntity, numInstancesToAdd);
							}
						}
					}
				}
			}
			
			ImGui::End();
		
	}

	void InspectorPanel::RegisterComponentWidget(const InspectorPanelComponentWidget& inspectorPanelComponentWidget)
	{
		auto componentType = inspectorPanelComponentWidget.GetComponentType();

		auto iter = std::find_if(mComponentWidgets.begin(), mComponentWidgets.end(), [&](const InspectorPanelComponentWidget& widget)
			{
				return widget.GetComponentType() == componentType;
			});

		if (iter == mComponentWidgets.end())
		{
			mComponentWidgets.push_back(inspectorPanelComponentWidget);

			std::sort(mComponentWidgets.begin(), mComponentWidgets.end(), [](const InspectorPanelComponentWidget& w1, const InspectorPanelComponentWidget& w2)
				{
					return w1.GetSortOrder() < w2.GetSortOrder();
				});
		}
	}

	InspectorPanelComponentWidget* InspectorPanel::GetComponentWidgetForComponentTypeHash(u64 componentTypeHash)
	{
		for (InspectorPanelComponentWidget& componentWidget: mComponentWidgets)
		{
			if (componentWidget.GetComponentTypeHash() == componentTypeHash)
			{
				return &componentWidget;
			}
		}

		return nullptr;
	}

	Editor* InspectorPanel::GetEditor()
	{
		return mnoptrEditor;
	}
}

#endif