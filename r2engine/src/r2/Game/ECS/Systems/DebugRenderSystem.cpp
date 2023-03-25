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
		glm::vec3 position = t.accumTransform.position;
		float radius = c.radius;
		glm::vec4 color = c.color;
		float scale = c.scale;
		glm::vec3 dir = c.direction;

		switch (c.debugModelType)
		{
		case draw::DEBUG_QUAD:
			R2_CHECK(false, "Unsupported ATM");
			break;
		case draw::DEBUG_SPHERE:
			r2::draw::renderer::DrawSphere(position, radius, color, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CUBE:
			r2::draw::renderer::DrawCube(position, scale, color, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CYLINDER:
			r2::draw::renderer::DrawCylinder(position, dir, radius, scale, color, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CONE:
			r2::draw::renderer::DrawCone(position, dir, radius, scale, color, c.filled, c.depthTest);
			break;
		case draw::DEBUG_ARROW:
			r2::draw::renderer::DrawArrow(position, dir, scale, radius, color, c.filled, c.depthTest);
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

	void DebugRenderSystem::RenderDebugInstanced(const DebugRenderComponent& c, const TransformComponent& t, const InstanceComponentT<DebugRenderComponent>& instancedDebugRenderComponent,
		const InstanceComponentT<TransformComponent>& instancedTransformComponent)
	{
		R2_CHECK(instancedTransformComponent.numInstances == instancedDebugRenderComponent.numInstances, "We have mismatching transform and debug render instances");

		//need to make the positions
		const auto numInstances = instancedTransformComponent.numInstances;
		r2::SArray<glm::vec3>* positions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances + 1);

		//calculate the positions
		r2::sarr::Push(*positions, t.accumTransform.position);
		for (u32 i = 0; i < numInstances; ++i)
		{
			r2::sarr::Push(*positions, r2::sarr::At(*instancedTransformComponent.instances, i).accumTransform.position);
		}

		//make the colors
		r2::SArray<glm::vec4>* colors = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec4, numInstances + 1);

		r2::sarr::Push(*colors, c.color);
		for (u32 i = 0; i < numInstances; ++i)
		{
			r2::sarr::Push(*colors, r2::sarr::At(*instancedDebugRenderComponent.instances, i).color);
		}

		r2::SArray<glm::vec3>* directions = nullptr;
		r2::SArray<float>* radii = nullptr;
		r2::SArray<float>* scales = nullptr;

		if (c.debugModelType == draw::DEBUG_ARROW ||
			c.debugModelType == draw::DEBUG_CYLINDER ||
			c.debugModelType == draw::DEBUG_CONE)
		{
			radii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances + 1);
			r2::sarr::Push(*radii, c.radius);
			for (u32 i = 0; i < numInstances; ++i)
			{
				r2::sarr::Push(*radii, r2::sarr::At(*instancedDebugRenderComponent.instances, i).radius);
			}

			scales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances + 1);
			r2::sarr::Push(*scales, c.scale);
			for (u32 i = 0; i < numInstances; ++i)
			{
				r2::sarr::Push(*scales, r2::sarr::At(*instancedDebugRenderComponent.instances, i).scale);
			}

			directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances + 1);
			r2::sarr::Push(*directions, c.direction);
			for (u32 i = 0; i < numInstances; ++i)
			{
				r2::sarr::Push(*directions, r2::sarr::At(*instancedDebugRenderComponent.instances, i).direction);
			}
		}


		if (draw::DEBUG_SPHERE)
		{
			radii = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances + 1);
			r2::sarr::Push(*radii, c.radius);
			for (u32 i = 0; i < numInstances; ++i)
			{
				r2::sarr::Push(*radii, r2::sarr::At(*instancedDebugRenderComponent.instances, i).radius);
			}
		}

		if (draw::DEBUG_CUBE)
		{
			scales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances + 1);
			r2::sarr::Push(*scales, c.scale);
			for (u32 i = 0; i < numInstances; ++i)
			{
				r2::sarr::Push(*scales, r2::sarr::At(*instancedDebugRenderComponent.instances, i).scale);
			}
		}

		if (draw::DEBUG_LINE)
		{
			scales = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, float, numInstances + 1);
			r2::sarr::Push(*scales, c.scale);
			for (u32 i = 0; i < numInstances; ++i)
			{
				r2::sarr::Push(*scales, r2::sarr::At(*instancedDebugRenderComponent.instances, i).scale);
			}

			directions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances + 1);
			r2::sarr::Push(*directions, c.direction);
			for (u32 i = 0; i < numInstances; ++i)
			{
				r2::sarr::Push(*directions, r2::sarr::At(*instancedDebugRenderComponent.instances, i).direction);
			}
		}

		switch (c.debugModelType)
		{
		case draw::DEBUG_QUAD:
			R2_CHECK(false, "Unsupported ATM");
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

		if (directions)
		{
			FREE(directions, *MEM_ENG_SCRATCH_PTR);
		}

		if (scales)
		{
			FREE(scales, *MEM_ENG_SCRATCH_PTR);
		}

		if (radii)
		{
			FREE(radii, *MEM_ENG_SCRATCH_PTR);
		}

		FREE(colors, *MEM_ENG_SCRATCH_PTR);
		FREE(positions, *MEM_ENG_SCRATCH_PTR);
	}
}

#endif