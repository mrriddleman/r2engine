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

#include "r2/Render/Model/Materials/MaterialHelpers.h"
#include "r2/Render/Model/Materials/Material_generated.h"

//this is dumb but...
#include "r2/Render/Animation/AnimationPlayer.h"

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

	InspectorPanelRenderComponentDataSource::InspectorPanelRenderComponentDataSource()
		:InspectorPanelComponentDataSource("Render Component", 0, 0)
		, mOpenMaterialsWindow(false)
		, mMaterialToEdit{}
	{

	}

	InspectorPanelRenderComponentDataSource::InspectorPanelRenderComponentDataSource(r2::ecs::ECSCoordinator* coordinator)
		: InspectorPanelComponentDataSource("Render Component", coordinator->GetComponentType<ecs::RenderComponent>(), coordinator->GetComponentTypeHash<ecs::RenderComponent>())
		, mOpenMaterialsWindow(false)
		, mMaterialToEdit{}
	{

	}

	void InspectorPanelRenderComponentDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::RenderComponent* renderComponentPtr = coordinator->GetComponentPtr<ecs::RenderComponent>(theEntity);
		ecs::RenderComponent& renderComponent = *renderComponentPtr;

		r2::ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

		{
			GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

			const r2::draw::Model* model = nullptr;
			const r2::asset::AssetFile* currentModelAssetfile = nullptr;

			if (gameAssetManager.HasAsset({ renderComponent.assetModelName , r2::asset::RMODEL }))
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

							bool useSameBoneTransformsForAllInstances = renderComponent.drawParameters.flags.IsSet(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
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

					std::string materialNameString = flatMaterial->stringName()->str();
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
							if (ImGui::Selectable(suitableMaterial->stringName()->str().c_str(), flatMaterial->stringName()->str() == suitableMaterial->stringName()->str()))
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
				MaterialEditor(mMaterialToEdit, mOpenMaterialsWindow);
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

	void InspectorPanelRenderComponentDataSource::MaterialEditor(const r2::mat::MaterialName& materialName, bool& windowOpen)
	{
#ifdef R2_ASSET_PIPELINE

		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		r2::asset::AssetLib& assetLib = MENG.GetAssetLib();

		r2::asset::MaterialManifestAssetFile* materialManifestFile = (r2::asset::MaterialManifestAssetFile*)r2::asset::lib::GetManifest(assetLib, materialName.packName);

		R2_CHECK(materialManifestFile != nullptr, "Should never be the case");

		std::vector<r2::mat::Material>& materials = materialManifestFile->GetMaterials();

		r2::mat::Material* foundMaterial = nullptr;
		//find the material we care about
		for (u32 i = 0; i < materials.size(); ++i)
		{
			if (materials[i].materialName == materialName)
			{
				foundMaterial = &materials[i];
				break;
			}
		}

		R2_CHECK(foundMaterial != nullptr, "Should always be the case");

		static std::string s_meshPassStrings[] = { "FORWARD", "TRANSPARENT" };
		static float CONTENT_WIDTH = 600;
		static float CONTENT_HEIGHT = 1000;

		static const char* const* s_shaderPropertyTypeStrings = flat::EnumNamesShaderPropertyType();
		static const char* const* s_texturePackingTypeStrings = flat::EnumNamesShaderPropertyPackingType();
		static const char* const* s_minTextureFilterStrings = flat::EnumNamesMinTextureFilter();
		static const char* const* s_magTextureFilterStrings = flat::EnumNamesMagTextureFilter();
		static const char* const* s_textureWrapModeStrings = flat::EnumNamesTextureWrapMode();

		static const std::vector<flat::ShaderPropertyType> s_floatPropertyTypes = {
			flat::ShaderPropertyType_ROUGHNESS,
			flat::ShaderPropertyType_METALLIC,
			flat::ShaderPropertyType_REFLECTANCE,
			flat::ShaderPropertyType_AMBIENT_OCCLUSION,
			flat::ShaderPropertyType_CLEAR_COAT,
			flat::ShaderPropertyType_CLEAR_COAT_ROUGHNESS,
			flat::ShaderPropertyType_HEIGHT_SCALE,
			flat::ShaderPropertyType_ANISOTROPY
		};

		static const std::vector<flat::ShaderPropertyType> s_colorPropertyTypes = {
			flat::ShaderPropertyType_ALBEDO,
			flat::ShaderPropertyType_EMISSION,
			flat::ShaderPropertyType_DETAIL
		};

		static const std::vector<flat::ShaderPropertyType> s_texturePropertyTypes = {
			flat::ShaderPropertyType_ALBEDO,
			flat::ShaderPropertyType_NORMAL,
			flat::ShaderPropertyType_EMISSION,
			flat::ShaderPropertyType_METALLIC,
			flat::ShaderPropertyType_ROUGHNESS,
			flat::ShaderPropertyType_AMBIENT_OCCLUSION,
			flat::ShaderPropertyType_HEIGHT,
			flat::ShaderPropertyType_ANISOTROPY,
			flat::ShaderPropertyType_DETAIL,

			flat::ShaderPropertyType_CLEAR_COAT,
			flat::ShaderPropertyType_CLEAR_COAT_ROUGHNESS,
			flat::ShaderPropertyType_CLEAR_COAT_NORMAL

			//@TODO(Serge): we have more property types but I don't think they are supported atm
			//eg. SHEEN_COLOR, SHEEN_ROUGHNESS, BENT_NORMAL, ANISOTROPY_DIRECTION
		};

		static const std::vector<flat::ShaderPropertyType> s_boolPropertyTypes = {
			flat::ShaderPropertyType_DOUBLE_SIDED
		};

		ImGui::SetNextWindowContentSize(ImVec2(CONTENT_WIDTH, CONTENT_HEIGHT));

		bool hasMaterialChanged = false;

		if (ImGui::Begin("Material Editor", &windowOpen))
		{
			//We can't change the material name after the fact right now since certain objects (Level files, models etc) hold on to
			//their material names for later. Only when we make a new material can we modify the name
			//we would somehow have to know everyone who has the material name and change theirs.
			//Or we would have to change how MaterialName works in that everyone only have a reference to it - too much work for now
			//The other way to solve it is generate a GUID for each material made (and pack probably) and use that as the reference - not the name itself (https://github.com/graeme-hill/crossguid)
			std::string materialNameLabel = std::string("Material Name: ") + foundMaterial->stringName;

			ImGui::Text(materialNameLabel.c_str());
			ImGui::PushItemWidth(CONTENT_WIDTH / 3);
			ImGui::SameLine();
			if (ImGui::Button("Save"))
			{
				materialManifestFile->SaveManifest();
			}

			ImGui::PopItemWidth();

			int transparencyType = foundMaterial->transparencyType;

			ImGui::Text("Transparency:");

			if (ImGui::RadioButton("Opaque", &transparencyType, flat::eTransparencyType_OPAQUE))
			{
				hasMaterialChanged = true;
			}
			if (ImGui::RadioButton("Transparent", &transparencyType, flat::eTransparencyType_TRANSPARENT))
			{
				hasMaterialChanged = true;
			}

			foundMaterial->transparencyType = static_cast<flat::eTransparencyType>(transparencyType);

			auto& meshPasses = foundMaterial->shaderEffectPasses.meshPasses;

			for (u32 i = flat::eMeshPass_FORWARD; i <= flat::eMeshPass_TRANSPARENT; ++i) //only doing the ones we support
			{
				ImGui::PushID(i);
				auto& shaderEffect = meshPasses[i];

				std::string shaderEffectTitle = std::string("Shader Effect: ") + s_meshPassStrings[i];

				ImGui::Text(shaderEffectTitle.c_str());
				ImGui::SameLine();
				ImGui::PushItemWidth(CONTENT_WIDTH/2.0f);
				if (ImGui::BeginCombo("##label shaderName", shaderEffect.assetNameString.c_str()))
				{
					//@TODO(Serge): list all the shader effects we have - not sure how we're going to do that atm
					ImGui::EndCombo();
				}

				ImGui::PopItemWidth();

				ImGui::PopID();
			}

			//@TODO(Serge): implement shader params
			//for now just implement the ones we care about - float params, color params and texture params

			if (ImGui::CollapsingHeader("Float Shader Params", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				int paramToRemove = -1;
				ImVec2 size = ImGui::GetContentRegionAvail();

				for (size_t i = 0; i < foundMaterial->shaderParams.floatParams.size(); ++i)
				{
					auto& floatParam = foundMaterial->shaderParams.floatParams.at(i);

					const char* propertyType = s_shaderPropertyTypeStrings[floatParam.propertyType];

					bool open = ImGui::TreeNodeEx(propertyType, ImGuiTreeNodeFlags_AllowItemOverlap);
					
					ImGui::PushItemWidth(50);
					ImGui::SameLine(size.x - 50);
					ImGui::PushID(i);
					if (ImGui::SmallButton("Remove"))
					{
						paramToRemove = static_cast<int>(i);
						ImGui::PopItemWidth();
						ImGui::PopID();
						break;
					}
					ImGui::PopItemWidth();

					if (open)
					{
						ImGui::Text("Property Type: %s", propertyType);
						ImGui::SameLine();


						ImGui::Text("Value: ");
						ImGui::SameLine();

						std::string floatInputValueLabel = std::string("##label floatinputvalue") + std::string(propertyType);
						if (ImGui::InputFloat(floatInputValueLabel.c_str(), &floatParam.value))
						{
							hasMaterialChanged = true;
						}

						ImGui::TreePop();
					}
						
					ImGui::PopID();

					
				}

				if (paramToRemove != -1)
				{
					foundMaterial->shaderParams.floatParams.erase(foundMaterial->shaderParams.floatParams.begin() + paramToRemove);
					hasMaterialChanged = true;
				}

				std::vector<flat::ShaderPropertyType> availableFloatProperties;

				for (u32 i = 0; i < s_floatPropertyTypes.size(); ++i)
				{
					auto iter = std::find_if(foundMaterial->shaderParams.floatParams.begin(), foundMaterial->shaderParams.floatParams.end(), [i](const r2::draw::ShaderFloatParam& floatParam)
						{
							return floatParam.propertyType == s_floatPropertyTypes[i];
						});

					if (iter == foundMaterial->shaderParams.floatParams.end())
					{
						availableFloatProperties.push_back(s_floatPropertyTypes[i]);
					}
				}

				flat::ShaderPropertyType floatPropertyTypeToAdd = flat::ShaderPropertyType_ALBEDO;

				std::string availableFloatPropertyTypePreview = "";

				if (availableFloatProperties.size() > 0)
				{
					floatPropertyTypeToAdd = availableFloatProperties[0];
					availableFloatPropertyTypePreview = s_shaderPropertyTypeStrings[floatPropertyTypeToAdd];
				}

				ImGui::Text("Add New Float Property: ");
				ImGui::SameLine();

				ImGui::PushItemWidth(CONTENT_WIDTH / 3.0f);
				if (ImGui::BeginCombo("##label availablefloatpropertyTypes", availableFloatPropertyTypePreview.c_str()))
				{
					for (size_t i = 0; i < availableFloatProperties.size(); ++i)
					{
						if (ImGui::Selectable(s_shaderPropertyTypeStrings[availableFloatProperties[i]], floatPropertyTypeToAdd == availableFloatProperties[i]))
						{
							floatPropertyTypeToAdd = availableFloatProperties[i];
							
						}
					}

					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				const bool disableAddNewFloatProperty = availableFloatProperties.empty() || floatPropertyTypeToAdd == flat::ShaderPropertyType_ALBEDO;
				if (disableAddNewFloatProperty)
				{
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}

				ImGui::SameLine();
				if (ImGui::Button("Add Float Property"))
				{
					r2::draw::ShaderFloatParam newFloatParam;
					newFloatParam.propertyType = floatPropertyTypeToAdd;
					newFloatParam.value = 0.0f;
					foundMaterial->shaderParams.floatParams.push_back(newFloatParam);
					hasMaterialChanged = true;
				}

				if (disableAddNewFloatProperty)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}
			}
			
			//Color Params
			if (ImGui::CollapsingHeader("Color Shader Params", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				int paramToRemove = -1;
				auto size = ImGui::GetContentRegionAvail();

				for (size_t i = 0; i < foundMaterial->shaderParams.colorParams.size(); ++i)
				{
					auto& colorParam = foundMaterial->shaderParams.colorParams.at(i);

					const char* propertyType = s_shaderPropertyTypeStrings[colorParam.propertyType];


					bool open = ImGui::TreeNodeEx(propertyType, ImGuiTreeNodeFlags_AllowItemOverlap);

					ImGui::PushItemWidth(50);
					ImGui::SameLine(size.x - 50);
					ImGui::PushID(i);
					if (ImGui::SmallButton("Remove"))
					{
						paramToRemove = static_cast<int>(i);
						ImGui::PopItemWidth();
						ImGui::PopID();
						break;
					}
					ImGui::PopItemWidth();

					if (open)
					{
						ImGui::Text("Property Type: %s", propertyType);

						ImGui::Text("Value: ");
						ImGui::SameLine();

						std::string colorInputValueLabel = std::string("##label colorinputvalue") + std::string(propertyType);

						if (ImGui::ColorEdit4(colorInputValueLabel.c_str(), glm::value_ptr(colorParam.value)))
						{
							hasMaterialChanged = true;
						}

						ImGui::TreePop();
					}

					ImGui::PopID();
						
				}

				if (paramToRemove != -1)
				{
					foundMaterial->shaderParams.colorParams.erase(foundMaterial->shaderParams.colorParams.begin() + paramToRemove);
					hasMaterialChanged = true;
				}

				std::vector<flat::ShaderPropertyType> availableColorProperties;

				for (u32 i = 0; i < s_colorPropertyTypes.size(); ++i)
				{
					auto iter = std::find_if(foundMaterial->shaderParams.colorParams.begin(), foundMaterial->shaderParams.colorParams.end(), [i](const r2::draw::ShaderColorParam& colorParam)
						{
							return colorParam.propertyType == s_colorPropertyTypes[i];
						});

					if (iter == foundMaterial->shaderParams.colorParams.end())
					{
						availableColorProperties.push_back(s_colorPropertyTypes[i]);
					}
				}

				flat::ShaderPropertyType colorPropertyTypeToAdd = flat::ShaderPropertyType_ROUGHNESS;

				std::string availableColorPropertyTypePreview = "";

				if (availableColorProperties.size() > 0)
				{
					colorPropertyTypeToAdd = availableColorProperties[0];
					availableColorPropertyTypePreview = s_shaderPropertyTypeStrings[colorPropertyTypeToAdd];
				}

				ImGui::Text("Add New Color Property: ");
				ImGui::SameLine();
				ImGui::PushItemWidth(CONTENT_WIDTH / 3.0f);
				if (ImGui::BeginCombo("##label availablecolorpropertyTypes", availableColorPropertyTypePreview.c_str()))
				{
					for (size_t i = 0; i < availableColorProperties.size(); ++i)
					{
						if (ImGui::Selectable(s_shaderPropertyTypeStrings[availableColorProperties[i]], colorPropertyTypeToAdd == availableColorProperties[i]))
						{
							colorPropertyTypeToAdd = availableColorProperties[i];
						}
					}

					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				const bool disableAddNewColorProperty = availableColorProperties.empty() || colorPropertyTypeToAdd == flat::ShaderPropertyType_ROUGHNESS;
				if (disableAddNewColorProperty)
				{
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}

				ImGui::SameLine();
				if (ImGui::Button("Add Color Property"))
				{
					r2::draw::ShaderColorParam newColorParam;
					newColorParam.propertyType = colorPropertyTypeToAdd;
					newColorParam.value = glm::vec4(0, 0, 0, 1);
					foundMaterial->shaderParams.colorParams.push_back(newColorParam);
					hasMaterialChanged = true;
				}

				if (disableAddNewColorProperty)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}
			}

			if (ImGui::CollapsingHeader("Texture Shader Params", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				int paramToRemove = -1;
				char textureName[r2::fs::FILE_PATH_LENGTH];
				ImVec2 size = ImGui::GetContentRegionAvail();

				for (size_t i = 0; i < foundMaterial->shaderParams.textureParams.size(); ++i)
				{
					
					const std::string I_STRING = std::to_string(i);

					auto& textureParam = foundMaterial->shaderParams.textureParams.at(i);

					const char* propertyType = s_shaderPropertyTypeStrings[textureParam.propertyType];


					bool open = ImGui::TreeNodeEx(propertyType, ImGuiTreeNodeFlags_AllowItemOverlap);

					ImGui::PushItemWidth(50);
					ImGui::SameLine(size.x - 50);
					ImGui::PushID(i);
					if (ImGui::SmallButton("Remove"))
					{
						paramToRemove = static_cast<int>(i);
						ImGui::PopItemWidth();
						ImGui::PopID();
						break;
					}
					ImGui::PopItemWidth();

					if(open)
					{
						ImGui::Text("Property Type: %s", propertyType);

						ImGui::Text("Value: ");
						ImGui::SameLine();

						std::string textureInputValueLabel = std::string("##label textureinputvalue") + std::string(propertyType);

						//@NOTE(Serge): for now we don't want to list all our textures, just show what the texture is
						//@TODO(Serge): be able to modify this later
						r2::asset::AssetHandle textureAssetHandle = { textureParam.value, gameAssetManager.GetAssetCacheSlot() };
						const r2::asset::AssetFile* assetFile = gameAssetManager.GetAssetFile(textureAssetHandle);
						if (assetFile)
						{
							const char* textureAssetFilePath = assetFile->FilePath();

							r2::fs::utils::CopyFileNameWithParentDirectories(textureAssetFilePath, textureName, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::TEXTURE));
						}
						else
						{
							r2::util::PathCpy(textureName, "");
						}

						ImGui::Text(textureName);

						//if (ImGui::BeginCombo(textureInputValueLabel.c_str(), "Texture Name here"))
						//{
						//	
						//	//@TODO(Serge): how do we fill this?
						//	ImGui::EndCombo();
						//}

						//@NOTE(Serge): this should change with the texture you set above
						std::string texturePackName = std::string("Texture Pack: ") + textureParam.texturePackNameString;
						ImGui::Text(texturePackName.c_str());

						ImGui::Text("Packing Type: ");
						ImGui::SameLine();
						std::string packingTypeLabel = std::string("##label packingType") + I_STRING;
						if (ImGui::BeginCombo(packingTypeLabel.c_str(), s_texturePackingTypeStrings[textureParam.packingType]))
						{
							for (int pt = flat::ShaderPropertyPackingType_R; pt <= flat::ShaderPropertyPackingType_RGBA; pt++)
							{
								if (ImGui::Selectable(s_texturePackingTypeStrings[pt], pt == textureParam.packingType))
								{
									textureParam.packingType = static_cast<flat::ShaderPropertyPackingType>(pt);
									hasMaterialChanged = true;
								}
							}

							ImGui::EndCombo();
						}

						//min filtering
						ImGui::Text("Min Filter: ");
						ImGui::SameLine();
						std::string minFilterLabel = std::string("##label minfilter") + I_STRING;
						if (ImGui::BeginCombo(minFilterLabel.c_str(), s_minTextureFilterStrings[textureParam.minFilter]))
						{
							for (int mf = flat::MinTextureFilter_LINEAR; mf <= flat::MinTextureFilter_LINEAR_MIPMAP_LINEAR; ++mf)
							{
								if (ImGui::Selectable(s_minTextureFilterStrings[mf], mf == textureParam.minFilter))
								{
									textureParam.minFilter = static_cast<flat::MinTextureFilter>(mf);
									hasMaterialChanged = true;
								}
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Mag Filter: ");
						ImGui::SameLine();
						std::string magFilterLabel = std::string("##label magfilter") + I_STRING;
						if (ImGui::BeginCombo(magFilterLabel.c_str(), s_magTextureFilterStrings[textureParam.magFilter]))
						{
							for (int mf = flat::MagTextureFilter_LINEAR; mf <= flat::MagTextureFilter_NEAREST; ++mf)
							{
								if (ImGui::Selectable(s_magTextureFilterStrings[mf], mf == textureParam.magFilter))
								{
									textureParam.magFilter = static_cast<flat::MagTextureFilter>(mf);
									hasMaterialChanged = true;
								}
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Anisotropy: ");
						ImGui::SameLine();
						std::string anisotropyFilterLabel = std::string("##label anisotropyfiltering") + I_STRING;
						if (ImGui::DragFloat(anisotropyFilterLabel.c_str(), &textureParam.anisotropicFiltering, 1.0, 0.0, 16.0)) //@TODO(Serge): replace 16 with the max amount for the system
						{
							//@TODO(Serge): Update the render material somehow
							hasMaterialChanged = true;
						}

						//wrap modes

						ImGui::Text("Wrap S: ");
						ImGui::SameLine();
						std::string wrapSLabel = std::string("##label wrapS") + I_STRING;
						if (ImGui::BeginCombo(wrapSLabel.c_str(), s_textureWrapModeStrings[textureParam.wrapS]))
						{
							for (int wm = flat::TextureWrapMode_REPEAT; wm <= flat::TextureWrapMode_MIRRORED_REPEAT; ++wm)
							{
								if (ImGui::Selectable(s_textureWrapModeStrings[wm], wm == textureParam.wrapS))
								{
									textureParam.wrapS = static_cast<flat::TextureWrapMode>(wm);
									hasMaterialChanged = true;
								}
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Wrap T: ");
						ImGui::SameLine();
						std::string wrapTLabel = std::string("##label wrapT") + I_STRING;
						if (ImGui::BeginCombo(wrapTLabel.c_str(), s_textureWrapModeStrings[textureParam.wrapT]))
						{
							for (int wm = flat::TextureWrapMode_REPEAT; wm <= flat::TextureWrapMode_MIRRORED_REPEAT; ++wm)
							{
								if (ImGui::Selectable(s_textureWrapModeStrings[wm], wm == textureParam.wrapT))
								{
									textureParam.wrapT = static_cast<flat::TextureWrapMode>(wm);
									hasMaterialChanged = true;
								}
							}
							ImGui::EndCombo();
						}

						ImGui::Text("Wrap R: ");
						ImGui::SameLine();
						std::string wrapRLabel = std::string("##label wrapR") + I_STRING;
						if (ImGui::BeginCombo(wrapRLabel.c_str(), s_textureWrapModeStrings[textureParam.wrapR]))
						{
							for (int wm = flat::TextureWrapMode_REPEAT; wm <= flat::TextureWrapMode_MIRRORED_REPEAT; ++wm)
							{
								if (ImGui::Selectable(s_textureWrapModeStrings[wm], wm == textureParam.wrapR))
								{
									textureParam.wrapR = static_cast<flat::TextureWrapMode>(wm);
									hasMaterialChanged = true;
								}
							}
							ImGui::EndCombo();
						}

						ImGui::TreePop();
					}

					
					ImGui::PopID();
				}

				if (paramToRemove != -1)
				{
					foundMaterial->shaderParams.textureParams.erase(foundMaterial->shaderParams.textureParams.begin() + paramToRemove);
					hasMaterialChanged = true;
				}

				std::vector<flat::ShaderPropertyType> availableTextureProperties;

				for (u32 i = 0; i < s_texturePropertyTypes.size(); ++i)
				{
					auto iter = std::find_if(foundMaterial->shaderParams.textureParams.begin(), foundMaterial->shaderParams.textureParams.end(), [i](const r2::draw::ShaderTextureParam& textureParam)
						{
							return textureParam.propertyType == s_texturePropertyTypes[i];
						});

					if (iter == foundMaterial->shaderParams.textureParams.end())
					{
						availableTextureProperties.push_back(s_texturePropertyTypes[i]);
					}
				}

				flat::ShaderPropertyType texturePropertyTypeToAdd = flat::ShaderPropertyType_REFLECTANCE;

				std::string availableTexturePropertyTypePreview = "";

				if (availableTextureProperties.size() > 0)
				{
					texturePropertyTypeToAdd = availableTextureProperties[0];
					availableTexturePropertyTypePreview = s_shaderPropertyTypeStrings[texturePropertyTypeToAdd];
				}


				ImGui::Text("Add New Texture Property: ");
				ImGui::SameLine();
				ImGui::PushItemWidth(CONTENT_WIDTH / 3.0f);
				if (ImGui::BeginCombo("##label availabletexturepropertyTypes", availableTexturePropertyTypePreview.c_str()))
				{
					for (size_t i = 0; i < availableTextureProperties.size(); ++i)
					{
						if (ImGui::Selectable(s_shaderPropertyTypeStrings[availableTextureProperties[i]], texturePropertyTypeToAdd == availableTextureProperties[i]))
						{
							texturePropertyTypeToAdd = availableTextureProperties[i];
						}
					}

					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				const bool disableAddNewTextureProperty = availableTextureProperties.empty() || texturePropertyTypeToAdd == flat::ShaderPropertyType_REFLECTANCE;
				if (disableAddNewTextureProperty)
				{
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}

				ImGui::SameLine();
				if (ImGui::Button("Add Texture Property"))
				{
					r2::draw::ShaderTextureParam newTextureParam;
					newTextureParam.propertyType = texturePropertyTypeToAdd;
					newTextureParam.value = STRING_ID("");
					newTextureParam.texturePackName = newTextureParam.value;
					newTextureParam.texturePackNameString = "";
					newTextureParam.packingType = flat::ShaderPropertyPackingType_RGBA;
					newTextureParam.minFilter = flat::MinTextureFilter_LINEAR;
					newTextureParam.magFilter = flat::MagTextureFilter_LINEAR;
					newTextureParam.anisotropicFiltering = 0.0f;
					newTextureParam.wrapS = flat::TextureWrapMode_REPEAT;
					newTextureParam.wrapT = flat::TextureWrapMode_REPEAT;
					newTextureParam.wrapR = flat::TextureWrapMode_REPEAT;

					foundMaterial->shaderParams.textureParams.push_back(newTextureParam);
					hasMaterialChanged = true;
				}

				if (disableAddNewTextureProperty)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}
			}

			//add bool param for double sided
			if (ImGui::CollapsingHeader("Bool Shader Params", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				int paramToRemove = -1;
				ImVec2 size = ImGui::GetContentRegionAvail();

				for (size_t i = 0; i < foundMaterial->shaderParams.boolParams.size(); ++i)
				{
					auto& boolParam = foundMaterial->shaderParams.boolParams.at(i);

					const char* propertyType = s_shaderPropertyTypeStrings[boolParam.propertyType];

					bool open = ImGui::TreeNodeEx(propertyType, ImGuiTreeNodeFlags_AllowItemOverlap);
					ImGui::PushID(i);
					ImGui::PushItemWidth(50);
					ImGui::SameLine(size.x - 50);

					if (ImGui::SmallButton("Remove"))
					{
						paramToRemove = static_cast<int>(i);
						ImGui::PopItemWidth();
						ImGui::PopID();
						break;
					}
					
					ImGui::PopItemWidth();

					if(open)
					{
						ImGui::Text("Property Type: %s", propertyType);

						if (ImGui::Checkbox(propertyType, &boolParam.value))
						{
							hasMaterialChanged = true;
						}
						ImGui::TreePop();
					}

					ImGui::PopID();
				}

				if (paramToRemove != -1)
				{
					foundMaterial->shaderParams.boolParams.erase(foundMaterial->shaderParams.boolParams.begin() + paramToRemove);
					hasMaterialChanged = true;
				}

				std::vector<flat::ShaderPropertyType> availableBoolProperties;

				for (u32 i = 0; i < s_boolPropertyTypes.size(); ++i)
				{
					auto iter = std::find_if(foundMaterial->shaderParams.boolParams.begin(), foundMaterial->shaderParams.boolParams.end(), [i](const r2::draw::ShaderBoolParam& boolParam)
						{
							return boolParam.propertyType == s_boolPropertyTypes[i];
						});

					if (iter == foundMaterial->shaderParams.boolParams.end())
					{
						availableBoolProperties.push_back(s_boolPropertyTypes[i]);
					}
				}

				flat::ShaderPropertyType boolPropertyTypeToAdd = flat::ShaderPropertyType_ROUGHNESS;

				std::string availableBoolPropertyTypePreview = "";

				if (availableBoolProperties.size() > 0)
				{
					boolPropertyTypeToAdd = availableBoolProperties[0];
					availableBoolPropertyTypePreview = s_shaderPropertyTypeStrings[boolPropertyTypeToAdd];
				}

				ImGui::Text("Add New Boolean Property: ");
				ImGui::SameLine();
				ImGui::PushItemWidth(CONTENT_WIDTH / 3.0f);
				if (ImGui::BeginCombo("##label availableboolpropertyTypes", availableBoolPropertyTypePreview.c_str()))
				{
					for (size_t i = 0; i < availableBoolProperties.size(); ++i)
					{
						if (ImGui::Selectable(s_shaderPropertyTypeStrings[availableBoolProperties[i]], boolPropertyTypeToAdd == availableBoolProperties[i]))
						{
							boolPropertyTypeToAdd = availableBoolProperties[i];
						}
					}

					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				const bool disableAddNewBoolProperty = availableBoolProperties.empty() || boolPropertyTypeToAdd == flat::ShaderPropertyType_ROUGHNESS;
				if (disableAddNewBoolProperty)
				{
					ImGui::BeginDisabled(true);
					ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.5);
				}

				ImGui::SameLine();
				if (ImGui::Button("Add Boolean Property"))
				{
					r2::draw::ShaderBoolParam newBoolParam;
					newBoolParam.propertyType = boolPropertyTypeToAdd;
					newBoolParam.value = false;
					foundMaterial->shaderParams.boolParams.push_back(newBoolParam);
					hasMaterialChanged = true;
				}

				if (disableAddNewBoolProperty)
				{
					ImGui::PopStyleVar();
					ImGui::EndDisabled();
				}
			}



			ImGui::End();
		}

		if (hasMaterialChanged)
		{
			flatbuffers::FlatBufferBuilder builder;
			const flat::Material* flatMaterial = r2::mat::MakeFlatMaterialFromMaterial(builder, *foundMaterial);

			r2::SArray<r2::draw::tex::Texture>* textures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::Texture, 200);
			r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::CubemapTexture, r2::draw::tex::Cubemap);

			r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();
			R2_CHECK(renderMaterialCache != nullptr, "This should never be nullptr");

			bool isLoaded = r2::draw::rmat::IsMaterialLoadedOnGPU(*renderMaterialCache, materialName.name);

			bool result = gameAssetManager.GetTexturesForFlatMaterial(flatMaterial, textures, cubemaps);
			R2_CHECK(result, "This should always work");

			r2::draw::tex::CubemapTexture* cubemapTextureToUse = nullptr;
			r2::SArray<r2::draw::tex::Texture>* texturesToUse = nullptr;

			if (r2::sarr::Size(*textures) > 0)
			{
				texturesToUse = textures;
			}

			if (r2::sarr::Size(*cubemaps) > 0)
			{
				cubemapTextureToUse = &r2::sarr::At(*cubemaps, 0);
			}

			if (isLoaded)
			{
				gameAssetManager.LoadMaterialTextures(flatMaterial); //@NOTE(Serge): this actually does nothing at the moment since this pack is probably loaded already
				result = r2::draw::rmat::UploadMaterialTextureParams(*renderMaterialCache, flatMaterial, texturesToUse, cubemapTextureToUse, true);
				R2_CHECK(result, "This should always work");
			}

			FREE(cubemaps, *MEM_ENG_SCRATCH_PTR);
			FREE(textures, *MEM_ENG_SCRATCH_PTR);	
		}

#endif
	}
}

#endif