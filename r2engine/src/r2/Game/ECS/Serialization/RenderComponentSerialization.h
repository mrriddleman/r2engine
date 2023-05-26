#ifndef __RENDER_COMPONENT_SERIALIZATION_H__
#define __RENDER_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Serialization/RenderComponentArrayData_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/Renderer.h"

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
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<RenderComponent>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::RenderComponentData>>* renderComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::RenderComponentData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const RenderComponent& renderComponent = r2::sarr::At(components, i);

			flat::StencilOpBuilder stencilOpBuilder(fbb);
			stencilOpBuilder.add_depthFail(renderComponent.drawParameters.stencilState.op.depthFail);
			stencilOpBuilder.add_stencilFail(renderComponent.drawParameters.stencilState.op.stencilFail);
			stencilOpBuilder.add_depthAndStencilPass(renderComponent.drawParameters.stencilState.op.depthAndStencilPass);

			auto flatStencilOp = stencilOpBuilder.Finish();

			flat::StencilFuncBuilder stencilFuncBuilder(fbb);
			stencilFuncBuilder.add_func(renderComponent.drawParameters.stencilState.func.func);
			stencilFuncBuilder.add_mask(renderComponent.drawParameters.stencilState.func.mask);
			stencilFuncBuilder.add_ref(renderComponent.drawParameters.stencilState.func.ref);

			auto flatStencilFunc = stencilFuncBuilder.Finish();

			flat::StencilStateBuilder stencilStateBuilder(fbb);
			stencilStateBuilder.add_stencilEnabled(renderComponent.drawParameters.stencilState.stencilEnabled);
			stencilStateBuilder.add_stencilWriteEnabled(renderComponent.drawParameters.stencilState.stencilWriteEnabled);
			stencilStateBuilder.add_stencilOp(flatStencilOp);
			stencilStateBuilder.add_stencilFunc(flatStencilFunc);

			auto flatStencilState = stencilStateBuilder.Finish();

			std::vector<flatbuffers::Offset<flat::BlendFunc>> blendFuncs;

			for (u32 j = 0; j < renderComponent.drawParameters.blendState.numBlendFunctions; ++j)
			{
				flat::BlendFuncBuilder blendFuncBuilder(fbb);

				blendFuncBuilder.add_blendDrawBuffer(renderComponent.drawParameters.blendState.blendFunctions[j].blendDrawBuffer);
				blendFuncBuilder.add_dfactor(renderComponent.drawParameters.blendState.blendFunctions[j].dfactor);
				blendFuncBuilder.add_sfactor(renderComponent.drawParameters.blendState.blendFunctions[j].sfactor);

				blendFuncs.push_back(blendFuncBuilder.Finish());
			}

			auto blendFuncVec = fbb.CreateVector(blendFuncs);

			flat::BlendStateBuilder blendStateBuilder(fbb);
			blendStateBuilder.add_blendingEnabled(renderComponent.drawParameters.blendState.blendingEnabled);
			blendStateBuilder.add_blendEquation(renderComponent.drawParameters.blendState.blendEquation);
			blendStateBuilder.add_blendFunctions(blendFuncVec);
			
			auto flatBlendState = blendStateBuilder.Finish();

			flat::CullStateBuilder cullStateBuilder(fbb);
			cullStateBuilder.add_cullFace(renderComponent.drawParameters.cullState.cullFace);
			cullStateBuilder.add_cullingEnabled(renderComponent.drawParameters.cullState.cullingEnabled);
			cullStateBuilder.add_frontFace(renderComponent.drawParameters.cullState.frontFace);

			auto flatCullState = cullStateBuilder.Finish();

			flat::DrawParametersBuilder drawParametersBuilder(fbb);

			drawParametersBuilder.add_layer(renderComponent.drawParameters.layer);
			drawParametersBuilder.add_flags(renderComponent.drawParameters.flags.GetRawValue());
			drawParametersBuilder.add_stencilState(flatStencilState);
			drawParametersBuilder.add_blendState(flatBlendState);
			drawParametersBuilder.add_cullState(flatCullState);

			auto flatDrawParams = drawParametersBuilder.Finish();

			flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::OverrideMaterial>>> overrideMaterialOffset = 0;
			if (renderComponent.optrMaterialOverrideNames)
			{
				const auto numMaterialOverrides = r2::sarr::Size(*renderComponent.optrMaterialOverrideNames);
				std::vector<flatbuffers::Offset<flat::OverrideMaterial>> flatOverrideMaterials;

				for (u32 j = 0; j < numMaterialOverrides; ++j)
				{
					const auto& materialOverride = r2::sarr::At(*renderComponent.optrMaterialOverrideNames, j);

					flat::OverrideMaterialBuilder overrideMaterialBuilder(fbb);
					overrideMaterialBuilder.add_materialName(materialOverride.materialName);
					overrideMaterialBuilder.add_materialSystem(materialOverride.materialSystemName);

					flatOverrideMaterials.push_back(overrideMaterialBuilder.Finish());
				}

				overrideMaterialOffset = fbb.CreateVector(flatOverrideMaterials);
			}

			flat::RenderComponentDataBuilder renderComponentBuilder(fbb);

			renderComponentBuilder.add_assetModelHash(renderComponent.assetModelHash);
			renderComponentBuilder.add_primitiveType(renderComponent.primitiveType);
			renderComponentBuilder.add_isAnimated(renderComponent.isAnimated);
			renderComponentBuilder.add_drawParams(flatDrawParams);
			renderComponentBuilder.add_overrideMaterials(overrideMaterialOffset);

			r2::sarr::Push(*renderComponents, renderComponentBuilder.Finish());

		}

		auto vec = fbb.CreateVector(renderComponents->mData, renderComponents->mSize);

		flat::RenderComponentArrayDataBuilder renderComponentArrayDataBuilder(fbb);

		renderComponentArrayDataBuilder.add_renderComponentArray(vec);

		fbb.Finish(renderComponentArrayDataBuilder.Finish());

		FREE(renderComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<RenderComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		const flat::RenderComponentArrayData* renderComponentArrayData = flatbuffers::GetRoot<flat::RenderComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = renderComponentArrayData->renderComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::RenderComponentData* flatRenderComponent = componentVector->Get(i);
			
			RenderComponent renderComponent;
			renderComponent.gpuModelRefHandle = r2::draw::vb::InvalidGPUModelRefHandle;
			renderComponent.optrMaterialOverrideNames = nullptr;

			renderComponent.assetModelHash = flatRenderComponent->assetModelHash();
			renderComponent.isAnimated = flatRenderComponent->isAnimated();
			renderComponent.primitiveType = flatRenderComponent->primitiveType();

			renderComponent.drawParameters.layer = (r2::draw::DrawLayer)flatRenderComponent->drawParams()->layer();
			renderComponent.drawParameters.flags = { flatRenderComponent->drawParams()->flags() };
			
			r2::draw::renderer::SetDefaultBlendState(renderComponent.drawParameters);
			r2::draw::renderer::SetDefaultStencilState(renderComponent.drawParameters);
			r2::draw::renderer::SetDefaultCullState(renderComponent.drawParameters);

			renderComponent.drawParameters.stencilState.stencilEnabled = flatRenderComponent->drawParams()->stencilState()->stencilEnabled();
			renderComponent.drawParameters.stencilState.stencilWriteEnabled = flatRenderComponent->drawParams()->stencilState()->stencilWriteEnabled();
			renderComponent.drawParameters.stencilState.op.depthAndStencilPass = flatRenderComponent->drawParams()->stencilState()->stencilOp()->depthAndStencilPass();
			renderComponent.drawParameters.stencilState.op.depthFail = flatRenderComponent->drawParams()->stencilState()->stencilOp()->depthFail();
			renderComponent.drawParameters.stencilState.op.stencilFail = flatRenderComponent->drawParams()->stencilState()->stencilOp()->stencilFail();
			renderComponent.drawParameters.stencilState.func.func = flatRenderComponent->drawParams()->stencilState()->stencilFunc()->func();
			renderComponent.drawParameters.stencilState.func.mask = flatRenderComponent->drawParams()->stencilState()->stencilFunc()->mask();
			renderComponent.drawParameters.stencilState.func.ref = flatRenderComponent->drawParams()->stencilState()->stencilFunc()->ref();
			renderComponent.drawParameters.blendState.blendingEnabled = flatRenderComponent->drawParams()->blendState()->blendingEnabled();
			renderComponent.drawParameters.blendState.blendEquation = flatRenderComponent->drawParams()->blendState()->blendEquation();
			renderComponent.drawParameters.blendState.numBlendFunctions = flatRenderComponent->drawParams()->blendState()->blendFunctions()->size();
			for (flatbuffers::uoffset_t j = 0; j < renderComponent.drawParameters.blendState.numBlendFunctions; ++j)
			{
				renderComponent.drawParameters.blendState.blendFunctions[j].blendDrawBuffer = flatRenderComponent->drawParams()->blendState()->blendFunctions()->Get(j)->blendDrawBuffer();
				renderComponent.drawParameters.blendState.blendFunctions[j].dfactor = flatRenderComponent->drawParams()->blendState()->blendFunctions()->Get(j)->dfactor();
				renderComponent.drawParameters.blendState.blendFunctions[j].sfactor = flatRenderComponent->drawParams()->blendState()->blendFunctions()->Get(j)->sfactor();
			}
			renderComponent.drawParameters.cullState.cullFace = flatRenderComponent->drawParams()->cullState()->cullFace();
			renderComponent.drawParameters.cullState.cullingEnabled = flatRenderComponent->drawParams()->cullState()->cullingEnabled();
			renderComponent.drawParameters.cullState.frontFace = flatRenderComponent->drawParams()->cullState()->frontFace();

			if (flatRenderComponent->overrideMaterials())
			{
				renderComponent.optrMaterialOverrideNames = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, RenderMaterialOverride, flatRenderComponent->overrideMaterials()->size());

				for (flatbuffers::uoffset_t j = 0; j < flatRenderComponent->overrideMaterials()->size(); j++)
				{
					const auto* flatOverrideMaterial = flatRenderComponent->overrideMaterials()->Get(j);

					RenderMaterialOverride renderMaterialOverride;
					renderMaterialOverride.materialName = flatOverrideMaterial->materialName();
					renderMaterialOverride.materialSystemName = flatOverrideMaterial->materialSystem();

					r2::sarr::Push(*renderComponent.optrMaterialOverrideNames, renderMaterialOverride);
				}
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

			if (renderComponent.optrMaterialOverrideNames)
			{
				FREE(renderComponent.optrMaterialOverrideNames, *MEM_ENG_SCRATCH_PTR);
				renderComponent.optrMaterialOverrideNames = nullptr;
			}
			
		}
	}
}

#endif
