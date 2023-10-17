#include "r2pch.h"

#ifdef R2_DEBUG

#include "r2/Game/ECS/Systems/DebugRenderSystem.h"
#include "r2/Game/ECS/Components/DebugRenderComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Render/Renderer/Renderer.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"

namespace r2::ecs
{
	struct InstanceIndices
	{
		r2::SArray<u32>* indices = nullptr;
	};

	DebugRenderSystem::DebugRenderSystem()
	{
		mKeepSorted = false;
	}

	DebugRenderSystem::~DebugRenderSystem()
	{

	}

	void DebugRenderSystem::Render()
	{
		R2_CHECK(mnoptrCoordinator != nullptr, "Not sure why this should ever be nullptr?");
		R2_CHECK(mEntities != nullptr, "Not sure why this should ever be nullptr?");

		const auto numEntities = r2::sarr::Size(*mEntities);

		for (u32 i = 0; i < numEntities; ++i)
		{
			Entity e = r2::sarr::At(*mEntities, i);

			const DebugRenderComponent& debugRenderComponent = mnoptrCoordinator->GetComponent<DebugRenderComponent>(e);
			const TransformComponent& transformComponent = mnoptrCoordinator->GetComponent<TransformComponent>(e);
			const InstanceComponentT<DebugRenderComponent>* instanceDebugRenderComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<DebugRenderComponent>>(e);
			const InstanceComponentT<TransformComponent>* instanceTransformComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<TransformComponent>>(e);

			bool useInstancing = instanceDebugRenderComponent != nullptr && instanceTransformComponent != nullptr;

			if (!useInstancing)
			{
				RenderDebug(debugRenderComponent, transformComponent);
			}
			else
			{
				RenderDebugInstanced(debugRenderComponent, transformComponent, *instanceDebugRenderComponent, *instanceTransformComponent);
			}
		}
	}

	void DebugRenderSystem::RenderDebug(const DebugRenderComponent& c, const TransformComponent& t)
	{
		glm::vec3 position = t.accumTransform.position + c.offset;
		float radius = c.radius;
		glm::vec4 color = c.color;
		glm::vec3 scale = c.scale;
		glm::vec3 dir = c.direction;

		switch (c.debugModelType)
		{
		case draw::DEBUG_QUAD:
			r2::draw::renderer::DrawQuad(position, glm::vec2(scale.x, scale.y), dir, color, c.filled, c.depthTest);
			break;
		case draw::DEBUG_SPHERE:
			r2::draw::renderer::DrawSphere(position, radius, color, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CUBE:
			r2::draw::renderer::DrawCube(position, scale.x, color, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CYLINDER:
			r2::draw::renderer::DrawCylinder(position, dir, radius, scale.x, color, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CONE:
			r2::draw::renderer::DrawCone(position, dir, radius, scale.x, color, c.filled, c.depthTest);
			break;
		case draw::DEBUG_ARROW:
			r2::draw::renderer::DrawArrow(position, dir, scale.x, radius, color, c.filled, c.depthTest);
			break;
		case draw::DEBUG_LINE:
		{
			//@NOTE: may need to redo this if we want the p0 position to be different than the position of the transform
			glm::vec3 p1 = position + scale * dir;
			r2::draw::renderer::DrawLine(position, p1, color, !c.depthTest);
		}
		break;
		default:
			R2_CHECK(false, "Unsupported ATM");
			break;
		}
	}

	void DrawInstancedDebugModel(
		r2::draw::DebugModelType debugModelType,
		r2::SArray<void*>* tempAllocations,
		const r2::SArray<DebugRenderComponent>& debugRenderComponents,
		const r2::SArray<TransformComponent>& transformComponents,
		bool filled,
		bool depthTest)
	{
		R2_CHECK(tempAllocations != nullptr, "You passed in nullptr for the tempAllocation array");
		R2_CHECK(r2::sarr::Size(debugRenderComponents) == r2::sarr::Size(transformComponents), "These should always be the same");

		//need to make the positions
		const auto numInstances = r2::sarr::Size(transformComponents);

		r2::SArray<glm::vec3>* positions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances);
		r2::sarr::Push(*tempAllocations, (void*)positions);

		//make the colors
		r2::SArray<glm::vec4>* colors = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec4, numInstances);
		r2::sarr::Push(*tempAllocations, (void*)colors);

		for (u32 i = 0; i < numInstances; ++i)
		{
			const DebugRenderComponent& debugRenderComponent = r2::sarr::At(debugRenderComponents, i);

			const auto& position = r2::sarr::At(transformComponents, i).accumTransform.position;
			const auto& offset = debugRenderComponent.offset;

			r2::sarr::Push(*positions, position + offset);
			r2::sarr::Push(*colors, debugRenderComponent.color);
		}

		r2::SArray<glm::vec3>* directions = nullptr;
		r2::SArray<float>* radii = nullptr;
		r2::SArray<float>* scales = nullptr;

		if (debugModelType == draw::DEBUG_ARROW ||
			debugModelType == draw::DEBUG_CYLINDER ||
			debugModelType == draw::DEBUG_CONE)
		{
			radii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)radii);
			scales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)scales);
			directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)directions);

			for (u32 i = 0; i < numInstances; ++i)
			{
				const DebugRenderComponent& debugRenderComponent = r2::sarr::At(debugRenderComponents, i);

				r2::sarr::Push(*radii, debugRenderComponent.radius);
				r2::sarr::Push(*scales, debugRenderComponent.scale.x);
				r2::sarr::Push(*directions, debugRenderComponent.direction);
			}
		}

		if (debugModelType == draw::DEBUG_SPHERE)
		{
			radii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)radii);

			for (u32 i = 0; i < numInstances; ++i)
			{
				const auto& radius = r2::sarr::At(debugRenderComponents, i).radius;
				r2::sarr::Push(*radii, radius);
			}
		}

		if (debugModelType == draw::DEBUG_CUBE)
		{
			scales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)scales);

			for (u32 i = 0; i < numInstances; ++i)
			{
				const auto& scale = r2::sarr::At(debugRenderComponents, i).scale.x;
				r2::sarr::Push(*scales, scale);
			}
		}

		if (debugModelType == draw::DEBUG_LINE)
		{
			scales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)scales);

			directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)directions);

			for (u32 i = 0; i < numInstances; ++i)
			{
				const DebugRenderComponent& debugRenderComponent = r2::sarr::At(debugRenderComponents, i);

				r2::sarr::Push(*scales, debugRenderComponent.scale.x);
				r2::sarr::Push(*directions, debugRenderComponent.direction);
			}
		}

		if (debugModelType == draw::DEBUG_QUAD)
		{
			directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)directions);
			
			for (u32 i = 0; i < numInstances; ++i)
			{
				const auto& direction = r2::sarr::At(debugRenderComponents, i).direction;
				r2::sarr::Push(*directions, direction);
			}
		}

		switch (debugModelType)
		{
		case draw::DEBUG_QUAD:
		{
			r2::SArray<glm::vec2>* quadScales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec2, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)quadScales);

			for (u32 i = 0; i < numInstances; ++i)
			{
				glm::vec3 scale = r2::sarr::At(debugRenderComponents, i).scale;
				r2::sarr::Push(*quadScales, glm::vec2(scale.x, scale.y));
			}

			r2::draw::renderer::DrawQuadInstanced(*positions, *quadScales, *directions, *colors, filled, depthTest);
		}
		break;
		case draw::DEBUG_SPHERE:
			r2::draw::renderer::DrawSphereInstanced(*positions, *radii, *colors, filled, depthTest);
			break;
		case draw::DEBUG_CUBE:
			r2::draw::renderer::DrawCubeInstanced(*positions, *scales, *colors, filled, depthTest);
			break;
		case draw::DEBUG_CYLINDER:
			r2::draw::renderer::DrawCylinderInstanced(*positions, *directions, *radii, *scales, *colors, filled, depthTest);
			break;
		case draw::DEBUG_CONE:
			r2::draw::renderer::DrawConeInstanced(*positions, *directions, *radii, *scales, *colors, filled, depthTest);
			break;
		case draw::DEBUG_ARROW:
			r2::draw::renderer::DrawArrowInstanced(*positions, *directions, *scales, *radii, *colors, filled, depthTest);
			break;
		case draw::DEBUG_LINE:
		{
			//@NOTE: may need to redo this if we want the p0 position to be different than the position of the transform
			for (size_t i = 0; i <= numInstances; i++)
			{
				glm::vec3 p0 = r2::sarr::At(*positions, i);
				glm::vec3 p1 = p0 + r2::sarr::At(*scales, i) * r2::sarr::At(*directions, i);

				r2::draw::renderer::DrawLine(p0, p1, r2::sarr::At(*colors, i), !depthTest);
			}
		}
		break;
		default:
			R2_CHECK(false, "Unsupported ATM");
			break;
		}
	}

	void DebugRenderSystem::RenderDebugInstanced(const DebugRenderComponent& c, const TransformComponent& t, const InstanceComponentT<DebugRenderComponent>& instancedDebugRenderComponent,
		const InstanceComponentT<TransformComponent>& instancedTransformComponent)
	{
		auto numInstancesToUse = glm::min(instancedTransformComponent.numInstances + 1, instancedDebugRenderComponent.numInstances + 1);

		r2::SArray<void*>* tempAllocations = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, void*, 100);

		struct DebugRenderBatchInfo
		{
			bool depthTest = false;
			bool filled = false;

			r2::SArray<DebugRenderComponent>* debugRenderComponents = nullptr;
			r2::SArray<TransformComponent>* transformComponents = nullptr;
		};

		//figure out how many of each type we need
		r2::SArray<DebugRenderBatchInfo>* debugRenderComponentsPerModelType[r2::draw::NUM_DEBUG_MODELS];

		for (u32 i = 0; i < draw::NUM_DEBUG_MODELS; ++i)
		{
			debugRenderComponentsPerModelType[i] = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, DebugRenderBatchInfo, numInstancesToUse);
			r2::sarr::Push(*tempAllocations, (void*)debugRenderComponentsPerModelType[i]);
		}

		DebugRenderBatchInfo baseRenderBatchInfo;

		baseRenderBatchInfo.depthTest = c.depthTest;
		baseRenderBatchInfo.filled = c.filled;
		baseRenderBatchInfo.debugRenderComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, DebugRenderComponent, numInstancesToUse);
		r2::sarr::Push(*tempAllocations, (void*)baseRenderBatchInfo.debugRenderComponents);
		baseRenderBatchInfo.transformComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, TransformComponent, numInstancesToUse);
		r2::sarr::Push(*tempAllocations, (void*)baseRenderBatchInfo.transformComponents);
		r2::sarr::Push(*baseRenderBatchInfo.debugRenderComponents, c);
		r2::sarr::Push(*baseRenderBatchInfo.transformComponents, t);

		r2::sarr::Push(*debugRenderComponentsPerModelType[c.debugModelType], baseRenderBatchInfo);

		for (s32 i = 0; i < static_cast<s32>(numInstancesToUse) - 1; ++i)
		{
			const DebugRenderComponent& nextDebugRenderComponent = r2::sarr::At(*instancedDebugRenderComponent.instances, i);
			const TransformComponent& nextTransformComponent = r2::sarr::At(*instancedTransformComponent.instances, i);

			r2::SArray<DebugRenderBatchInfo>* batchInfoArray = debugRenderComponentsPerModelType[nextDebugRenderComponent.debugModelType];
			bool found = false;

			for (u32 j = 0;!found && j < r2::sarr::Size(*batchInfoArray); ++j)
			{
				DebugRenderBatchInfo& batchInfo = r2::sarr::At(*batchInfoArray, j);

				if (batchInfo.depthTest == nextDebugRenderComponent.depthTest && batchInfo.filled == nextDebugRenderComponent.filled)
				{
					found = true;
					r2::sarr::Push(*batchInfo.debugRenderComponents, nextDebugRenderComponent);
					r2::sarr::Push(*batchInfo.transformComponents, nextTransformComponent);
				}
			}

			if (!found)
			{
				DebugRenderBatchInfo newBatchInfo;

				newBatchInfo.depthTest = nextDebugRenderComponent.depthTest;
				newBatchInfo.filled = nextDebugRenderComponent.filled;

				newBatchInfo.debugRenderComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, DebugRenderComponent, numInstancesToUse);
				r2::sarr::Push(*newBatchInfo.debugRenderComponents, nextDebugRenderComponent);
				r2::sarr::Push(*tempAllocations, (void*)newBatchInfo.debugRenderComponents);
				
				newBatchInfo.transformComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, TransformComponent, numInstancesToUse);
				r2::sarr::Push(*newBatchInfo.transformComponents, nextTransformComponent);
				r2::sarr::Push(*tempAllocations, (void*)newBatchInfo.transformComponents);

				r2::sarr::Push(*batchInfoArray, newBatchInfo);
			}
		}

		for (u32 i = 0; i < r2::draw::NUM_DEBUG_MODELS; ++i)
		{
			const auto debugRenderComponentsForThisType = r2::sarr::Size(*debugRenderComponentsPerModelType[i]);

			if (debugRenderComponentsForThisType > 0)
			{
				for (u32 j = 0; j < debugRenderComponentsForThisType; ++j)
				{
					const auto& batchInfo = r2::sarr::At(*debugRenderComponentsPerModelType[i], j);
					DrawInstancedDebugModel(static_cast<r2::draw::DebugModelType>(i), tempAllocations, *batchInfo.debugRenderComponents, *batchInfo.transformComponents, batchInfo.filled, batchInfo.depthTest);
				}
			}
		}

		s32 numTempAllocations = r2::sarr::Size(*tempAllocations);

		for (s32 i = numTempAllocations - 1; i >= 0; --i)
		{
			FREE(r2::sarr::At(*tempAllocations, i), *MEM_ENG_SCRATCH_PTR);
		}

		FREE(tempAllocations, *MEM_ENG_SCRATCH_PTR);
	}
}

#endif