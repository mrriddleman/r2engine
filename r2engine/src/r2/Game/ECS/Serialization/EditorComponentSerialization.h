#ifndef __EDITOR_COMPONENT_H__
#define __EDITOR_COMPONENT_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/EditorComponent.h"

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
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<EditorComponent>& components)
	{
		builder.Vector([&]() {
			const auto numComponents = r2::sarr::Size(components);
			for (u32 i = 0; i < numComponents; ++i)
			{
				const auto& c = r2::sarr::At(components, i);
				builder.Vector([&]() {
					builder.String(c.editorName);
					builder.UInt(c.flags.GetRawValue());
				});
			}
		});
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<EditorComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(r2::sarr::Size(components) == 0, "Shouldn't have anything in there yet?");
		R2_CHECK(componentArrayData != nullptr, "Shouldn't be nullptr");

		auto componentVector = componentArrayData->componentArray_flexbuffer_root().AsVector();

		for (size_t i = 0; i < componentVector.size(); ++i)
		{
			const auto& component = componentVector[i].AsVector();

			EditorComponent editorComponent;

			editorComponent.editorName = component[0].AsString().str();
			editorComponent.flags = { component[1].AsUInt32() };

			r2::sarr::Push(components, editorComponent);
		}
	}

}

#endif // __EDITOR_COMPONENT_H__
