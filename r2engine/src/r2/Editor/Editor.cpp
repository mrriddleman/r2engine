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
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/AudioListenerComponent.h"
#include "r2/Game/ECS/Components/AudioEmitterComponent.h"
#include "r2/Game/ECS/Components/AudioParameterComponent.h"
#include "r2/Game/ECS/Components/AudioEmitterActionComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "r2/Editor/EditorEvents/EditorLevelEvents.h"



#ifdef R2_DEBUG
#include "r2/Game/ECS/Components/DebugRenderComponent.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#endif
#include "imgui.h"

#include "r2/Core/Engine.h"
#include "r2/Game/ECSWorld/ECSWorld.h"
#include "r2/Game/Level/LevelManager.h"
#include "r2/Game/SceneGraph/SceneGraph.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"

//#include "r2/Render/Model/Materials/MaterialParamsPackHelpers.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Render/Renderer/Renderer.h"

//@TEST: for test code only - REMOVE!
#include "r2/Core/Application.h"
#include "r2/Core/File/PathUtils.h"

namespace r2
{
	Editor::Editor()
		:mCurrentEditorLevel(nullptr)
	{
		
	}

	void Editor::Init()
	{
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
		
		CreateNewLevel("NewGroup", "NewLevel");
	}

	void Editor::Shutdown()
	{
		for (const auto& widget : mEditorWidgets)
		{
			widget->Shutdown();
		}

		mEditorWidgets.clear();
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
		for (const auto& widget : mEditorWidgets)
		{
			widget->Update();
		}
	}

	void Editor::Render()
	{

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
		CENG.GetLevelManager().SaveNewLevelFile(*mCurrentEditorLevel);
	}

	void Editor::LoadLevel(const std::string& filePathName)
	{
		if (mCurrentEditorLevel)
		{
			UnloadCurrentLevel();
		}

		LevelName levelAssetName = LevelManager::MakeLevelNameFromPath(filePathName.c_str());

		mCurrentEditorLevel = CENG.GetLevelManager().LoadLevel(levelAssetName);

		evt::EditorLevelLoadedEvent e(mCurrentEditorLevel->GetLevelAssetName());

		PostEditorEvent(e);
	}

	void Editor::UnloadCurrentLevel()
	{
		CreateNewLevel("NewGroup", "NewLevel");
	}

	void Editor::CreateNewLevel(const std::string& groupName, const std::string& levelName)
	{
		if (mCurrentEditorLevel)
		{
			evt::EditorLevelWillUnLoadEvent e(mCurrentEditorLevel->GetLevelAssetName());

			PostEditorEvent(e);

			CENG.GetLevelManager().UnloadLevel(mCurrentEditorLevel);
			mCurrentEditorLevel = nullptr;
		}

		std::filesystem::path levelURI = std::filesystem::path(groupName) / std::filesystem::path(levelName);
		levelURI.replace_extension(".rlvl");

		mCurrentEditorLevel = CENG.GetLevelManager().MakeNewLevel(levelName.c_str(), groupName.c_str(), r2::asset::GetAssetNameForFilePath(levelURI.string().c_str(), r2::asset::LEVEL));
		evt::EditorLevelLoadedEvent e(mCurrentEditorLevel->GetLevelAssetName());

		PostEditorEvent(e);

		//@Temporary
		{
			r2::audio::AudioEngine audioEngine;

			ecs::Entity newEntity = GetSceneGraph().CreateEntity();

			ecs::AudioListenerComponent audioListenerComponent;
			audioListenerComponent.listener = audioEngine.DEFAULT_LISTENER;
			audioListenerComponent.entityToFollow = ecs::INVALID_ENTITY;

			GetECSCoordinator()->AddComponent<ecs::AudioListenerComponent>(newEntity, audioListenerComponent);

			ecs::EditorComponent editorComponent;
			editorComponent.editorName = "Audio Listener";
			editorComponent.flags.Clear();

			GetECSCoordinator()->AddComponent<ecs::EditorComponent>(newEntity, editorComponent);
		}
	}

	void Editor::ReloadLevel()
	{
		if (mCurrentEditorLevel)
		{
			const auto levelAssetName = mCurrentEditorLevel->GetLevelAssetName();
			Level* theLevel = CENG.GetLevelManager().ReloadLevel(levelAssetName);

			if (theLevel)
			{
				mCurrentEditorLevel = theLevel;
			}
		}
	}

	const Level& Editor::GetEditorLevelConst() const
	{
		return *mCurrentEditorLevel;
	}

	Level& Editor::GetEditorLevelRef() 
	{
		return *mCurrentEditorLevel;
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
				GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();
				r2::ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

				r2::asset::Asset microbatAsset = r2::asset::Asset("micro_bat.rmdl", r2::asset::RMODEL);

				r2::draw::ModelHandle microbatModelHandle = gameAssetManager.LoadAsset(microbatAsset);

				auto microbatAnimModel = gameAssetManager.GetAssetDataConst<r2::draw::Model>(microbatModelHandle);

				r2::draw::vb::GPUModelRefHandle gpuModelRefHandle = r2::draw::renderer::UploadModel(microbatAnimModel);

				AddModelToLevel(microbatAsset.HashID(), *microbatAnimModel);

				ecs::RenderComponent renderComponent;
				renderComponent.assetModelName = microbatAsset.HashID();
				renderComponent.optrMaterialOverrideNames = nullptr;
				renderComponent.gpuModelRefHandle = gpuModelRefHandle;
				renderComponent.primitiveType = (u32)draw::PrimitiveType::TRIANGLES;
				renderComponent.isAnimated = true;
				renderComponent.drawParameters.layer = r2::draw::DL_CHARACTER;
				renderComponent.drawParameters.flags.Clear();
				renderComponent.drawParameters.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);

				r2::draw::renderer::SetDefaultCullState(renderComponent.drawParameters);
				r2::draw::renderer::SetDefaultStencilState(renderComponent.drawParameters);
				r2::draw::renderer::SetDefaultBlendState(renderComponent.drawParameters);
				

				ecs::SkeletalAnimationComponent skeletalAnimationComponent;
				skeletalAnimationComponent.animModelAssetName = microbatAsset.HashID();
				skeletalAnimationComponent.animModel = microbatAnimModel;
				skeletalAnimationComponent.shouldUseSameTransformsForAllInstances = renderComponent.drawParameters.flags.IsSet(r2::draw::eDrawFlags::USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES);
				skeletalAnimationComponent.startingAnimationIndex = 0; 
				skeletalAnimationComponent.startTime = 0; 
				skeletalAnimationComponent.shouldLoop = true;
				skeletalAnimationComponent.currentAnimationIndex = skeletalAnimationComponent.startingAnimationIndex;


				skeletalAnimationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, r2::sarr::Size(*skeletalAnimationComponent.animModel->optrBoneInfo));
				r2::sarr::Clear(*skeletalAnimationComponent.shaderBones);

				ecs::DebugBoneComponent debugBoneComponent;
				debugBoneComponent.color = glm::vec4(1, 1, 0, 1);
				debugBoneComponent.debugBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::DebugBone, r2::sarr::Size(*skeletalAnimationComponent.animModel->optrBoneInfo));
				r2::sarr::Clear(*debugBoneComponent.debugBones);

				ecs::Entity theNewEntity = e.GetEntity();

				MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::SkeletalAnimationComponent>(theNewEntity, skeletalAnimationComponent);
				MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::DebugBoneComponent>(theNewEntity, debugBoneComponent);
				MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::RenderComponent>(theNewEntity, renderComponent);
				

				ecs::AudioEmitterComponent audioEmitterComponent;
				r2::util::PathCpy(audioEmitterComponent.eventName, "event:/MyEvent");
				audioEmitterComponent.releaseAfterPlay = true;
				audioEmitterComponent.startCondition = ecs::PLAY_ON_EVENT;
				audioEmitterComponent.allowFadeoutWhenStopping = true;
				audioEmitterComponent.numParameters = 0;

				MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::AudioEmitterComponent>(theNewEntity, audioEmitterComponent);

				//@TEMPORARY!!!! - we would need some other mechanism for adding/loading the bank, probably through the asset catalog tool or something
				AddSoundBankToLevel(r2::asset::GetAssetNameForFilePath("TestBank1.bank", r2::asset::SOUND));

				r2::audio::AudioEngine audioEngine;
				char bankPath[r2::fs::FILE_PATH_LENGTH];
				r2::fs::utils::BuildPathFromCategory(r2::fs::utils::SOUNDS, "TestBank1.bank", bankPath);

				audioEngine.LoadBank(bankPath, 0);



			//	ecs::AudioEmitterActionComponent audioEmitterActionComponent;
			//	audioEmitterActionComponent.action = ecs::AEA_CREATE;

			//	MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::AudioEmitterActionComponent>(theNewEntity, audioEmitterActionComponent);

				//transform instance
				{
					ecs::InstanceComponentT<ecs::TransformComponent> instancedTransformComponent;
					instancedTransformComponent.numInstances = 2;
					instancedTransformComponent.instances = ECS_WORLD_MAKE_SARRAY(ecsWorld, ecs::TransformComponent, 2);

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
					instancedDebugBoneComponent.instances = ECS_WORLD_MAKE_SARRAY(ecsWorld, ecs::DebugBoneComponent, 2);

					ecs::DebugBoneComponent debugBoneInstance1;
					debugBoneInstance1.color = glm::vec4(1, 0, 0, 1);
					debugBoneInstance1.debugBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::DebugBone, r2::sarr::Size(*skeletalAnimationComponent.animModel->optrBoneInfo));
					r2::sarr::Clear(*debugBoneInstance1.debugBones);

					ecs::DebugBoneComponent debugBoneInstance2;
					debugBoneInstance2.color = glm::vec4(1, 0, 1, 1);
					debugBoneInstance2.debugBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::DebugBone, r2::sarr::Size(*skeletalAnimationComponent.animModel->optrBoneInfo));
					r2::sarr::Clear(*debugBoneInstance2.debugBones);

					r2::sarr::Push(*instancedDebugBoneComponent.instances, debugBoneInstance1);
					r2::sarr::Push(*instancedDebugBoneComponent.instances, debugBoneInstance2);

					MENG.GetECSWorld().GetECSCoordinator()->AddComponent<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(theNewEntity, instancedDebugBoneComponent);
				}

				//Skeletal animation instance component
				{
					ecs::InstanceComponentT<ecs::SkeletalAnimationComponent> instancedSkeletalAnimationComponent;
					instancedSkeletalAnimationComponent.numInstances = 2;
					instancedSkeletalAnimationComponent.instances = ECS_WORLD_MAKE_SARRAY(ecsWorld, ecs::SkeletalAnimationComponent, 2);

					ecs::SkeletalAnimationComponent skeletalAnimationInstance1;
					skeletalAnimationInstance1.animModelAssetName = microbatAsset.HashID();
					skeletalAnimationInstance1.animModel = microbatAnimModel;
					skeletalAnimationInstance1.shouldUseSameTransformsForAllInstances = skeletalAnimationComponent.shouldUseSameTransformsForAllInstances;
					skeletalAnimationInstance1.startingAnimationIndex = 1;
					skeletalAnimationInstance1.startTime = 0; 
					skeletalAnimationInstance1.shouldLoop = true;
					
					skeletalAnimationInstance1.currentAnimationIndex = skeletalAnimationInstance1.startingAnimationIndex;

					skeletalAnimationInstance1.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, r2::sarr::Size(*skeletalAnimationInstance1.animModel->optrBoneInfo));
					r2::sarr::Clear(*skeletalAnimationInstance1.shaderBones);



					ecs::SkeletalAnimationComponent skeletalAnimationInstance2;
					skeletalAnimationInstance2.animModelAssetName = microbatAsset.HashID();
					skeletalAnimationInstance2.animModel = microbatAnimModel;
					skeletalAnimationInstance2.shouldUseSameTransformsForAllInstances = skeletalAnimationComponent.shouldUseSameTransformsForAllInstances;
					skeletalAnimationInstance2.startingAnimationIndex = 2;
					skeletalAnimationInstance2.startTime = 0;
					skeletalAnimationInstance2.shouldLoop = true;

					skeletalAnimationInstance2.currentAnimationIndex = skeletalAnimationInstance2.startingAnimationIndex;


					skeletalAnimationInstance2.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, r2::sarr::Size(*skeletalAnimationInstance2.animModel->optrBoneInfo));
					r2::sarr::Clear(*skeletalAnimationInstance2.shaderBones);

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
					debugRenderComponent.offset = glm::vec3(0);
					debugRenderComponent.radius = 1.0f;

					ecs::InstanceComponentT<ecs::DebugRenderComponent> instancedDebugRenderComponent;
					instancedDebugRenderComponent.numInstances = 2;
					instancedDebugRenderComponent.instances = ECS_WORLD_MAKE_SARRAY(ecsWorld, ecs::DebugRenderComponent, 2);

					ecs::DebugRenderComponent debugRenderComponent1;
					debugRenderComponent1.color = glm::vec4(1, 0, 0, 1);
					debugRenderComponent1.debugModelType = draw::DEBUG_QUAD;
					debugRenderComponent1.direction = glm::normalize(glm::vec3(-1, 1, 0.5));
					debugRenderComponent1.depthTest = true;
					debugRenderComponent1.filled = true;
					debugRenderComponent1.scale = glm::vec3(1, 2, 1);
					debugRenderComponent1.offset = glm::vec3(0);
					debugRenderComponent1.radius = 1.0f;


					ecs::DebugRenderComponent debugRenderComponent2;
					debugRenderComponent2.color = glm::vec4(1, 0, 1, 1);
					debugRenderComponent2.debugModelType = draw::DEBUG_QUAD;
					debugRenderComponent2.direction = glm::normalize(glm::vec3(1, -1, -0.5));
					debugRenderComponent2.depthTest = true;
					debugRenderComponent2.filled = true;
					debugRenderComponent2.scale = glm::vec3(2, 1, 1);
					debugRenderComponent2.offset = glm::vec3(0);
					debugRenderComponent2.radius = 1.0f;

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

		for (const auto& widget : mEditorWidgets)
		{
			widget->OnEvent(e);
		}
	}

	ecs::SceneGraph& Editor::GetSceneGraph()
	{
		return MENG.GetECSWorld().GetSceneGraph();
	}

	ecs::ECSCoordinator* Editor::GetECSCoordinator()
	{
		return MENG.GetECSWorld().GetECSCoordinator();
	}

	void Editor::AddModelToLevel(u64 modelHandle, const r2::draw::Model& model)
	{
		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		const auto* assetFile = gameAssetManager.GetAssetFile(r2::asset::Asset(modelHandle, r2::asset::RMODEL));

		auto* modelAssets = mCurrentEditorLevel->GetModelAssets();
		auto* materials = mCurrentEditorLevel->GetMaterials();

		r2::asset::AssetHandle modelAssetHandle = { modelHandle, gameAssetManager.GetAssetCacheSlot() };

		if (r2::sarr::IndexOf(*modelAssets, modelAssetHandle) == -1)
		{
			//@NOTE(Serge): may want to load here - dunno yet
			r2::sarr::Push(*modelAssets, modelAssetHandle);
		}

		const u32 numMaterialNames = r2::sarr::Size(*model.optrMaterialNames);

		for (u32 i = 0; i < numMaterialNames; ++i)
		{
			r2::mat::MaterialName materialName =r2::sarr::At(*model.optrMaterialNames, i);
			if (r2::sarr::IndexOf(*materials, materialName) == -1)
			{
				r2::sarr::Push(*materials, materialName);
			}
		}
	}

	void Editor::AddSoundBankToLevel(u64 soundBankAssetName)
	{
		auto* soundBanks = mCurrentEditorLevel->GetSoundBankAssetNames();
	
		if (r2::sarr::IndexOf(*soundBanks, soundBankAssetName) == -1)
		{
			r2::sarr::Push(*soundBanks, soundBankAssetName);
		}
	}
}

#endif