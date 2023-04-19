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
					builder.UInt(renderComponent.isAnimated);
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

	template<>
	inline void DeSerializeComponentArray(r2::SArray<RenderComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		auto componentVector = componentArrayData->componentArray_flexbuffer_root().AsVector();

		for (size_t i = 0; i < componentVector.size(); ++i)
		{
			auto flexRenderComponent = componentVector[i].AsVector();
			RenderComponent renderComponent;
			renderComponent.optrOverrideMaterials = nullptr;
			renderComponent.optrMaterialOverrideNames = nullptr;
			renderComponent.gpuModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;

			renderComponent.assetModelHash = flexRenderComponent[0].AsUInt64();
			renderComponent.primitiveType = flexRenderComponent[1].AsUInt32();
			renderComponent.isAnimated = flexRenderComponent[2].AsUInt32();

			auto flexDrawParams = flexRenderComponent[3].AsVector();

			renderComponent.drawParameters.layer = (r2::draw::DrawLayer)flexDrawParams[0].AsUInt32();
			renderComponent.drawParameters.flags = {flexDrawParams[1].AsUInt32()};
			renderComponent.drawParameters.stencilState.stencilEnabled = flexDrawParams[2].AsUInt32();
			renderComponent.drawParameters.stencilState.stencilWriteEnabled = flexDrawParams[3].AsUInt32();
			renderComponent.drawParameters.stencilState.op.stencilFail = flexDrawParams[4].AsUInt32();
			renderComponent.drawParameters.stencilState.op.depthFail = flexDrawParams[5].AsUInt32();
			renderComponent.drawParameters.stencilState.op.depthAndStencilPass = flexDrawParams[6].AsUInt32();
			renderComponent.drawParameters.stencilState.func.func = flexDrawParams[7].AsUInt32();
			renderComponent.drawParameters.stencilState.func.ref = flexDrawParams[8].AsUInt32();
			renderComponent.drawParameters.stencilState.func.mask = flexDrawParams[9].AsUInt32();
			renderComponent.drawParameters.blendState.blendingEnabled = flexDrawParams[10].AsUInt32();
			renderComponent.drawParameters.blendState.blendEquation = flexDrawParams[11].AsUInt32();
			
			for (u32 j = 0; j < 4; ++j)
			{
				auto flexBlendFunction = flexDrawParams[12u + j].AsVector();
				renderComponent.drawParameters.blendState.blendFunctions[j].blendDrawBuffer = flexBlendFunction[0].AsUInt32();
				renderComponent.drawParameters.blendState.blendFunctions[j].sfactor = flexBlendFunction[1].AsUInt32();
				renderComponent.drawParameters.blendState.blendFunctions[j].dfactor = flexBlendFunction[2].AsUInt32();
			}

			renderComponent.drawParameters.blendState.numBlendFunctions = flexDrawParams[16].AsUInt32();
			renderComponent.drawParameters.cullState.cullingEnabled = flexDrawParams[17].AsUInt32();
			renderComponent.drawParameters.cullState.cullFace = flexDrawParams[18].AsUInt32();
			renderComponent.drawParameters.cullState.frontFace = flexDrawParams[19].AsUInt32();

			auto flexMaterialOverrideNames = flexRenderComponent[4].AsVector();

			u32 numMaterialOverrides = flexMaterialOverrideNames.size();

			renderComponent.optrMaterialOverrideNames = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ecs::RenderMaterialOverride, numMaterialOverrides);

			for (u32 j = 0; j < numMaterialOverrides; ++j)
			{
				auto flexMaterialOverride = flexMaterialOverrideNames[j].AsVector();

				ecs::RenderMaterialOverride renderMaterialOverride;
				renderMaterialOverride.materialSystemName = flexMaterialOverride[0].AsUInt64();
				renderMaterialOverride.materialName = flexMaterialOverride[1].AsUInt64();

				r2::sarr::Push(*renderComponent.optrMaterialOverrideNames, renderMaterialOverride);
			}

			r2::sarr::Push(components, renderComponent);
		}
	}

	template<>
	inline void CleanupDeserializeComponentArray(r2::SArray<RenderComponent>& components)
	{
		s32 size = r2::sarr::Size(components);

		for (s32 i = size - 1; i >= 0; --i)
		{
			RenderComponent& renderComponent = r2::sarr::At(components, i);
			FREE(renderComponent.optrMaterialOverrideNames, *MEM_ENG_SCRATCH_PTR);
			renderComponent.optrMaterialOverrideNames = nullptr;
		}
	}
}

#endif
