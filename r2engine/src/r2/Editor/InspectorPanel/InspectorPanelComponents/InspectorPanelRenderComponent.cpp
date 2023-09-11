#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelRenderComponent.h"

#include "r2/Core/Engine.h"
#include "r2/Core/File/PathUtils.h"

#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"

#include "r2/Game/GameAssetManager/GameAssetManager.h"

#include "r2/Render/Model/Model.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/Renderer.h"

#include "r2/Core/Engine.h"
#include "r2/Game/ECSWorld/ECSWorld.h"

//this is dumb but...
#include "r2/Render/Animation/AnimationPlayer.h"

#include "imgui.h"

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

		if (renderComponent.isAnimated && !hasSkeletalAnimationComponent)
		{
			//add the skeletal animation component
			r2::ecs::SkeletalAnimationComponent newSkeletalAnimationComponent;
			newSkeletalAnimationComponent.animModel = renderModel;
			newSkeletalAnimationComponent.animModelAssetName = renderModel->assetName;

			R2_CHECK(renderModel->optrAnimations && r2::sarr::Size(*renderModel->optrAnimations) > 0, "we need to have animations");

			newSkeletalAnimationComponent.startingAnimationIndex = 0;
			newSkeletalAnimationComponent.shouldLoop = true;
			newSkeletalAnimationComponent.shouldUseSameTransformsForAllInstances = useSameBoneTransformsForInstances;
			newSkeletalAnimationComponent.currentAnimationIndex = newSkeletalAnimationComponent.startingAnimationIndex;
			newSkeletalAnimationComponent.startTime = 0;
			newSkeletalAnimationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, r2::sarr::Size(*renderModel->optrBoneInfo));

			coordinator->AddComponent<r2::ecs::SkeletalAnimationComponent>(entity, newSkeletalAnimationComponent);

			updateShaderBones = true;

			
		}
		else if (renderComponent.isAnimated && hasSkeletalAnimationComponent)
		{
			//in this case we need to make sure that the old skeletal animation component is setup correctly
			r2::ecs::SkeletalAnimationComponent& animationComponent = coordinator->GetComponent<r2::ecs::SkeletalAnimationComponent>(entity);
			animationComponent.animModel = renderModel;
			animationComponent.animModelAssetName = renderModel->assetName;

			R2_CHECK(renderModel->optrAnimations && r2::sarr::Size(*renderModel->optrAnimations) > 0, "we need to have animations");

			//Put the starting animation index back to 0 since we don't know how many animations we have vs what we used to have
			animationComponent.startingAnimationIndex = 0;
			animationComponent.shouldLoop = true;
			animationComponent.shouldUseSameTransformsForAllInstances = useSameBoneTransformsForInstances;
			animationComponent.currentAnimationIndex = animationComponent.startingAnimationIndex;
			animationComponent.startTime = 0;

			if (r2::sarr::Capacity(*animationComponent.shaderBones) != r2::sarr::Capacity(*renderModel->optrBoneInfo))
			{
				ECS_WORLD_FREE(ecsWorld, animationComponent.shaderBones);

				animationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, r2::sarr::Capacity(*renderModel->optrBoneInfo));
				
			}
			r2::sarr::Clear(*animationComponent.shaderBones);

			updateShaderBones = true;
			
			
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

			auto ticks = CENG.GetTicks();
			r2::draw::PlayAnimationForAnimModel(
				ticks,
				animationComponent.startTime,
				animationComponent.shouldLoop,
				*renderModel,
				r2::sarr::At(*renderModel->optrAnimations, animationComponent.currentAnimationIndex),
				*animationComponent.shaderBones,
				debugBones, 0);
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

				R2_CHECK(renderModel->optrAnimations && r2::sarr::Size(*renderModel->optrAnimations) > 0, "we need to have animations");

				//Put the starting animation index back to 0 since we don't know how many animations we have vs what we used to have
				nextAnimationComponent.startingAnimationIndex = 0;
				nextAnimationComponent.shouldLoop = true;
				nextAnimationComponent.shouldUseSameTransformsForAllInstances = useSameBoneTransformsForInstances;
				nextAnimationComponent.currentAnimationIndex = nextAnimationComponent.startingAnimationIndex;
				nextAnimationComponent.startTime = 0;

				if (r2::sarr::Capacity(*nextAnimationComponent.shaderBones) != r2::sarr::Capacity(*renderModel->optrBoneInfo))
				{
					ECS_WORLD_FREE(ecsWorld, nextAnimationComponent.shaderBones);

					nextAnimationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, r2::sarr::Capacity(*renderModel->optrBoneInfo));
				}
				r2::sarr::Clear(*nextAnimationComponent.shaderBones);

				if (updateShaderBones && !useSameBoneTransformsForInstances)
				{
					r2::ecs::SkeletalAnimationComponent& animationComponent = r2::sarr::At(*instancedAnimationComponent.instances, i);
					r2::SArray<r2::draw::DebugBone>* debugBones = nullptr;

					auto* instancedDebugBoneComponent = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT<ecs::DebugBoneComponent>>(entity);

					if (instancedDebugBoneComponent)
					{
						r2::ecs::DebugBoneComponent& debugBoneComponent = r2::sarr::At(*instancedDebugBoneComponent->instances, i);;
						debugBones = debugBoneComponent.debugBones;
					}

					auto ticks = CENG.GetTicks();
					r2::draw::PlayAnimationForAnimModel(
						ticks,
						animationComponent.startTime,
						animationComponent.shouldLoop,
						*renderModel,
						r2::sarr::At(*renderModel->optrAnimations, animationComponent.currentAnimationIndex),
						*animationComponent.shaderBones,
						debugBones, 0);
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

				R2_CHECK(renderModel->optrAnimations&& r2::sarr::Size(*renderModel->optrAnimations) > 0, "we need to have animations");

				newSkeletalAnimationComponent.startingAnimationIndex = 0;
				newSkeletalAnimationComponent.shouldLoop = true;
				newSkeletalAnimationComponent.shouldUseSameTransformsForAllInstances = useSameBoneTransformsForInstances;
				newSkeletalAnimationComponent.currentAnimationIndex = newSkeletalAnimationComponent.startingAnimationIndex;
				newSkeletalAnimationComponent.startTime = 0;
				newSkeletalAnimationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, r2::sarr::Size(*renderModel->optrBoneInfo));

				r2::sarr::Push(*instancedAnimationComponent.instances, newSkeletalAnimationComponent);

				if (updateShaderBones && !useSameBoneTransformsForInstances)
				{
					r2::ecs::SkeletalAnimationComponent& animationComponent = newSkeletalAnimationComponent;
					r2::SArray<r2::draw::DebugBone>* debugBones = nullptr;

					auto* instancedDebugBoneComponent = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT<ecs::DebugBoneComponent>>(entity);

					if (instancedDebugBoneComponent)
					{
						r2::ecs::DebugBoneComponent& debugBoneComponent = r2::sarr::At(*instancedDebugBoneComponent->instances, i);;
						debugBones = debugBoneComponent.debugBones;
					}

					auto ticks = CENG.GetTicks();
					r2::draw::PlayAnimationForAnimModel(
						ticks,
						animationComponent.startTime,
						animationComponent.shouldLoop,
						*renderModel,
						r2::sarr::At(*renderModel->optrAnimations, animationComponent.currentAnimationIndex),
						*animationComponent.shaderBones,
						debugBones, 0);
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

	void RenderComponentInstance(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity entity, int id, r2::ecs::RenderComponent& renderComponent)
	{

		r2::ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

//		const r2::ecs::EditorComponent* editorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(entity);

		//std::string nodeName = std::string("Entity - ") + std::to_string(entity) + std::string(" - Debug Bone Instance - ") + std::to_string(id);
		//if (editorComponent)
		//{
		//	nodeName = editorComponent->editorName + std::string(" - Debug Bone Instance - ") + std::to_string(id);
		//}

		//if (ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth, "%s", nodeName.c_str()))
		{
			GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

			const r2::draw::Model* model = nullptr;
			const r2::asset::AssetFile* currentModelAssetfile = nullptr;

			if (gameAssetManager.HasAsset({ renderComponent.assetModelName , r2::asset::RMODEL}))
			{
				model = gameAssetManager.GetAssetDataConst<r2::draw::Model>(renderComponent.assetModelName);
				currentModelAssetfile = gameAssetManager.GetAssetFile(r2::asset::Asset(model->assetName, r2::asset::RMODEL));
			}
			else
			{
				model = r2::draw::renderer::GetDefaultModel(renderComponent.assetModelName);
				r2::asset::FileList primitiveModels = r2::draw::renderer::GetModelFiles();
				const auto numPrimitiveModels = r2::sarr::Size(*primitiveModels);
				for (u32 i = 0; i < numPrimitiveModels; ++i)
				{
					r2::asset::AssetFile* assetFile = r2::sarr::At(*primitiveModels, i);

					if (assetFile->GetAssetHandle(0) == renderComponent.assetModelName)
					{
						currentModelAssetfile = assetFile;
						break;
					}
				}
			}


			std::string modelFileName = GetModelNameForAssetFile(currentModelAssetfile);

			//@TODO(Serge): make this into a ImGui::Combo
			//ImGui::Text("Model Name: %s", modelFileName.c_str());

			std::vector<r2::asset::AssetFile*> rModelFiles = gameAssetManager.GetAllAssetFilesForAssetType(r2::asset::RMODEL);

			//@TODO(Serge): remove the primitive model lines once we have refactored the r2::asset::MODEL stuff
			{
				r2::asset::FileList primitiveModels = r2::draw::renderer::GetModelFiles();
				const auto numPrimitiveModels = r2::sarr::Size(*primitiveModels);
				for (u32 i = 0; i < numPrimitiveModels; ++i)
				{
					r2::asset::AssetFile* assetFile = r2::sarr::At(*primitiveModels, i);
					std::filesystem::path assetFilePath = assetFile->FilePath();

					if (assetFilePath.extension().string() == ".modl" &&
						(assetFilePath.stem().string() != "FullscreenTriangle" && 
						 assetFilePath.stem().string() != "Skybox"))
					{
						rModelFiles.push_back(assetFile);
					}
				}
			}
			

			ImGui::Text("Render model:");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##label rendermodel", modelFileName.c_str()))
			{
				for (u32 i = 0; i < rModelFiles.size(); ++i)
				{
					r2::asset::AssetFile* assetFile = rModelFiles[i];
					std::string nextModelFileName = GetModelNameForAssetFile(assetFile);

					if (ImGui::Selectable(nextModelFileName.c_str(), nextModelFileName == modelFileName))
					{
						if (nextModelFileName != modelFileName)
						{
							//@TODO(Serge): implement
							std::filesystem::path assetFilePath = assetFile->FilePath();
							auto assetHandle = assetFile->GetAssetHandle(0);
							const r2::draw::Model* renderModel = nullptr;

							if (assetFilePath.extension().string() == ".modl")
							{
								renderModel = r2::draw::renderer::GetDefaultModel(assetHandle);
							}
							else
							{
								renderModel = gameAssetManager.GetAssetData<r2::draw::Model>(assetHandle);
							}

							R2_CHECK(renderModel != nullptr, "Should never happen");

							renderComponent.assetModelName = renderModel->assetName;

							renderComponent.gpuModelRefHandle = r2::draw::renderer::GetModelRefHandleForModelAssetName(renderComponent.assetModelName);

							if (!r2::draw::renderer::IsModelRefHandleValid(renderComponent.gpuModelRefHandle))
							{
								renderComponent.gpuModelRefHandle = r2::draw::renderer::UploadModel(renderModel);
							}
							
							renderComponent.isAnimated = renderModel->optrBoneInfo != nullptr;

							if (renderComponent.isAnimated)
							{
								renderComponent.drawParameters.layer = draw::DL_CHARACTER;
							}
							else
							{
								renderComponent.drawParameters.layer = draw::DL_WORLD;
							}

							//@TODO(Serge): figure out how the override materials will work here
							//				I think we'll have to delete the array and make a new one based on 
							//				the new renderModel's materialNames

							bool useSameBoneTransformsForAllInstances = renderComponent.drawParameters.flags.IsSet(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
							//now we need to either add or remove the skeletal animation component based on if this is a static or dynamic model
							UpdateSkeletalAnimationComponentIfNecessary(renderModel, true, coordinator, entity, id, renderComponent);
						}
					}
				}

				ImGui::EndCombo();
			}


			//@TODO(Serge): make this into a ImGui::Combo
		//	ImGui::Text("Model Type: %s", GetIsAnimatedString(renderComponent.isAnimated).c_str());

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

				UpdateSkeletalAnimationComponentIfNecessary(model, useSameBoneTransformsForAllInstances, coordinator, entity, id, renderComponent);
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

			/*
				struct RenderComponent
				{
					u64 assetModelHash;
					u32 primitiveType;
					b32 isAnimated;
					r2::draw::DrawParameters drawParameters;
					r2::draw::vb::GPUModelRefHandle gpuModelRefHandle;
					r2::SArray<r2::mat::MaterialName>* optrMaterialOverrideNames;
				};
			*/

			//@TODO(Serge): implement material overrides - no clue how the memory works atm

			

		//	ImGui::TreePop();
		}
	}

	void InspectorPanelRenderComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	{
		r2::ecs::RenderComponent& renderComponent = coordinator->GetComponent<r2::ecs::RenderComponent>(theEntity);
		RenderComponentInstance(coordinator, theEntity, 0, renderComponent);
	}
}

#endif