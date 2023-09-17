#include "r2pch.h"

#include "r2/Game/ECSWorld/ECSWorld.h"


#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/AudioListenerComponent.h"
#include "r2/Game/ECS/Components/AudioEmitterComponent.h"
#include "r2/Game/ECS/Components/AudioParameterComponent.h"
#include "r2/Game/ECS/Components/AudioEmitterActionComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"

#include "r2/Game/ECS/Systems/AudioEmitterSystem.h"
#include "r2/Game/ECS/Systems/AudioListenerSystem.h"
#include "r2/Game/ECS/Systems/RenderSystem.h"
#include "r2/Game/ECS/Systems/SkeletalAnimationSystem.h"
#include "r2/Game/ECS/Systems/SceneGraphSystem.h"
#include "r2/Game/ECS/Systems/SceneGraphTransformUpdateSystem.h"

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
		,moptrSceneGraphSystem(nullptr)
		,moptrSceneGraphTransformUpdateSystem(nullptr)
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
		
		bool sceneGraphInitialized = mSceneGraph.Init(moptrSceneGraphSystem, moptrSceneGraphTransformUpdateSystem, mECSCoordinator);
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
		mSceneGraph.LoadLevel(*this, level, levelData);

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
				mComponentAllocations.push_back({ debugBoneComponent.debugBones });

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
				mComponentAllocations.push_back({ instancedDebugBoneComponent.instances });

				for (u32 j = 0; j < numInstances; ++j)
				{
					const auto numDebugBones = r2::sarr::Size(*r2::sarr::At(*instancedSkeletalAnimationComponent.instances, j).animModel->optrBoneInfo);

					ecs::DebugBoneComponent debugBoneInstance1;
					debugBoneInstance1.color = glm::vec4(1, 1, 0, 1);
					debugBoneInstance1.debugBones = MAKE_SARRAY(mMallocArena, r2::draw::DebugBone, numDebugBones);
					r2::sarr::Clear(*debugBoneInstance1.debugBones);
					mComponentAllocations.push_back({ debugBoneInstance1.debugBones});

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
		mSceneGraph.Shutdown();

		UnRegisterEngineSystems();
		UnRegisterEngineComponents();

		for (auto iter = mComponentAllocations.rbegin(); iter != mComponentAllocations.rend(); ++iter)
		{
			FREE(iter->memPtr, mMallocArena);
		}

		RESET_ARENA(*mArena);

		FREE_EMPLACED_ARENA(mArena);
	}

	r2::ecs::ECSCoordinator* ECSWorld::GetECSCoordinator()
	{
		return mECSCoordinator;
	}

	r2::ecs::SceneGraph& ECSWorld::GetSceneGraph()
	{
		return mSceneGraph;
	}

	void ECSWorld::FreeRenderComponent(void* data)
	{
		ecs::RenderComponent* renderComponent = static_cast<ecs::RenderComponent*>(data);
		R2_CHECK(renderComponent != nullptr, "Should never happen");

		if (renderComponent->optrMaterialOverrideNames != nullptr)
		{
			ECS_WORLD_FREE(*this, renderComponent->optrMaterialOverrideNames);
			renderComponent->optrMaterialOverrideNames = nullptr;
		}
	}

	void ECSWorld::FreeSkeletalAnimationComponent(void* data)
	{
		ecs::SkeletalAnimationComponent* skeletalAnimationComponent = static_cast<ecs::SkeletalAnimationComponent*>(data);
		R2_CHECK(skeletalAnimationComponent != nullptr, "Should never happen");

		if (skeletalAnimationComponent->shaderBones != nullptr)
		{
			ECS_WORLD_FREE(*this, skeletalAnimationComponent->shaderBones);
			skeletalAnimationComponent->shaderBones = nullptr;
		}
	}

	void ECSWorld::FreeInstancedSkeletalAnimationComponent(void* data)
	{
		ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>* instancedSkeletalAnimationComponent = static_cast<ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>*>(data);
		R2_CHECK(instancedSkeletalAnimationComponent != nullptr, "Should never happen");

		if (instancedSkeletalAnimationComponent->instances != nullptr)
		{
			for (u32 i = 0; i < instancedSkeletalAnimationComponent->numInstances; ++i)
			{
				ecs::SkeletalAnimationComponent& animationComponent = r2::sarr::At(*instancedSkeletalAnimationComponent->instances, i);

				if (animationComponent.shaderBones != nullptr)
				{
					ECS_WORLD_FREE(*this, animationComponent.shaderBones);
					animationComponent.shaderBones = nullptr;
				}
			}

			ECS_WORLD_FREE(*this, instancedSkeletalAnimationComponent->instances);
			instancedSkeletalAnimationComponent->instances = nullptr;
			instancedSkeletalAnimationComponent->numInstances = 0;
		}
	}

	void ECSWorld::FreeInstancedTransformComponent(void* data)
	{
		ecs::InstanceComponentT<ecs::TransformComponent>* instancedTransformComponent = static_cast<ecs::InstanceComponentT<ecs::TransformComponent>*>(data);
		R2_CHECK(instancedTransformComponent != nullptr, "Should never happen");

		if (instancedTransformComponent->instances != nullptr)
		{
			ECS_WORLD_FREE(*this, instancedTransformComponent->instances);
			instancedTransformComponent->instances = nullptr;
			instancedTransformComponent->numInstances = 0;
		}
	}

#ifdef R2_DEBUG
	void ECSWorld::FreeDebugBoneComponent(void* data)
	{
		ecs::DebugBoneComponent* debugBoneComponent = static_cast<ecs::DebugBoneComponent*>(data);
		R2_CHECK(debugBoneComponent != nullptr, "Should never happen");

		if (debugBoneComponent->debugBones != nullptr)
		{
			ECS_WORLD_FREE(*this, debugBoneComponent->debugBones);
			debugBoneComponent->debugBones = nullptr;
		}
	}

	void ECSWorld::FreeInstancedDebugRenderComponent(void* data)
	{
		ecs::InstanceComponentT<ecs::DebugRenderComponent>* instancedRenderComponent = static_cast<ecs::InstanceComponentT<ecs::DebugRenderComponent>*>(data);
		R2_CHECK(instancedRenderComponent != nullptr, "Should never happen");

		if (instancedRenderComponent->instances != nullptr)
		{
			ECS_WORLD_FREE(*this, instancedRenderComponent->instances);
			instancedRenderComponent->instances = nullptr;
			instancedRenderComponent->numInstances = 0;
		}
	}

	void ECSWorld::FreeInstancedDebugBoneComponent(void* data)
	{
		ecs::InstanceComponentT<ecs::DebugBoneComponent>* instancedDebugBoneComponent = static_cast<ecs::InstanceComponentT<ecs::DebugBoneComponent>*>(data);
		R2_CHECK(instancedDebugBoneComponent != nullptr, "Should never happen");

		if (instancedDebugBoneComponent->instances != nullptr)
		{
			for (u32 i = 0; i < instancedDebugBoneComponent->numInstances; ++i)
			{
				ecs::DebugBoneComponent& animationComponent = r2::sarr::At(*instancedDebugBoneComponent->instances, i);

				if (animationComponent.debugBones != nullptr)
				{
					ECS_WORLD_FREE(*this, animationComponent.debugBones);
					animationComponent.debugBones = nullptr;
				}
			}

			ECS_WORLD_FREE(*this, instancedDebugBoneComponent->instances);
			instancedDebugBoneComponent->instances = nullptr;
			instancedDebugBoneComponent->numInstances = 0;
		}
	}
#endif

	void ECSWorld::RegisterEngineComponents()
	{
		FreeComponentFunc freeRenderComponentFunc = std::bind(&ECSWorld::FreeRenderComponent, this, std::placeholders::_1);
		FreeComponentFunc freeSkeletalAnimationComponentFunc = std::bind(&ECSWorld::FreeSkeletalAnimationComponent, this, std::placeholders::_1);
		FreeComponentFunc freeInstancedSkeletalAnimationComponentFunc = std::bind(&ECSWorld::FreeInstancedSkeletalAnimationComponent, this, std::placeholders::_1);
		FreeComponentFunc freeInstancedTransformComponentFunc = std::bind(&ECSWorld::FreeInstancedTransformComponent, this, std::placeholders::_1);

#ifdef R2_DEBUG
		FreeComponentFunc freeDebugBoneComponent = std::bind(&ECSWorld::FreeDebugBoneComponent, this, std::placeholders::_1);
		FreeComponentFunc freeInstancedDebugBoneComponent = std::bind(&ECSWorld::FreeInstancedDebugBoneComponent, this, std::placeholders::_1);
		FreeComponentFunc freeInstancedDebugRenderComponent = std::bind(&ECSWorld::FreeInstancedDebugRenderComponent, this, std::placeholders::_1);
#endif

		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::HierarchyComponent>(*mArena, "HeirarchyComponent", true, false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::TransformComponent>(*mArena, "TransformComponent", true, false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::TransformDirtyComponent>(*mArena, "TransformDirtyComponent", false, false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::RenderComponent>(*mArena, "RenderComponent", true, false, freeRenderComponentFunc);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::SkeletalAnimationComponent>(*mArena, "SkeletalAnimationComponent", true, false, freeSkeletalAnimationComponentFunc);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::AudioListenerComponent>(*mArena, "AudioListenerComponent", true, false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::AudioEmitterComponent>(*mArena, "AudioEmitterComponent", true, false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::AudioParameterComponent>(*mArena, "AudioParameterComponent", false, false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::AudioEmitterActionComponent>(*mArena, "AudioEmitterActionComponent", false, false, nullptr);

		//add some more components to the coordinator for the editor to use

		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::EditorComponent>(*mArena, "EditorComponent", true, false, nullptr);

		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::TransformComponent>>(*mArena, "InstancedTranfromComponent", true, true, freeInstancedTransformComponentFunc);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::SkeletalAnimationComponent>>(*mArena, "InstancedSkeletalAnimationComponent", true, true, freeInstancedSkeletalAnimationComponentFunc);

#ifdef R2_DEBUG
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::DebugRenderComponent>(*mArena, "DebugRenderComponent", false, false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::DebugBoneComponent>(*mArena, "DebugBoneComponent", false, false, freeDebugBoneComponent);

		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::DebugRenderComponent>>(*mArena, "InstancedDebugRenderComponent", false, true, freeInstancedDebugRenderComponent);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::InstanceComponentT<ecs::DebugBoneComponent>>(*mArena, "InstancedDebugBoneComponent", false, true, freeInstancedDebugBoneComponent);
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

		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::EditorComponent>(*mArena);

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
		const auto heirarchyComponentType = mECSCoordinator->GetComponentType<ecs::HierarchyComponent>();
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
//#ifdef R2_DEBUG
//		skeletalAnimationSystemSignature.set(debugBoneComponentType);
//#endif
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


		moptrSceneGraphSystem = (ecs::SceneGraphSystem*)mECSCoordinator->RegisterSystem<mem::StackArena, ecs::SceneGraphSystem>(*mArena);

		if (moptrSceneGraphSystem == nullptr)
		{
			R2_CHECK(false, "Couldn't register the SceneGraphSystem");
			return;
		}

		ecs::Signature systemSignature;

		
		systemSignature.set(heirarchyComponentType);
		systemSignature.set(transformComponentType);

		mECSCoordinator->SetSystemSignature<ecs::SceneGraphSystem>(systemSignature);

		moptrSceneGraphTransformUpdateSystem = (ecs::SceneGraphTransformUpdateSystem*)mECSCoordinator->RegisterSystem<r2::mem::StackArena, ecs::SceneGraphTransformUpdateSystem>(*mArena);

		if (moptrSceneGraphTransformUpdateSystem == nullptr)
		{
			R2_CHECK(false, "Couldn't register the SceneGraphTransformUpdateSystem");
			return;
		}

		ecs::Signature systemUpdateSignature;
		systemUpdateSignature.set(heirarchyComponentType);
		systemUpdateSignature.set(transformComponentType);
		systemUpdateSignature.set(mECSCoordinator->GetComponentType<ecs::TransformDirtyComponent>());

		mECSCoordinator->SetSystemSignature<ecs::SceneGraphTransformUpdateSystem>(systemUpdateSignature);

	}

	void ECSWorld::UnRegisterEngineSystems()
	{
		mECSCoordinator->UnRegisterSystem<mem::StackArena, ecs::SceneGraphTransformUpdateSystem>(*mArena);
		mECSCoordinator->UnRegisterSystem<mem::StackArena, ecs::SceneGraphSystem>(*mArena);

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
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(ecs::ECSCoordinator::MemorySizeOfSystemType<r2::ecs::SceneGraphSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(ecs::ECSCoordinator::MemorySizeOfSystemType<r2::ecs::SceneGraphTransformUpdateSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);

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

		return memorySize;
	}

}