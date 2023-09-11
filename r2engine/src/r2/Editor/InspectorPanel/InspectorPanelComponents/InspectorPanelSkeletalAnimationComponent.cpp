#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelSkeletalAnimationComponent.h"

#include "r2/Core/Engine.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"

#include "r2/Core/File/PathUtils.h"

#include "imgui.h"

namespace r2::edit
{
	void DisplayAnimationDetails(r2::ecs::SkeletalAnimationComponent& animationComponent, u32& animationIndex, int id, const r2::draw::Animation* animation)
	{
		R2_CHECK(animation != nullptr, "Should never happen");

		ImGui::PushID(id);

		if (ImGui::BeginCombo("##label animation", animation->animationName.c_str()))
		{
			const u32 numAnimations = r2::sarr::Size(*animationComponent.animModel->optrAnimations);

			for (u32 i = 0; i < numAnimations; ++i)
			{
				const r2::draw::Animation* nextAnimation = r2::sarr::At(*animationComponent.animModel->optrAnimations, i);
				if (ImGui::Selectable(nextAnimation->animationName.c_str(), animation->assetName == nextAnimation->assetName))
				{
					animationIndex = i;
					animation = nextAnimation;
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("Duration: %f", animation->duration);
		ImGui::Text("Ticks Per Second: %f", animation->ticksPerSeconds);
		ImGui::PopID();
	}

	void AnimationComponentInstance(r2::ecs::SkeletalAnimationComponent& animationComponent, int id, Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	{
		const r2::ecs::EditorComponent* editorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(theEntity);

		std::string nodeName = std::string("Entity - ") + std::to_string(theEntity) + std::string(" - Animation Instance - ") + std::to_string(id);
		if (editorComponent)
		{
			nodeName = editorComponent->editorName + std::string(" - Animation Instance - ") + std::to_string(id);
		}

		if (ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth, "%s", nodeName.c_str()))
		{
			if (ImGui::TreeNodeEx("Current Animation"))
			{
				DisplayAnimationDetails(animationComponent, animationComponent.currentAnimationIndex, id, r2::sarr::At(*animationComponent.animModel->optrAnimations, animationComponent.currentAnimationIndex));

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Starting Animation"))
			{
				const r2::draw::Animation* startingAnimation = r2::sarr::At(*animationComponent.animModel->optrAnimations, animationComponent.startingAnimationIndex);
				R2_CHECK(startingAnimation != nullptr, "This should never happen");
				DisplayAnimationDetails(animationComponent, animationComponent.startingAnimationIndex, id+1, startingAnimation);
				ImGui::TreePop();
			}

			int startTime = animationComponent.startTime;
			ImGui::Text("Start Time: ");
			ImGui::SameLine();

			const r2::draw::Animation* currentAnimation = r2::sarr::At(*animationComponent.animModel->optrAnimations, animationComponent.currentAnimationIndex);
			if (ImGui::DragInt("##label starttime", &startTime, 1, 0, r2::util::SecondsToMilliseconds(currentAnimation->duration * (1.0f / currentAnimation->ticksPerSeconds))))
			{
				animationComponent.startTime = startTime;
			}

			bool shouldLoop = animationComponent.shouldLoop;
			if (ImGui::Checkbox("Loop", &shouldLoop))
			{
				animationComponent.shouldLoop = shouldLoop;
			}

			ImGui::TreePop();
		}
	}

	void InspectorPanelSkeletalAnimationComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	{
		r2::ecs::SkeletalAnimationComponent& animationComponent = coordinator->GetComponent<r2::ecs::SkeletalAnimationComponent>(theEntity);

		AnimationComponentInstance(animationComponent, 0, editor, theEntity, coordinator);

		r2::ecs::InstanceComponentT<r2::ecs::SkeletalAnimationComponent>* instancedAnimations = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT< r2::ecs::SkeletalAnimationComponent>>(theEntity);

		if (instancedAnimations)
		{
			const auto numInstances = instancedAnimations->numInstances;

			for (u32 i = 0; i < numInstances; ++i)
			{
				r2::ecs::SkeletalAnimationComponent& nextAnimationComponent = r2::sarr::At(*instancedAnimations->instances, i);
				AnimationComponentInstance(nextAnimationComponent, i + 1, editor, theEntity, coordinator);
			}
		}
	}
}

#endif