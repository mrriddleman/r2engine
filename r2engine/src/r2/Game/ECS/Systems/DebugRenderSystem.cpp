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
			const InstanceComponent* instanceComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponent>(e);

			bool useInstancing = instanceComponent != nullptr;

			if (!useInstancing)
			{
				RenderDebug(debugRenderComponent, transformComponent);
			}
			else
			{
				RenderDebugInstanced(debugRenderComponent, transformComponent, *instanceComponent);
			}
		}
	}

	void DebugRenderSystem::RenderDebug(const DebugRenderComponent& c, const TransformComponent& t)
	{
		glm::vec3 position = t.accumTransform.position;
		float radius = 1.0f;
		if (c.radii)
		{
			radius = r2::sarr::At(*c.radii, 0);
		}

		glm::vec4 color = r2::sarr::At(*c.colors, 0);

		float scale = 1.0f;
		if (c.scales)
		{
			scale = r2::sarr::At(*c.scales, 0);
		}

		glm::vec3 dir = glm::vec3(0);
		if (c.directions)
		{
			dir = r2::sarr::At(*c.directions, 0);
		}

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

	void DebugRenderSystem::RenderDebugInstanced(const DebugRenderComponent& c, const TransformComponent& t, const InstanceComponent& instanceComponent)
	{
		//need to make the positions
		u32 numInstances = r2::sarr::Size(*instanceComponent.offsets);
		r2::SArray<glm::vec3>* positions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::vec3, numInstances + 1);

		//calculate the positions
		glm::vec3 basePosition = t.accumTransform.position;

		r2::sarr::Push(*positions, basePosition);

		for (u32 i = 0; i < numInstances; ++i)
		{
			r2::sarr::Push(*positions, basePosition + r2::sarr::At(*instanceComponent.offsets, i));
		}

		switch (c.debugModelType)
		{
		case draw::DEBUG_QUAD:
			R2_CHECK(false, "Unsupported ATM");
			break;
		case draw::DEBUG_SPHERE:
			r2::draw::renderer::DrawSphereInstanced(*positions, *c.radii, *c.colors, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CUBE:
			r2::draw::renderer::DrawCubeInstanced(*positions, *c.scales, *c.colors, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CYLINDER:
			r2::draw::renderer::DrawCylinderInstanced(*positions, *c.directions, *c.radii, *c.scales, *c.colors, c.filled, c.depthTest);
			break;
		case draw::DEBUG_CONE:
			r2::draw::renderer::DrawConeInstanced(*positions, *c.directions, *c.radii, *c.scales, *c.colors, c.filled, c.depthTest);
			break;
		case draw::DEBUG_ARROW:
			r2::draw::renderer::DrawArrowInstanced(*positions, *c.directions, *c.scales, *c.radii, *c.colors, c.filled, c.depthTest);
			break;
		case draw::DEBUG_LINE:
		{
			//@NOTE: may need to redo this if we want the p0 position to be different than the position of the transform
			for (size_t i = 0; i <= numInstances; i++)
			{
				glm::vec3 p0 = r2::sarr::At(*positions, i);
				glm::vec3 p1 = p0 + r2::sarr::At(*c.scales, i) * r2::sarr::At(*c.directions, i);

				r2::draw::renderer::DrawLine(p0, p1, r2::sarr::At(*c.colors, i), !c.depthTest);
			}
		}
		break;
		default:
			R2_CHECK(false, "Unsupported ATM");
			break;
		}

		FREE(positions, *MEM_ENG_SCRATCH_PTR);
	}

}

#endif