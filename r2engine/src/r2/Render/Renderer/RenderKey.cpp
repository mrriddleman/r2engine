#include "r2pch.h"

#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Renderer/RendererImpl.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Utils/Timer.h"

namespace r2::draw::key
{
	const u8 Basic::FSL_GAME = 0;
	const u8 Basic::FSL_EFFECT = 1;
	const u8 Basic::FSL_HUD = 2;
	const u8 Basic::FSL_OUTPUT = 3;

	const u8 NUM_MATERIAL_SYSTEM_BITS = 4;



	bool CompareDebugKey(const DebugKey& a, const DebugKey& b)
	{
		return a.keyValue < b.keyValue;
	}

	DebugKey GenerateDebugKey(r2::draw::DrawLayer drawLayer, bool depthTest, u8 translucency, PrimitiveType primitiveType, r2::draw::ShaderHandle shaderID)
	{
		DebugKey key;

		key.keyValue |= ENCODE_KEY_VALUE(shaderID, DebugKey::DEBUG_KEY_BITS_SHADER_ID, DebugKey::DEBUG_KEY_SHADER_ID_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE(primitiveType, DebugKey::DEBUG_KEY_BITS_PRIMITIVE_TYPE, DebugKey::DEBUG_KEY_PRIMITIVE_TYPE_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((depthTest ? 1 : 0), DebugKey::DEBUG_KEY_BITS_DEPTH_ENABLED, DebugKey::DEBUG_KEY_DEPTH_ENABLED_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE(translucency, DebugKey::DEBUG_KEY_BITS_TRANSLUCENCY, DebugKey::DEBUG_KEY_TRANSLUCENCY_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE(drawLayer, DebugKey::DEBUG_KEY_BITS_LAYER, DebugKey::DEBUG_KEY_LAYER_OFFSET);

		return key;
	}

	void DecodeDebugKey(const DebugKey& key)
	{
		if (key.keyValue == 0)
			return;

		u32 shaderID = DECODE_KEY_VALUE(key.keyValue, DebugKey::DEBUG_KEY_BITS_SHADER_ID, DebugKey::DEBUG_KEY_SHADER_ID_OFFSET);
		u32 primitiveType = DECODE_KEY_VALUE(key.keyValue, DebugKey::DEBUG_KEY_BITS_PRIMITIVE_TYPE, DebugKey::DEBUG_KEY_PRIMITIVE_TYPE_OFFSET);
		u32 depthEnabled = DECODE_KEY_VALUE(key.keyValue, DebugKey::DEBUG_KEY_BITS_DEPTH_ENABLED, DebugKey::DEBUG_KEY_DEPTH_ENABLED_OFFSET);
		u32 translucency = DECODE_KEY_VALUE(key.keyValue, DebugKey::DEBUG_KEY_BITS_TRANSLUCENCY, DebugKey::DEBUG_KEY_TRANSLUCENCY_OFFSET);
		u32 layer = DECODE_KEY_VALUE(key.keyValue, DebugKey::DEBUG_KEY_BITS_LAYER, DebugKey::DEBUG_KEY_LAYER_OFFSET);

		r2::draw::rendererimpl::SetViewportKey(0);
		r2::draw::rendererimpl::SetViewportLayer(DrawLayer::DL_DEBUG);
		r2::draw::rendererimpl::SetShaderID(shaderID);

	}

	bool CompareBasicKey(const Basic& a, const Basic& b)
	{
		return a.keyValue < b.keyValue;
	}

	Basic GenerateBasicKey(u8 fullscreenLayer, u8 viewport, DrawLayer viewportLayer, u8 translucency, u32 depth, r2::draw::ShaderHandle shaderID, u8 pass)
	{
		Basic key;

		key.keyValue |= ENCODE_KEY_VALUE((u64)fullscreenLayer, Basic::KEY_BITS_FULL_SCREEN_LAYER, Basic::KEY_FULL_SCREEN_LAYER_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)viewport, Basic::KEY_BITS_VIEWPORT, Basic::KEY_VIEWPORT_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)viewportLayer, Basic::KEY_BITS_VIEWPORT_LAYER, Basic::KEY_VIEWPORT_LAYER_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)translucency, Basic::KEY_BITS_TRANSLUCENCY, Basic::KEY_TRANSLUCENCY_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)depth, Basic::KEY_BITS_DEPTH, Basic::KEY_DEPTH_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)shaderID, Basic::KEY_BITS_MATERIAL_ID, Basic::KEY_MATERIAL_ID_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)pass, Basic::KEY_BITS_PASS, Basic::KEY_PASS_OFFSET);

		return key;
	}

	void DecodeBasicKey(const Basic& key)
	{
		//kind of a huge hack right now
		//we need some way of saying don't do anything for things like Fill*Buffer
		if (key.keyValue == 0)
		{
			return;
		}

		u32 fullScreenLayer = DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_FULL_SCREEN_LAYER, Basic::KEY_FULL_SCREEN_LAYER_OFFSET);
		u32 viewport = DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_VIEWPORT, Basic::KEY_VIEWPORT_OFFSET);
		u32 viewportLayer = DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_VIEWPORT_LAYER, Basic::KEY_VIEWPORT_LAYER_OFFSET);
		u32 translucency = DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_TRANSLUCENCY, Basic::KEY_TRANSLUCENCY_OFFSET);
		u32 depth = static_cast<u32>(DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_DEPTH, Basic::KEY_DEPTH_OFFSET));
		u32 shaderID = DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_MATERIAL_ID, Basic::KEY_MATERIAL_ID_OFFSET);
		u32 pass = DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_PASS, Basic::KEY_PASS_OFFSET);

		if (shaderID == 0)
			return;

	//	printf("DecodeBasicKey - shaderID: %zu, depth: %zu\n", shaderID, depth);


	/*	MaterialHandle materialHandle;

		u32 materialSystemOffset = Basic::KEY_BITS_MATERIAL_ID - NUM_MATERIAL_SYSTEM_BITS;

		materialHandle.slot = DECODE_KEY_VALUE(materialID, NUM_MATERIAL_SYSTEM_BITS, materialSystemOffset);
		materialHandle.handle = DECODE_KEY_VALUE(materialID, materialSystemOffset, 0);*/

		//@TODO(Serge): hmm not sure if this should be here or be in this form
		r2::draw::rendererimpl::SetViewportKey(viewport);
		
		r2::draw::rendererimpl::SetViewportLayer(viewportLayer);

		r2::draw::rendererimpl::SetShaderID(shaderID);
		
		
		//r2::draw::rendererimpl::SetMaterialID(materialHandle);
	}


	bool CompareShadowKey(const ShadowKey& a, const ShadowKey& b)
	{
		return a.keyValue < b.keyValue;
	}

	ShadowKey GenerateShadowKey(ShadowKey::Type type, u8 shaderOrder, r2::draw::ShaderHandle shader, bool isDynamic, light::LightType lightType, u32 depth)
	{
		ShadowKey theKey;

		theKey.keyValue |= ENCODE_KEY_VALUE(type, ShadowKey::SHADOW_KEY_BITS_TYPE, ShadowKey::SHADOW_KEY_TYPE_OFFSET);

		if (type == ShadowKey::Type::NORMAL || type == ShadowKey::Type::POINT_LIGHT)
		{
			theKey.keyValue |= ENCODE_KEY_VALUE(isDynamic ? 1 : 0, ShadowKey::SHADOW_KEY_BITS_IS_DYNAMIC, ShadowKey::SHADOW_KEY_IS_DYNAMIC_OFFSET);
			theKey.keyValue |= ENCODE_KEY_VALUE(lightType, ShadowKey::SHADOW_KEY_BITS_LIGHT_TYPE, ShadowKey::SHADOW_KEY_LIGHT_TYPE_OFFSET);
			theKey.keyValue |= ENCODE_KEY_VALUE(depth, ShadowKey::SHADOW_KEY_BITS_DEPTH, ShadowKey::SHADOW_KEY_DEPTH_OFFSET);
		}
		else if(type == ShadowKey::Type::COMPUTE)
		{
			theKey.keyValue |= ENCODE_KEY_VALUE(shaderOrder, ShadowKey::SHADOW_KEY_BITS_SHADER_ORDER, ShadowKey::SHADOW_KEY_SHADER_ORDER_OFFSET);
			theKey.keyValue |= ENCODE_KEY_VALUE(shader, ShadowKey::SHADOW_KEY_BITS_SHADER_ID, ShadowKey::SHADOW_KEY_SHADER_ID_OFFSET);
		}
		else if (type == ShadowKey::Type::CLEAR)
		{
			//nothing
		}
		

		return theKey;
	}

	void DecodeShadowKey(const ShadowKey& key)
	{
		ShadowKey::Type type = static_cast<ShadowKey::Type>(DECODE_KEY_VALUE(key.keyValue, ShadowKey::SHADOW_KEY_BITS_TYPE, ShadowKey::SHADOW_KEY_TYPE_OFFSET));

		ShaderHandle shaderHandle = InvalidShader;
		if (type == ShadowKey::Type::NORMAL || type == ShadowKey::Type::POINT_LIGHT)
		{
			bool isDynamic = DECODE_KEY_VALUE(key.keyValue, ShadowKey::SHADOW_KEY_BITS_IS_DYNAMIC, ShadowKey::SHADOW_KEY_IS_DYNAMIC_OFFSET);
			light::LightType lightType = static_cast<light::LightType>(DECODE_KEY_VALUE(key.keyValue, ShadowKey::SHADOW_KEY_BITS_LIGHT_TYPE, ShadowKey::SHADOW_KEY_LIGHT_TYPE_OFFSET));
			u16 depthValue = DECODE_KEY_VALUE(key.keyValue, ShadowKey::SHADOW_KEY_BITS_DEPTH, ShadowKey::SHADOW_KEY_DEPTH_OFFSET);
			
			r2::draw::rendererimpl::SetViewportLayer(isDynamic ? DrawLayer::DL_CHARACTER : DrawLayer::DL_WORLD);

			shaderHandle = r2::draw::renderer::GetShadowDepthShaderHandle(isDynamic, lightType);

			r2::draw::rendererimpl::SetShaderID(shaderHandle);
		}
		else if(type == ShadowKey::Type::COMPUTE)
		{
			u8 shaderOrder = DECODE_KEY_VALUE(key.keyValue, ShadowKey::SHADOW_KEY_BITS_SHADER_ORDER, ShadowKey::SHADOW_KEY_SHADER_ORDER_OFFSET);
			shaderHandle = DECODE_KEY_VALUE(key.keyValue, ShadowKey::SHADOW_KEY_BITS_SHADER_ID, ShadowKey::SHADOW_KEY_SHADER_ID_OFFSET);

			r2::draw::rendererimpl::SetShaderID(shaderHandle);
		}
		else if (type == ShadowKey::Type::CLEAR)
		{
			bool isDynamic = DECODE_KEY_VALUE(key.keyValue, ShadowKey::SHADOW_KEY_BITS_IS_DYNAMIC, ShadowKey::SHADOW_KEY_IS_DYNAMIC_OFFSET);

			shaderHandle = r2::draw::renderer::GetShadowDepthShaderHandle(isDynamic, light::LightType::LT_DIRECTIONAL_LIGHT);

			r2::draw::rendererimpl::SetShaderID(shaderHandle);
		}

		
	}


	bool CompareDepthKey(const DepthKey& a, const DepthKey& b)
	{
		return a.keyValue < b.keyValue;
	}

	DepthKey GenerateDepthKey(DepthKey::DepthType type, u8 shaderOrder, ShaderHandle shader, DrawLayer drawLayer)
	{
	//	PROFILE_SCOPE("GenerateDepthKey");

		DepthKey theKey;

		theKey.keyValue |= ENCODE_KEY_VALUE(type, DepthKey::DEPTH_KEY_BITS_DEPTH_TYPE, DepthKey::DEPTH_KEY_DEPTH_TYPE_OFFSET);

		if (type == DepthKey::NORMAL)
		{
			theKey.keyValue |= ENCODE_KEY_VALUE(drawLayer, DepthKey::DEPTH_KEY_BITS_DRAW_LAYER, DepthKey::DEPTH_KEY_DRAW_LAYER_OFFSET);
			theKey.keyValue |= ENCODE_KEY_VALUE(shader, DepthKey::DEPTH_KEY_BITS_SHADER_ID, DepthKey::DEPTH_KEY_DEPTH_OFFSET);
		}
		else
		{
			theKey.keyValue |= ENCODE_KEY_VALUE(shaderOrder, DepthKey::DEPTH_KEY_BITS_SHADER_ORDER, DepthKey::DEPTH_KEY_SHADER_ORDER_OFFSET);
			theKey.keyValue |= ENCODE_KEY_VALUE(shader, DepthKey::DEPTH_KEY_BITS_SHADER_ID, DepthKey::DEPTH_KEY_SHADER_ID_OFFSET);
		}

		return theKey;
	}

	void DecodeDepthKey(const DepthKey& key)
	{
		DepthKey::DepthType type = static_cast<DepthKey::DepthType>( DECODE_KEY_VALUE(key.keyValue, DepthKey::DEPTH_KEY_BITS_DEPTH_TYPE, DepthKey::DEPTH_KEY_DEPTH_TYPE_OFFSET) );

		ShaderHandle shaderHandle = InvalidShader;
		if (type == DepthKey::NORMAL)
		{
			bool isDynamic = DECODE_KEY_VALUE(key.keyValue, DepthKey::DEPTH_KEY_BITS_DRAW_LAYER, DepthKey::DEPTH_KEY_DRAW_LAYER_OFFSET);

			shaderHandle = DECODE_KEY_VALUE(key.keyValue, DepthKey::DEPTH_KEY_BITS_SHADER_ID, DepthKey::DEPTH_KEY_DEPTH_OFFSET);
			
			r2::draw::rendererimpl::SetViewportLayer(isDynamic ? DrawLayer::DL_CHARACTER : DrawLayer::DL_WORLD);
		}
		else if(type == DepthKey::COMPUTE || type == DepthKey::RESOLVE)
		{
			shaderHandle = DECODE_KEY_VALUE(key.keyValue, DepthKey::DEPTH_KEY_BITS_SHADER_ID, DepthKey::DEPTH_KEY_SHADER_ID_OFFSET);
		}
		
		if (type != DepthKey::UNUSED)
		{
			r2::draw::rendererimpl::SetShaderID(shaderHandle);
		}
	}


	SortBatchKey GenerateSortBatchKey(u8 viewportLayer, r2::draw::ShaderHandle forwardShader, r2::draw::ShaderHandle depthShader, u32 drawState)
	{
		key::SortBatchKey sortBatchKey;

		sortBatchKey.keyValue |= ENCODE_KEY_VALUE((u64)viewportLayer, SortBatchKey::SORT_KEY_BITS_LAYER, SortBatchKey::KEY_VIEWPORT_LAYER_OFFSET);
		sortBatchKey.keyValue |= ENCODE_KEY_VALUE((u64)forwardShader, SortBatchKey::SORT_KEY_BITS_FORWARD_SHADER_ID, SortBatchKey::KEY_FORWARD_SHADER_ID_OFFSET);
		sortBatchKey.keyValue |= ENCODE_KEY_VALUE((u64)depthShader, SortBatchKey::SORT_KEY_BITS_DEPTH_SHADER_ID, SortBatchKey::KEY_DEPTH_SHADER_ID_OFFSET);
		sortBatchKey.keyValue |= ENCODE_KEY_VALUE((u64)drawState, SortBatchKey::SORT_KEY_BITS_DRAW_STATE, SortBatchKey::KEY_DRAW_STATE_OFFSET);

		return sortBatchKey;
	}

	RenderSystemKey GenerateRenderSystemKey(bool hasMaterialOverrides, PrimitiveType primitiveType, u32 drawParamaters)
	{
		key::RenderSystemKey renderSystemKey;

		renderSystemKey.keyValue |= ENCODE_KEY_VALUE((u64)(hasMaterialOverrides ? 1 : 0), RenderSystemKey::RENDER_SYSTEM_KEY_BITS_HAS_MATERIAL_OVERRIDES, RenderSystemKey::RENDER_SYSTEM_KEY_HAS_MATERIAL_OVERRIDES_OFFSET);
		renderSystemKey.keyValue |= ENCODE_KEY_VALUE((u64)primitiveType, RenderSystemKey::RENDER_SYSTEM_KEY_BITS_PRIMITIVE_TYPE, RenderSystemKey::RENDER_SYSTEM_KEY_PRIMITIVE_TYPE_OFFSET);
		renderSystemKey.keyValue |= ENCODE_KEY_VALUE((u64)drawParamaters, RenderSystemKey::RENDER_SYSTEM_KEY_BITS_DRAW_PARAMETERS, RenderSystemKey::RENDER_SYSTEM_KEY_HAS_MATERIAL_OVERRIDES_OFFSET);

		return renderSystemKey;
	}

	u8 GetBlendingFunctionKeyValue(const BlendState& state)
	{
		u8 blendingFunctionKeyValue = TR_OPAQUE;

		if (state.blendEquation == BLEND_EQUATION_ADD)
		{
			blendingFunctionKeyValue = TR_ADDITIVE;
		}
		else if (state.blendEquation == BLEND_EQUATION_SUBTRACT)
		{
			blendingFunctionKeyValue = TR_SUBTRACTIVE;
		}

		return blendingFunctionKeyValue;
	}
}