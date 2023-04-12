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
}

#endif // __EDITOR_COMPONENT_H__
