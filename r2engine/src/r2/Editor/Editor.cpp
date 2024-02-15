#include "r2pch.h"
#if defined(R2_EDITOR) && defined(R2_IMGUI)
#include "r2/Core/Events/Events.h"


#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorMainMenuBar.h"
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/EditorAssetBrowser/EditorAssetPanel.h"
#include "r2/Editor/EditorAssetBrowser/EditorLevelAssetPanel.h"
#include "r2/Editor/EditorScenePanel.h"
#include "r2/Editor/EditorLevelRenderSettingsPanel.h"
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

#include "r2/Render/Backends/SDL_OpenGL/ImGuiImageHelpers.h"
#include "r2/Render/Model/Textures/Texture.h"


#ifdef R2_DEBUG
#include "r2/Game/ECS/Components/DebugRenderComponent.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#endif
#include "imgui.h"

#include "r2/Game/Level/Level.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Engine.h"
#include "r2/Game/ECSWorld/ECSWorld.h"
#include "r2/Game/Level/LevelManager.h"
#include "r2/Game/SceneGraph/SceneGraph.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/Asset.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Core/Assets/AssetReference.h"
//@TEST: for test code only - REMOVE!
#include "r2/Core/Application.h"
#include "r2/Core/File/PathUtils.h"

namespace r2
{
	Editor::Editor()
		:mCurrentEditorLevel(nullptr)
		, mEditorFolderImageWidth(0)
		, mEditorFolderImageHeight(0)
		, mEditorFolderImage(0)
		, mMainMenuBar(nullptr)
		, mShowGrid(false)
	{
		
	}

	void Editor::Init()
	{
		//Do all of the panels/widgets setup here
		std::unique_ptr<edit::MainMenuBar> mainMenuBar = std::make_unique<edit::MainMenuBar>();
		mEditorWidgets.push_back(std::move(mainMenuBar));
		mMainMenuBar = static_cast<edit::MainMenuBar*>( mEditorWidgets.back().get() );

		R2_CHECK(mMainMenuBar != nullptr, "Should never happen");

		std::unique_ptr<edit::LevelRenderSettingsPanel> levelRenderSettingsPanel = std::make_unique<edit::LevelRenderSettingsPanel>();
		mEditorWidgets.push_back(std::move(levelRenderSettingsPanel));

		std::unique_ptr<edit::InspectorPanel> inspectorPanel = std::make_unique<edit::InspectorPanel>();
		mEditorWidgets.push_back(std::move(inspectorPanel));

		std::unique_ptr<edit::AssetPanel> assetPanel = std::make_unique<edit::AssetPanel>();
		mEditorWidgets.push_back(std::move(assetPanel));

		std::unique_ptr<edit::LevelAssetPanel> levelAssetPanel = std::make_unique<edit::LevelAssetPanel>();
		mEditorWidgets.push_back(std::move(levelAssetPanel));

		std::unique_ptr<edit::ScenePanel> scenePanel = std::make_unique<edit::ScenePanel>();
		mEditorWidgets.push_back(std::move(scenePanel));


		std::filesystem::path editorFolderPath = std::filesystem::path(R2_ENGINE_ASSET_PATH) / "editor";

		mEditorFolderImage = edit::CreateTextureFromFile((editorFolderPath / "DirectoryIcon.png").string(), mEditorFolderImageWidth, mEditorFolderImageHeight, r2::draw::tex::WRAP_MODE_CLAMP_TO_EDGE, r2::draw::tex::FILTER_LINEAR, r2::draw::tex::FILTER_LINEAR);
		IM_ASSERT(mEditorFolderImage != 0);

		mEditorFileImage = edit::CreateTextureFromFile((editorFolderPath / "FileIcon.png").string(), mEditorFileImageWidth, mEditorFileImageHeight, r2::draw::tex::WRAP_MODE_CLAMP_TO_EDGE, r2::draw::tex::FILTER_LINEAR, r2::draw::tex::FILTER_LINEAR);
		IM_ASSERT(mEditorFileImage != 0);


		mEditorCamera.fov = glm::radians(70.0f);
		mEditorCamera.aspectRatio = static_cast<float>(CENG.DisplaySize().width) / static_cast<float>(CENG.DisplaySize().height);
		mEditorCamera.nearPlane = 0.1;
		mEditorCamera.farPlane = 1000.0f;

		mPerspectiveController.SetCamera(&mEditorCamera, glm::vec3(0.0f, -5.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 10.0f);

		r2::draw::renderer::SetRenderCamera(&mEditorCamera);

		CreateNewLevel("NewGroup", "NewLevel");

		char materialsPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(R2_ENGINE_INTERNAL_MATERIALS_MANIFESTS_BIN, materialsPath, "engine_material_pack.mpak");

		mEngineMaterialPackName = r2::asset::Asset::GetAssetNameForFilePath(materialsPath, r2::asset::MATERIAL_PACK_MANIFEST);

#ifdef R2_ASSET_PIPELINE
		mGridMaterialName = { {STRING_ID("EditorCartesianGrid"), "EditorCartesianGrid"}, mEngineMaterialPackName };
#else
		mGridMaterialName = { {STRING_ID("EditorCartesianGrid")}, materialParamsPackName };
#endif

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
				else if (e.KeyCode() == r2::io::KEY_g && ((e.Modifiers() & r2::io::Key::CONTROL_PRESSED) == r2::io::Key::CONTROL_PRESSED))
				{
					ToggleGrid();
					return true;
				}


				return false;
			});


		for (const auto& widget : mEditorWidgets)
		{
			widget->OnEvent(e);
		}

		if (&mPerspectiveController.GetCamera() == r2::draw::renderer::GetRenderCamera())
		{
			mPerspectiveController.OnEvent(e);
		}
		
	}

	void Editor::Update()
	{
		if (&mPerspectiveController.GetCamera() == r2::draw::renderer::GetRenderCamera())
		{
			mPerspectiveController.Update();
		}

		for (const auto& widget : mEditorWidgets)
		{
			widget->Update();
		}
	}

	void Editor::Render()
	{
		if(mShowGrid)
			r2::draw::renderer::DrawGrid();
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
		char binLevelPath[r2::fs::FILE_PATH_LENGTH];
		char rawLevelPath[r2::fs::FILE_PATH_LENGTH];

		const auto& application =  MENG.GetApplication();

		std::string levelBinURI = std::string(mCurrentEditorLevel->GetGroupName()) + r2::fs::utils::PATH_SEPARATOR + std::string(mCurrentEditorLevel->GetLevelName()) + ".rlvl";
		std::string levelRawURI = std::string(mCurrentEditorLevel->GetGroupName()) + r2::fs::utils::PATH_SEPARATOR + std::string(mCurrentEditorLevel->GetLevelName()) + ".json";

		r2::fs::utils::AppendSubPath(application.GetLevelPackDataBinPath().c_str(), binLevelPath, levelBinURI.c_str());
		r2::fs::utils::AppendSubPath(application.GetLevelPackDataJSONPath().c_str(), rawLevelPath, levelRawURI.c_str());

		r2::asset::Asset newLevelAsset = r2::asset::Asset::MakeAssetFromFilePath(binLevelPath, r2::asset::LEVEL);

		r2::asset::AssetLib& assetLib = MENG.GetAssetLib();

		r2::ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

		//Write out the new level file
		bool saved = r2::asset::pln::SaveLevelData(ecsWorld.GetECSCoordinator(), binLevelPath, rawLevelPath, *mCurrentEditorLevel);
		R2_CHECK(saved, "We couldn't save the file: %s\n", binLevelPath);

		//first check to see if we have asset for this
		if (!r2::asset::lib::HasAsset(assetLib, newLevelAsset))
		{
			r2::asset::lib::ImportAsset(assetLib, r2::asset::CreateNewAssetReference(binLevelPath, rawLevelPath, r2::asset::LEVEL), r2::asset::LEVEL);
		}
	}

	void Editor::LoadLevel(const std::string& filePathName)
	{
		if (mCurrentEditorLevel)
		{
			UnloadCurrentLevel();
		}

		LevelName levelAssetName = LevelManager::MakeLevelNameFromPath(filePathName.c_str());

		mCurrentEditorLevel = CENG.GetLevelManager().LoadLevel(levelAssetName);

		evt::EditorLevelLoadedEvent e(mCurrentEditorLevel->GetLevelAssetName(), filePathName);

		PostEditorEvent(e);
	}

	void Editor::UnloadCurrentLevel()
	{
		CreateNewLevel("NewGroup", "NewLevel");
	}

	void Editor::CreateNewLevel(const std::string& groupName, const std::string& levelNameStr)
	{
		if (mCurrentEditorLevel)
		{
			evt::EditorLevelWillUnLoadEvent e(mCurrentEditorLevel->GetLevelAssetName());

			PostEditorEvent(e);

			CENG.GetLevelManager().UnloadLevel(mCurrentEditorLevel);
			mCurrentEditorLevel = nullptr;
		}

		std::filesystem::path levelURI = std::filesystem::path(groupName) / std::filesystem::path(levelNameStr);
		levelURI.replace_extension(".rlvl");

		LevelName levelName = r2::asset::MakeAssetNameFromPath(levelURI.string().c_str(), r2::asset::LEVEL);

		mCurrentEditorLevel = CENG.GetLevelManager().MakeNewLevel(levelNameStr.c_str(), groupName.c_str(), levelName, mEditorCamera);

		evt::EditorLevelLoadedEvent e(mCurrentEditorLevel->GetLevelAssetName(), "");

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
		r2::evt::EventDispatcher dispatcher(e);

		dispatcher.Dispatch<r2::evt::EditorLevelLoadedEvent>([this](const r2::evt::EditorLevelLoadedEvent& e) {
			if (CENG.IsEditorActive())
			{
				//copy the level camera to ours to mimic how the level will initially look
				Level* level = CENG.GetLevelManager().GetLevel(e.GetLevel());

				Camera* levelCamera = level->GetCurrentCamera();

				mEditorCamera = *levelCamera;

				r2::draw::renderer::SetRenderCamera(&mEditorCamera);
			}

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

	void Editor::OpenCreateNewLevelModal()
	{
		if (mMainMenuBar)
		{
			mMainMenuBar->OpenCreateNewLevelWindow();
		}
	}

	void Editor::ToggleGrid()
	{
		mShowGrid = !mShowGrid;
	}

	void Editor::SetRenderToEditorCamera()
	{
		r2::draw::renderer::SetRenderCamera(&mEditorCamera);
	}

	void Editor::SwitchPerspectiveControllerCamera(Camera* camera)
	{
		mPerspectiveController.SetCamera(camera, camera->position, camera->facing, 10.0);
	}

	void Editor::SetDefaultEditorPerspectiveCameraController()
	{
		mPerspectiveController.SetCamera(&mEditorCamera, mEditorCamera.position, mEditorCamera.facing, 10.0);
	}

	bool Editor::IsEditorCameraInControl()
	{
		return &mPerspectiveController.GetCamera() == &mEditorCamera;
	}

	void Editor::AddModelToLevel(const r2::asset::AssetName& modelAsset)
	{
		auto& levelManager = CENG.GetLevelManager();
		levelManager.ImportModelToLevel(mCurrentEditorLevel, modelAsset);
	}

	void Editor::AddMaterialToLevel(const r2::mat::MaterialName& materialName)
	{
		auto& levelManager = CENG.GetLevelManager();
		levelManager.ImportMaterialToLevel(mCurrentEditorLevel, materialName);
	}

	void Editor::AddSoundBankToLevel(u64 soundBankAssetName, const std::string& name)
	{
		r2::asset::AssetName soundBankName;
		soundBankName.hashID = soundBankAssetName;
		soundBankName.assetNameString = name;

		auto& levelManager = CENG.GetLevelManager();
		levelManager.ImportSoundToLevel(mCurrentEditorLevel, soundBankName);
	}
}

#endif