#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelAudioListenerComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/AudioListenerComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Audio/AudioEngine.h"
#include "imgui.h"

namespace r2::edit
{
	const std::string DEFAULT_LISTENER_STRING = "Default Listener";

	InspectorPanelAudioListenerComponentDataSource::InspectorPanelAudioListenerComponentDataSource()
		:InspectorPanelComponentDataSource("Audio Listener Component", 0, 0)
	{

	}

	InspectorPanelAudioListenerComponentDataSource::InspectorPanelAudioListenerComponentDataSource(r2::ecs::ECSCoordinator* coordinator)
		: InspectorPanelComponentDataSource("Audio Listener Component", coordinator->GetComponentType<ecs::AudioListenerComponent>(), coordinator->GetComponentTypeHash<ecs::AudioListenerComponent>())
	{

	}

	void InspectorPanelAudioListenerComponentDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, s32 instance)
	{
		r2::ecs::AudioListenerComponent& audioListenerComponent = coordinator->GetComponent<r2::ecs::AudioListenerComponent>(theEntity);

		std::string listenerPreview = DEFAULT_LISTENER_STRING;

		r2::audio::AudioEngine audioEngine;

		//@NOTE(Serge): Right now we only have support for a default listener 
		if (ImGui::BeginCombo("Audio Listener", listenerPreview.c_str()))
		{
			if (ImGui::Selectable(DEFAULT_LISTENER_STRING.c_str(), audioListenerComponent.listener == audioEngine.DEFAULT_LISTENER))
			{
				audioListenerComponent.listener = audioEngine.DEFAULT_LISTENER;
			}

			ImGui::EndCombo();
		}

		std::string entityToFollowPreview = "No Entity";

		if (audioListenerComponent.entityToFollow != r2::ecs::INVALID_ENTITY)
		{
			r2::ecs::EditorComponent& editorComponent = coordinator->GetComponent<r2::ecs::EditorComponent>(theEntity);
			entityToFollowPreview = editorComponent.editorName;
		}

		const r2::SArray<r2::ecs::Entity>& livingEntities = coordinator->GetAllLivingEntities();

		if (ImGui::BeginCombo("Entity to Follow", entityToFollowPreview.c_str()))
		{
			for (u32 i = 0; i < livingEntities.mSize; ++i)
			{
				ecs::Entity entity = r2::sarr::At(livingEntities, i);

				if (entity == theEntity)
				{
					continue;
				}

				r2::ecs::EditorComponent& editorComponent = coordinator->GetComponent<r2::ecs::EditorComponent>(entity);

				if (ImGui::Selectable(editorComponent.editorName.c_str(), entity == audioListenerComponent.entityToFollow))
				{
					audioListenerComponent.entityToFollow = entity;
				}
			}

			ImGui::EndCombo();
		}
	}

	bool InspectorPanelAudioListenerComponentDataSource::InstancesEnabled() const
	{
		return false;
	}

	u32 InspectorPanelAudioListenerComponentDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		return 0;
	}

	void* InspectorPanelAudioListenerComponentDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return &coordinator->GetComponent<ecs::AudioListenerComponent>(theEntity);
	}

	void* InspectorPanelAudioListenerComponentDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return nullptr;
	}

	void InspectorPanelAudioListenerComponentDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{

	}

	void InspectorPanelAudioListenerComponentDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{

	}

	void InspectorPanelAudioListenerComponentDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{

	}

	void InspectorPanelAudioListenerComponentDataSource::AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances)
	{

	}

}

#endif