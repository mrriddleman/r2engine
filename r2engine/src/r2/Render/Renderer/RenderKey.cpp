#include "r2pch.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Renderer/RendererImpl.h"

namespace r2::draw::key
{
	const u8 Basic::FSL_GAME = 0;
	const u8 Basic::FSL_EFFECT = 1;
	const u8 Basic::FSL_HUD = 2;

	const u8 Basic::VP_DEFAULT = 0;

	const u8 Basic::VPL_WORLD = 0;
	const u8 Basic::VPL_EFFECT = 1;
	const u8 Basic::VPL_SKYBOX = 2;
	const u8 Basic::VPL_HUD = 3;

	const u8 Basic::TR_OPAQUE = 0;
	const u8 Basic::TR_NORMAL = 1;
	const u8 Basic::TR_ADDITIVE = 2;
	const u8 Basic::TR_SUBTRACTIVE = 3;


	bool CompareKey(const Basic& k1, const Basic& k2)
	{
		return k1.keyValue < k2.keyValue;
	}

	Basic GenerateKey(u8 fullscreenLayer, u8 viewport, u8 viewportLayer, u8 translucency, u32 depth, u32 materialID)
	{
		Basic key;

		key.keyValue |= ENCODE_KEY_VALUE((u64)fullscreenLayer, Basic::KEY_BITS_FULL_SCREEN_LAYER, Basic::KEY_FULL_SCREEN_LAYER_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)viewport, Basic::KEY_BITS_VIEWPORT, Basic::KEY_VIEWPORT_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)viewportLayer, Basic::KEY_BITS_VIEWPORT_LAYER, Basic::KEY_VIEWPORT_LAYER_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)translucency, Basic::KEY_BITS_TRANSLUCENCY, Basic::KEY_TRANSLUCENCY_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)depth, Basic::KEY_BITS_DEPTH, Basic::KEY_DEPTH_OFFSET);
		key.keyValue |= ENCODE_KEY_VALUE((u64)materialID, Basic::KEY_BITS_MATERIAL_ID, Basic::KEY_MATERIAL_ID_OFFSET);

		return key;
	}

	void DecodeBasicKey(const Basic& key)
	{
		u32 fullScreenLayer = DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_FULL_SCREEN_LAYER, Basic::KEY_FULL_SCREEN_LAYER_OFFSET);
		u32 viewport = DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_VIEWPORT, Basic::KEY_VIEWPORT_OFFSET);
		u32 viewportLayer = DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_VIEWPORT_LAYER, Basic::KEY_VIEWPORT_LAYER_OFFSET);
		u32 translucency = DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_TRANSLUCENCY, Basic::KEY_TRANSLUCENCY_OFFSET);
		u32 depth = static_cast<u32>(DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_DEPTH, Basic::KEY_DEPTH_OFFSET));
		u32 materialID = DECODE_KEY_VALUE(key.keyValue, Basic::KEY_BITS_MATERIAL_ID, Basic::KEY_MATERIAL_ID_OFFSET);

		//@TODO(Serge): hmm not sure if this should be here or be in this form
		r2::draw::rendererimpl::SetViewport(viewport);
		r2::draw::rendererimpl::SetViewportLayer(viewportLayer);
		r2::draw::rendererimpl::SetMaterialID(materialID);
	}
}