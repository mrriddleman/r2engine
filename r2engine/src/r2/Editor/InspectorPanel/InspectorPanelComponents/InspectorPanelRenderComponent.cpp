#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelRenderComponent.h"
#include "r2/Editor/Editor.h"

#include "r2/Core/Engine.h"
#include "r2/Core/File/PathUtils.h"

#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"

#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Game/Level/Level.h"

#include "r2/Render/Model/Model.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/Renderer.h"

#include "r2/Core/Engine.h"
#include "r2/Game/ECSWorld/ECSWorld.h"

#include "r2/Render/Model/Materials/MaterialHelpers.h"
#include "r2/Render/Model/Materials/Material_generated.h"

#include "r2/Editor/EditorMaterialEditor/MaterialEditor.h"
#include "r2/Render/Animation/Pose.h"
//this is dumb but...
//#include "r2/Render/Animation/AnimationPlayer.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/AssetFiles/MaterialManifestAssetFile.h"

#endif

#include "imgui.h"

#include "r2/ImGui/CustomFont.h"
#include <glm/gtc/type_ptr.hpp>

namespace r2::edit
{
	std::string GetModelNameForAssetFile(const r2::asset::AssetFile* assetFile)
	{
		R2_CHECK(assetFile != nullptr, "The current animation asset file is nullptr");

		const char* currentModelFilePath = assetFile->FilePath();

		char modelName[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::CopyFileName(currentModelFilePath, modelName);

		return std::string(modelName);
	}

	std::string GetIsAnimatedString(bool isAnimated)
	{
		if (isAnimated)
		{
			return "Animated";
		}

		return "Static";
	}

	std::string GetPrimitiveTypeString(r2::draw::PrimitiveType primitiveType)
	{
		switch (primitiveType)
		{
		case r2::draw::PrimitiveType::POINTS:
			return "POINTS";
			break;
		case r2::draw::PrimitiveType::LINES:
			return "LINES";
			break;
		case r2::draw::PrimitiveType::LINE_LOOP:
			return "LINE LOOP";
			break;
		case r2::draw::PrimitiveType::LINE_STRIP:
			return "LINE STRIP";
			break;
		case r2::draw::PrimitiveType::TRIANGLES:
			return "TRIANGLES";
			break;
		case r2::draw::PrimitiveType::TRIANGLE_STRIP:
			return "TRIANGLE STRIP";
			break;
		case r2::draw::PrimitiveType::TRIANGLE_FAN:
			return "TRIANGLE FAN";
			break;
		default:
			R2_CHECK(false, "Unsupported Primitive Type");
			break;
		}

		return "";
	}

	std::string GetDrawLayerString(r2::draw::DrawLayer layer)
	{
		switch (layer)
		{
		case r2::draw::DL_CLEAR:
			return "CLEAR";
			break;
		case r2::draw::DL_COMPUTE:
			return "COMPUTE";
			break;
		case r2::draw::DL_WORLD:
			return "WORLD";
			break;
		case r2::draw::DL_CHARACTER:
			return "CHARACTER";
			break;
		case r2::draw::DL_SKYBOX:
			return "SKYBOX";
			break;
		case r2::draw::DL_TRANSPARENT:
			return "TRANSPARENT";
			break;
		case r2::draw::DL_EFFECT:
			return "EFFECT";
			break;
		case r2::draw::DL_HUD:
			return "HUD";
			break;
		case r2::draw::DL_SCREEN:
			return "SCREEN";
			break;
		case r2::draw::DL_DEBUG:
			return "DEBUG";
			break;
		default:
			R2_CHECK(false, "Unsupported Draw Layer");
			break;
		}
		return "";
	}

	void StencilOpItem(const std::string& textLabel, const std::string& comboPreview, const std::string& comboLabel, u32& value)
	{
		ImGui::Text(textLabel.c_str());
		ImGui::SameLine();
		if (ImGui::BeginCombo(comboLabel.c_str(), comboPreview.c_str()))
		{
			if (ImGui::Selectable("KEEP", value == r2::draw::KEEP))
			{
				value == r2::draw::KEEP;
			}

			if (ImGui::Selectable("ZERO", value == r2::draw::ZERO))
			{
				value = r2::draw::ZERO;
			}

			if (ImGui::Selectable("REPLACE", value == r2::draw::REPLACE))
			{
				value = r2::draw::REPLACE;
			}

			ImGui::EndCombo();
		}
	}

	std::string GetStencilOpValue(u32 value)
	{
		if (value == r2::draw::KEEP)
		{
			return "KEEP";
		}
		if (value == r2::draw::ZERO)
		{
			return "ZERO";
		}
		if (value == r2::draw::REPLACE)
		{
			return "REPLACE";
		}
		R2_CHECK(false, "UNSUPPORTED VALUE");
		return "";
	}

	std::string GetStencilFunctionName(u32 func)
	{
		if (func == r2::draw::NEVER)
		{
			return "NEVER";
		}
		if (func == r2::draw::LESS)
		{
			return "LESS";
		}
		if (func == r2::draw::LEQUAL)
		{
			return "LEQUAL";
		}
		if (func == r2::draw::GREATER)
		{
			return "GREATER";
		}
		if (func == r2::draw::GEQUAL)
		{
			return "GEQUAL";
		}
		if (func == r2::draw::EQUAL)
		{
			return "EQUAL";
		}
		if (func == r2::draw::NOTEQUAL)
		{
			return "NOT EQUAL";
		}
		if (func == r2::draw::ALWAYS)
		{
			return "ALWAYS";
		}

		R2_CHECK(false, "Unsupported Stencil Function");
		return "";
	}

	std::string GetBlendEquationString(u32 blendEquation)
	{
		if (blendEquation == r2::draw::BLEND_EQUATION_ADD)
		{
			return "ADD";
		}

		if (blendEquation == r2::draw::BLEND_EQUATION_SUBTRACT)
		{
			return "SUBTRACT";
		}

		if (blendEquation == r2::draw::BLEND_EQUATION_REVERSE_SUBTRACT)
		{
			return "REVERSE SUBTRACT";
		}

		if (blendEquation == r2::draw::BLEND_EQUATION_MIN)
		{
			return "MIN";
		}

		if (blendEquation == r2::draw::BLEND_EQUATION_MAX)
		{
			return "MAX";
		}

		R2_CHECK(false, "Unsupported Blend Equation");

		return "";
	}

	void FreeSkeletalAnimationComponentData(r2::ecs::ECSWorld& ecsWorld, r2::ecs::SkeletalAnimationComponent& skeletalAnimationComponent)
	{
		ECS_WORLD_FREE(ecsWorld, skeletalAnimationComponent.animationPose->mParents);
		ECS_WORLD_FREE(ecsWorld, skeletalAnimationComponent.animationPose->mJointTransforms);
		ECS_WORLD_FREE(ecsWorld, skeletalAnimationComponent.animationPose);
		ECS_WORLD_FREE(ecsWorld, skeletalAnimationComponent.shaderBones);
	}

	void AllocateNewSkeletalAnimationComponentData(r2::ecs::ECSWorld& ecsWorld, const r2::draw::Model& renderModel, r2::ecs::SkeletalAnimationComponent& skeletalAnimationComponent)
	{
		const auto numJoints = r2::anim::pose::Size(*renderModel.animSkeleton.mRestPose);

		skeletalAnimationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, numJoints);
		skeletalAnimationComponent.animationPose = ECS_WORLD_ALLOC(ecsWorld, r2::anim::Pose);
		skeletalAnimationComponent.animationPose->mJointTransforms = ECS_WORLD_MAKE_SARRAY(ecsWorld, math::Transform, numJoints);
		skeletalAnimationComponent.animationPose->mParents = ECS_WORLD_MAKE_SARRAY(ecsWorld, s32, numJoints);

		r2::anim::pose::Copy(*skeletalAnimationComponent.animationPose, *renderModel.animSkeleton.mRestPose);
		skeletalAnimationComponent.animationTime = 0.0f;
	}

	void UpdateSkeletalAnimationComponentIfNecessary(
		const r2::draw::Model* renderModel,
		bool useSameBoneTransformsForInstances,
		r2::ecs::ECSCoordinator* coordinator,
		r2::ecs::Entity entity,
		int id,
		r2::ecs::RenderComponent& renderComponent)
	{
		r2::ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

		bool hasSkeletalAnimationComponent = coordinator->HasComponent<r2::ecs::SkeletalAnimationComponent>(entity);
		bool hasDebugBoneComponent = coordinator->HasComponent<r2::ecs::DebugBoneComponent>(entity);
		bool updateShaderBones = false;
		auto numJoints = 0;
		
		if (renderComponent.isAnimated)
		{
			numJoints = r2::anim::pose::Size(*renderModel->animSkeleton.mRestPose);
			R2_CHECK(renderModel->optrAnimationClips && r2::sarr::Size(*renderModel->optrAnimationClips) > 0, "we need to have animations");
		}
			

		if (renderComponent.isAnimated && !hasSkeletalAnimationComponent)
		{
			//add the skeletal animation component
			r2::ecs::SkeletalAnimationComponent newSkeletalAnimationComponent;
			newSkeletalAnimationComponent.animModel = renderModel;
			newSkeletalAnimationComponent.animModelAssetName = renderModel->assetName;

			

			newSkeletalAnimationComponent.startingAnimationIndex = 0;
			newSkeletalAnimationComponent.shouldLoop = true;
			newSkeletalAnimationComponent.shouldUseSameTransformsForAllInstances = useSameBoneTransformsForInstances;
			newSkeletalAnimationComponent.currentAnimationIndex = newSkeletalAnimationComponent.startingAnimationIndex;
			newSkeletalAnimationComponent.startTime = 0;
			
			AllocateNewSkeletalAnimationComponentData(ecsWorld, *renderModel, newSkeletalAnimationComponent);

			/*auto numJoints = newSkeletalAnimationComponent.animModel->animSkeleton.mRestPose->mJointTransforms->mSize;
			newSkeletalAnimationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, numJoints);
			newSkeletalAnimationComponent.animationPose = ECS_WORLD_ALLOC(ecsWorld, r2::anim::Pose);
			newSkeletalAnimationComponent.animationPose->mJointTransforms = ECS_WORLD_MAKE_SARRAY(ecsWorld, math::Transform, numJoints);
			newSkeletalAnimationComponent.animationPose->mParents = ECS_WORLD_MAKE_SARRAY(ecsWorld, s32, numJoints);*/

			/*r2::anim::pose::Copy(*newSkeletalAnimationComponent.animationPose, *newSkeletalAnimationComponent.animModel->animSkeleton.mRestPose);
			newSkeletalAnimationComponent.animationTime = 0.0f;*/

			coordinator->AddComponent<r2::ecs::SkeletalAnimationComponent>(entity, newSkeletalAnimationComponent);

			updateShaderBones = true;

			if (!renderComponent.drawParameters.flags.IsSet(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES))
			{
				renderComponent.drawParameters.flags.Set(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
			}
		}
		else if (renderComponent.isAnimated && hasSkeletalAnimationComponent)
		{
			//in this case we need to make sure that the old skeletal animation component is setup correctly
			r2::ecs::SkeletalAnimationComponent& animationComponent = coordinator->GetComponent<r2::ecs::SkeletalAnimationComponent>(entity);
			animationComponent.animModel = renderModel;
			animationComponent.animModelAssetName = renderModel->assetName;


			//Put the starting animation index back to 0 since we don't know how many animations we have vs what we used to have
			animationComponent.startingAnimationIndex = 0;
			animationComponent.shouldLoop = true;
			animationComponent.shouldUseSameTransformsForAllInstances = useSameBoneTransformsForInstances;
			animationComponent.currentAnimationIndex = animationComponent.startingAnimationIndex;
			animationComponent.startTime = 0;

			

			if (r2::sarr::Capacity(*animationComponent.shaderBones) != numJoints)
			{
				FreeSkeletalAnimationComponentData(ecsWorld, animationComponent);
				AllocateNewSkeletalAnimationComponentData(ecsWorld, *renderModel, animationComponent);
				/*ECS_WORLD_FREE(ecsWorld, animationComponent.animationPose->mParents);
				ECS_WORLD_FREE(ecsWorld, animationComponent.animationPose->mJointTransforms);
				ECS_WORLD_FREE(ecsWorld, animationComponent.animationPose);
				ECS_WORLD_FREE(ecsWorld, animationComponent.shaderBones);*/

				/*animationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, numJoints);

				animationComponent.animationPose = ECS_WORLD_ALLOC(ecsWorld, r2::anim::Pose);

				animationComponent.animationPose->mJointTransforms = ECS_WORLD_MAKE_SARRAY(ecsWorld, math::Transform, numJoints);
				animationComponent.animationPose->mParents = ECS_WORLD_MAKE_SARRAY(ecsWorld, s32, numJoints);

				r2::anim::pose::Copy(*animationComponent.animationPose, *renderModel->animSkeleton.mRestPose);
				animationComponent.animationTime = 0.0f;*/

			}
			r2::sarr::Clear(*animationComponent.shaderBones);

			updateShaderBones = true;
			
			if (hasDebugBoneComponent)
			{
				r2::ecs::DebugBoneComponent& debugBoneComponent = coordinator->GetComponent<r2::ecs::DebugBoneComponent>(entity);

				ECS_WORLD_FREE(ecsWorld, debugBoneComponent.debugBones);

				debugBoneComponent.debugBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::DebugBone, numJoints);

				r2::sarr::Clear(*debugBoneComponent.debugBones);
			}
		}
		else if (!renderComponent.isAnimated && hasSkeletalAnimationComponent)
		{
			//remove the skeletal animation component

			//free the shader bones first since that's not included when we remove the component
			coordinator->RemoveComponent<r2::ecs::SkeletalAnimationComponent>(entity);

			if (hasDebugBoneComponent)
			{
				coordinator->RemoveComponent<r2::ecs::DebugBoneComponent>(entity);
			}
		}
		else //!renderComponent.isAnimated && !coordinator->HasComponent<r2::ecs::SkeletalAnimationComponent>(entity)
		{
			//nothing to do?
		}

		//@NOTE(Serge): use the animation player to populate the shaderBones - this is necessary since we won't go through an Update cycle before we draw next
		if (updateShaderBones)
		{
			r2::ecs::SkeletalAnimationComponent& animationComponent = coordinator->GetComponent<r2::ecs::SkeletalAnimationComponent>(entity);
			r2::SArray<r2::draw::DebugBone>* debugBones = nullptr;
			if (hasDebugBoneComponent)
			{
				r2::ecs::DebugBoneComponent& debugBoneComponent = coordinator->GetComponent<r2::ecs::DebugBoneComponent>(entity);
				debugBones = debugBoneComponent.debugBones;
			}

			r2::anim::pose::GetMatrixPalette(*animationComponent.animationPose, animationComponent.animModel->animSkeleton, animationComponent.shaderBones, 0);

			if (debugBones)
			{
				r2::anim::pose::GetDebugBones(*animationComponent.animationPose, debugBones);
			}
			//auto ticks = CENG.GetTicks();
			//r2::draw::PlayAnimationForAnimModel(
			//	ticks,
			//	animationComponent.startTime,
			//	animationComponent.shouldLoop,
			//	*renderModel,
			//	r2::sarr::At(*renderModel->optrAnimations, animationComponent.currentAnimationIndex),
			//	*animationComponent.shaderBones,
			//	debugBones, 0);
		}

		bool hasInstancedSkeletalAnimationComponent = coordinator->HasComponent<r2::ecs::InstanceComponentT<r2::ecs::SkeletalAnimationComponent>>(entity);
		bool hasInstancedTransformComponent = coordinator->HasComponent<r2::ecs::InstanceComponentT<ecs::TransformComponent>>(entity);

		bool isInstanced = (hasInstancedSkeletalAnimationComponent || hasInstancedTransformComponent);

		if (renderComponent.isAnimated && hasInstancedSkeletalAnimationComponent)
		{
			//change
			r2::ecs::InstanceComponentT<r2::ecs::SkeletalAnimationComponent>& instancedAnimationComponent = coordinator->GetComponent<r2::ecs::InstanceComponentT<r2::ecs::SkeletalAnimationComponent>>(entity);

			const auto numInstances = instancedAnimationComponent.numInstances;

			for (u32 i = 0; i < numInstances; ++i)
			{
				r2::ecs::SkeletalAnimationComponent& nextAnimationComponent = r2::sarr::At(*instancedAnimationComponent.instances, i);

				nextAnimationComponent.animModel = renderModel;
				nextAnimationComponent.animModelAssetName = renderModel->assetName;

				//Put the starting animation index back to 0 since we don't know how many animations we have vs what we used to have
				nextAnimationComponent.startingAnimationIndex = 0;
				nextAnimationComponent.shouldLoop = true;
				nextAnimationComponent.shouldUseSameTransformsForAllInstances = useSameBoneTransformsForInstances;
				nextAnimationComponent.currentAnimationIndex = nextAnimationComponent.startingAnimationIndex;
				nextAnimationComponent.startTime = 0;

				
				if (r2::sarr::Capacity(*nextAnimationComponent.shaderBones) != numJoints)
				{
					FreeSkeletalAnimationComponentData(ecsWorld, nextAnimationComponent);
					/*ECS_WORLD_FREE(ecsWorld, nextAnimationComponent.animationPose->mParents);
					ECS_WORLD_FREE(ecsWorld, nextAnimationComponent.animationPose->mJointTransforms);
					ECS_WORLD_FREE(ecsWorld, nextAnimationComponent.animationPose);
					ECS_WORLD_FREE(ecsWorld, nextAnimationComponent.shaderBones);*/
					AllocateNewSkeletalAnimationComponentData(ecsWorld, *renderModel, nextAnimationComponent);
					/*nextAnimationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, numJoints);
					nextAnimationComponent.animationPose = ECS_WORLD_ALLOC(ecsWorld, r2::anim::Pose);
					nextAnimationComponent.animationPose->mJointTransforms = ECS_WORLD_MAKE_SARRAY(ecsWorld, math::Transform, numJoints);
					nextAnimationComponent.animationPose->mParents = ECS_WORLD_MAKE_SARRAY(ecsWorld, s32, numJoints);

					r2::anim::pose::Copy(*nextAnimationComponent.animationPose, *renderModel->animSkeleton.mRestPose);
					nextAnimationComponent.animationTime = 0.0f;*/

				}
				r2::sarr::Clear(*nextAnimationComponent.shaderBones);


				auto* instancedDebugBoneComponent = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT<ecs::DebugBoneComponent>>(entity);

				if (instancedDebugBoneComponent)
				{
					auto& debugComponentInstance = r2::sarr::At(*instancedDebugBoneComponent->instances, i);
					ECS_WORLD_FREE(ecsWorld, debugComponentInstance.debugBones);

					debugComponentInstance.debugBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::DebugBone, numJoints);

					r2::sarr::Clear(*debugComponentInstance.debugBones);
				}

				if (updateShaderBones)
				{
					r2::ecs::SkeletalAnimationComponent& animationComponent = r2::sarr::At(*instancedAnimationComponent.instances, i);
					r2::SArray<r2::draw::DebugBone>* debugBones = nullptr;

					if (instancedDebugBoneComponent)
					{
						r2::ecs::DebugBoneComponent& debugBoneComponent = r2::sarr::At(*instancedDebugBoneComponent->instances, i);;
						debugBones = debugBoneComponent.debugBones;
					}

					r2::anim::pose::GetMatrixPalette(*animationComponent.animationPose, animationComponent.animModel->animSkeleton, animationComponent.shaderBones, 0);

					if (debugBones)
					{
						r2::anim::pose::GetDebugBones(*animationComponent.animationPose, debugBones);
					}

					/*auto ticks = CENG.GetTicks();
					r2::draw::PlayAnimationForAnimModel(
						ticks,
						animationComponent.startTime,
						animationComponent.shouldLoop,
						*renderModel,
						r2::sarr::At(*renderModel->optrAnimations, animationComponent.currentAnimationIndex),
						*animationComponent.shaderBones,
						debugBones, 0);*/
				}
			}
		}
		else if (renderComponent.isAnimated && !hasInstancedSkeletalAnimationComponent && hasInstancedTransformComponent)
		{
			//add
			r2::ecs::InstanceComponentT<r2::ecs::SkeletalAnimationComponent> instancedAnimationComponent;

			const r2::ecs::InstanceComponentT<ecs::TransformComponent>& instancedTransformComponent = coordinator->GetComponent<r2::ecs::InstanceComponentT<ecs::TransformComponent>>(entity);

			instancedAnimationComponent.numInstances = instancedTransformComponent.numInstances;

			instancedAnimationComponent.instances = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::ecs::SkeletalAnimationComponent, instancedAnimationComponent.numInstances);

			for (u32 i = 0; i < instancedTransformComponent.numInstances; i++)
			{
				r2::ecs::SkeletalAnimationComponent newSkeletalAnimationComponent;
				newSkeletalAnimationComponent.animModel = renderModel;
				newSkeletalAnimationComponent.animModelAssetName = renderModel->assetName;

				newSkeletalAnimationComponent.startingAnimationIndex = 0;
				newSkeletalAnimationComponent.shouldLoop = true;
				newSkeletalAnimationComponent.shouldUseSameTransformsForAllInstances = useSameBoneTransformsForInstances;
				newSkeletalAnimationComponent.currentAnimationIndex = newSkeletalAnimationComponent.startingAnimationIndex;
				newSkeletalAnimationComponent.startTime = 0;
				AllocateNewSkeletalAnimationComponentData(ecsWorld, *renderModel, newSkeletalAnimationComponent);
				//newSkeletalAnimationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, numJoints);
				//newSkeletalAnimationComponent.animationPose = ECS_WORLD_ALLOC(ecsWorld, r2::anim::Pose);
				//newSkeletalAnimationComponent.animationPose->mJointTransforms = ECS_WORLD_MAKE_SARRAY(ecsWorld, math::Transform, numJoints);
				//newSkeletalAnimationComponent.animationPose->mParents = ECS_WORLD_MAKE_SARRAY(ecsWorld, s32, numJoints);

				//r2::anim::pose::Copy(*newSkeletalAnimationComponent.animationPose, *renderModel->animSkeleton.mRestPose);
				//newSkeletalAnimationComponent.animationTime = 0.0f;

				r2::sarr::Push(*instancedAnimationComponent.instances, newSkeletalAnimationComponent);

				if (updateShaderBones)
				{
					r2::ecs::SkeletalAnimationComponent& animationComponent = newSkeletalAnimationComponent;
					r2::SArray<r2::draw::DebugBone>* debugBones = nullptr;

					auto* instancedDebugBoneComponent = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT<ecs::DebugBoneComponent>>(entity);

					if (instancedDebugBoneComponent)
					{
						r2::ecs::DebugBoneComponent& debugBoneComponent = r2::sarr::At(*instancedDebugBoneComponent->instances, i);;
						debugBones = debugBoneComponent.debugBones;
					}

					r2::anim::pose::GetMatrixPalette(*animationComponent.animationPose, animationComponent.animModel->animSkeleton, animationComponent.shaderBones, 0);

					if (debugBones)
					{
						r2::anim::pose::GetDebugBones(*animationComponent.animationPose, debugBones);
					}

					//auto ticks = CENG.GetTicks();
					//r2::draw::PlayAnimationForAnimModel(
					//	ticks,
					//	animationComponent.startTime,
					//	animationComponent.shouldLoop,
					//	*renderModel,
					//	r2::sarr::At(*renderModel->optrAnimations, animationComponent.currentAnimationIndex),
					//	*animationComponent.shaderBones,
					//	debugBones, 0);
				}
			}

			coordinator->AddComponent<r2::ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(entity, instancedAnimationComponent);

		}
		else if (!renderComponent.isAnimated && hasInstancedSkeletalAnimationComponent)
		{
			//remove
			coordinator->RemoveComponent<r2::ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(entity);

			if (coordinator->HasComponent<r2::ecs::InstanceComponentT<ecs::DebugBoneComponent>>(entity))
			{
				coordinator->RemoveComponent<r2::ecs::InstanceComponentT<ecs::DebugBoneComponent>>(entity);
			}
		}
		else//!renderComponent.isAnimated && !hasInstancedSkeletalAnimationComponent
		{
			//nothing to do
		}

	}

	InspectorPanelRenderComponentDataSource::InspectorPanelRenderComponentDataSource()
		:InspectorPanelComponentDataSource("Render Component", 0, 0)
		, mOpenMaterialsWindow(false)
		, mnoptrEditor(nullptr)
		, mMaterialToEdit{}
	{

	}

	InspectorPanelRenderComponentDataSource::InspectorPanelRenderComponentDataSource(r2::Editor* noptrEditor, r2::ecs::ECSCoordinator* coordinator)
		: InspectorPanelComponentDataSource("Render Component", coordinator->GetComponentType<ecs::RenderComponent>(), coordinator->GetComponentTypeHash<ecs::RenderComponent>())
		, mnoptrEditor(noptrEditor)
		, mOpenMaterialsWindow(false)
		, mMaterialToEdit{}
	{

	}

	void InspectorPanelRenderComponentDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::RenderComponent* renderComponentPtr = coordinator->GetComponentPtr<ecs::RenderComponent>(theEntity);
		ecs::RenderComponent& renderComponent = *renderComponentPtr;

		r2::ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();
		r2::asset::AssetLib& assetLib = MENG.GetAssetLib();

		const r2::Level& currentEditorLevel = mnoptrEditor->GetEditorLevelConst();


		{
			GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();
			
			const r2::draw::Model* model = nullptr;

			std::string currentModelAssetName = renderComponent.assetModelName.assetNameString;


			r2::SArray<r2::asset::AssetName>* modelLevelAssetNames = currentEditorLevel.GetModelAssets();
			R2_CHECK(modelLevelAssetNames != nullptr, "Should never happen");

			std::vector<r2::asset::AssetName> modelAssetNames;

			const auto numLevelModelAssetNames = r2::sarr::Size(*modelLevelAssetNames);

			for (u32 i = 0; i < numLevelModelAssetNames; ++i)
			{
				modelAssetNames.push_back(r2::sarr::At(*modelLevelAssetNames, i));
			}

			//@TODO(Serge): remove the primitive model lines once we have refactored the r2::asset::MODEL stuff
			{
				std::vector<r2::asset::AssetName> modelFiles = r2::asset::lib::GetAllAssetNamesForType(assetLib, r2::asset::MODEL);

				modelAssetNames.insert(modelAssetNames.end(), modelFiles.begin(), modelFiles.end());
			}


			ImGui::Text("Render model:");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##label rendermodel", currentModelAssetName.c_str()))
			{
				for (u32 i = 0; i < modelAssetNames.size(); ++i)
				{
					const r2::asset::AssetName& modelAssetName = modelAssetNames[i];

					if (ImGui::Selectable(modelAssetName.assetNameString.c_str(), modelAssetName.assetNameString == currentModelAssetName))
					{
						if (modelAssetName.assetNameString != currentModelAssetName)
						{
							std::filesystem::path assetFilePath = modelAssetName.assetNameString;
							//auto assetHandle = assetFile->GetAssetHandle(0);
							const r2::draw::Model* renderModel = nullptr;

							if (assetFilePath.extension().string() == ".modl")
							{
								renderModel = r2::draw::renderer::GetDefaultModel({ modelAssetName.hashID });
							}
							else
							{
								renderModel = gameAssetManager.GetAssetData<r2::draw::Model>(modelAssetName.hashID);
							}

							R2_CHECK(renderModel != nullptr, "Should never happen");

							renderComponent.assetModelName = renderModel->assetName;

							renderComponent.gpuModelRefHandle = r2::draw::renderer::GetModelRefHandleForModelAssetName(renderComponent.assetModelName);

							//if (!r2::draw::renderer::IsModelRefHandleValid(renderComponent.gpuModelRefHandle))
							//{
							//	renderComponent.gpuModelRefHandle = r2::draw::renderer::UploadModel(renderModel);
							//}

							renderComponent.isAnimated = renderModel->animSkeleton.mBindPose != nullptr;


							if (renderComponent.isAnimated)
							{
								renderComponent.drawParameters.layer = draw::DL_CHARACTER;
							}
							else
							{
								renderComponent.drawParameters.layer = draw::DL_WORLD;
							}

							//bool useSameBoneTransformsForAllInstances = renderComponent.drawParameters.flags.IsSet(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
							renderComponent.drawParameters.flags.Set(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
							//now we need to either add or remove the skeletal animation component based on if this is a static or dynamic model
							UpdateSkeletalAnimationComponentIfNecessary(renderModel, true, coordinator, theEntity, 0, renderComponent);
						}
					}
				}

				ImGui::EndCombo();
			}

			

			if (ImGui::CollapsingHeader("Materials"))
			{
				ImGui::Indent();
				
				const r2::draw::vb::GPUModelRef* modelRef = r2::draw::renderer::GetGPUModelRef(renderComponent.gpuModelRefHandle);

				R2_CHECK(modelRef->numMaterials == r2::sarr::Size(*modelRef->materialNames), "Should be the same");

				//we need to figure out if we're using overrides or the default (GPUModelRef) materials

				bool overridesDisabled = false;
				if (renderComponent.optrMaterialOverrideNames)
				{
					overridesDisabled = true;
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}

				if (ImGui::Button("Override Materials"))
				{
					renderComponent.optrMaterialOverrideNames = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::mat::MaterialName, modelRef->numMaterials);

					for (u32 i = 0; i < modelRef->numMaterials; ++i)
					{
						r2::sarr::Push(*renderComponent.optrMaterialOverrideNames, r2::sarr::At(*modelRef->materialNames, i));
					}
				}

				ImGui::SameLine();

				if (overridesDisabled)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}
				
				
				bool isClearOverridesDisabled = !renderComponent.optrMaterialOverrideNames;
				if (isClearOverridesDisabled)
				{
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}

				if (ImGui::Button("Clear overrides"))
				{
					ECS_WORLD_FREE(ecsWorld, renderComponent.optrMaterialOverrideNames);
					renderComponent.optrMaterialOverrideNames = nullptr;
					isClearOverridesDisabled = false;
				}

				if (isClearOverridesDisabled)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}

				u32 numMaterials = modelRef->numMaterials;
				r2::SArray<r2::mat::MaterialName>* materialsToUse = modelRef->materialNames;

				if (renderComponent.optrMaterialOverrideNames != nullptr)
				{
					materialsToUse = renderComponent.optrMaterialOverrideNames;
					numMaterials = r2::sarr::Size(*renderComponent.optrMaterialOverrideNames);
				}

				if (!renderComponent.optrMaterialOverrideNames)
				{
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}


				flat::eMeshPass meshPass = flat::eMeshPass_FORWARD;
				if (renderComponent.drawParameters.layer == draw::DL_TRANSPARENT)
				{
					meshPass == flat::eMeshPass_TRANSPARENT;
				}

				const flat::Material* firstMaterial = GetMaterialForMaterialName(r2::sarr::At(*modelRef->materialNames, 0));
				std::vector<r2::mat::MaterialParam> suitableMaterials = r2::mat::GetAllMaterialsThatMatchVertexLayout(meshPass, firstMaterial->shaderEffectPasses()->shaderEffectPasses()->Get(meshPass)->staticVertexLayout(),
					firstMaterial->shaderEffectPasses()->shaderEffectPasses()->Get(meshPass)->dynamicVertexLayout());

				for (u32 j = 0; j < numMaterials; ++j)
				{
					const auto& materialName = r2::sarr::At(*materialsToUse, j);

					const flat::Material* flatMaterial = r2::mat::GetMaterialForMaterialName(materialName);

					std::string materialNameString = flatMaterial->assetName()->stringName()->str();
					ImGui::PushID(j);

					ImGui::Text("Slot %u:", j); 
					ImGui::SameLine();

					ImGui::PushItemWidth(230.0f);

					if (ImGui::BeginCombo("##label materialslot", materialNameString.c_str()))
					{
						for (u32 k = 0; k < suitableMaterials.size(); ++k)
						{
							r2::mat::MaterialParam materialParam = suitableMaterials[k];
							const flat::Material* suitableMaterial = materialParam.flatMaterial;
							if (ImGui::Selectable(suitableMaterial->assetName()->stringName()->str().c_str(), flatMaterial->assetName()->stringName()->str() == suitableMaterial->assetName()->stringName()->str()))
							{
								r2::sarr::At(*materialsToUse, j) = materialParam.materialName;
							}
						}

						ImGui::EndCombo();
					}

					ImGui::PopItemWidth();

					ImGui::SameLine();
					if (ImGui::Button(ICON_IGFD_EDIT))
					{
						mOpenMaterialsWindow = true;
						mMaterialToEdit = materialName;
					}

					ImGui::PopID();
				}

				if (!renderComponent.optrMaterialOverrideNames)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}

				ImGui::Unindent();
			}

			if (mOpenMaterialsWindow)
			{
				EditExistingMaterial(mMaterialToEdit, mOpenMaterialsWindow);
			}

			std::string currentPrimitiveType = GetPrimitiveTypeString(static_cast<r2::draw::PrimitiveType>(renderComponent.primitiveType));
			ImGui::Text("Primitive Type: ");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##label primitivetype", currentPrimitiveType.c_str()))
			{
				for (u32 i = 0; i < static_cast<u32>(r2::draw::PrimitiveType::NUM_PRIMITIVE_TYPES); ++i)
				{
					std::string nextPrimitiveString = GetPrimitiveTypeString(static_cast<r2::draw::PrimitiveType>(i));
					if (ImGui::Selectable(nextPrimitiveString.c_str(), i == renderComponent.primitiveType))
					{
						renderComponent.primitiveType = i;
					}
				}

				ImGui::EndCombo();
			}

			std::string currentDrawLayer = GetDrawLayerString(renderComponent.drawParameters.layer);
			ImGui::Text("Draw Layer: ");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##label drawlayer", currentDrawLayer.c_str()))
			{
				for (u32 i = r2::draw::DL_WORLD; i <= r2::draw::DL_DEBUG; ++i)
				{
					if ((renderComponent.isAnimated && (
						i == r2::draw::DL_CHARACTER ||
						i == r2::draw::DL_TRANSPARENT ||
						i == r2::draw::DL_EFFECT ||
						i == r2::draw::DL_DEBUG)) ||
						(!renderComponent.isAnimated && (
							i == r2::draw::DL_WORLD ||
							i == r2::draw::DL_TRANSPARENT ||
							i == r2::draw::DL_EFFECT ||
							i == r2::draw::DL_HUD ||
							i == r2::draw::DL_SKYBOX ||
							i == r2::draw::DL_SCREEN ||
							i == r2::draw::DL_DEBUG
							)))
					{
						std::string drawLayer = GetDrawLayerString(static_cast<r2::draw::DrawLayer>(i));
						if (ImGui::Selectable(drawLayer.c_str(), static_cast<r2::draw::DrawLayer>(i) == renderComponent.drawParameters.layer))
						{
							renderComponent.drawParameters.layer = static_cast<r2::draw::DrawLayer>(i);

							if (renderComponent.drawParameters.layer == draw::DL_TRANSPARENT && renderComponent.drawParameters.blendState.blendingEnabled == false)
							{
								renderComponent.drawParameters.blendState.blendingEnabled = true;
							}

							if (renderComponent.drawParameters.blendState.blendingEnabled && renderComponent.drawParameters.layer != draw::DL_TRANSPARENT)
							{
								renderComponent.drawParameters.blendState.blendingEnabled = false;
							}
						}
					}
				}

				ImGui::EndCombo();
			}

			ImGui::Text("Flags: ");
			bool depthTest = renderComponent.drawParameters.flags.IsSet(r2::draw::eDrawFlags::DEPTH_TEST);
			if (ImGui::Checkbox("Depth Enabled", &depthTest))
			{
				if (depthTest)
				{
					renderComponent.drawParameters.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);
				}
				else
				{
					renderComponent.drawParameters.flags.Remove(r2::draw::eDrawFlags::DEPTH_TEST);
				}
			}

			bool useSameBoneTransformsForAllInstances = renderComponent.drawParameters.flags.IsSet(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
			if (ImGui::Checkbox("Use Same Bone Transforms for all instances", &useSameBoneTransformsForAllInstances))
			{
				if (useSameBoneTransformsForAllInstances)
				{
					renderComponent.drawParameters.flags.Set(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
				}
				else
				{
					renderComponent.drawParameters.flags.Remove(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
				}

				UpdateSkeletalAnimationComponentIfNecessary(model, useSameBoneTransformsForAllInstances, coordinator, theEntity, 0, renderComponent);
			}

			if (ImGui::TreeNode("Stencil State"))
			{
				bool stencilEnabled = renderComponent.drawParameters.stencilState.stencilEnabled;
				if (ImGui::Checkbox("Stencil Enabled", &stencilEnabled))
				{
					renderComponent.drawParameters.stencilState.stencilEnabled = stencilEnabled;
				}

				bool stencilWriteEnabled = renderComponent.drawParameters.stencilState.stencilWriteEnabled;
				if (ImGui::Checkbox("Stencil Write Enabled", &stencilWriteEnabled))
				{
					renderComponent.drawParameters.stencilState.stencilWriteEnabled = stencilWriteEnabled;
				}

				if (ImGui::TreeNode("Stencil Op"))
				{
					StencilOpItem("Stencil Fail:", GetStencilOpValue(renderComponent.drawParameters.stencilState.op.stencilFail), "##label stencilfail", renderComponent.drawParameters.stencilState.op.stencilFail);
					StencilOpItem("Depth Fail:", GetStencilOpValue(renderComponent.drawParameters.stencilState.op.depthFail), "##label depthfail", renderComponent.drawParameters.stencilState.op.depthFail);
					StencilOpItem("Depth and Stencil Pass:", GetStencilOpValue(renderComponent.drawParameters.stencilState.op.depthAndStencilPass), "##label dppass", renderComponent.drawParameters.stencilState.op.depthAndStencilPass);
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Stencil Function"))
				{
					ImGui::Text("Function: ");
					ImGui::SameLine();
					if (ImGui::BeginCombo("##label stencilfunc", GetStencilFunctionName(renderComponent.drawParameters.stencilState.func.func).c_str()))
					{
						if (ImGui::Selectable("NEVER", renderComponent.drawParameters.stencilState.func.func == r2::draw::NEVER))
						{
							renderComponent.drawParameters.stencilState.func.func = r2::draw::NEVER;
						}
						if (ImGui::Selectable("LESS", renderComponent.drawParameters.stencilState.func.func == r2::draw::LESS))
						{
							renderComponent.drawParameters.stencilState.func.func = r2::draw::LESS;
						}
						if (ImGui::Selectable("LEQUAL", renderComponent.drawParameters.stencilState.func.func == r2::draw::LEQUAL))
						{
							renderComponent.drawParameters.stencilState.func.func = r2::draw::LEQUAL;
						}
						if (ImGui::Selectable("GREATER", renderComponent.drawParameters.stencilState.func.func == r2::draw::GREATER))
						{
							renderComponent.drawParameters.stencilState.func.func = r2::draw::GREATER;
						}
						if (ImGui::Selectable("GEQUAL", renderComponent.drawParameters.stencilState.func.func == r2::draw::GEQUAL))
						{
							renderComponent.drawParameters.stencilState.func.func = r2::draw::GEQUAL;
						}
						if (ImGui::Selectable("EQUAL", renderComponent.drawParameters.stencilState.func.func == r2::draw::EQUAL))
						{
							renderComponent.drawParameters.stencilState.func.func = r2::draw::EQUAL;
						}
						if (ImGui::Selectable("NOT EQUAL", renderComponent.drawParameters.stencilState.func.func == r2::draw::NOTEQUAL))
						{
							renderComponent.drawParameters.stencilState.func.func = r2::draw::NOTEQUAL;
						}
						if (ImGui::Selectable("ALWAYS", renderComponent.drawParameters.stencilState.func.func == r2::draw::ALWAYS))
						{
							renderComponent.drawParameters.stencilState.func.func = r2::draw::ALWAYS;
						}

						ImGui::EndCombo();
					}

					std::string stencilRefPreview = "";

					if (renderComponent.drawParameters.stencilState.func.ref == 0)
					{
						stencilRefPreview = "ZERO";
					}
					else if (renderComponent.drawParameters.stencilState.func.ref == 1)
					{
						stencilRefPreview = "ONE";
					}
					else
					{
						R2_CHECK(false, "Unsupported Stencil Function Reference Value");
					}

					ImGui::Text("Ref: ");
					ImGui::SameLine();
					if (ImGui::BeginCombo("##label stencilref", stencilRefPreview.c_str()))
					{
						if (ImGui::Selectable("ZERO", renderComponent.drawParameters.stencilState.func.ref == 0))
						{
							renderComponent.drawParameters.stencilState.func.ref = 0;
						}
						if (ImGui::Selectable("ONE", renderComponent.drawParameters.stencilState.func.ref == 1))
						{
							renderComponent.drawParameters.stencilState.func.ref = 1;
						}
						ImGui::EndCombo();
					}

					std::string stencilMaskPreview = "";

					if (renderComponent.drawParameters.stencilState.func.mask == 0x00)
					{
						stencilMaskPreview = "0x00";
					}
					else if (renderComponent.drawParameters.stencilState.func.mask == 0xFF)
					{
						stencilMaskPreview = "0xFF";
					}
					else
					{
						R2_CHECK(false, "Unsupported Stencil Function Mask Value");
					}

					ImGui::Text("Mask: ");
					ImGui::SameLine();

					if (ImGui::BeginCombo("##label stencilmask", stencilMaskPreview.c_str()))
					{
						if (ImGui::Selectable("0x00", renderComponent.drawParameters.stencilState.func.mask == 0x00))
						{
							renderComponent.drawParameters.stencilState.func.mask = 0x00;
						}
						if (ImGui::Selectable("0xFF", renderComponent.drawParameters.stencilState.func.mask == 0xFF))
						{
							renderComponent.drawParameters.stencilState.func.mask = 0xFF;
						}
						ImGui::EndCombo();
					}

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Blend State"))
			{
				bool blendingEnabled = renderComponent.drawParameters.blendState.blendingEnabled;
				if (ImGui::Checkbox("Blending Enabled", &blendingEnabled))
				{
					renderComponent.drawParameters.blendState.blendingEnabled = blendingEnabled;
				}

				ImGui::Text("Equation: ");
				ImGui::SameLine();
				if (ImGui::BeginCombo("##label blendequation", GetBlendEquationString(renderComponent.drawParameters.blendState.blendEquation).c_str()))
				{
					if (ImGui::Selectable("ADD", renderComponent.drawParameters.blendState.blendEquation == r2::draw::BLEND_EQUATION_ADD))
					{
						renderComponent.drawParameters.blendState.blendEquation = r2::draw::BLEND_EQUATION_ADD;
					}

					if (ImGui::Selectable("SUBTRACT", renderComponent.drawParameters.blendState.blendEquation == r2::draw::BLEND_EQUATION_SUBTRACT))
					{
						renderComponent.drawParameters.blendState.blendEquation = r2::draw::BLEND_EQUATION_SUBTRACT;
					}

					ImGui::EndCombo();
				}

				ImGui::TreePop();
			}
		}
	}

	bool InspectorPanelRenderComponentDataSource::InstancesEnabled() const
	{
		return false;
	}

	u32 InspectorPanelRenderComponentDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		return 0;
	}

	void* InspectorPanelRenderComponentDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return &coordinator->GetComponent<ecs::RenderComponent>(theEntity);
	}

	void* InspectorPanelRenderComponentDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return nullptr;
	}

	void InspectorPanelRenderComponentDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
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
	}

	void InspectorPanelRenderComponentDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{

	}

	void InspectorPanelRenderComponentDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::RenderComponent newRenderComponent;
		newRenderComponent.assetModelName = r2::draw::renderer::GetDefaultModel(draw::CUBE)->assetName;
		newRenderComponent.gpuModelRefHandle = r2::draw::renderer::GetModelRefHandleForModelAssetName(newRenderComponent.assetModelName);
		newRenderComponent.isAnimated = false;
		newRenderComponent.primitiveType = static_cast<u32>( r2::draw::PrimitiveType::TRIANGLES );
		newRenderComponent.drawParameters.layer = draw::DL_WORLD;
		newRenderComponent.drawParameters.flags.Clear();
		newRenderComponent.drawParameters.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);
		newRenderComponent.optrMaterialOverrideNames = nullptr;

		r2::draw::renderer::SetDefaultDrawParameters(newRenderComponent.drawParameters);

		coordinator->AddComponent<ecs::RenderComponent>(theEntity, newRenderComponent);
	}

	void InspectorPanelRenderComponentDataSource::AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances) 
	{

	}

	

}

#endif