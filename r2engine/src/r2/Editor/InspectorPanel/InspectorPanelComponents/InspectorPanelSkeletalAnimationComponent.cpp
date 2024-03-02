#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelSkeletalAnimationComponent.h"

#include "r2/Core/Engine.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "R2/Render/Animation/AnimationClip.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Render/Animation/Pose.h"

#include "imgui.h"

namespace r2::edit
{
	void DisplayAnimationDetails(r2::ecs::SkeletalAnimationComponent& animationComponent, s32& animationIndex, int id)
	{

		ImGui::PushID(id);

		std::string animationNameStr = "None";
		std::vector<std::string> animationNames;
		animationNames.push_back(animationNameStr);

		for (size_t i = 0; i < r2::sarr::Size(*animationComponent.animModel->optrAnimationClips); ++i)
		{
			const std::string& animationName = r2::sarr::At(*animationComponent.animModel->optrAnimationClips, i)->mAssetName.assetNameString;

			animationNames.push_back(animationName);
		}

		std::string preview = animationNames[animationIndex + 1];

		if (ImGui::BeginCombo("##label animation", preview.c_str()))
		{
			const u32 numAnimations = r2::sarr::Size(*animationComponent.animModel->optrAnimationClips);

			for (s32 i = 0; i < (s32)numAnimations+1; ++i)
			{
				if (ImGui::Selectable(animationNames[i].c_str(), (i-1) == animationIndex))
				{
					animationIndex = i-1;
				}
			}

			ImGui::EndCombo();
		}

		if (animationIndex >= 0)
		{
			r2::anim::AnimationClip* animation = r2::sarr::At(*animationComponent.animModel->optrAnimationClips, animationIndex);
			
			if (&animationIndex == &animationComponent.currentAnimationIndex)
			{
				ImGui::SliderFloat("Animation Time", &animationComponent.animationTime, 0.0f, animation->GetDuration());
			}

			ImGui::Text("Duration: %f", animation->GetDuration());
			ImGui::Text("Start Time: %f", animation->mStartTime);
			ImGui::Text("End Time: %f", animation->mEndTime);
		}


		

		ImGui::PopID();
	}

	//void AnimationComponentInstance(r2::ecs::SkeletalAnimationComponent& animationComponent, int id, Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	//{
	//	const r2::ecs::EditorComponent* editorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(theEntity);

	//	std::string nodeName = std::string("Entity - ") + std::to_string(theEntity) + std::string(" - Animation Instance - ") + std::to_string(id);
	//	if (editorComponent)
	//	{
	//		nodeName = editorComponent->editorName + std::string(" - Animation Instance - ") + std::to_string(id);
	//	}

	//	if (ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth, "%s", nodeName.c_str()))
	//	{
	//		if (ImGui::TreeNodeEx("Current Animation"))
	//		{
	//			DisplayAnimationDetails(animationComponent, animationComponent.currentAnimationIndex, id, r2::sarr::At(*animationComponent.animModel->optrAnimations, animationComponent.currentAnimationIndex));

	//			ImGui::TreePop();
	//		}

	//		if (ImGui::TreeNodeEx("Starting Animation"))
	//		{
	//			const r2::draw::Animation* startingAnimation = r2::sarr::At(*animationComponent.animModel->optrAnimations, animationComponent.startingAnimationIndex);
	//			R2_CHECK(startingAnimation != nullptr, "This should never happen");
	//			DisplayAnimationDetails(animationComponent, animationComponent.startingAnimationIndex, id+1, startingAnimation);
	//			ImGui::TreePop();
	//		}

	//		int startTime = animationComponent.startTime;
	//		ImGui::Text("Start Time: ");
	//		ImGui::SameLine();

	//		const r2::draw::Animation* currentAnimation = r2::sarr::At(*animationComponent.animModel->optrAnimations, animationComponent.currentAnimationIndex);
	//		if (ImGui::DragInt("##label starttime", &startTime, 1, 0, r2::util::SecondsToMilliseconds(currentAnimation->duration * (1.0f / currentAnimation->ticksPerSeconds))))
	//		{
	//			animationComponent.startTime = startTime;
	//		}

	//		bool shouldLoop = animationComponent.shouldLoop;
	//		if (ImGui::Checkbox("Loop", &shouldLoop))
	//		{
	//			animationComponent.shouldLoop = shouldLoop;
	//		}

	//		ImGui::TreePop();
	//	}
	//}

	//void InspectorPanelSkeletalAnimationComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	//{
	//	r2::ecs::SkeletalAnimationComponent& animationComponent = coordinator->GetComponent<r2::ecs::SkeletalAnimationComponent>(theEntity);

	//	AnimationComponentInstance(animationComponent, 0, editor, theEntity, coordinator);

	//	r2::ecs::InstanceComponentT<r2::ecs::SkeletalAnimationComponent>* instancedAnimations = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT< r2::ecs::SkeletalAnimationComponent>>(theEntity);

	//	if (instancedAnimations)
	//	{
	//		const auto numInstances = instancedAnimations->numInstances;

	//		for (u32 i = 0; i < numInstances; ++i)
	//		{
	//			r2::ecs::SkeletalAnimationComponent& nextAnimationComponent = r2::sarr::At(*instancedAnimations->instances, i);
	//			AnimationComponentInstance(nextAnimationComponent, i + 1, editor, theEntity, coordinator);
	//		}
	//	}
	//}

	InspectorPanelSkeletonAnimationComponentDataSource::InspectorPanelSkeletonAnimationComponentDataSource()
		:InspectorPanelComponentDataSource("Animation Component", 0, 0)
	{
	}

	InspectorPanelSkeletonAnimationComponentDataSource::InspectorPanelSkeletonAnimationComponentDataSource(r2::ecs::ECSCoordinator* coordinator)
		:InspectorPanelComponentDataSource("Animation Component", coordinator->GetComponentType<ecs::SkeletalAnimationComponent>(), coordinator->GetComponentTypeHash<ecs::SkeletalAnimationComponent>())
	{

	}

	void InspectorPanelSkeletonAnimationComponentDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, s32 instance)
	{
		ecs::SkeletalAnimationComponent* animationComponentPtr = static_cast<ecs::SkeletalAnimationComponent*>(componentData);
		ecs::SkeletalAnimationComponent& animationComponent = *animationComponentPtr;

		if (ImGui::TreeNodeEx("Current Animation"))
		{
			DisplayAnimationDetails(animationComponent, animationComponent.currentAnimationIndex, 0);

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Starting Animation"))
		{
			DisplayAnimationDetails(animationComponent, animationComponent.startingAnimationIndex, 1);
			ImGui::TreePop();
		}

		int startTime = animationComponent.startTime;
		ImGui::Text("Start Time: ");
		ImGui::SameLine();

		if (animationComponent.currentAnimationIndex > 0)
		{
			const r2::anim::AnimationClip* currentAnimation = r2::sarr::At(*animationComponent.animModel->optrAnimationClips, animationComponent.currentAnimationIndex);
			if (ImGui::DragInt("##label starttime", &startTime, 1, 0, r2::util::SecondsToMilliseconds(currentAnimation->GetDuration())))
			{
				animationComponent.startTime = startTime;
			}
		}
		
		bool shouldLoop = animationComponent.shouldLoop;
		if (ImGui::Checkbox("Loop", &shouldLoop))
		{
			animationComponent.shouldLoop = shouldLoop;
		}
	}

	bool InspectorPanelSkeletonAnimationComponentDataSource::InstancesEnabled() const
	{
		return true;
	}

	u32 InspectorPanelSkeletonAnimationComponentDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>* instancedSkeletalAnimationComponent = coordinator->GetComponentPtr<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(theEntity);

		if (instancedSkeletalAnimationComponent)
		{
			return instancedSkeletalAnimationComponent->numInstances;
		}

		return 0;
	}

	void* InspectorPanelSkeletonAnimationComponentDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return &coordinator->GetComponent<ecs::SkeletalAnimationComponent>(theEntity);
	}

	void* InspectorPanelSkeletonAnimationComponentDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>* instancedSkeletalAnimationComponent = coordinator->GetComponentPtr<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(theEntity);

		if (instancedSkeletalAnimationComponent)
		{
			R2_CHECK(i < instancedSkeletalAnimationComponent->numInstances, "Trying to access instance: %u but only have %u instances", i, instancedSkeletalAnimationComponent->numInstances);

			return &r2::sarr::At(*instancedSkeletalAnimationComponent->instances, i);
		}

		return nullptr;
	}

	void InspectorPanelSkeletonAnimationComponentDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{

	}

	void InspectorPanelSkeletonAnimationComponentDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>* instancedSkeletonAnimationComponent = coordinator->GetComponentPtr<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(theEntity);
		if (!instancedSkeletonAnimationComponent)
		{
			return;
		}

		R2_CHECK(i < instancedSkeletonAnimationComponent->numInstances, "Trying to access instance: %u but only have %u instances", i, instancedSkeletonAnimationComponent->numInstances);

		r2::sarr::RemoveElementAtIndexShiftLeft(*instancedSkeletonAnimationComponent->instances, i);
		instancedSkeletonAnimationComponent->numInstances--;
	}

	void InspectorPanelSkeletonAnimationComponentDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{

	}

	void InspectorPanelSkeletonAnimationComponentDataSource::AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances)
	{
		r2::ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>* instancedSkeletalAnimationComponentToUse = AddNewInstanceCapacity<ecs::SkeletalAnimationComponent>(coordinator, theEntity, numInstances);

		ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

		const ecs::SkeletalAnimationComponent& skeletalAnimationComponent = coordinator->GetComponent<ecs::SkeletalAnimationComponent>(theEntity);
		const ecs::RenderComponent& renderComponent = coordinator->GetComponent<ecs::RenderComponent>(theEntity);

		const r2::draw::Model* renderModel = skeletalAnimationComponent.animModel;

		for (u32 i = 0; i < numInstances; i++)
		{
			r2::ecs::SkeletalAnimationComponent newSkeletalAnimationComponent;
			newSkeletalAnimationComponent.animModel = renderModel;
			newSkeletalAnimationComponent.animModelAssetName = renderModel->assetName;

			R2_CHECK(renderModel->optrAnimationClips && r2::sarr::Size(*renderModel->optrAnimationClips) > 0, "we need to have animations");

			newSkeletalAnimationComponent.startingAnimationIndex = -1;
			newSkeletalAnimationComponent.shouldLoop = true;
			newSkeletalAnimationComponent.shouldUseSameTransformsForAllInstances = renderComponent.drawParameters.flags.IsSet(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
			newSkeletalAnimationComponent.currentAnimationIndex = newSkeletalAnimationComponent.startingAnimationIndex;
			newSkeletalAnimationComponent.startTime = 0;
			newSkeletalAnimationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, r2::anim::pose::Size(*renderModel->animSkeleton.mRestPose));

			newSkeletalAnimationComponent.animationPose = ECS_WORLD_ALLOC(ecsWorld, anim::Pose);
			u32 numJoints = r2::anim::pose::Size(*newSkeletalAnimationComponent.animModel->animSkeleton.mRestPose);

			newSkeletalAnimationComponent.animationPose->mJointTransforms = ECS_WORLD_MAKE_SARRAY(ecsWorld, math::Transform, numJoints);
			newSkeletalAnimationComponent.animationPose->mParents = ECS_WORLD_MAKE_SARRAY(ecsWorld, s32, numJoints);

			r2::anim::pose::Copy(*newSkeletalAnimationComponent.animationPose, *newSkeletalAnimationComponent.animModel->animSkeleton.mRestPose);
			newSkeletalAnimationComponent.animationTime = 0.0f;



			r2::sarr::Push(*instancedSkeletalAnimationComponentToUse->instances, newSkeletalAnimationComponent);
		}

		instancedSkeletalAnimationComponentToUse->numInstances += numInstances;
	}

	bool InspectorPanelSkeletonAnimationComponentDataSource::CanAddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return false;
	}

	bool InspectorPanelSkeletonAnimationComponentDataSource::ShouldDisableRemoveComponentButton() const
	{
		return true;
	}

}

#endif