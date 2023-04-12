#ifndef __RENDER_COMPONENT_SERIALIZATION_H__
#define __RENDER_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/RenderComponent.h"

namespace r2::ecs
{
	/*
		struct RenderMaterialOverride
		{
			u64 materialSystemName;
			u64 materialName;
		};

		struct RenderComponent
		{
			u64 assetModelHash;
			r2::draw::PrimitiveType primitiveType;
			r2::draw::DrawParameters drawParameters;
			r2::draw::vb::GPUModelRefHandle gpuModelRefHandle;
			r2::SArray<r2::draw::MaterialHandle>* optrOverrideMaterials;
			r2::SArray<RenderMaterialOverride>* optrMaterialOverrideNames;
		};
	*/

	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<RenderComponent>& components)
	{
		builder.Vector([&]() {
			const auto numComponents = r2::sarr::Size(components);
			for (u32 i = 0; i < numComponents; ++i)
			{
				const auto& renderComponent = r2::sarr::At(components, i);
				builder.Vector([&]() {
					builder.UInt(renderComponent.assetModelHash);
					builder.UInt(static_cast<u32>(renderComponent.primitiveType));
					builder.Vector([&]() {
						builder.UInt(renderComponent.drawParameters.layer);
						builder.UInt(renderComponent.drawParameters.flags.GetRawValue());

						//@TODO(Serge): these will have to change when we switch to Vulkan - make the values completely agnostic
						builder.UInt(renderComponent.drawParameters.stencilState.stencilEnabled);
						builder.UInt(renderComponent.drawParameters.stencilState.stencilWriteEnabled);
						builder.UInt(renderComponent.drawParameters.stencilState.op.stencilFail);
						builder.UInt(renderComponent.drawParameters.stencilState.op.depthFail);
						builder.UInt(renderComponent.drawParameters.stencilState.op.depthAndStencilPass);
						builder.UInt(renderComponent.drawParameters.stencilState.func.func);
						builder.UInt(renderComponent.drawParameters.stencilState.func.ref);
						builder.UInt(renderComponent.drawParameters.stencilState.func.mask);
						builder.UInt(renderComponent.drawParameters.blendState.blendingEnabled);
						builder.UInt(renderComponent.drawParameters.blendState.blendEquation);
						
						
						for (u32 j = 0; j < 4; ++j)
						{
							builder.Vector([&]() {
								builder.UInt(renderComponent.drawParameters.blendState.blendFunctions[j].blendDrawBuffer);
								builder.UInt(renderComponent.drawParameters.blendState.blendFunctions[j].sfactor);
								builder.UInt(renderComponent.drawParameters.blendState.blendFunctions[j].dfactor);
							});
						}
						builder.UInt(renderComponent.drawParameters.blendState.numBlendFunctions);
						builder.UInt(renderComponent.drawParameters.cullState.cullingEnabled);
						builder.UInt(renderComponent.drawParameters.cullState.cullFace);
						builder.UInt(renderComponent.drawParameters.cullState.frontFace);

					});

					builder.Vector([&]() {
						if (renderComponent.optrMaterialOverrideNames)
						{
							const auto numMaterialOverrides = r2::sarr::Size(*renderComponent.optrMaterialOverrideNames);
							for (u32 j = 0; j < numMaterialOverrides; ++j)
							{
								const auto& materialOverrideName = r2::sarr::At(*renderComponent.optrMaterialOverrideNames, j);
								builder.TypedVector([&]() {
									builder.UInt(materialOverrideName.materialSystemName);
									builder.UInt(materialOverrideName.materialName);
								});
							}
						}
					});
				});
			}
		});
	}
}

#endif
