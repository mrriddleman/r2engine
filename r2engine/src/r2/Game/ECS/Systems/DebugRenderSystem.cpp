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
		r2::SArray<u32>* indicesPerType,
		r2::SArray<void*>* tempAllocations,
		const DebugRenderComponent& c,
		const TransformComponent& t,
		const InstanceComponentT<DebugRenderComponent>& instancedDebugRenderComponent,
		const InstanceComponentT<TransformComponent>& instancedTransformComponent)
	{
		//need to make the positions
		const auto numInstances = r2::sarr::Capacity(*indicesPerType);

		r2::SArray<glm::vec3>* positions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances);
		r2::sarr::Push(*tempAllocations, (void*)positions);
		const bool addC = c.debugModelType == debugModelType;

		//calculate the positions
		if (addC)
		{
			r2::sarr::Push(*positions, t.accumTransform.position + c.offset);
		}

		for (u32 i = 0; i < r2::sarr::Size(*indicesPerType); ++i)
		{
			const auto& position = r2::sarr::At(*instancedTransformComponent.instances, r2::sarr::At(*indicesPerType, i)).accumTransform.position;
			const auto& offset = r2::sarr::At(*instancedDebugRenderComponent.instances, r2::sarr::At(*indicesPerType, i)).offset;

			r2::sarr::Push(*positions, position + offset);
		}

		//make the colors
		r2::SArray<glm::vec4>* colors = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec4, numInstances);
		r2::sarr::Push(*tempAllocations, (void*)colors);

		if (addC)
		{
			r2::sarr::Push(*colors, c.color);
		}
		
		for (u32 i = 0; i < r2::sarr::Size(*indicesPerType); ++i)
		{
			const auto& color = r2::sarr::At(*instancedDebugRenderComponent.instances, r2::sarr::At(*indicesPerType, i)).color;
			r2::sarr::Push(*colors, color);
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

			if (addC)
			{
				r2::sarr::Push(*radii, c.radius);
			}

			for (u32 i = 0; i < r2::sarr::Size(*indicesPerType); ++i)
			{
				const auto& radius = r2::sarr::At(*instancedDebugRenderComponent.instances, r2::sarr::At(*indicesPerType, i)).radius;
				r2::sarr::Push(*radii, radius);
			}

			scales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)scales);

			if (addC)
			{
				r2::sarr::Push(*scales, c.scale.x);
			}
			
			for (u32 i = 0; i < r2::sarr::Size(*indicesPerType); ++i)
			{
				const auto& scale = r2::sarr::At(*instancedDebugRenderComponent.instances, r2::sarr::At(*indicesPerType, i)).scale.x;
				r2::sarr::Push(*scales, scale);
			}

			directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)directions);

			if (addC)
			{
				r2::sarr::Push(*directions, c.direction);
			}
			
			for (u32 i = 0; i < r2::sarr::Size(*indicesPerType); ++i)
			{
				const auto& direction = r2::sarr::At(*instancedDebugRenderComponent.instances, r2::sarr::At(*indicesPerType, i)).direction;
				r2::sarr::Push(*directions, direction);
			}
		}

		if (debugModelType == draw::DEBUG_SPHERE)
		{
			radii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)radii);

			if (addC)
			{
				r2::sarr::Push(*radii, c.radius);
			}

			for (u32 i = 0; i < r2::sarr::Size(*indicesPerType); ++i)
			{
				const auto& radius = r2::sarr::At(*instancedDebugRenderComponent.instances, r2::sarr::At(*indicesPerType, i)).radius;
				r2::sarr::Push(*radii, radius);
			}
		}

		if (debugModelType == draw::DEBUG_CUBE)
		{
			scales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)scales);

			if (addC)
			{
				r2::sarr::Push(*scales, c.scale.x);
			}

			for (u32 i = 0; i < r2::sarr::Size(*indicesPerType); ++i)
			{
				const auto& scale = r2::sarr::At(*instancedDebugRenderComponent.instances, r2::sarr::At(*indicesPerType, i)).scale.x;
				r2::sarr::Push(*scales, scale);
			}
		}

		if (debugModelType == draw::DEBUG_LINE)
		{
			scales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances);

			r2::sarr::Push(*tempAllocations, (void*)scales);
			if (addC)
			{
				r2::sarr::Push(*scales, c.scale.x);
			}
			
			for (u32 i = 0; i < r2::sarr::Size(*indicesPerType); ++i)
			{
				const auto& scale = r2::sarr::At(*instancedDebugRenderComponent.instances, r2::sarr::At(*indicesPerType, i)).scale.x;
				r2::sarr::Push(*scales, scale);
			}

			directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances);
			r2::sarr::Push(*tempAllocations, (void*)directions);

			if (addC)
			{
				r2::sarr::Push(*directions, c.direction);
			}

			for (u32 i = 0; i < r2::sarr::Size(*indicesPerType); ++i)
			{
				const auto& direction = r2::sarr::At(*instancedDebugRenderComponent.instances, r2::sarr::At(*indicesPerType, i)).direction;
				r2::sarr::Push(*directions, direction);
			}
		}

		if (debugModelType == draw::DEBUG_QUAD)
		{
			directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances + 1);

			r2::sarr::Push(*tempAllocations, (void*)directions);

			if (addC)
			{
				r2::sarr::Push(*directions, c.direction);
			}
			
			for (u32 i = 0; i < r2::sarr::Size(*indicesPerType); ++i)
			{
				const auto& direction = r2::sarr::At(*instancedDebugRenderComponent.instances, r2::sarr::At(*indicesPerType, i)).direction;
				r2::sarr::Push(*directions, direction);
			}
		}

		switch (debugModelType)
		{
		case draw::DEBUG_QUAD:
		{
			r2::SArray<glm::vec2>* quadScales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec2, numInstances);

			r2::sarr::Push(*tempAllocations, (void*)quadScales);

			if (addC)
			{
				r2::sarr::Push(*quadScales, glm::vec2(c.scale.x, c.scale.y));
			}

			for (u32 i = 0; i < r2::sarr::Size(*indicesPerType); ++i)
			{
				glm::vec3 scale = r2::sarr::At(*instancedDebugRenderComponent.instances, r2::sarr::At(*indicesPerType, i)).scale;

				r2::sarr::Push(*quadScales, glm::vec2(scale.x, scale.y));
			}
			r2::draw::renderer::DrawQuadInstanced(*positions, *quadScales, *directions, *colors, c.filled, c.depthTest);

			//	FREE(quadScales, *MEM_ENG_SCRATCH_PTR);
		}
		break;
		case draw::DEBUG_SPHERE:
			r2::draw::renderer::DrawSphereInstanced(*positions, *radii, *colors, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CUBE:
			r2::draw::renderer::DrawCubeInstanced(*positions, *scales, *colors, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CYLINDER:
			r2::draw::renderer::DrawCylinderInstanced(*positions, *directions, *radii, *scales, *colors, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CONE:
			r2::draw::renderer::DrawConeInstanced(*positions, *directions, *radii, *scales, *colors, c.filled, c.depthTest);
			break;
		case draw::DEBUG_ARROW:
			r2::draw::renderer::DrawArrowInstanced(*positions, *directions, *scales, *radii, *colors, c.filled, c.depthTest);
			break;
		case draw::DEBUG_LINE:
		{
			//@NOTE: may need to redo this if we want the p0 position to be different than the position of the transform
			for (size_t i = 0; i <= numInstances; i++)
			{
				glm::vec3 p0 = r2::sarr::At(*positions, i);
				glm::vec3 p1 = p0 + r2::sarr::At(*scales, i) * r2::sarr::At(*directions, i);

				r2::draw::renderer::DrawLine(p0, p1, r2::sarr::At(*colors, i), !c.depthTest);
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
		R2_CHECK(instancedTransformComponent.numInstances == instancedDebugRenderComponent.numInstances, "We have mismatching transform and debug render instances");

		r2::SArray<void*>* tempAllocations = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, void*, 100);

		//figure out how many of each type we need
		u32 numInstancesPerType[r2::draw::NUM_DEBUG_MODELS];
		memset(numInstancesPerType, 0, sizeof(u32) * r2::draw::NUM_DEBUG_MODELS);

		numInstancesPerType[c.debugModelType] = 1;

		for (u32 i = 0; i < instancedDebugRenderComponent.numInstances; ++i)
		{
			const DebugRenderComponent& nextDebugRenderComponent = r2::sarr::At(*instancedDebugRenderComponent.instances, i);
			numInstancesPerType[nextDebugRenderComponent.debugModelType]++;
		}

		InstanceIndices indicesPerType[r2::draw::NUM_DEBUG_MODELS];

		for (u32 i = 0; i < r2::draw::NUM_DEBUG_MODELS; ++i)
		{
			if (numInstancesPerType[i] > 0)
			{
				indicesPerType[i].indices = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u32, numInstancesPerType[i]);
				r2::sarr::Push(*tempAllocations, (void*)indicesPerType[i].indices);
			}
		}

		for (u32 i = 0; i < instancedDebugRenderComponent.numInstances; ++i)
		{
			const DebugRenderComponent& nextDebugRenderComponent = r2::sarr::At(*instancedDebugRenderComponent.instances, i);

			r2::sarr::Push(*indicesPerType[nextDebugRenderComponent.debugModelType].indices, i);
		}

		for (u32 i = 0; i < r2::draw::NUM_DEBUG_MODELS; ++i)
		{
			if (numInstancesPerType[i] > 0)
			{
				DrawInstancedDebugModel(static_cast<r2::draw::DebugModelType>(i), indicesPerType[i].indices, tempAllocations, c, t, instancedDebugRenderComponent, instancedTransformComponent);
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