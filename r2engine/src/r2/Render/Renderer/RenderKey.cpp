#include "r2pch.h"

#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Renderer/RendererImpl.h"

namespace r2::draw::key
{
	const u8 Basic::FSL_GAME = 0;
	const u8 Basic::FSL_EFFECT = 1;
	const u8 Basic::FSL_HUD = 2;

	const u8 Basic::VP_DEFAULT = 0;
	



	const u8 NUM_MATERIAL_SYSTEM_BITS = 4;



	bool CompareDebugKey(const DebugKey& a, const DebugKey& b)
	{
		return a.keyValue < b.keyValue;
	}

	DebugKey GenerateDebugKey(u16 shaderID, PrimitiveType primitiveType, bool depthTest, u8 translucency, u16 depth)
	{
		DebugKey key;

		key.keyValue |= ENCODE_KEY_VALUE((u32)shaderID, DebugKey::DEBUG_KEY_BITS_SHADER_ID, DebugKey::DEBUG_KEY_SHADER_ID_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u32)primitiveType, DebugKey::DEBUG_KEY_BITS_PRIMITIVE_TYPE, DebugKey::DEBUG_KEY_PRIMITIVE_TYPE_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u32)(depthTest ? 1 : 0), DebugKey::DEBUG_KEY_BITS_DEPTH_ENABLED, DebugKey::DEBUG_KEY_DEPTH_ENABLED_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u32)(translucency), DebugKey::DEBUG_KEY_BITS_TRANSLUCENCY, DebugKey::DEBUG_KEY_TRANSLUCENCY_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u32)depth, DebugKey::DEBUG_KEY_BITS_DEPTH, DebugKey::DEBUG_KEY_DEPTH_OFFSET);

		return key;
	}

	void DecodeDebugKey(const DebugKey& key)
	{
		if (key.keyValue == 0)
			return;

		u32 shaderID = DECODE_KEY_VALUE((u32)key.keyValue, DebugKey::DEBUG_KEY_BITS_SHADER_ID, DebugKey::DEBUG_KEY_SHADER_ID_OFFSET);
		u32 primitiveType = DECODE_KEY_VALUE((u32)key.keyValue, DebugKey::DEBUG_KEY_BITS_PRIMITIVE_TYPE, DebugKey::DEBUG_KEY_PRIMITIVE_TYPE_OFFSET);
		u32 depthEnabled = DECODE_KEY_VALUE((u32)key.keyValue, DebugKey::DEBUG_KEY_BITS_DEPTH_ENABLED, DebugKey::DEBUG_KEY_DEPTH_ENABLED_OFFSET);
		u32 translucency = DECODE_KEY_VALUE((u32)key.keyValue, DebugKey::DEBUG_KEY_BITS_TRANSLUCENCY, DebugKey::DEBUG_KEY_TRANSLUCENCY_OFFSET);
		u32 depth = DECODE_KEY_VALUE((u32)key.keyValue, DebugKey::DEBUG_KEY_BITS_DEPTH, DebugKey::DEBUG_KEY_DEPTH_OFFSET);

		r2::draw::rendererimpl::SetViewportKey(0);
		r2::draw::rendererimpl::SetViewportLayer(DrawLayer::DL_DEBUG);
		r2::draw::rendererimpl::SetShaderID(shaderID);

	}

	bool CompareBasicKey(const Basic& a, const Basic& b)
	{
		return a.keyValue < b.keyValue;
	}

	Basic GenerateBasicKey(u8 fullscreenLayer, u8 viewport, DrawLayer viewportLayer, u8 translucency, u32 depth, r2::draw::ShaderHandle shaderID)
	{
		Basic key;

		key.keyValue |= ENCODE_KEY_VALUE((u64)fullscreenLayer, Basic::KEY_BITS_FULL_SCREEN_LAYER, Basic::KEY_FULL_SCREEN_LAYER_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)viewport, Basic::KEY_BITS_VIEWPORT, Basic::KEY_VIEWPORT_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)viewportLayer, Basic::KEY_BITS_VIEWPORT_LAYER, Basic::KEY_VIEWPORT_LAYER_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)translucency, Basic::KEY_BITS_TRANSLUCENCY, Basic::KEY_TRANSLUCENCY_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)depth, Basic::KEY_BITS_DEPTH, Basic::KEY_DEPTH_OFFSET);
		
//		u32 materialID = 0;

//		u32 materialSystemOffset = Basic::KEY_BITS_MATERIAL_ID - NUM_MATERIAL_SYSTEM_BITS;

//		materialID |= ENCODE_KEY_VALUE((u64)materialHandle.slot, NUM_MATERIAL_SYSTEM_BITS, materialSystemOffset);
//		materialID |= ENCODE_KEY_VALUE((u64)materialHandle.handle, materialSystemOffset, 0);

		key.keyValue |= ENCODE_KEY_VALUE((u64)shaderID, Basic::KEY_BITS_MATERIAL_ID, Basic::KEY_MATERIAL_ID_OFFSET);

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
}