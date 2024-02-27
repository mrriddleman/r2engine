#include "FacingUpdateSystem.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "FacingComponent.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>

FacingUpdateSystem::FacingUpdateSystem()
{

}

void FacingUpdateSystem::Update()
{
	//This will only get called if there's a FacingComponent, TransformComponent, and a TransformDirtyComponent
	const auto numEntities = r2::sarr::Size(*mEntities);

	for (u32 i = 0; i < numEntities; ++i)
	{
		r2::ecs::Entity e = r2::sarr::At(*mEntities, i);

		const r2::ecs::TransformComponent& transformComponent = mnoptrCoordinator->GetComponent<r2::ecs::TransformComponent>(e);
		FacingComponent& facingComponent = mnoptrCoordinator->GetComponent<FacingComponent>(e);

		//we need to figure out how to snap the transform to the closest cardinal direction

		glm::vec3 eulerAngles = glm::eulerAngles(transformComponent.accumTransform.rotation);

		glm::mat3 rotationMat = glm::mat3(1.0);
		glm::vec3 transformedVector = glm::rotateZ(glm::vec3(0, -1, 0), eulerAngles.z);

		if (glm::abs(transformedVector.x) > glm::abs(transformedVector.y))
		{
			facingComponent.facing = glm::sign(transformedVector.x) * glm::vec3(1, 0, 0);
		}
		else
		{
			facingComponent.facing = glm::sign(transformedVector.y) * glm::vec3(0, 1, 0);
		}
	}
}
