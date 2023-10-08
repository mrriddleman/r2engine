#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelAudioEmitterComponent.h"
#include "r2/Game/ECS/Components/AudioEmitterComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Audio/AudioEngine.h"
#include "imgui.h"

namespace r2::edit
{
	std::string GetStartConditionString(r2::ecs::AudioEmitterStartCondition startCondition)
	{
		switch (startCondition)
		{
		case r2::ecs::PLAY_ON_CREATE:
			return "Play on Create";
			break;
		case r2::ecs::PLAY_ON_EVENT:
			return "Play On Event";
			break;
		case r2::ecs::NUM_AUDIO_EMITTER_START_TYPES:
			break;
		default:
			R2_CHECK(false, "Unsupported Start Condition");
			break;
		}

		return "";

	}

	void PreviewAudioEmitterComponent(const r2::ecs::AudioEmitterComponent& audioEmitterComponent, const r2::ecs::TransformComponent* transformComponent)
	{
		r2::audio::AudioEngine audioEngine;

		r2::audio::AudioEngine::EventInstanceHandle eventInstanceHandle = audioEngine.CreateEventInstance(audioEmitterComponent.eventName);

		//set all of the initial parameters that are associated with this emitter
		for (u32 p = 0; p < audioEmitterComponent.numParameters; ++p)
		{
			audioEngine.SetEventParameterByName(
				eventInstanceHandle,
				audioEmitterComponent.parameters[p].parameterName,
				audioEmitterComponent.parameters[p].parameterValue);
		}

		if (audioEngine.IsEvent3D(audioEmitterComponent.eventName) && transformComponent)
		{
			r2::audio::AudioEngine::Attributes3D attributes;
			attributes.up = transformComponent->accumTransform.rotation * glm::vec3(0, 0, 1);
			attributes.look = transformComponent->accumTransform.rotation * glm::vec3(0, 1, 0);
			attributes.position = transformComponent->accumTransform.position;
			attributes.velocity = glm::vec3(0);

			audioEngine.PlayEvent(eventInstanceHandle, attributes, true);
		}
		else
		{
			audioEngine.PlayEvent(eventInstanceHandle, true);
		}
	}

	InspectorPanelAudioEmitterComponentDataSource::InspectorPanelAudioEmitterComponentDataSource()
		:InspectorPanelComponentDataSource("Audio Emitter Component", 0, 0)
	{

	}

	InspectorPanelAudioEmitterComponentDataSource::InspectorPanelAudioEmitterComponentDataSource(r2::ecs::ECSCoordinator* coordinator)
		: InspectorPanelComponentDataSource("Audio Emitter Component", coordinator->GetComponentType<ecs::AudioEmitterComponent>(), coordinator->GetComponentTypeHash<ecs::AudioEmitterComponent>())
	{

	}

	void InspectorPanelAudioEmitterComponentDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::AudioEmitterComponent* audioEmitterComponentPtr = static_cast<ecs::AudioEmitterComponent*>(componentData);
		ecs::AudioEmitterComponent& audioEmitterComponent = *audioEmitterComponentPtr;

		r2::audio::AudioEngine audioEngine;

		std::string previewEventName = "No Event";
		if (strlen(audioEmitterComponent.eventName) > 0)
		{
			previewEventName = audioEmitterComponent.eventName;
		}

		std::vector<std::string> eventNames;
		audioEngine.GetEventNames(eventNames);
		ImGui::Text("Event: ");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##label Audio Event Name", previewEventName.c_str()))
		{
			for (u32 i = 0; i < eventNames.size(); ++i)
			{
				const std::string& eventName = eventNames[i];

				if (ImGui::Selectable(eventName.c_str(), std::string(audioEmitterComponent.eventName) == eventName))
				{
					strcpy(audioEmitterComponent.eventName, eventName.c_str());
					audioEmitterComponent.numParameters = 0;
				}
			}

			ImGui::EndCombo();
		}
		ImGui::SameLine();

		bool hasAudioEvent = audioEngine.HasEvent(audioEmitterComponent.eventName);

		if (!hasAudioEvent)
		{
			ImGui::BeginDisabled(true);
			ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
		}

		if (ImGui::Button(">Preview"))
		{
			ecs::TransformComponent* transformComponent = coordinator->GetComponentPtr<ecs::TransformComponent>(theEntity);
			PreviewAudioEmitterComponent(audioEmitterComponent, transformComponent);
		}

		std::string previewStartCondition = GetStartConditionString(audioEmitterComponent.startCondition);

		ImGui::Text("Start: ");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##label startcondition", previewStartCondition.c_str()))
		{
			for (u32 i = ecs::PLAY_ON_CREATE; i < ecs::NUM_AUDIO_EMITTER_START_TYPES; ++i)
			{
				std::string startConditionString = GetStartConditionString(static_cast<r2::ecs::AudioEmitterStartCondition>(i));
				if (ImGui::Selectable(startConditionString.c_str(), static_cast<r2::ecs::AudioEmitterStartCondition>(i) == audioEmitterComponent.startCondition))
				{
					audioEmitterComponent.startCondition = static_cast<r2::ecs::AudioEmitterStartCondition>(i);
				}
			}

			ImGui::EndCombo();
		}

		if (!hasAudioEvent)
		{
			ImGui::PopStyleVar();
			ImGui::EndDisabled();
		}

		std::vector<audio::AudioEngine::AudioEngineParameterDesc> parameters;

		if (hasAudioEvent)
		{
			audioEngine.GetEventParameters(audioEmitterComponent.eventName, parameters);
		}

		if (!parameters.empty())
		{
			const size_t numParams = std::min(std::max(parameters.size(), (size_t)audioEmitterComponent.numParameters), (size_t)r2::ecs::MAX_AUDIO_EMITTER_PARAMETERS);

			if (std::string(audioEmitterComponent.eventName) != previewEventName ||
				audioEmitterComponent.numParameters != numParams)
			{
				audioEmitterComponent.numParameters = numParams;
				//reset the parameters
				for (size_t i = 0; i < numParams; ++i)
				{
					strcpy(audioEmitterComponent.parameters[i].parameterName, parameters[i].name.c_str());
					if (audioEmitterComponent.parameters[i].parameterValue < parameters[i].minValue ||
						audioEmitterComponent.parameters[i].parameterValue > parameters[i].maxValue)
					{
						audioEmitterComponent.parameters[i].parameterValue = parameters[i].defaultValue;
					}
				}
			}

			if (numParams > 0)
			{
				ImGui::Text("Parameters");
			}

			for (size_t i = 0; i < numParams; ++i)
			{
				if (ImGui::TreeNodeEx(parameters[i].name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth, "%s", parameters[i].name.c_str()))
				{
					const auto& param = parameters[i];
					ImGui::Text(param.name.c_str());
					ImGui::SameLine();
					ImGui::SliderFloat("##label value", &audioEmitterComponent.parameters[i].parameterValue, param.minValue, param.maxValue);
					ImGui::SameLine();
					if (ImGui::SmallButton("R"))
					{
						audioEmitterComponent.parameters[i].parameterValue = param.defaultValue;
					}
					ImGui::TreePop();
				}
			}
		}

		bool allowFadeout = audioEmitterComponent.allowFadeoutWhenStopping;
		if (ImGui::Checkbox("Allow fadeout when stopping", &allowFadeout))
		{
			audioEmitterComponent.allowFadeoutWhenStopping = allowFadeout;
		}

		bool releaseAfterPlay = audioEmitterComponent.releaseAfterPlay;
		if (ImGui::Checkbox("Release After Play", &releaseAfterPlay))
		{
			audioEmitterComponent.releaseAfterPlay = releaseAfterPlay;
		}
	}

	bool InspectorPanelAudioEmitterComponentDataSource::InstancesEnabled() const
	{
		return false;
	}

	u32 InspectorPanelAudioEmitterComponentDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		return 0;
	}

	void* InspectorPanelAudioEmitterComponentDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return &coordinator->GetComponent<ecs::AudioEmitterComponent>(theEntity);
	}

	void* InspectorPanelAudioEmitterComponentDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return nullptr;
	}

	void InspectorPanelAudioEmitterComponentDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		coordinator->RemoveComponent<r2::ecs::AudioEmitterComponent>(theEntity);
	}

	void InspectorPanelAudioEmitterComponentDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{

	}

	void InspectorPanelAudioEmitterComponentDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::AudioEmitterComponent audioEmitterComponent;

		audioEmitterComponent.eventInstanceHandle = r2::audio::AudioEngine::InvalidEventInstanceHandle;
		r2::util::PathCpy(audioEmitterComponent.eventName, "No Event");
		audioEmitterComponent.numParameters = 0;
		audioEmitterComponent.startCondition = ecs::PLAY_ON_EVENT;
		audioEmitterComponent.allowFadeoutWhenStopping = false;
		audioEmitterComponent.releaseAfterPlay = true;

		coordinator->AddComponent<ecs::AudioEmitterComponent>(theEntity, audioEmitterComponent);
	}

	void InspectorPanelAudioEmitterComponentDataSource::AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances)
	{

	}

}

#endif