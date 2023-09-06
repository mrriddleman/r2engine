#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelSkeletalAnimationComponent.h"

#include "r2/Core/Engine.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Core/File/PathUtils.h"

#include "imgui.h"

namespace r2::edit
{
	void DisplayAnimationDetails(int id, const r2::draw::Animation* animation)
	{
		R2_CHECK(animation != nullptr, "Should never happen");

		ImGui::PushID(id);
		ImGui::Text("Animation Name: %s", animation->animationName.c_str());
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
				DisplayAnimationDetails(id, r2::sarr::At(*animationComponent.animModel->optrAnimations, animationComponent.currentAnimationIndex));

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Starting Animation"))
			{
				const r2::draw::Animation* startingAnimation = r2::sarr::At(*animationComponent.animModel->optrAnimations, animationComponent.startingAnimationIndex);
				R2_CHECK(startingAnimation != nullptr, "This should never happen");
				DisplayAnimationDetails(id+1, startingAnimation);
				ImGui::TreePop();
			}

			//@TODO(Serge): we should somehow be able to change the Model - maybe this will be done in the Render Component
			//				right now we're not doing that. 
			//				We should also be able to change the starting animation - however we have no way of filtering the animations
			//				When we do the Animation System refactor, we'll need a way of tying an animated model with the animations
			//				Perhaps with some kind of character class or something. Not doing this yet.

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

			//bool useSameAnimationForAllInstances = animationComponent.shouldUseSameTransformsForAllInstances;

			//@TODO(Serge): This is a bit problematic atm, need to rethink later. Maybe this should force the other
			//				instances to use the current animation or something - would make more sense. Or get rid of it
			//if (ImGui::Checkbox("Use Animation Data for all instances", &useSameAnimationForAllInstances))
			//{
			//	animationComponent.shouldUseSameTransformsForAllInstances = useSameAnimationForAllInstances;
			//}

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