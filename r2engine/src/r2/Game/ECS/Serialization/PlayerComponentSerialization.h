#ifndef __PLAYER_COMPONENT_SERIALIZATION_H__
#define __PLAYER_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Serialization/PlayerComponentArrayData_generated.h"
#include "r2/Game/ECS/Components/PlayerComponent.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"

#include "r2/Core/Containers/SArray.h"


namespace r2::ecs
{
	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<PlayerComponent>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::Player>>* playerComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::Player>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const PlayerComponent& playerComponent = r2::sarr::At(components, i);

			flat::PlayerBuilder playerBuilder(fbb);
			playerBuilder.add_playerID(playerComponent.playerID);

			r2::sarr::Push(*playerComponents, playerBuilder.Finish());
		}

		auto vec = fbb.CreateVector(playerComponents->mData, playerComponents->mSize);

		flat::PlayerComponentArrayDataBuilder playerComponentArrayDataBuilder(fbb);

		playerComponentArrayDataBuilder.add_playerDataComponentArray(vec);

		fbb.Finish(playerComponentArrayDataBuilder.Finish());

		FREE(playerComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	inline void DeSerializeComponentArray(ECSWorld& ecsWorld, r2::SArray<PlayerComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(r2::sarr::Size(components) == 0, "Shouldn't have anything in there yet?");
		R2_CHECK(componentArrayData != nullptr, "Shouldn't be nullptr");

		const flat::PlayerComponentArrayData* playerComponentArrayData = flatbuffers::GetRoot<flat::PlayerComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = playerComponentArrayData->playerDataComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::Player* flatPlayerData = componentVector->Get(i);

			PlayerComponent playerComponent;

			playerComponent.playerID = flatPlayerData->playerID();
			//@TODO(Serge): now set the input type - not sure how we're going to do that since the game is what controls this - for now set to defaults
			//playerComponent.inputType = {};

			r2::sarr::Push(components, playerComponent);
		}
	}
}

#endif