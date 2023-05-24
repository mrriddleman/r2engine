#include "r2pch.h"

#include "r2/Game/ECSWorld/ECSWorld.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Systems/RenderSystem.h"
#include "r2/Game/ECS/Systems/SkeletalAnimationSystem.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#ifdef R2_DEBUG
#include "r2/Game/ECS/Components/DebugRenderComponent.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#include "r2/Game/ECS/Systems/DebugBonesRenderSystem.h"
#include "r2/Game/ECS/Systems/DebugRenderSystem.h"
#endif
#include "r2/Game/Level/LevelManager.h"
#include "r2/Core/Application.h"
#include "r2/Platform/Platform.h"
#include "r2/Render/Animation/AnimationCache.h"

namespace r2::ecs
{
	ECSWorld::ECSWorld()
		:mMemoryAreaHandle{}
		,mSubAreaHandle{}
		,mArena(nullptr)
		,mECSCoordinator(nullptr)
		,moptrRenderSystem(nullptr)
		,moptrSkeletalAnimationSystem(nullptr)
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

		return result;
	}

	void ECSWorld::Shutdown()
	{
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

	r2::ecs::RenderSystem* ECSWorld::GetRenderSystem() const
	{
		return moptrRenderSystem;
	}

	r2::ecs::SkeletalAnimationSystem* ECSWorld::GetSkeletalAnimationSystem()
	{
		return moptrSkeletalAnimationSystem;
	}

#ifdef R2_DEBUG
	r2::ecs::DebugRenderSystem* ECSWorld::GetDebugRenderSystem()
	{
		return moptrDebugRenderSystem;
	}

	r2::ecs::DebugBonesRenderSystem* ECSWorld::GetDebugBonesRenderSystem()
	{
		return moptrDebugBonesRenderSystem;
	}
#endif // R2_DEBUG

	void* ECSWorld::HydrateRenderComponents(void* data)
	{
		r2::SArray<ecs::RenderComponent>* tempRenderComponents = static_cast<r2::SArray<ecs::RenderComponent>*>(data);

		const auto numRenderComponents = r2::sarr::Size(*tempRenderComponents);

		r2::draw::ModelCache* editorModelSystem = CENG.GetLevelManager().GetModelSystem();
		r2::draw::MaterialSystem* editorMaterialSystem = CENG.GetApplication().GetEditorMaterialSystem();

		for (u32 i = 0; i < numRenderComponents; ++i)
		{
			ecs::RenderComponent& renderComponent = r2::sarr::At(*tempRenderComponents, i);

			r2::asset::Asset modelAsset = r2::asset::Asset(renderComponent.assetModelHash, r2::asset::RMODEL);

			r2::draw::ModelHandle modelHandle = r2::draw::modlche::LoadModel(editorModelSystem, modelAsset);

			r2::draw::vb::GPUModelRefHandle gpuModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
			if (renderComponent.isAnimated)
			{
				const r2::draw::AnimModel* animModel = r2::draw::modlche::GetAnimModel(editorModelSystem, modelHandle);
				gpuModelRefHandle = r2::draw::renderer::UploadAnimModel(animModel);
			}
			else
			{
				const r2::draw::Model* model = r2::draw::modlche::GetModel(editorModelSystem, modelHandle);
				gpuModelRefHandle = r2::draw::renderer::UploadModel(model);
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

	void* ECSWorld::HydrateSkeletalAnimationComponents(void* data)
	{
		r2::SArray<ecs::SkeletalAnimationComponent>* tempSkeletalAnimationComponents = static_cast<r2::SArray<ecs::SkeletalAnimationComponent>*>(data);

		if (!tempSkeletalAnimationComponents)
		{
			return nullptr;
		}

		r2::draw::ModelCache* editorModelSystem = CENG.GetLevelManager().GetModelSystem();
		r2::draw::AnimationCache* editorAnimationCache = CENG.GetLevelManager().GetAnimationCache();

		const auto numSkeletalAnimationComponents = r2::sarr::Size(*tempSkeletalAnimationComponents);

		for (u32 i = 0; i < numSkeletalAnimationComponents; ++i)
		{
			ecs::SkeletalAnimationComponent& skeletalAnimationComponent = r2::sarr::At(*tempSkeletalAnimationComponents, i);

			r2::asset::Asset modelAsset = r2::asset::Asset(skeletalAnimationComponent.animModelAssetName, r2::asset::RMODEL);

			r2::draw::ModelHandle modelHandle = r2::draw::modlche::LoadModel(editorModelSystem, modelAsset);

			const r2::draw::AnimModel* animModel = r2::draw::modlche::GetAnimModel(editorModelSystem, modelHandle);

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

		r2::draw::ModelCache* editorModelSystem = CENG.GetLevelManager().GetModelSystem();
		r2::draw::AnimationCache* editorAnimationCache = CENG.GetLevelManager().GetAnimationCache();

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

				r2::draw::ModelHandle modelHandle = r2::draw::modlche::LoadModel(editorModelSystem, modelAsset);

				const r2::draw::AnimModel* animModel = r2::draw::modlche::GetAnimModel(editorModelSystem, modelHandle);

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

		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::HeirarchyComponent>(*mArena, "HeirarchyComponent", true, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::TransformComponent>(*mArena, "TransformComponent", true, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::TransformDirtyComponent>(*mArena, "TransformDirtyComponent", false, nullptr);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::RenderComponent>(*mArena, "RenderComponent", true, renderComponentHydrationFunc);
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::SkeletalAnimationComponent>(*mArena, "SkeletalAnimationComponent", true, skeletalAnimationComponentHydrationFunc);

		//add some more components to the coordinator for the editor to use
		mECSCoordinator->RegisterComponent<mem::StackArena, ecs::EditorComponent>(*mArena, "EditorComponent", true, nullptr);

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

		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::EditorComponent>(*mArena);

		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::SkeletalAnimationComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::RenderComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::TransformDirtyComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::TransformComponent>(*mArena);
		mECSCoordinator->UnRegisterComponent<mem::StackArena, ecs::HeirarchyComponent>(*mArena);
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
	}

	void ECSWorld::UnRegisterEngineSystems()
	{
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
		memorySize += r2::ecs::RenderSystem::MemorySize(maxNumInstances, avgMaxNumMeshesPerModel*maxNumModels, maxNumBones, memProperties);
#ifdef R2_DEBUG
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(ecs::ECSCoordinator::MemorySizeOfSystemType<r2::ecs::DebugRenderSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(ecs::ECSCoordinator::MemorySizeOfSystemType<r2::ecs::DebugBonesRenderSystem>(memProperties), ALIGNMENT, stackHeaderSize, boundsChecking);
#endif // R2_DEBUG

		//add all of the Engine Component memory now
		memorySize += ComponentArray<HeirarchyComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<TransformComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<TransformDirtyComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<RenderComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<SkeletalAnimationComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += ComponentArray<EditorComponent>::MemorySize(maxNumEntities, ALIGNMENT, stackHeaderSize, boundsChecking);

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