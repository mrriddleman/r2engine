#include "r2pch.h"

#include "r2/Game/ECSWorld/ECSWorld.h"
#include "r2/Game/ECS/ECSCoordinator.h"

#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Systems/RenderSystem.h"
#include "r2/Game/ECS/Systems/SkeletalAnimationSystem.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "r2/Game/ECS/Components/AudioListenerComponent.h"
#include "r2/Game/ECS/Components/AudioEmitterComponent.h"
#include "r2/Game/ECS/Components/AudioParameterComponent.h"
#include "r2/Game/ECS/Components/AudioEmitterActionComponent.h"
#include "r2/Game/ECS/Systems/AudioEmitterSystem.h"
#include "r2/Game/ECS/Systems/AudioListenerSystem.h"

#ifdef R2_DEBUG
#include "r2/Game/ECS/Components/DebugRenderComponent.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#include "r2/Game/ECS/Systems/DebugBonesRenderSystem.h"
#include "r2/Game/ECS/Systems/DebugRenderSystem.h"
#endif
#ifdef R2_EDITOR
#include "r2/Game/ECS/Components/EditorComponent.h"
#endif

#include "r2/Game/Level/LevelManager.h"
#include "r2/Core/Application.h"
#include "r2/Platform/Platform.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"

namespace r2::ecs
{
	ECSWorld::ECSWorld()
		:mMemoryAreaHandle{}
		,mSubAreaHandle{}
		,mArena(nullptr)
		,mECSCoordinator(nullptr)
		,moptrRenderSystem(nullptr)
		,moptrSkeletalAnimationSystem(nullptr)
		,moptrAudioListenerSystem(nullptr)
		,moptrAudioEmitterSystem(nullptr)
#ifdef R2_DEBUG
		,moptrDebugRenderSystem(nullptr)
		,moptrDebugBonesRenderSystem(nullptr)
#endif
		, mMallocArena({})
	{
	}

	ECSWorld::~ECSWorld()
	{
	}

	bool ECSWorld::Init(r2::mem::MemoryArea::Handle memoryAreaHandle, u32 maxNumComponents, u32 maxNumEntities, u32 maxNumSystems)
	{
		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
		if (!memoryArea)
		{
			R2_CHECK(false, "Failed to get the memory area!");
			return false;
		}
		
		u64 memorySizeNeeded = MemorySize(maxNumComponents, maxNumEntities, maxNumSystems);

		if (memoryArea->UnAllocatedSpace() < memorySizeNeeded)
		{
			R2_CHECK(false, "We don't have enough space to create the ECSWorld. Space left: %llu, space needed: %llu, difference: %llu \n", memoryArea->UnAllocatedSpace(), memorySizeNeeded, (memorySizeNeeded - memoryArea->UnAllocatedSpace()));
			return false;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = memoryArea->AddSubArea(memorySizeNeeded, "ECSWorldArea");
		R2_CHECK(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid, "sub area handle is invalid!");


		r2::mem::MemoryArea::SubArea* subArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle)->GetSubArea(subAreaHandle);
		if (!subArea)
		{
			R2_CHECK(false, "Failed to get the sub area memory for the ECSWorld");
			return false;
		}

		mArena = EMPLACE_STACK_ARENA(*subArea);

		mMemoryAreaHandle = memoryAreaHandle;
		mSubAreaHandle = subAreaHandle;
		mECSCoordinator = ALLOC(ECSCoordinator, *mArena);

		R2_CHECK(mECSCoordinator != nullptr, "We couldn't allocate the ECSCoordinator");

		bool result = mECSCoordinator->Init<r2::mem::StackArena>(*mArena, maxNumComponents, maxNumEntities, 1, maxNumSystems);

		R2_CHECK(result, "We couldn't Init the mECSCoordinator!");

		RegisterEngineComponents();
		RegisterEngineSystems();
		
		bool sceneGraphInitialized = mSceneGraph.Init<r2::mem::StackArena>(*mArena, mECSCoordinator);
		R2_CHECK(sceneGraphInitialized, "The scene graph didn't initialize?");

		return result && sceneGraphInitialized;
	}

	void ECSWorld::Update()
	{
		moptrAudioListenerSystem->Update();

		mSceneGraph.Update();
		moptrSkeletalAnimationSystem->Update();

		//These should probably be near the last things to update so the game has the ability to add audio events
		moptrAudioEmitterSystem->Update();
	}

	void ECSWorld::Render()
	{
		moptrRenderSystem->Render();
#ifdef R2_DEBUG
		moptrDebugRenderSystem->Render();
		moptrDebugBonesRenderSystem->Render();
#endif
	}

	bool ECSWorld::LoadLevel(const Level& level, const flat::LevelData* levelData)
	{
		mSceneGraph.LoadLevel(level, levelData);

		PostLoadLevel(level, levelData);

		return true;
	}

	void ECSWorld::PostLoadLevel(const Level& level, const flat::LevelData* levelData)
	{
		const r2::SArray<ecs::Entity>& allEntities = mECSCoordinator->GetAllLivingEntities();

		const auto numEntities = r2::sarr::Size(allEntities);

		for (u64 i = 0; i < numEntities; ++i)
		{
			ecs::Entity e = r2::sarr::At(allEntities, i);

			if (mECSCoordinator->HasComponent<ecs::SkeletalAnimationComponent>(e)
#ifdef R2_DEBUG
				&& !mECSCoordinator->HasComponent<ecs::DebugBoneComponent>(e)
#endif		
				)
			{
#ifdef R2_DEBUG
				const ecs::SkeletalAnimationComponent& skeletalAnimationComponent = mECSCoordinator->GetComponent<ecs::SkeletalAnimationComponent>(e);

				ecs::DebugBoneComponent debugBoneComponent;
				debugBoneComponent.color = glm::vec4(1, 1, 0, 1);
				debugBoneComponent.debugBones = MAKE_SARRAY(mMallocArena, r2::draw::DebugBone, r2::sarr::Size(*skeletalAnimationComponent.animModel->optrBoneInfo));
				r2::sarr::Clear(*debugBoneComponent.debugBones);
				mComponentAllocations.push_back(debugBoneComponent.debugBones);

				mECSCoordinator->AddComponent<ecs::DebugBoneComponent>(e, debugBoneComponent);
#endif
			}

			if (mECSCoordinator->HasComponent<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(e)
#ifdef R2_DEBUG
				&& !mECSCoordinator->HasComponent<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(e)
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
					const auto numDebugBones = r2::sarr::Size(*r2::sarr::At(*instancedSkeletalAnimationComponent.instances, j).animModel->optrBoneInfo);

					ecs::DebugBoneComponent debugBoneInstance1;
					debugBoneInstance1.color = glm::vec4(1, 1, 0, 1);
					debugBoneInstance1.debugBones = MAKE_SARRAY(mMallocArena, r2::draw::DebugBone, numDebugBones);
					r2::sarr::Clear(*debugBoneInstance1.debugBones);
					mComponentAllocations.push_back(debugBoneInstance1.debugBones);

					r2::sarr::Push(*instancedDebugBoneComponent.instances, debugBoneInstance1);
				}

				mECSCoordinator->AddComponent<ecs::InstanceComponentT<ecs::DebugBoneComponent>>(e, instancedDebugBoneComponent);
#endif
			}
		}
	}

	bool ECSWorld::UnloadLevel(const Level& level)
	{
		mSceneGraph.UnloadLevel(level);
		return true;
	}

	void ECSWorld::Shutdown()
	{
		mSceneGraph.Shutdown<r2::mem::StackArena>(*mArena);

		UnRegisterEngineSystems();
		UnRegisterEngineComponents();

		for (auto iter = mComponentAllocations.rbegin(); iter != mComponentAllocations.rend(); ++iter)
		{
			FREE(*iter, mMallocArena);
		}

		RESET_ARENA(*mArena);

		FREE_EMPLACED_ARENA(mArena);
	}

	r2::ecs::ECSCoordinator* ECSWorld::GetECSCoordinator() const
	{
		return mECSCoordinator;
	}

	SceneGraph& ECSWorld::GetSceneGraph()
	{
		return mSceneGraph;
	}

	void* ECSWorld::HydrateRenderComponents(void* data)
	{
		r2::SArray<ecs::RenderComponent>* tempRenderComponents = static_cast<r2::SArray<ecs::RenderComponent>*>(data);

		const auto numRenderComponents = r2::sarr::Size(*tempRenderComponents);

		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		for (u32 i = 0; i < numRenderComponents; ++i)
		{
			ecs::RenderComponent& renderComponent = r2::sarr::At(*tempRenderComponents, i);

			r2::asset::Asset modelAsset = r2::asset::Asset(renderComponent.assetModelHash, r2::asset::RMODEL);

			r2::draw::ModelHandle modelHandle = gameAssetManager.LoadAsset(modelAsset);

			r2::draw::vb::GPUModelRefHandle gpuModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;

			const r2::draw::Model* model = gameAssetManager.GetAssetDataConst<r2::draw::Model>(modelHandle); 
			gpuModelRefHandle = r2::draw::renderer::UploadModel(model);

			R2_CHECK(r2::draw::vb::InvalidGPUModelRefHandle != gpuModelRefHandle, "We don't have a valid gpuModelRefHandle!");

			renderComponent.gpuModelRefHandle = gpuModelRefHandle;
		}

		return tempRenderComponents;
	}

	void* ECSWorld::HydrateSkeletalAnimationComponents(void* data)
	{
		r2::SArray<ecs::SkeletalAnimationComponent>* tempSkeletalAnimationComponents = static_cast<r2::SArray<ecs::SkeletalAnimationComponent>*>(data);

		if (!tempSkeletalAnimationComponents)
		{
			return nullptr;
		}

		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		const auto numSkeletalAnimationComponents = r2::sarr::Size(*tempSkeletalAnimationComponents);

		for (u32 i = 0; i < numSkeletalAnimationComponents; ++i)
		{
			ecs::SkeletalAnimationComponent& skeletalAnimationComponent = r2::sarr::At(*tempSkeletalAnimationComponents, i);

			r2::asset::Asset modelAsset = r2::asset::Asset(skeletalAnimationComponent.animModelAssetName, r2::asset::RMODEL);

			r2::draw::ModelHandle modelHandle = gameAssetManager.LoadAsset(modelAsset);

			const r2::draw::Model* animModel = gameAssetManager.GetAssetDataConst<r2::draw::Model>(modelHandle);

			skeletalAnimationComponent.animModel = animModel;
			skeletalAnimationComponent.currentAnimationIndex = skeletalAnimationComponent.startingAnimationIndex;

			//@HACK!!! - we should have some kind of proper scheme to allocate this memory
			skeletalAnimationComponent.shaderBones = MAKE_SARRAY(mMallocArena, r2::draw::ShaderBoneTransform, r2::sarr::Size(*animModel->optrBoneInfo));
			mComponentAllocations.push_back(skeletalAnimationComponent.shaderBones);

			r2::sarr::Clear(*skeletalAnimationComponent.shaderBones);
		}

		return tempSkeletalAnimationComponents;
	}

	void* ECSWorld::HydrateInstancedSkeletalAnimationComponents(void* data)
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

		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

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
				skeletalAnimationComponent.startingAnimationIndex = tempSkeletalAnimationComponent.startingAnimationIndex;
				skeletalAnimationComponent.startTime = tempSkeletalAnimationComponent.startTime;


				r2::asset::Asset modelAsset = r2::asset::Asset(skeletalAnimationComponent.animModelAssetName, r2::asset::RMODEL);

				r2::draw::ModelHandle modelHandle = gameAssetManager.LoadAsset(modelAsset);

				const r2::draw::Model* animModel = gameAssetManager.GetAssetDataConst<r2::draw::Model>(modelHandle);

				skeletalAnimationComponent.animModel = animModel;
				skeletalAnimationComponent.currentAnimationIndex = skeletalAnimationComponent.startingAnimationIndex;


				//@HACK!!! - we should have some kind of proper scheme to allocate this memory
				skeletalAnimationComponent.shaderBones = MAKE_SARRAY(mMallocArena, r2::draw::ShaderBoneTransform, r2::sarr::Size(*animModel->optrBoneInfo));
				mComponentAllocations.push_back(skeletalAnimationComponent.shaderBones);

				r2::sarr::Clear(*skeletalAnimationComponent.shaderBones);

				r2::sarr::Push(*instancedSkeletalAnimationComponent.instances, skeletalAnimationComponent);
			}

			r2::sarr::Push(*instancedSkeletalAnimationComponents, instancedSkeletalAnimationComponent);

		}

		return instancedSkeletalAnimationComponents;
	}

	void* ECSWorld::HydrateInstancedTransformComponents(void* data)
	{
		r2::SArray<ecs::InstanceComponentT<ecs::TransformComponent>>* tempInstancedTransformComponents = static_cast<r2::SArray<ecs::InstanceComponentT<ecs::TransformComponent>>*>(data);
		if (!tempInstancedTransformComponents)
		{
			return nullptr;
		}

		r2::SArray<ecs::InstanceComponentT<ecs::TransformComponent>>* instancedTransformComponents = nullptr;

		instancedTransformComponents = MAKE_SARRAY(mMallocArena, ecs::InstanceComponentT<ecs::TransformComponent>, r2::sarr::Size(*tempInstancedTransformComponents));
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

	void ECSWorld::RegisterEngineComponents()
	{
		ecs::ComponentArrayHydrationFunction renderComponentHydrationFunc = std::bind(&ECSWorld::HydrateRenderComponents, this, std::placeholders::_1);
		ecs::ComponentArrayHydrationFunction skeletalAnimationComponentHydrationFunc = std::bind(&ECSWorld::HydrateSkeletalAnimationComponents, this, std::placeholders::_1);
		ecs::ComponentArrayHydrationFunction instancedTransformComponentHydrationFunc = std::bind(&ECSWorld::HydrateInstancedTransformComponents, this, std::placeholders::_1);
		ecs::ComponentArrayHydrationFunction instancedSkeletalAnimationComponentHydrationFunc = std::bind(&ECSWorld::HydrateInstancedSkeletalAnimationComponents, this, std::placeholders::_1);

		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::HierarchyComponent>(*mArena, "HeirarchyComponent", true, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::TransformComponent>(*mArena, "TransformComponent", true, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::TransformDirtyComponent>(*mArena, "TransformDirtyComponent", false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::RenderComponent>(*mArena, "RenderComponent", true, renderComponentHydrationFunc);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::SkeletalAnimationComponent>(*mArena, "SkeletalAnimationComponent", true, skeletalAnimationComponentHydrationFunc);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::AudioListenerComponent>(*mArena, "AudioListenerComponent", true, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::AudioEmitterComponent>(*mArena, "AudioEmitterComponent", true, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::AudioParameterComponent>(*mArena, "AudioParameterComponent", false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::AudioEmitterActionComponent>(*mArena, "AudioEmitterActionComponent", false, nullptr);

		//add some more components to the coordinator for the editor to use
#ifdef R2_EDITOR
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::EditorComponent>(*mArena, "EditorComponent", true, nullptr);
#endif
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::TransformComponent>>(*mArena, "InstancedTranfromComponent", true, instancedTransformComponentHydrationFunc);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(*mArena, "InstancedSkeletalAnimationComponent", true, instancedSkeletalAnimationComponentHydrationFunc);

#ifdef R2_DEBUG
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::DebugRenderComponent>(*mArena, "DebugRenderComponent", false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::DebugBoneComponent>(*mArena, "DebugBoneComponent", false, nullptr);

		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::DebugRenderComponent>>(*mArena, "InstancedDebugRenderComponent", false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::DebugBoneComponent>>(*mArena, "InstancedDebugBoneComponent", false, nullptr);
#endif
	}

	void ECSWorld::UnRegisterEngineComponents()
	{
#ifdef R2_DEBUG
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::DebugBoneComponent>>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::DebugRenderComponent>>(*mArena);

		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::DebugBoneComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::DebugRenderComponent>(*mArena);
#endif

		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::TransformComponent>>(*mArena);
#ifdef R2_EDITOR
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::EditorComponent>(*mArena);
#endif
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::AudioEmitterActionComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::AudioParameterComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::AudioEmitterComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::AudioListenerComponent>(*mArena);

		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::SkeletalAnimationComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::RenderComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::TransformDirtyComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::TransformComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::HierarchyComponent>(*mArena);
	}
	
	void ECSWorld::RegisterEngineSystems()
	{
		const auto transformComponentType = mECSCoordinator->GetComponentType<ecs::TransformComponent>();
		const auto renderComponentType = mECSCoordinator->GetComponentType<ecs::RenderComponent>();
		const auto skeletalAnimationComponentType = mECSCoordinator->GetComponentType<ecs::SkeletalAnimationComponent>();
#ifdef R2_DEBUG
		const auto debugRenderComponentType = mECSCoordinator->GetComponentType<ecs::DebugRenderComponent>();
		const auto debugBoneComponentType = mECSCoordinator->GetComponentType<ecs::DebugBoneComponent>();
#endif

		moptrRenderSystem = (ecs::RenderSystem*)mECSCoordinator->RegisterSystem<mem::StackArena, ecs::RenderSystem>(*mArena);
		ecs::Signature renderSystemSignature;
		renderSystemSignature.set(transformComponentType);
		renderSystemSignature.set(renderComponentType);
		mECSCoordinator->SetSystemSignature<ecs::RenderSystem>(renderSystemSignature);

		u32 maxNumModels = r2::draw::renderer::GetMaxNumModelsLoadedAtOneTimePerLayout();
		u32 avgMaxNumMeshesPerModel = r2::draw::renderer::GetAVGMaxNumMeshesPerModel();
		u32 maxNumInstances = r2::draw::renderer::GetMaxNumInstances();
		u32 maxNumBones = r2::draw::renderer::GetMaxNumBones();

		r2::mem::utils::MemoryProperties memSizeStruct;
		memSizeStruct.alignment = 16;
		memSizeStruct.headerSize = mArena->HeaderSize();

		memSizeStruct.boundsChecking = 0;
#ifdef R2_DEBUG
		memSizeStruct.boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		moptrRenderSystem->Init<mem::StackArena>(*mArena, maxNumInstances, avgMaxNumMeshesPerModel * maxNumModels, maxNumBones);

		moptrSkeletalAnimationSystem = (ecs::SkeletalAnimationSystem*)mECSCoordinator->RegisterSystem<mem::StackArena, ecs::SkeletalAnimationSystem>(*mArena);

		ecs::Signature skeletalAnimationSystemSignature;
		skeletalAnimationSystemSignature.set(skeletalAnimationComponentType);
#ifdef R2_DEBUG
		skeletalAnimationSystemSignature.set(debugBoneComponentType);
#endif
		mECSCoordinator->SetSystemSignature<ecs::SkeletalAnimationSystem>(skeletalAnimationSystemSignature);

#ifdef R2_DEBUG
		moptrDebugBonesRenderSystem = (ecs::DebugBonesRenderSystem*)mECSCoordinator->RegisterSystem<mem::StackArena, ecs::DebugBonesRenderSystem>(*mArena);
		ecs::Signature debugBonesSystemSignature;
		debugBonesSystemSignature.set(debugBoneComponentType);
		debugBonesSystemSignature.set(transformComponentType);

		mECSCoordinator->SetSystemSignature<ecs::DebugBonesRenderSystem>(debugBonesSystemSignature);

		moptrDebugRenderSystem = (ecs::DebugRenderSystem*)mECSCoordinator->RegisterSystem<mem::StackArena, ecs::DebugRenderSystem>(*mArena);
		ecs::Signature debugRenderSystemSignature;

		debugRenderSystemSignature.set(debugRenderComponentType);
		debugRenderSystemSignature.set(transformComponentType);

		mECSCoordinator->SetSystemSignature<ecs::DebugRenderSystem>(debugRenderSystemSignature);
#endif

		const auto audioListenerComponentType = mECSCoordinator->GetComponentType<ecs::AudioListenerComponent>();
		const auto audioEmitterComponentType = mECSCoordinator->GetComponentType<ecs::AudioEmitterComponent>();

		moptrAudioListenerSystem = (ecs::AudioListenerSystem*)mECSCoordinator->RegisterSystem<mem::StackArena, ecs::AudioListenerSystem>(*mArena);
		ecs::Signature audioListenerSystemSignature;
		audioListenerSystemSignature.set(transformComponentType);
		audioListenerSystemSignature.set(audioListenerComponentType);
		mECSCoordinator->SetSystemSignature<ecs::AudioListenerSystem>(audioListenerSystemSignature);

		moptrAudioEmitterSystem = (ecs::AudioEmitterSystem*)mECSCoordinator->RegisterSystem<mem::StackArena, ecs::AudioEmitterSystem>(*mArena);
		ecs::Signature audioEmitterSystemSignature;
		audioEmitterSystemSignature.set(audioEmitterComponentType);
		mECSCoordinator->SetSystemSignature<ecs::AudioEmitterSystem>(audioEmitterSystemSignature);

	}

	void ECSWorld::UnRegisterEngineSystems()
	{
		mECSCoordinator->UnRegisterSystem<mem::StackArena, ecs::AudioEmitterSystem>(*mArena);
		mECSCoordinator->UnRegisterSystem<mem::StackArena, ecs::AudioListenerSystem>(*mArena);

#ifdef R2_DEBUG
		mECSCoordinator->UnRegisterSystem<mem::StackArena, ecs::DebugRenderSystem>(*mArena);
		mECSCoordinator->UnRegisterSystem<mem::StackArena, ecs::DebugBonesRenderSystem>(*mArena);
#endif
		mECSCoordinator->UnRegisterSystem<mem::StackArena, ecs::SkeletalAnimationSystem>(*mArena);
		moptrRenderSystem->Shutdown<mem::StackArena>(*mArena);
		mECSCoordinator->UnRegisterSystem<mem::StackArena, ecs::RenderSystem>(*mArena);
	}

	u64 ECSWorld::MemorySize(u32 maxNumComponents, u32 maxNumEntities, u32 maxNumSystems)
	{
		u32 maxNumModels = r2::draw::renderer::GetMaxNumModelsLoadedAtOneTimePerLayout();
		u32 avgMaxNumMeshesPerModel = r2::draw::renderer::GetAVGMaxNumMeshesPerModel();
		u32 maxNumInstances = r2::draw::renderer::GetMaxNumInstances();
		u32 maxNumBones = r2::draw::renderer::GetMaxNumBones();

		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();
		u32 boundsChecking = 0;

#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		u64 memorySize = 0;

		r2::mem::utils::MemoryProperties memProperties;
		memProperties.alignment = ALIGNMENT;
		memProperties.boundsChecking = boundsChecking;
		memProperties.headerSize = stackHeaderSize;

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::ecs::ECSCoordinator::MemorySize(maxNumComponents, maxNumEntities, maxNumSystems, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(ecs::ECSCoordinator::MemorySizeOfSystemType<r2::ecs::SkeletalAnimationSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(ecs::ECSCoordinator::MemorySizeOfSystemType<r2::ecs::AudioListenerSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(ecs::ECSCoordinator::MemorySizeOfSystemType<r2::ecs::AudioEmitterSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);

		memorySize += r2::ecs::RenderSystem::MemorySize(maxNumInstances, avgMaxNumMeshesPerModel*maxNumModels, maxNumBones, memProperties);


#ifdef R2_DEBUG
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(ecs::ECSCoordinator::MemorySizeOfSystemType<r2::ecs::DebugRenderSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(ecs::ECSCoordinator::MemorySizeOfSystemType<r2::ecs::DebugBonesRenderSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);
#endif // R2_DEBUG

		//add all of the Engine Component memory here
		memorySize += ComponentArray<HierarchyComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<TransformComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<TransformDirtyComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<RenderComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<SkeletalAnimationComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);

		memorySize += ComponentArray<AudioListenerComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<AudioEmitterComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<AudioParameterComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<AudioEmitterActionComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);

#ifdef R2_EDITOR
		memorySize += ComponentArray<EditorComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
#endif
		memorySize += ComponentArray<InstanceComponentT<ecs::TransformComponent>>::MemorySizeForInstancedComponentArray(maxNumEntities, maxNumInstances, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<InstanceComponentT<SkeletalAnimationComponent>>::MemorySizeForInstancedComponentArray(maxNumEntities, maxNumInstances, ALIGNMENT, stackHeaderSize, boundsChecking);
#ifdef R2_DEBUG
		memorySize += ComponentArray<DebugRenderComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<DebugBoneComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);

		memorySize += ComponentArray<InstanceComponentT<DebugRenderComponent>>::MemorySizeForInstancedComponentArray(maxNumEntities, maxNumInstances, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<InstanceComponentT<DebugBoneComponent>>::MemorySizeForInstancedComponentArray(maxNumEntities, maxNumInstances, ALIGNMENT, stackHeaderSize, boundsChecking);
#endif

		auto sceneGraphMemory = SceneGraph::MemorySize(memProperties);
		memorySize += sceneGraphMemory;

		return memorySize;
	}

}