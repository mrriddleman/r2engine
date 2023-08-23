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

	void PreviewAudioEmitterComponent(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity, const r2::ecs::AudioEmitterComponent& audioEmitterComponent)
	{
		r2::audio::AudioEngine audioEngine;

		r2::audio::AudioEngine::EventInstanceHandle eventInstanceHandle = audioEngine.CreateEventInstance(audioEmitterComponent.eventName);

		//set all of the initial parameters that are associated with this emitter
		for (u32 p = 0; p < audioEmitterComponent.numParameters; ++p)
		{
			audioEngine.SetEventParameterByName(
				audioEmitterComponent.eventInstanceHandle,
				audioEmitterComponent.parameters[p].parameterName,
				audioEmitterComponent.parameters[p].parameterValue);
		}

		const r2::ecs::TransformComponent* transformComponent = coordinator->GetComponentPtr<r2::ecs::TransformComponent>(theEntity);

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

	void InspectorPanelAudioEmitterComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	{
		r2::ecs::AudioEmitterComponent& audioEmitterComponent = coordinator->GetComponent<r2::ecs::AudioEmitterComponent>(theEntity);

		r2::audio::AudioEngine audioEngine;

		std::string previewEventName = "No Event";
		if (strlen(audioEmitterComponent.eventName) > 0)
		{
			previewEventName = audioEmitterComponent.eventName;
		}

		std::vector<std::string> eventNames;
		audioEngine.GetEventNames(eventNames);

		if (ImGui::BeginCombo("Audio Event Name", previewEventName.c_str()))
		{
			for (u32 i = 0; i < eventNames.size(); ++i)
			{
				const std::string& eventName = eventNames[i];

				if (ImGui::Selectable(eventName.c_str(), std::string(audioEmitterComponent.eventName) == eventName))
				{
					strcpy(audioEmitterComponent.eventName, eventName.c_str());
				}
			}

			ImGui::EndCombo();
		}
		ImGui::SameLine();

		if (ImGui::Button("Preview"))
		{
			PreviewAudioEmitterComponent(coordinator, theEntity, audioEmitterComponent);
		}

		std::string previewStartCondition = GetStartConditionString(audioEmitterComponent.startCondition);

		if (ImGui::BeginCombo("##label startcondition", previewStartCondition.c_str()))
		{
			for (u32 i = ecs::PLAY_ON_CREATE; i < ecs::NUM_AUDIO_EMITTER_START_TYPES; ++i)
			{
				std::string startConditionString = GetStartConditionString(static_cast<r2::ecs::AudioEmitterStartCondition>(i));
				if (ImGui::Selectable(startConditionString.c_str(), static_cast<r2::ecs::AudioEmitterStartCondition>(i) == audioEmitterComponent.startCondition))
				{
					audioEmitterComponent.startCondition = static_cast<r2::ecs::AudioEmitterStartCondition>( i );
				}
			}

			ImGui::EndCombo();
		}

		std::vector<audio::AudioEngine::AudioEngineParameterDesc> parameters;

		audioEngine.GetEventParameters(audioEmitterComponent.eventName, parameters);

		if (!parameters.empty())
		{
			const size_t numParams = std::min(parameters.size(), (size_t)audioEmitterComponent.numParameters);

			if (std::string(audioEmitterComponent.eventName) != previewEventName)
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

			ImGui::Text("Parameters");
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
}

#endif