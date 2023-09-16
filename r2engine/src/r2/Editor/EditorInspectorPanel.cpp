#include "r2pch.h"

#if defined(R2_EDITOR) && defined(R2_IMGUI)
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Core/Events/Event.h"
#include "r2/Editor/Editor.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "imgui.h"
#include "r2/Core/Engine.h"
#include "r2/Core/Application.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelEditorComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponentDataSource.h"

namespace r2::edit
{

	InspectorPanel::InspectorPanel()
		:mSelectedEntity(ecs::INVALID_ENTITY)
		,mCurrentComponentIndexToAdd(-1)
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


		dispatcher.Dispatch<r2::evt::EditorEntitySelectedEvent>([this](const r2::evt::EditorEntitySelectedEvent& e)
			{
				mSelectedEntity = e.GetEntity();
				return e.ShouldConsume();
			});
	}

	void InspectorPanel::Update()
	{

	}

	void InspectorPanel::Render(u32 dockingSpaceID)
	{
		ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);

		bool open = true;
		
		if (ImGui::Begin("Inspector", &open))
		{
			ecs::ECSCoordinator* coordinator = mnoptrEditor->GetECSCoordinator();

			if (coordinator && mSelectedEntity != ecs::INVALID_ENTITY)
			{
				const ecs::Signature& entitySignature = coordinator->GetSignature(mSelectedEntity);

				R2_CHECK(entitySignature.test(coordinator->GetComponentType<r2::ecs::EditorComponent>()), "Should always have an editor component");

				InspectorPanelEditorComponent(mnoptrEditor, mSelectedEntity, coordinator);

				//Add Component here
				std::vector<std::string> componentNames = coordinator->GetAllRegisteredNonInstancedComponentNames();
				std::vector<u64> componentTypeHashes = coordinator->GetAllRegisteredNonInstancedComponentTypeHashes();

				std::string currentComponentName = "";
				if (mCurrentComponentIndexToAdd >= 0)
				{
					currentComponentName = componentNames[mCurrentComponentIndexToAdd];
				}

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

				ImGui::SameLine();

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

				if (ImGui::Button("Add Component"))
				{
					inspectorPanelComponentWidget->AddComponentToEntity(coordinator, mSelectedEntity);
				}

				if (!hasAddComponentFunc)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}

				/*if (coordinator->HasComponent<ecs::TransformComponent>(mSelectedEntity) &&
					coordinator->HasComponent<ecs::RenderComponent>(mSelectedEntity))
				{
					auto* transformComponentWidget = GetComponentWidgetForComponentTypeHash(coordinator->GetComponentTypeHash<ecs::TransformComponent>());
					
					R2_CHECK(transformComponentWidget != nullptr, "Should always exist");

					InspectorPanelComponentWidget* animationComponentWidget = nullptr;

					if (coordinator->GetComponent<ecs::RenderComponent>(mSelectedEntity).isAnimated)
					{
						animationComponentWidget = GetComponentWidgetForComponentTypeHash(coordinator->GetComponentTypeHash<ecs::SkeletalAnimationComponent>());
					}

					if (transformComponentWidget)
					{
						ImGui::SameLine();
						if (ImGui::Button("Add Instance"))
						{
							auto addTransformInstanceFunc = transformComponentWidget->GetAddInstanceComponentFunc();

							addTransformInstanceFunc(mnoptrEditor, mSelectedEntity, coordinator);

							if (animationComponentWidget)
							{
								auto addAnimationInstanceFunc = animationComponentWidget->GetAddInstanceComponentFunc();

								if (addAnimationInstanceFunc)
								{
									addAnimationInstanceFunc(mnoptrEditor, mSelectedEntity, coordinator);
								}
							}
						}
					}
				}*/

				for (size_t i=0; i < mComponentWidgets.size(); ++i)
				{
					if (entitySignature.test(mComponentWidgets[i].GetComponentType()))
					{
						mComponentWidgets[i].ImGuiDraw(*this, mSelectedEntity);
					}
				}
			}
			
			ImGui::End();
		}
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