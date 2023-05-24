#include "r2pch.h"
#if defined(R2_EDITOR) && defined(R2_IMGUI)
#include "r2/Core/Events/Events.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorMainMenuBar.h"
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/EditorAssetPanel.h"
#include "r2/Editor/EditorScenePanel.h"
#include "r2/Editor/EditorEvents/EditorEvent.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Systems/RenderSystem.h"
#include "r2/Game/ECS/Systems/SkeletalAnimationSystem.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "r2/Editor/EditorEvents/EditorLevelEvents.h"
#ifdef R2_DEBUG
#include "r2/Game/ECS/Components/DebugRenderComponent.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#include "r2/Game/ECS/Systems/DebugBonesRenderSystem.h"
#include "r2/Game/ECS/Systems/DebugRenderSystem.h"
#endif
#include "imgui.h"

#include "r2/Core/Engine.h"
#include "r2/Game/ECSWorld/ECSWorld.h"

//@TEST: for test code only - REMOVE!
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Utils/Random.h"
#include "r2/Platform/Platform.h"
#include "r2/Core/Application.h"
#include "r2/Game/Level/LevelCache.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Render/Animation/AnimationCache.h"


namespace 
{
	//Figure out render system defaults
	constexpr u32 MAX_NUM_STATIC_BATCHES = 32;
	constexpr u32 MAX_NUM_DYNAMIC_BATCHES = 32;
	constexpr u32 MAX_LEVELS = 100;

}

namespace r2
{
	Editor::Editor()
		:mEditorMemoryAreaHandle(r2::mem::MemoryArea::Invalid)
		,mMallocArena(r2::mem::utils::MemBoundary())
		,microbatAnimModel(nullptr)
	{

	}

	void Editor::Init()
	{
		mRandom.Randomize();

		//Do all of the panels/widgets setup here
		std::unique_ptr<edit::MainMenuBar> mainMenuBar = std::make_unique<edit::MainMenuBar>();
		mEditorWidgets.push_back(std::move(mainMenuBar));

		std::unique_ptr<edit::InspectorPanel> inspectorPanel = std::make_unique<edit::InspectorPanel>();
		mEditorWidgets.push_back(std::move(inspectorPanel));

		std::unique_ptr<edit::AssetPanel> assetPanel = std::make_unique<edit::AssetPanel>();
		mEditorWidgets.push_back(std::move(assetPanel));

		std::unique_ptr<edit::ScenePanel> scenePanel = std::make_unique<edit::ScenePanel>();
		mEditorWidgets.push_back(std::move(scenePanel));

		//now init all of the widgets
		for (const auto& widget : mEditorWidgets)
		{
			widget->Init(this);
		}

		microbatAnimModel = nullptr;
	}

	void Editor::Shutdown()
	{
		for (const auto& widget : mEditorWidgets)
		{
			widget->Shutdown();
		}

		mEditorWidgets.clear();

		//@TEMPORARY!!!
		if (microbatAnimModel)
		{
			r2::draw::ModelCache* editorModelSystem = CENG.GetApplication().GetEditorModelSystem();
			r2::draw::modlche::ReturnAnimModel(editorModelSystem, microbatAnimModel);

		}

		for (auto iter = mComponentAllocations.rbegin(); iter != mComponentAllocations.rend(); ++iter)
		{
			FREE(*iter, mMallocArena);
		}
	}

	void Editor::OnEvent(evt::Event& e)
	{
		r2::evt::EventDispatcher dispatcher(e);

		dispatcher.Dispatch<r2::evt::KeyPressedEvent>([this](const r2::evt::KeyPressedEvent& e)
			{
				if (e.KeyCode() == r2::io::KEY_z &&
					((e.Modifiers() & r2::io::Key::CONTROL_PRESSED) == r2::io::Key::CONTROL_PRESSED) &&
					((e.Modifiers() & r2::io::Key::SHIFT_PRESSED_KEY) == 0))
				{
					UndoLastAction();
					return true;
				}
				else if (e.KeyCode() == r2::io::KEY_z && 
					((e.Modifiers() & r2::io::Key::CONTROL_PRESSED) == r2::io::Key::CONTROL_PRESSED) &&
					((e.Modifiers() & r2::io::Key::SHIFT_PRESSED_KEY) == r2::io::Key::SHIFT_PRESSED_KEY))
				{
					RedoLastAction();
					return true;
				}
				else if (e.KeyCode() == r2::io::KEY_s &&
					((e.Modifiers() & r2::io::Key::CONTROL_PRESSED) == r2::io::Key::CONTROL_PRESSED))
				{
					Save();
					return true;
				}

				return false;
			});


		for (const auto& widget : mEditorWidgets)
		{
			widget->OnEvent(e);
		}
	}

	void Editor::Update()
	{
		CENG.GetLevelManager().Update();
		MENG.GetECSWorld().GetSkeletalAnimationSystem()->Update();

		for (const auto& widget : mEditorWidgets)
		{
			widget->Update();
		}
	}

	void Editor::Render()
	{
		MENG.GetECSWorld().GetRenderSystem()->Render();


#ifdef R2_DEBUG
		MENG.GetECSWorld().GetDebugRenderSystem()->Render();
		MENG.GetECSWorld().GetDebugBonesRenderSystem()->Render();
#endif
	}

	void Editor::RenderImGui(u32 dockingSpaceID)
	{
		for (const auto& widget : mEditorWidgets)
		{
			widget->Render(dockingSpaceID);
		}
	}

	void Editor::PostNewAction(std::unique_ptr<edit::EditorAction> action)
	{
		//Do the action
		action->Redo();
		mUndoStack.push_back(std::move(action));
	}

	void Editor::UndoLastAction()
	{
		if (!mUndoStack.empty())
		{
			std::unique_ptr<edit::EditorAction> action = std::move(mUndoStack.back());

			mUndoStack.pop_back();

			action->Undo();

			mRedoStack.push_back(std::move(action));
		}
	}

	void Editor::RedoLastAction()
	{
		if (!mRedoStack.empty())
		{
			std::unique_ptr<edit::EditorAction> action = std::move(mRedoStack.back());

			mRedoStack.pop_back();

			action->Redo();

			mUndoStack.push_back(std::move(action));
		}
	}

	void Editor::Save()
	{
		//@Test - very temporary
		std::filesystem::path levelBinURI = "testGroup/Level1.rlvl";
		std::filesystem::path levelRawURI = "testGroup/Level1.json";

		std::filesystem::path levelDataBinPath = CENG.GetApplication().GetLevelPackDataBinPath();
		std::filesystem::path levelDataRawPath = CENG.GetApplication().GetLevelPackDataJSONPath();

		r2::draw::ModelCache* editorModelSystem = CENG.GetApplication().GetEditorModelSystem();
		r2::draw::AnimationCache* editorAnimationCache = CENG.GetApplication().GetEditorAnimationCache();

		CENG.GetLevelManager().SaveNewLevelFile(1, (levelDataBinPath / levelBinURI).string().c_str(), (levelDataRawPath / levelRawURI).string().c_str(), *editorModelSystem, *editorAnimationCache);

//		r2::lvlche::SaveNewLevelFile(*moptrLevelCache, mSceneGraph.GetECSCoordinator(), 1, (levelDataBinPath / levelBinURI).string().c_str(), (levelDataRawPath / levelRawURI).string().c_str());

		//r2::asset::pln::SaveLevelData(mCoordinator, 1, (levelDataBinPath / levelBinURI).string(), (levelDataRawPath / levelRawURI).string());
	}

	void Editor::LoadLevel(const std::string& filePathName, const std::string& parentDirectory)
	{
		LevelName levelAssetName = LevelManager::MakeLevelNameFromPath(filePathName.c_str());

		const Level* newLevel = CENG.GetLevelManager().LoadLevel(levelAssetName);

		evt::EditorLevelLoadedEvent e(*newLevel);

		PostEditorEvent(e);
	}

	std::string Editor::GetAppLevelPath() const
	{
		return CENG.GetApplication().GetLevelPackDataBinPath();
	}

	void Editor::PostEditorEvent(r2::evt::EditorEvent& e)
	{
		//@TODO(Serge): listen to the entity creation event and add in the appropriate components for testing
		//@NOTE: all test code!				
		r2::evt::EventDispatcher dispatcher(e);
		dispatcher.Dispatch<r2::evt::EditorEntityCreatedEvent>([this](const r2::evt::EditorEntityCreatedEvent& e)
			{
				r2::draw::DefaultModel modelType = (r2::draw::DefaultModel)mRandom.RandomNum(r2::draw::QUAD, r2::draw::CYLINDER);

				r2::draw::ModelCache* editorModelSystem = CENG.GetApplication().GetEditorModelSystem();

				r2::draw::AnimationCache* editorAnimationCache = CENG.GetApplication().GetEditorAnimationCache();

				r2::asset::Asset microbatAsset = r2::asset::Asset("micro_bat.rmdl", r2::asset::RMODEL);

				r2::draw::ModelHandle microbatModelHandle = r2::draw::modlche::LoadModel(editorModelSystem, microbatAsset);

				microbatAnimModel = r2::draw::modlche::GetAnimModel(editorModelSystem, microbatModelHandle);

				r2::draw::vb::GPUModelRefHandle gpuModelRefHandle = r2::draw::renderer::UploadAnimModel(microbatAnimModel);//r2::draw::renderer::GetDefaultModelRef(r2::draw::QUAD);

				ecs::RenderComponent renderComponent;
				renderComponent.assetModelHash = microbatAsset.HashID();
				renderComponent.optrOverrideMaterials = nullptr;
				renderComponent.optrMaterialOverrideNames = nullptr;
				renderComponent.gpuModelRefHandle = gpuModelRefHandle;
				renderComponent.primitiveType = (u32)draw::PrimitiveType::TRIANGLES;
				renderComponent.isAnimated = true;
				renderComponent.drawParameters.layer = r2::draw::DL_CHARACTER;
				renderComponent.drawParameters.flags.Clear();
				renderComponent.drawParameters.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);
				//renderComponent.drawParameters.flags.Set(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);

				r2::draw::renderer::SetDefaultCullState(renderComponent.drawParameters);
				r2::draw::renderer::SetDefaultStencilState(renderComponent.drawParameters);
				r2::draw::renderer::SetDefaultBlendState(renderComponent.drawParameters);

				//r2::draw::DebugModelType debugModelType;

				/*
				r2::draw::DebugModelType debugModelType;

				//for Arrow this is headBaseRadius
				r2::SArray<float>* radii;

				//for Arrow this is length
				//for Cone and Cylinder - this is height
				//for a line - this is length of the line
				r2::SArray<float>* scales;

				//For Lines, this is used for the direction of p1
				//we use the scale to determine the p1 of the line
				r2::SArray<glm::vec3>* directions;

				r2::SArray<glm::vec4>* colors;
				b32 filled;
				b32 depthTest;
				*/

				//ecs::DebugRenderComponent debugRenderComponent;
				//debugRenderComponent.colors = nullptr;
				//debugRenderComponent.directions = nullptr;
				//debugRenderComponent.radii = nullptr;
				//debugRenderComponent.scales = nullptr;

				//debugRenderComponent.debugModelType = draw::DEBUG_LINE;

				////don't love this
				//debugRenderComponent.radii = MAKE_SARRAY(mMallocArena, float, 1);
				//r2::sarr::Push(*debugRenderComponent.radii, 0.1f);
				//mComponentAllocations.push_back(debugRenderComponent.radii);

				//debugRenderComponent.scales = MAKE_SARRAY(mMallocArena, float, 1);
				//r2::sarr::Push(*debugRenderComponent.scales, 1.0f);
				//mComponentAllocations.push_back(debugRenderComponent.scales);

				//debugRenderComponent.directions = MAKE_SARRAY(mMallocArena, glm::vec3, 1);
				//r2::sarr::Push(*debugRenderComponent.directions, glm::vec3(1, 1, 1));
				//mComponentAllocations.push_back(debugRenderComponent.directions);

				//debugRenderComponent.colors = MAKE_SARRAY(mMallocArena, glm::vec4, 1);
				//r2::sarr::Push(*debugRenderComponent.colors, glm::vec4(0, 1, 0, 1));
				//mComponentAllocations.push_back(debugRenderComponent.colors);

				//debugRenderComponent.depthTest = true;
				//debugRenderComponent.filled = true;
				//// CENG.GetApplication().GetEditorAnimModel();

				

				//mCoordinator->AddComponent<ecs::DebugRenderComponent>(theNewEntity, debugRenderComponent);

				/*
				r2::SArray<u32>* startTimePerInstance;
				r2::SArray<b32>* loopPerInstance;
				r2::SArray<const r2::draw::Animation*>* animationsPerInstance;

				r2::SArray<r2::draw::ShaderBoneTransform>* shaderBones;
				*/
				r2::asset::Asset animationAsset0 = r2::asset::Asset("micro_bat_idle.ranm", r2::asset::RANIMATION);


				ecs::SkeletalAnimationComponent skeletalAnimationComponent;
				skeletalAnimationComponent.animModelAssetName = microbatAsset.HashID();
				skeletalAnimationComponent.animModel = microbatAnimModel;
				skeletalAnimationComponent.shouldUseSameTransformsForAllInstances = renderComponent.drawParameters.flags.IsSet(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
				skeletalAnimationComponent.startingAnimationAssetName = animationAsset0.HashID();
				skeletalAnimationComponent.startTime = 0; //.startTimePerInstance = MAKE_SARRAY(mMallocArena, u32, 1);
			//	r2::sarr::Push(*skeletalAnimationComponent.startTimePerInstance, 0u);
			//	mComponentAllocations.push_back(skeletalAnimationComponent.startTimePerInstance);

				skeletalAnimationComponent.shouldLoop = true;// = MAKE_SARRAY(mMallocArena, b32, 1);
			//	r2::sarr::Push(*skeletalAnimationComponent.loopPerInstance, 1u);
			//	mComponentAllocations.push_back(skeletalAnimationComponent.loopPerInstance);
	

				r2::draw::AnimationHandle animationHandle0 = r2::draw::animcache::LoadAnimation(*editorAnimationCache, animationAsset0);

				skeletalAnimationComponent.animation = r2::draw::animcache::GetAnimation(*editorAnimationCache, animationHandle0);


				skeletalAnimationComponent.shaderBones = MAKE_SARRAY(mMallocArena, r2::draw::ShaderBoneTransform, r2::sarr::Size(*skeletalAnimationComponent.animModel->boneInfo));
				r2::sarr::Clear(*skeletalAnimationComponent.shaderBones);
				mComponentAllocations.push_back(skeletalAnimationComponent.shaderBones);

				ecs::DebugBoneComponent debugBoneComponent;
				debugBoneComponent.color = glm::vec4(1, 1, 0, 1);
				debugBoneComponent.debugBones = MAKE_SARRAY(mMallocArena, r2::draw::DebugBone, r2::sarr::Size(*skeletalAnimationComponent.animModel->boneInfo));
				r2::sarr::Clear(*debugBoneComponent.debugBones);
				mComponentAllocations.push_back(debugBoneComponent.debugBones);

				ecs::Entity theNewEntity = e.GetEntity();

				MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::SkeletalAnimationComponent>(theNewEntity, skeletalAnimationComponent);
				MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::DebugBoneComponent>(theNewEntity, debugBoneComponent);
				MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::RenderComponent>(theNewEntity, renderComponent);
				CENG.GetApplication().AddComponentsToEntity(MENG.GetECSWorld().GetECSCoordinator(), theNewEntity);

				//transform instance
				{
					ecs::InstanceComponentT<ecs::TransformComponent> instancedTransformComponent;
					instancedTransformComponent.numInstances = 2;
					instancedTransformComponent.instances = MAKE_SARRAY(mMallocArena, ecs::TransformComponent, 2);
					mComponentAllocations.push_back(instancedTransformComponent.instances);

					ecs::TransformComponent transformInstance1;
					transformInstance1.localTransform.position = glm::vec3(2, 1, 2);
					transformInstance1.localTransform.scale = glm::vec3(0.01f);
					transformInstance1.localTransform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));

					r2::sarr::Push(*instancedTransformComponent.instances, transformInstance1);

					ecs::TransformComponent transformInstance2;
					transformInstance2.localTransform.position = glm::vec3(-2, 1, 2);
					transformInstance2.localTransform.scale = glm::vec3(0.01f);
					transformInstance2.localTransform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));

					r2::sarr::Push(*instancedTransformComponent.instances, transformInstance2);

					
					MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::InstanceComponentT<ecs::TransformComponent>>(theNewEntity, instancedTransformComponent);
				}
				
				//debug bone instance
				{
					ecs::InstanceComponentT<ecs::DebugBoneComponent> instancedDebugBoneComponent;
					instancedDebugBoneComponent.numInstances = 2;
					instancedDebugBoneComponent.instances = MAKE_SARRAY(mMallocArena, ecs::DebugBoneComponent, 2);
					mComponentAllocations.push_back(instancedDebugBoneComponent.instances);

					ecs::DebugBoneComponent debugBoneInstance1;
					debugBoneInstance1.color = glm::vec4(1, 0, 0, 1);
					debugBoneInstance1.debugBones = MAKE_SARRAY(mMallocArena, r2::draw::DebugBone, r2::sarr::Size(*skeletalAnimationComponent.animModel->boneInfo));
					r2::sarr::Clear(*debugBoneInstance1.debugBones);
					mComponentAllocations.push_back(debugBoneInstance1.debugBones);

					ecs::DebugBoneComponent debugBoneInstance2;
					debugBoneInstance2.color = glm::vec4(1, 0, 1, 1);
					debugBoneInstance2.debugBones = MAKE_SARRAY(mMallocArena, r2::draw::DebugBone, r2::sarr::Size(*skeletalAnimationComponent.animModel->boneInfo));
					r2::sarr::Clear(*debugBoneInstance2.debugBones);
					mComponentAllocations.push_back(debugBoneInstance2.debugBones);

					r2::sarr::Push(*instancedDebugBoneComponent.instances, debugBoneInstance1);
					r2::sarr::Push(*instancedDebugBoneComponent.instances, debugBoneInstance2);

					MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(theNewEntity, instancedDebugBoneComponent);
				}

				//Skeletal animation instance component
				{
					ecs::InstanceComponentT<ecs::SkeletalAnimationComponent> instancedSkeletalAnimationComponent;
					instancedSkeletalAnimationComponent.numInstances = 2;
					instancedSkeletalAnimationComponent.instances = MAKE_SARRAY(mMallocArena, ecs::SkeletalAnimationComponent, 2);
					mComponentAllocations.push_back(instancedSkeletalAnimationComponent.instances);


					r2::asset::Asset animationAsset1 = r2::asset::Asset("micro_bat_invert_idle.ranm", r2::asset::RANIMATION);
					r2::asset::Asset animationAsset2 = r2::asset::Asset("micro_bat_attack.ranm", r2::asset::RANIMATION);

					ecs::SkeletalAnimationComponent skeletalAnimationInstance1;
					skeletalAnimationInstance1.animModelAssetName = microbatAsset.HashID();
					skeletalAnimationInstance1.animModel = microbatAnimModel;
					skeletalAnimationInstance1.shouldUseSameTransformsForAllInstances = skeletalAnimationComponent.shouldUseSameTransformsForAllInstances;
					skeletalAnimationInstance1.startingAnimationAssetName = animationAsset1.HashID();
					skeletalAnimationInstance1.startTime = 0; 
					skeletalAnimationInstance1.shouldLoop = true;

					r2::draw::AnimationHandle animationHandle1 = r2::draw::animcache::LoadAnimation(*editorAnimationCache, animationAsset1);
					skeletalAnimationInstance1.animation = r2::draw::animcache::GetAnimation(*editorAnimationCache, animationHandle1);

					skeletalAnimationInstance1.shaderBones = MAKE_SARRAY(mMallocArena, r2::draw::ShaderBoneTransform, r2::sarr::Size(*skeletalAnimationInstance1.animModel->boneInfo));
					r2::sarr::Clear(*skeletalAnimationInstance1.shaderBones);
					mComponentAllocations.push_back(skeletalAnimationInstance1.shaderBones);


					ecs::SkeletalAnimationComponent skeletalAnimationInstance2;
					skeletalAnimationInstance2.animModelAssetName = microbatAsset.HashID();
					skeletalAnimationInstance2.animModel = microbatAnimModel;
					skeletalAnimationInstance2.shouldUseSameTransformsForAllInstances = skeletalAnimationComponent.shouldUseSameTransformsForAllInstances;
					skeletalAnimationInstance2.startingAnimationAssetName = animationAsset2.HashID();
					skeletalAnimationInstance2.startTime = 0;
					skeletalAnimationInstance2.shouldLoop = true;

					r2::draw::AnimationHandle animationHandle2 = r2::draw::animcache::LoadAnimation(*editorAnimationCache, animationAsset2);
					skeletalAnimationInstance2.animation = r2::draw::animcache::GetAnimation(*editorAnimationCache, animationHandle2);


					skeletalAnimationInstance2.shaderBones = MAKE_SARRAY(mMallocArena, r2::draw::ShaderBoneTransform, r2::sarr::Size(*skeletalAnimationInstance2.animModel->boneInfo));
					r2::sarr::Clear(*skeletalAnimationInstance2.shaderBones);
					mComponentAllocations.push_back(skeletalAnimationInstance2.shaderBones);

					r2::sarr::Push(*instancedSkeletalAnimationComponent.instances, skeletalAnimationInstance1);
					r2::sarr::Push(*instancedSkeletalAnimationComponent.instances, skeletalAnimationInstance2);

					MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(theNewEntity, instancedSkeletalAnimationComponent);
				}

				//debug render component + instances
				{
					ecs::DebugRenderComponent debugRenderComponent;
					debugRenderComponent.debugModelType = draw::DEBUG_QUAD;
					debugRenderComponent.direction = glm::normalize(glm::vec3(1, 1, 0.5));
					debugRenderComponent.depthTest = true;
					debugRenderComponent.filled = true;
					debugRenderComponent.color = glm::vec4(1, 1, 0, 1);
					debugRenderComponent.scale = glm::vec3(2, 2, 1);

					ecs::InstanceComponentT<ecs::DebugRenderComponent> instancedDebugRenderComponent;
					instancedDebugRenderComponent.numInstances = 2;
					instancedDebugRenderComponent.instances = MAKE_SARRAY(mMallocArena, ecs::DebugRenderComponent, 2);
					mComponentAllocations.push_back(instancedDebugRenderComponent.instances);

					ecs::DebugRenderComponent debugRenderComponent1;
					debugRenderComponent1.color = glm::vec4(1, 0, 0, 1);
					debugRenderComponent1.debugModelType = draw::DEBUG_QUAD;
					debugRenderComponent1.direction = glm::normalize(glm::vec3(-1, 1, 0.5));
					debugRenderComponent1.depthTest = true;
					debugRenderComponent1.filled = true;
					debugRenderComponent1.scale = glm::vec3(1, 2, 1);
					
					ecs::DebugRenderComponent debugRenderComponent2;
					debugRenderComponent2.color = glm::vec4(1, 0, 1, 1);
					debugRenderComponent2.debugModelType = draw::DEBUG_QUAD;
					debugRenderComponent2.direction = glm::normalize(glm::vec3(1, -1, -0.5));
					debugRenderComponent2.depthTest = true;
					debugRenderComponent2.filled = true;
					debugRenderComponent2.scale = glm::vec3(2, 1, 1);


					r2::sarr::Push(*instancedDebugRenderComponent.instances, debugRenderComponent1);
					r2::sarr::Push(*instancedDebugRenderComponent.instances, debugRenderComponent2);

					MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::DebugRenderComponent>(theNewEntity, debugRenderComponent);
					MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::InstanceComponentT<ecs::DebugRenderComponent>>(theNewEntity, instancedDebugRenderComponent);
				}

				ecs::TransformComponent& transformComponent = MENG.GetECSWorld().GetECSCoordinator()->GetComponent<ecs::TransformComponent>(theNewEntity);
				transformComponent.localTransform.position = glm::vec3(0, 0, 2);
				transformComponent.localTransform.scale = glm::vec3(0.01f);
				
				transformComponent.localTransform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));
				

			return e.ShouldConsume();
		});

		//@TODO(Serge): remove when DebugBoneComponent no long necessary for Skeletal Animation
		dispatcher.Dispatch<r2::evt::EditorLevelLoadedEvent>([this](const r2::evt::EditorLevelLoadedEvent& e)
			{
				const r2::SArray<ecs::Entity>& allEntities = MENG.GetECSWorld().GetECSCoordinator()->GetAllLivingEntities();

				const auto numEntities = r2::sarr::Size(allEntities);

				for (u64 i = 0; i < numEntities; ++i)
				{
					ecs::Entity e = r2::sarr::At(allEntities, i);

					if (MENG.GetECSWorld().GetECSCoordinator()->HasComponent<ecs::SkeletalAnimationComponent>(e)
#ifdef R2_DEBUG
						&& !MENG.GetECSWorld().GetECSCoordinator()->HasComponent<ecs::DebugBoneComponent>(e)
#endif		
						)
					{
#ifdef R2_DEBUG
						const ecs::SkeletalAnimationComponent& skeletalAnimationComponent = MENG.GetECSWorld().GetECSCoordinator()->GetComponent<ecs::SkeletalAnimationComponent>(e);

						ecs::DebugBoneComponent debugBoneComponent;
						debugBoneComponent.color = glm::vec4(1, 1, 0, 1);
						debugBoneComponent.debugBones = MAKE_SARRAY(mMallocArena, r2::draw::DebugBone, r2::sarr::Size(*skeletalAnimationComponent.animModel->boneInfo));
						r2::sarr::Clear(*debugBoneComponent.debugBones);
						mComponentAllocations.push_back(debugBoneComponent.debugBones);

						MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::DebugBoneComponent>(e, debugBoneComponent);
#endif
					}

					if (MENG.GetECSWorld().GetECSCoordinator()->HasComponent<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(e)
#ifdef R2_DEBUG
						&& !MENG.GetECSWorld().GetECSCoordinator()->HasComponent<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(e)
#endif
						)
					{
#ifdef R2_DEBUG
						const ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>& instancedSkeletalAnimationComponent = MENG.GetECSWorld().GetECSCoordinator()->GetComponent<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(e);

						const auto numInstances = instancedSkeletalAnimationComponent.numInstances;

						ecs::InstanceComponentT<ecs::DebugBoneComponent> instancedDebugBoneComponent;
						instancedDebugBoneComponent.numInstances = numInstances;
						instancedDebugBoneComponent.instances = MAKE_SARRAY(mMallocArena, ecs::DebugBoneComponent, numInstances);
						mComponentAllocations.push_back(instancedDebugBoneComponent.instances);

						for (u32 j = 0; j < numInstances; ++j)
						{
							const auto numDebugBones = r2::sarr::Size(*r2::sarr::At(*instancedSkeletalAnimationComponent.instances, j).animModel->boneInfo);

							ecs::DebugBoneComponent debugBoneInstance1;
							debugBoneInstance1.color = glm::vec4(1, 1, 0, 1);
							debugBoneInstance1.debugBones = MAKE_SARRAY(mMallocArena, r2::draw::DebugBone, numDebugBones);
							r2::sarr::Clear(*debugBoneInstance1.debugBones);
							mComponentAllocations.push_back(debugBoneInstance1.debugBones);

							r2::sarr::Push(*instancedDebugBoneComponent.instances, debugBoneInstance1);
						}

						MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(e, instancedDebugBoneComponent);
#endif
					}

				}


				return e.ShouldConsume();
			});


		for (const auto& widget : mEditorWidgets)
		{
			widget->OnEvent(e);
		}
	}

	SceneGraph& Editor::GetSceneGraph()
	{
		return CENG.GetLevelManager().GetSceneGraph();
	}

	SceneGraph* Editor::GetSceneGraphPtr()
	{
		return CENG.GetLevelManager().GetSceneGraphPtr();
	}

	ecs::ECSCoordinator* Editor::GetECSCoordinator()
	{
		return MENG.GetECSWorld().GetECSCoordinator();
	}

	r2::mem::MallocArena& Editor::GetMemoryArena()
	{
		return mMallocArena;
	}
}

#endif