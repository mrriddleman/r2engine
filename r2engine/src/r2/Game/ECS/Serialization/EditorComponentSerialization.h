#ifndef __EDITOR_COMPONENT_H__
#define __EDITOR_COMPONENT_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Serialization/EditorComponentArrayData_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"

namespace r2::ecs
{
	/*
		struct EditorComponent
		{
			std::string editorName;
			Flags<u32, u32> flags;
		};
	*/
	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<EditorComponent>& components)
	{	
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::EditorComponentData>>* editorComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::EditorComponentData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const EditorComponent& editorComponent = r2::sarr::At(components, i);

			flat::EditorComponentDataBuilder editorComponentBuilder(fbb);

			editorComponentBuilder.add_editorName(fbb.CreateString(editorComponent.editorName));
			editorComponentBuilder.add_flags(editorComponent.flags.GetRawValue());

			r2::sarr::Push(*editorComponents, editorComponentBuilder.Finish());
		}

		flat::EditorComponentArrayDataBuilder editorComponentArrayDataBuilder(fbb);

		editorComponentArrayDataBuilder.add_editorComponentArray(fbb.CreateVector(editorComponents->mData, editorComponents->mSize));

		fbb.Finish(editorComponentArrayDataBuilder.Finish());

		FREE(editorComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<EditorComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(r2::sarr::Size(components) == 0, "Shouldn't have anything in there yet?");
		R2_CHECK(componentArrayData != nullptr, "Shouldn't be nullptr");

		const flat::EditorComponentArrayData* editorComponentArrayData = flatbuffers::GetRoot<flat::EditorComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = editorComponentArrayData->editorComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::EditorComponentData* flatEditorComponent = componentVector->Get(i);

			EditorComponent editorComponent;
			editorComponent.editorName = flatEditorComponent->editorName()->str();
			editorComponent.flags = { flatEditorComponent->flags() };

			r2::sarr::Push(components, editorComponent);
		}
	}
}

#endif // __EDITOR_COMPONENT_H__
