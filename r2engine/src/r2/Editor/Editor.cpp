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

namespace 
{
	//Figure out render system defaults
	constexpr u32 MAX_NUM_STATIC_BATCHES = 32;
	constexpr u32 MAX_NUM_DYNAMIC_BATCHES = 32;
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
	{

	}

	void Editor::Init()
	{
		mCoordinator = ALLOC(ecs::ECSCoordinator, mMallocArena);

		mCoordinator->Init<mem::MallocArena>(mMallocArena, ecs::MAX_NUM_COMPONENTS, ecs::MAX_NUM_ENTITIES, 1, ecs::MAX_NUM_SYSTEMS);

		RegisterComponents();
		RegisterSystems();
		
		mRandom.Randomize();

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
	}

	void Editor::Shutdown()
	{
		for (const auto& widget : mEditorWidgets)
		{
			widget->Shutdown();
		}

		mEditorWidgets.clear();

		

		mSceneGraph.Shutdown<mem::MallocArena>(mMallocArena);


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
		printf("Save!\n");
	}

	void Editor::PostEditorEvent(r2::evt::EditorEvent& e)
	{
		//@TODO(Serge): listen to the entity creation event and add in the appropriate components for testing
		//@NOTE: all test code!				
		r2::evt::EventDispatcher dispatcher(e);
		dispatcher.Dispatch<r2::evt::EditorEntityCreatedEvent>([this](const r2::evt::EditorEntityCreatedEvent& e)
			{
				/*r2::draw::DefaultModel modelType = (r2::draw::DefaultModel)mRandom.RandomNum(r2::draw::QUAD, r2::draw::CYLINDER);

				r2::draw::vb::GPUModelRefHandle gpuModelRefHandle = r2::draw::renderer::GetDefaultModelRef(r2::draw::QUAD);

				ecs::RenderComponent renderComponent;
				renderComponent.optrOverrideMaterials = nullptr;
				renderComponent.gpuModelRefHandle = gpuModelRefHandle;
				renderComponent.primitiveType = draw::PrimitiveType::TRIANGLES;
				renderComponent.drawParameters.layer = r2::draw::DL_WORLD;
				renderComponent.drawParameters.flags.Clear();
				renderComponent.drawParameters.flags.Set(r2::draw::eDrawFlags::DEPTH_TEST);

				r2::draw::renderer::SetDefaultCullState(renderComponent.drawParameters);
				r2::draw::renderer::SetDefaultStencilState(renderComponent.drawParameters);
				r2::draw::renderer::SetDefaultBlendState(renderComponent.drawParameters);*/

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

				ecs::DebugRenderComponent debugRenderComponent;
				debugRenderComponent.colors = nullptr;
				debugRenderComponent.directions = nullptr;
				debugRenderComponent.radii = nullptr;
				debugRenderComponent.scales = nullptr;

				debugRenderComponent.debugModelType = draw::DEBUG_LINE;

				//don't love this
				debugRenderComponent.radii = MAKE_SARRAY(mMallocArena, float, 1);
				r2::sarr::Push(*debugRenderComponent.radii, 0.1f);
				mComponentAllocations.push_back(debugRenderComponent.radii);

				debugRenderComponent.scales = MAKE_SARRAY(mMallocArena, float, 1);
				r2::sarr::Push(*debugRenderComponent.scales, 1.0f);
				mComponentAllocations.push_back(debugRenderComponent.scales);

				debugRenderComponent.directions = MAKE_SARRAY(mMallocArena, glm::vec3, 1);
				r2::sarr::Push(*debugRenderComponent.directions, glm::vec3(1, 1, 1));
				mComponentAllocations.push_back(debugRenderComponent.directions);

				debugRenderComponent.colors = MAKE_SARRAY(mMallocArena, glm::vec4, 1);
				r2::sarr::Push(*debugRenderComponent.colors, glm::vec4(0, 1, 0, 1));
				mComponentAllocations.push_back(debugRenderComponent.colors);

				debugRenderComponent.depthTest = true;
				debugRenderComponent.filled = true;


				ecs::Entity theNewEntity = e.GetEntity();

				mCoordinator->AddComponent<ecs::DebugRenderComponent>(theNewEntity, debugRenderComponent);

				ecs::TransformComponent& transformComponent = mCoordinator->GetComponent<ecs::TransformComponent>(theNewEntity);
				transformComponent.localTransform.position = glm::vec3(0, 0, 2);
				//transformComponent.localTransform.rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(1, 0, 0));

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

	void Editor::RegisterComponents()
	{
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::HeirarchyComponent>(mMallocArena);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::TransformComponent>(mMallocArena);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::TransformDirtyComponent>(mMallocArena);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::InstanceComponent>(mMallocArena);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::RenderComponent>(mMallocArena);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::SkeletalAnimationComponent>(mMallocArena);

		//add some more components to the coordinator for the editor to use
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::EditorComponent>(mMallocArena);

#ifdef R2_DEBUG
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::DebugRenderComponent>(mMallocArena);
		mCoordinator->RegisterComponent<mem::MallocArena, ecs::DebugBoneComponent>(mMallocArena);
#endif
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

		r2::mem::utils::MemorySizeStruct memSizeStruct;
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

	void Editor::UnRegisterComponents()
	{
#ifdef R2_DEBUG
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::DebugBoneComponent>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::DebugRenderComponent>(mMallocArena);
#endif

		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::EditorComponent>(mMallocArena);

		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::SkeletalAnimationComponent>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::RenderComponent>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::InstanceComponent>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::TransformDirtyComponent>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::TransformComponent>(mMallocArena);
		mCoordinator->UnRegisterComponent<mem::MallocArena, ecs::HeirarchyComponent>(mMallocArena);
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