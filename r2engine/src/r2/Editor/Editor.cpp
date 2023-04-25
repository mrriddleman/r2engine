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
	constexpr u32 MAX_LEVELS = 1000;
	constexpr u32 LEVEL_CACHE_SIZE = Megabytes(1);

}

namespace r2
{
	Editor::Editor()
		:mMallocArena(r2::mem::utils::MemBoundary())
		,mCoordinator(nullptr)
		,mnoptrRenderSystem(nullptr)
		,mnoptrSkeletalAnimationSystem(nullptr)
#ifdef R2_DEBUG
		,mnoptrDebugBonesRenderSystem(nullptr)
		,mnoptrDebugRenderSystem(nullptr)
#endif // DEBUG
		,moptrLevelCache(nullptr)
		,mLevelData(nullptr)
		, mLevelHandle{}
		, microbatAnimModel(nullptr)
	{

	}

	void Editor::Init()
	{
		mCoordinator = ALLOC(ecs::ECSCoordinator, mMallocArena);

		mCoordinator->Init<mem::MallocArena>(mMallocArena, ecs::MAX_NUM_COMPONENTS, ecs::MAX_NUM_ENTITIES, 1, ecs::MAX_NUM_SYSTEMS);

		RegisterComponents();
		RegisterSystems();
		
		mRandom.Randomize();

		//@Temporary
		r2::mem::utils::MemoryProperties memProps;
		memProps.alignment = 16;
		memProps.boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
		memProps.headerSize = r2::mem::MallocAllocator::HeaderSize();

		const auto size = r2::lvlche::MemorySize(MAX_LEVELS, LEVEL_CACHE_SIZE, memProps);
		r2::mem::utils::MemBoundary levelCacheBoundary = MAKE_MEMORY_BOUNDARY_VERBOSE(mMallocArena, size, memProps.alignment, "Level Cache Memory Boundary");
		moptrLevelCache = r2::lvlche::CreateLevelCache(levelCacheBoundary, CENG.GetApplication().GetLevelPackDataBinPath().c_str(), MAX_LEVELS, LEVEL_CACHE_SIZE);



		mSceneGraph.Init<mem::MallocArena>(mMallocArena, mCoordinator);

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
			r2::draw::ModelSystem* editorModelSystem = CENG.GetApplication().GetEditorModelSystem();
			r2::draw::modlsys::ReturnAnimModel(editorModelSystem, microbatAnimModel);

		}

		mSceneGraph.Shutdown<mem::MallocArena>(mMallocArena);

		if (!r2::asset::IsInvalidAssetHandle(mLevelHandle))
		{
			r2::lvlche::UnloadLevelData(*moptrLevelCache, mLevelHandle);
		}
		
		r2::mem::utils::MemBoundary boundary = moptrLevelCache->mLevelCacheBoundary;
		r2::lvlche::Shutdown(moptrLevelCache);

		FREE(boundary.location, mMallocArena);

		UnRegisterSystems();
		UnRegisterComponents();

		for (auto iter = mComponentAllocations.rbegin(); iter != mComponentAllocations.rend(); ++iter)
		{
			FREE(*iter, mMallocArena);
		}

		mCoordinator->Shutdown<mem::MallocArena>(mMallocArena);

		FREE(mCoordinator, mMallocArena);
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
		mSceneGraph.Update();
		mnoptrSkeletalAnimationSystem->Update();

		for (const auto& widget : mEditorWidgets)
		{
			widget->Update();
		}
	}

	void Editor::Render()
	{
		mnoptrRenderSystem->Render();

#ifdef R2_DEBUG
		mnoptrDebugRenderSystem->Render();
		mnoptrDebugBonesRenderSystem->Render();
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

		r2::lvlche::SaveNewLevelFile(*moptrLevelCache, mSceneGraph.GetECSCoordinator(), 1, (levelDataBinPath / levelBinURI).string().c_str(), (levelDataRawPath / levelRawURI).string().c_str());

		//r2::asset::pln::SaveLevelData(mCoordinator, 1, (levelDataBinPath / levelBinURI).string(), (levelDataRawPath / levelRawURI).string());
	}

	void Editor::LoadLevel(const std::string& filePathName, const std::string& parentDirectory)
	{
		char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(filePathName.c_str(), sanitizedPath);

		char levelAssetName[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::CopyFileNameWithParentDirectories(sanitizedPath, levelAssetName, 1);

		mLevelHandle = r2::lvlche::LoadLevelData(*moptrLevelCache, levelAssetName);

		mLevelData = r2::lvlche::GetLevelData(*moptrLevelCache, mLevelHandle);

		R2_CHECK(mLevelData != nullptr, "Level Data is nullptr");

		r2::Level newLevel;
		newLevel.Init(mLevelData, mLevelHandle);

		mSceneGraph.LoadedNewLevel(newLevel);

	//	R2_CHECK(false, "Currently this is not enough - we need to actually load/get the data needed from each type of component, example: Render component needs to be filled out with the gpu handle");
		//@TODO(Serge): figure out how to do this in a nice way

		evt::EditorLevelLoadedEvent e(newLevel);

		PostEditorEvent(e);

		//printf("FilePathName: %s\n", filePathName.c_str());
		//printf("FilePath: %s\n", parentDirectory.c_str());
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

				r2::draw::ModelSystem* editorModelSystem = CENG.GetApplication().GetEditorModelSystem();

				r2::draw::AnimationCache* editorAnimationCache = CENG.GetApplication().GetEditorAnimationCache();

				r2::asset::Asset microbatAsset = r2::asset::Asset("micro_bat.rmdl", r2::asset::RMODEL);

				r2::draw::ModelHandle microbatModelHandle = r2::draw::modlsys::LoadModel(editorModelSystem, microbatAsset);

				microbatAnimModel = r2::draw::modlsys::GetAnimModel(editorModelSystem, microbatModelHandle);

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

				mCoordinator->AddComponent<ecs::SkeletalAnimationComponent>(theNewEntity, skeletalAnimationComponent);
				mCoordinator->AddComponent<ecs::DebugBoneComponent>(theNewEntity, debugBoneComponent);
				mCoordinator->AddComponent<ecs::RenderComponent>(theNewEntity, renderComponent);
				CENG.GetApplication().AddComponentsToEntity(mCoordinator, theNewEntity);

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

					mCoordinator->AddComponent<ecs::InstanceComponentT<ecs::TransformComponent>>(theNewEntity, instancedTransformComponent);
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

					mCoordinator->AddComponent<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(theNewEntity, instancedDebugBoneComponent);
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

					mCoordinator->AddComponent<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(theNewEntity, instancedSkeletalAnimationComponent);
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

					mCoordinator->AddComponent<ecs::DebugRenderComponent>(theNewEntity, debugRenderComponent);
					mCoordinator->AddComponent<ecs::InstanceComponentT<ecs::DebugRenderComponent>>(theNewEntity, instancedDebugRenderComponent);
				}

				ecs::TransformComponent& transformComponent = mCoordinator->GetComponent<ecs::TransformComponent>(theNewEntity);
				transformComponent.localTransform.position = glm::vec3(0, 0, 2);
				transformComponent.localTransform.scale = glm::vec3(0.01f);
				
				transformComponent.localTransform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));
				

			return e.ShouldConsume();
		});

		//@TODO(Serge): remove when DebugBoneComponent no long necessary for Skeletal Animation
		dispatcher.Dispatch<r2::evt::EditorLevelLoadedEvent>([this](const r2::evt::EditorLevelLoadedEvent& e)
			{
				const r2::SArray<ecs::Entity>& allEntities = mCoordinator->GetAllLivingEntities();

				const auto numEntities = r2::sarr::Size(allEntities);

				for (u64 i = 0; i < numEntities; ++i)
				{
					ecs::Entity e = r2::sarr::At(allEntities, i);

					if (mCoordinator->HasComponent<ecs::SkeletalAnimationComponent>(e) 
#ifdef R2_DEBUG
						&& !mCoordinator->HasComponent<ecs::DebugBoneComponent>(e)
#endif		
						)
					{
#ifdef R2_DEBUG
						const ecs::SkeletalAnimationComponent& skeletalAnimationComponent = mCoordinator->GetComponent<ecs::SkeletalAnimationComponent>(e);

						ecs::DebugBoneComponent debugBoneComponent;
						debugBoneComponent.color = glm::vec4(1, 1, 0, 1);
						debugBoneComponent.debugBones = MAKE_SARRAY(mMallocArena, r2::draw::DebugBone, r2::sarr::Size(*skeletalAnimationComponent.animModel->boneInfo));
						r2::sarr::Clear(*debugBoneComponent.debugBones);
						mComponentAllocations.push_back(debugBoneComponent.debugBones);

						mCoordinator->AddComponent<ecs::DebugBoneComponent>(e, debugBoneComponent);
#endif
					}

					if (mCoordinator->HasComponent<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(e) 
#ifdef R2_DEBUG
						&& !mCoordinator->HasComponent<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(e)
#endif
						)
					{
#ifdef R2_DEBUG
						const ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>& instancedSkeletalAnimationComponent = mCoordinator->GetComponent<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(e);

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

						mCoordinator->AddComponent<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(e, instancedDebugBoneComponent);
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
		return mSceneGraph;
	}

	SceneGraph* Editor::GetSceneGraphPtr()
	{
		return &mSceneGraph;
	}

	ecs::ECSCoordinator* Editor::GetECSCoordinator()
	{
		return mCoordinator;
	}

	r2::mem::MallocArena& Editor::GetMemoryArena()
	{
		return mMallocArena;
	}

	void* Editor::HydrateRenderComponents(void* data)
	{
		r2::SArray<ecs::RenderComponent>* tempRenderComponents = static_cast<r2::SArray<ecs::RenderComponent>*>(data);

		const auto numRenderComponents = r2::sarr::Size(*tempRenderComponents);
		
		r2::draw::ModelSystem* editorModelSystem = CENG.GetApplication().GetEditorModelSystem();
		r2::draw::MaterialSystem* editorMaterialSystem = CENG.GetApplication().GetEditorMaterialSystem();

		for (u32 i = 0; i < numRenderComponents; ++i)
		{
			ecs::RenderComponent& renderComponent = r2::sarr::At(*tempRenderComponents, i);

			r2::asset::Asset modelAsset = r2::asset::Asset(renderComponent.assetModelHash, r2::asset::RMODEL);

			r2::draw::ModelHandle modelHandle = r2::draw::modlsys::LoadModel(editorModelSystem, modelAsset);

			r2::draw::vb::GPUModelRefHandle gpuModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
			if (renderComponent.isAnimated)
			{
				const r2::draw::AnimModel* animModel = r2::draw::modlsys::GetAnimModel(editorModelSystem, modelHandle);
				gpuModelRefHandle = r2::draw::renderer::UploadAnimModel(animModel);
				r2::draw::modlsys::ReturnAnimModel(editorModelSystem, animModel);
			}
			else
			{
				const r2::draw::Model* model = r2::draw::modlsys::GetModel(editorModelSystem, modelHandle);
				gpuModelRefHandle = r2::draw::renderer::UploadModel(model);
				r2::draw::modlsys::ReturnModel(editorModelSystem, model);
			}

			renderComponent.gpuModelRefHandle = gpuModelRefHandle;

			if (renderComponent.optrMaterialOverrideNames)
			{
				const auto numMaterialOverrides = r2::sarr::Size(*renderComponent.optrMaterialOverrideNames);

				if (numMaterialOverrides > 0)
				{
					renderComponent.optrOverrideMaterials = MAKE_SARRAY(mMallocArena, r2::draw::MaterialHandle, numMaterialOverrides);
					mComponentAllocations.push_back(renderComponent.optrMaterialOverrideNames);

					for (u32 j = 0; j < numMaterialOverrides; ++j)
					{
						const ecs::RenderMaterialOverride& materialOverride = r2::sarr::At(*renderComponent.optrMaterialOverrideNames, j);

						const r2::draw::MaterialSystem* materialSystem = r2::draw::matsys::GetMaterialSystemBySystemName(materialOverride.materialSystemName);

						R2_CHECK(materialSystem == editorMaterialSystem, "Just a check to make sure these are the same");

						r2::draw::MaterialHandle nextOverrideMaterialHandle = r2::draw::mat::GetMaterialHandleFromMaterialName(*materialSystem, materialOverride.materialName);

						r2::sarr::Push(*renderComponent.optrOverrideMaterials, nextOverrideMaterialHandle);
					}
				}
			}
		}

		return tempRenderComponents;
	}

	void* Editor::HydrateSkeletalAnimationComponents(void* data)
	{
		r2::SArray<ecs::SkeletalAnimationComponent>* tempSkeletalAnimationComponents = static_cast<r2::SArray<ecs::SkeletalAnimationComponent>*>(data);

		if (!tempSkeletalAnimationComponents)
		{
			return nullptr;
		}

		r2::draw::ModelSystem* editorModelSystem = CENG.GetApplication().GetEditorModelSystem();
		r2::draw::AnimationCache* editorAnimationCache = CENG.GetApplication().GetEditorAnimationCache();

		const auto numSkeletalAnimationComponents = r2::sarr::Size(*tempSkeletalAnimationComponents);

		for (u32 i = 0; i < numSkeletalAnimationComponents; ++i)
		{
			ecs::SkeletalAnimationComponent& skeletalAnimationComponent = r2::sarr::At(*tempSkeletalAnimationComponents, i);

			r2::asset::Asset modelAsset = r2::asset::Asset(skeletalAnimationComponent.animModelAssetName, r2::asset::RMODEL);

			r2::draw::ModelHandle modelHandle = r2::draw::modlsys::LoadModel(editorModelSystem, modelAsset);

			const r2::draw::AnimModel* animModel = r2::draw::modlsys::GetAnimModel(editorModelSystem, modelHandle);

			skeletalAnimationComponent.animModel = animModel;


			r2::asset::Asset animationAsset = r2::asset::Asset(skeletalAnimationComponent.startingAnimationAssetName, r2::asset::RANIMATION);

			r2::draw::AnimationHandle animationHandle = r2::draw::animcache::LoadAnimation(*editorAnimationCache, animationAsset);
			
			skeletalAnimationComponent.animation = r2::draw::animcache::GetAnimation(*editorAnimationCache, animationHandle);

			skeletalAnimationComponent.shaderBones = MAKE_SARRAY(mMallocArena, r2::draw::ShaderBoneTransform, r2::sarr::Size(*animModel->boneInfo));
			mComponentAllocations.push_back(skeletalAnimationComponent.shaderBones);

			r2::sarr::Clear(*skeletalAnimationComponent.shaderBones);
		}

		return tempSkeletalAnimationComponents;
	}

	void* Editor::HydrateInstancedSkeletalAnimationComponents(void* data)
	{
		r2::SArray<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>* tempInstancedSkeletalAnimationComponents = static_cast<r2::SArray<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>*>(data);
		if (!tempInstancedSkeletalAnimationComponents)
		{
			return nullptr;
		}

		//@TODO(Serge): technically, we don't need to allocate this at all since the component array has the memory for it - figure out a way to not allocate the array again
		r2::SArray<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>* instancedSkeletalAnimationComponents = nullptr;

		instancedSkeletalAnimationComponents = MAKE_SARRAY(mMallocArena, ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>, r2::sarr::Size(*tempInstancedSkeletalAnimationComponents));
		mComponentAllocations.push_back(instancedSkeletalAnimationComponents);

		r2::draw::ModelSystem* editorModelSystem = CENG.GetApplication().GetEditorModelSystem();
		r2::draw::AnimationCache* editorAnimationCache = CENG.GetApplication().GetEditorAnimationCache();

		const auto numSkeletalAnimationComponents = r2::sarr::Size(*tempInstancedSkeletalAnimationComponents);

		for (u32 i = 0; i < numSkeletalAnimationComponents; ++i)
		{
			const ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>& tempInstancedSkeletalAnimationComponent = r2::sarr::At(*tempInstancedSkeletalAnimationComponents, i);

			ecs::InstanceComponentT<ecs::SkeletalAnimationComponent> instancedSkeletalAnimationComponent;
			instancedSkeletalAnimationComponent.numInstances = tempInstancedSkeletalAnimationComponent.numInstances;

			instancedSkeletalAnimationComponent.instances = MAKE_SARRAY(mMallocArena, ecs::SkeletalAnimationComponent, tempInstancedSkeletalAnimationComponent.numInstances);
			mComponentAllocations.push_back(instancedSkeletalAnimationComponent.instances);


			for (u32 j = 0; j < tempInstancedSkeletalAnimationComponent.numInstances; ++j)
			{
				const ecs::SkeletalAnimationComponent& tempSkeletalAnimationComponent = r2::sarr::At(*tempInstancedSkeletalAnimationComponent.instances, j);

				ecs::SkeletalAnimationComponent skeletalAnimationComponent;
				skeletalAnimationComponent.animModelAssetName = tempSkeletalAnimationComponent.animModelAssetName;
				skeletalAnimationComponent.shouldLoop = tempSkeletalAnimationComponent.shouldLoop;
				skeletalAnimationComponent.shouldUseSameTransformsForAllInstances = tempSkeletalAnimationComponent.shouldUseSameTransformsForAllInstances;
				skeletalAnimationComponent.startingAnimationAssetName = tempSkeletalAnimationComponent.startingAnimationAssetName;
				skeletalAnimationComponent.startTime = tempSkeletalAnimationComponent.startTime;


				r2::asset::Asset modelAsset = r2::asset::Asset(skeletalAnimationComponent.animModelAssetName, r2::asset::RMODEL);

				r2::draw::ModelHandle modelHandle = r2::draw::modlsys::LoadModel(editorModelSystem, modelAsset);

				const r2::draw::AnimModel* animModel = r2::draw::modlsys::GetAnimModel(editorModelSystem, modelHandle);

				skeletalAnimationComponent.animModel = animModel;

				r2::asset::Asset animationAsset = r2::asset::Asset(skeletalAnimationComponent.startingAnimationAssetName, r2::asset::RANIMATION);

				r2::draw::AnimationHandle animationHandle = r2::draw::animcache::LoadAnimation(*editorAnimationCache, animationAsset);

				skeletalAnimationComponent.animation = r2::draw::animcache::GetAnimation(*editorAnimationCache, animationHandle);


				skeletalAnimationComponent.shaderBones = MAKE_SARRAY(mMallocArena, r2::draw::ShaderBoneTransform, r2::sarr::Size(*animModel->boneInfo));
				mComponentAllocations.push_back(skeletalAnimationComponent.shaderBones);

				r2::sarr::Clear(*skeletalAnimationComponent.shaderBones);

				r2::sarr::Push(*instancedSkeletalAnimationComponent.instances, skeletalAnimationComponent);
			}

			r2::sarr::Push(*instancedSkeletalAnimationComponents, instancedSkeletalAnimationComponent);

		}

		return instancedSkeletalAnimationComponents;
	}

	void* Editor::HydrateInstancedTransformComponents(void* data)
	{
		r2::SArray<ecs::InstanceComponentT<ecs::TransformComponent>>* tempInstancedTransformComponents = static_cast<r2::SArray<ecs::InstanceComponentT<ecs::TransformComponent>>*>(data);
		if (!tempInstancedTransformComponents)
		{
			return nullptr;
		}

		r2::SArray<ecs::InstanceComponentT<ecs::TransformComponent>>* instancedTransformComponents = nullptr;

		instancedTransformComponents = MAKE_SARRAY(mMallocArena, ecs::InstanceComponentT<ecs::TransformComponent> , r2::sarr::Size(*tempInstancedTransformComponents));
		mComponentAllocations.push_back(instancedTransformComponents);

		for (u32 i = 0; i < r2::sarr::Size(*tempInstancedTransformComponents); ++i)
		{
			const ecs::InstanceComponentT<ecs::TransformComponent>& tempInstancedTransformComponent = r2::sarr::At(*tempInstancedTransformComponents, i);

			ecs::InstanceComponentT<ecs::TransformComponent> instancedTransformComponent;

			instancedTransformComponent.numInstances = tempInstancedTransformComponent.numInstances;
			instancedTransformComponent.instances = MAKE_SARRAY(mMallocArena, ecs::TransformComponent, tempInstancedTransformComponent.numInstances);
			mComponentAllocations.push_back(instancedTransformComponent.instances);

			r2::sarr::Copy(*instancedTransformComponent.instances, *tempInstancedTransformComponent.instances);

			r2::sarr::Push(*instancedTransformComponents, instancedTransformComponent);
		}

		return instancedTransformComponents;
	}

	void Editor::RegisterComponents()
	{
		ecs::ComponentArrayHydrationFunction renderComponentHydrationFunc = std::bind(&Editor::HydrateRenderComponents, this, std::placeholders::_1);
		ecs::ComponentArrayHydrationFunction skeletalAnimationComponentHydrationFunc = std::bind(&Editor::HydrateSkeletalAnimationComponents, this, std::placeholders::_1);
		ecs::ComponentArrayHydrationFunction instancedTransformComponentHydrationFunc = std::bind(&Editor::HydrateInstancedTransformComponents, this, std::placeholders::_1);
		ecs::ComponentArrayHydrationFunction instancedSkeletalAnimationComponentHydrationFunc = std::bind(&Editor::HydrateInstancedSkeletalAnimationComponents, this, std::placeholders::_1);

		mCoordinator->RegisterComponent<mem::MallocArena, ecs::HeirarchyComponent>(mMallocArena, "HeirarchyComponent",true, nullptr);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::TransformComponent>(mMallocArena, "TransformComponent", true, nullptr);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::TransformDirtyComponent>(mMallocArena, "TransformDirtyComponent", false, nullptr);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::RenderComponent>(mMallocArena, "RenderComponent", true, renderComponentHydrationFunc);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::SkeletalAnimationComponent>(mMallocArena, "SkeletalAnimationComponent", true, skeletalAnimationComponentHydrationFunc);
		
		//add some more components to the coordinator for the editor to use
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::EditorComponent>(mMallocArena, "EditorComponent", true, nullptr);

		mCoordinator->RegisterComponent<mem::MallocArena, ecs::InstanceComponentT<ecs::TransformComponent>>(mMallocArena, "InstancedTranfromComponent", true, instancedTransformComponentHydrationFunc);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(mMallocArena, "InstancedSkeletalAnimationComponent", true, instancedSkeletalAnimationComponentHydrationFunc);

#ifdef R2_DEBUG
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::DebugRenderComponent>(mMallocArena, "DebugRenderComponent", false, nullptr);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::DebugBoneComponent>(mMallocArena, "DebugBoneComponent", false, nullptr);

		mCoordinator->RegisterComponent<mem::MallocArena, ecs::InstanceComponentT<ecs::DebugRenderComponent>>(mMallocArena, "InstancedDebugRenderComponent", false, nullptr);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::InstanceComponentT<ecs::DebugBoneComponent>>(mMallocArena, "InstancedDebugBoneComponent", false, nullptr);
#endif

		CENG.GetApplication().RegisterComponents(mCoordinator);
	}

	void Editor::UnRegisterComponents()
	{
		CENG.GetApplication().UnRegisterComponents(mCoordinator);

#ifdef R2_DEBUG
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::InstanceComponentT<ecs::DebugBoneComponent>>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::InstanceComponentT<ecs::DebugRenderComponent>>(mMallocArena);

		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::DebugBoneComponent>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::DebugRenderComponent>(mMallocArena);
#endif

		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::InstanceComponentT<ecs::TransformComponent>>(mMallocArena);

		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::EditorComponent>(mMallocArena);

		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::SkeletalAnimationComponent>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::RenderComponent>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::TransformDirtyComponent>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::TransformComponent>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::HeirarchyComponent>(mMallocArena);
	}

	void Editor::RegisterSystems()
	{
		const auto transformComponentType = mCoordinator->GetComponentType<ecs::TransformComponent>();
		const auto renderComponentType = mCoordinator->GetComponentType<ecs::RenderComponent>();
		const auto skeletalAnimationComponentType = mCoordinator->GetComponentType<ecs::SkeletalAnimationComponent>();
#ifdef R2_DEBUG
		const auto debugRenderComponentType = mCoordinator->GetComponentType<ecs::DebugRenderComponent>();
		const auto debugBoneComponentType = mCoordinator->GetComponentType<ecs::DebugBoneComponent>();
#endif

		mnoptrRenderSystem = (ecs::RenderSystem*)mCoordinator->RegisterSystem<mem::MallocArena, ecs::RenderSystem>(mMallocArena);
		ecs::Signature renderSystemSignature;
		renderSystemSignature.set(transformComponentType);
		renderSystemSignature.set(renderComponentType);
		mCoordinator->SetSystemSignature<ecs::RenderSystem>(renderSystemSignature);

		//@TODO(Serge): Init render system here
		u32 maxNumModels = r2::draw::renderer::GetMaxNumModelsLoadedAtOneTimePerLayout();
		u32 avgMaxNumMeshesPerModel = r2::draw::renderer::GetAVGMaxNumMeshesPerModel();
		u32 avgMaxNumInstancesPerModel = r2::draw::renderer::GetMaxNumInstancesPerModel();
		u32 avgMaxNumBonesPerModel = r2::draw::renderer::GetAVGMaxNumBonesPerModel();

		r2::mem::utils::MemoryProperties memSizeStruct;
		memSizeStruct.alignment = 16;
		memSizeStruct.headerSize = mMallocArena.HeaderSize();

		memSizeStruct.boundsChecking = 0;
#ifdef R2_DEBUG
		memSizeStruct.boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		mnoptrRenderSystem->Init<mem::MallocArena>(mMallocArena, MAX_NUM_STATIC_BATCHES, MAX_NUM_DYNAMIC_BATCHES, maxNumModels, maxNumModels, avgMaxNumInstancesPerModel, avgMaxNumMeshesPerModel, avgMaxNumBonesPerModel);

		mnoptrSkeletalAnimationSystem = (ecs::SkeletalAnimationSystem*)mCoordinator->RegisterSystem<mem::MallocArena, ecs::SkeletalAnimationSystem>(mMallocArena);

		ecs::Signature skeletalAnimationSystemSignature;
		skeletalAnimationSystemSignature.set(skeletalAnimationComponentType);
#ifdef R2_DEBUG
		skeletalAnimationSystemSignature.set(debugBoneComponentType);
#endif
		mCoordinator->SetSystemSignature<ecs::SkeletalAnimationSystem>(skeletalAnimationSystemSignature);

#ifdef R2_DEBUG
		mnoptrDebugBonesRenderSystem = (ecs::DebugBonesRenderSystem*)mCoordinator->RegisterSystem<mem::MallocArena, ecs::DebugBonesRenderSystem>(mMallocArena);
		ecs::Signature debugBonesSystemSignature;
		debugBonesSystemSignature.set(debugBoneComponentType);
		debugBonesSystemSignature.set(transformComponentType);

		mCoordinator->SetSystemSignature<ecs::DebugBonesRenderSystem>(debugBonesSystemSignature);

		mnoptrDebugRenderSystem = (ecs::DebugRenderSystem*)mCoordinator->RegisterSystem<mem::MallocArena, ecs::DebugRenderSystem>(mMallocArena);
		ecs::Signature debugRenderSystemSignature;

		debugRenderSystemSignature.set(debugRenderComponentType);
		debugRenderSystemSignature.set(transformComponentType);

		mCoordinator->SetSystemSignature<ecs::DebugRenderSystem>(debugRenderSystemSignature);
#endif

	}

	void Editor::UnRegisterSystems()
	{
#ifdef R2_DEBUG
		mCoordinator->UnRegisterSystem<mem::MallocArena, ecs::DebugRenderSystem>(mMallocArena);
		mCoordinator->UnRegisterSystem<mem::MallocArena, ecs::DebugBonesRenderSystem>(mMallocArena);
#endif
		mCoordinator->UnRegisterSystem<mem::MallocArena, ecs::SkeletalAnimationSystem>(mMallocArena);

		mnoptrRenderSystem->Shutdown<mem::MallocArena>(mMallocArena);

		mCoordinator->UnRegisterSystem<mem::MallocArena, ecs::RenderSystem>(mMallocArena);
	}
}

#endif