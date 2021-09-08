#ifndef __RENDER_KEY_H__
#define __RENDER_KEY_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Model/Material.h"

#define MAX_BIT_VAL(x) static_cast<u64>((2 << (static_cast<u64>(x)-1))-1)
#define ENCODE_KEY_VALUE(value, num_bits, offset) ((MAX_BIT_VAL(num_bits) & static_cast<u64>(value)) << static_cast<u64>(offset))
#define DECODE_KEY_VALUE(key, num_bits, offset) ( ((MAX_BIT_VAL(num_bits) << static_cast<u64>(offset)) & static_cast<u64>(key)) >> static_cast<u64>(offset))

namespace r2::draw::key
{
	struct Basic
	{
		//@TODO(Serge): change material ID to shaderID
		/* 2 bits             4 bits    4 bits            2 bits       24 bits    24 bits    4 bits  
		+------------------+----------+----------------+--------------+-------+-------------+------+
		| Fullscreen Layer | Viewport | Viewport Layer | Translucency | Depth | Shader   ID | Pass |
		+------------------+----------+----------------+--------------+-------+-------------+------+
		*/
		enum: u64
		{
			KEY_BITS_TOTAL = 0x40ull,

			KEY_BITS_FULL_SCREEN_LAYER = 0x2ull,
			KEY_BITS_VIEWPORT = 0x4ull,
			KEY_BITS_VIEWPORT_LAYER = 0x4ull,
			KEY_BITS_TRANSLUCENCY = 0x2ull,
			KEY_BITS_DEPTH = 0x18ull,
			KEY_BITS_MATERIAL_ID = 0x18ull,
			KEY_BITS_PASS = 0x4ull,

			KEY_FULL_SCREEN_LAYER_OFFSET = KEY_BITS_TOTAL - KEY_BITS_FULL_SCREEN_LAYER,
			KEY_VIEWPORT_OFFSET = KEY_FULL_SCREEN_LAYER_OFFSET - KEY_BITS_VIEWPORT,
			KEY_VIEWPORT_LAYER_OFFSET = KEY_VIEWPORT_OFFSET - KEY_BITS_VIEWPORT_LAYER,
			KEY_TRANSLUCENCY_OFFSET = KEY_VIEWPORT_LAYER_OFFSET - KEY_BITS_TRANSLUCENCY,
			KEY_DEPTH_OFFSET = KEY_TRANSLUCENCY_OFFSET - KEY_BITS_DEPTH,
			KEY_MATERIAL_ID_OFFSET = KEY_DEPTH_OFFSET - KEY_BITS_MATERIAL_ID,
			KEY_PASS_OFFSET = 0
		};

		static const u8 FSL_GAME;
		static const u8 FSL_EFFECT;
		static const u8 FSL_HUD;

		static const u8 VP_DEFAULT;
		


		static const u8 TR_OPAQUE;
		static const u8 TR_NORMAL;
		static const u8 TR_ADDITIVE;
		static const u8 TR_SUBTRACTIVE;

		u64 keyValue = 0;

	};

	bool CompareKey(const Basic& a, const Basic& b);

	Basic GenerateKey(u8 fullscreenLayer, u8 viewport, DrawLayer viewportLayer, u8 translucency, u32 depth, r2::draw::ShaderHandle shaderID);
	
	void DecodeBasicKey(const Basic& key);

}

#endif