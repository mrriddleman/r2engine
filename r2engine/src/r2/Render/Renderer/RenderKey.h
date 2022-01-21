#ifndef __RENDER_KEY_H__
#define __RENDER_KEY_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Model/Material.h"

#define MAX_BIT_VAL(x) static_cast<u64>((2 << (static_cast<u64>(x)-1))-1)
#define ENCODE_KEY_VALUE(value, num_bits, offset) ((MAX_BIT_VAL(num_bits) & static_cast<u64>(value)) << static_cast<u64>(offset))
#define DECODE_KEY_VALUE(key, num_bits, offset) ( ((MAX_BIT_VAL(num_bits) << static_cast<u64>(offset)) & static_cast<u64>(key)) >> static_cast<u64>(offset))

namespace r2::draw::key
{
	const u8 TR_OPAQUE = 0;
	const u8 TR_NORMAL = 1;
	const u8 TR_ADDITIVE = 2;
	const u8 TR_SUBTRACTIVE = 3;


	struct Basic
	{
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
		




		u64 keyValue = 0;

	};


	struct DebugKey
	{
		/*
		+--11 bits--+-----4 bits-----+-----1 bit-----+----2 bits----+--14 bits--+
		| Shader ID | Primitive Type | Depth Enabled | Translucency |   Depth   |
		+-----------+----------------+---------------+--------------+-----------+
		*/
		u32 keyValue = 0;

		enum : u32
		{
			DEBUG_KEY_BITS_TOTAL =  BytesToBits(sizeof(keyValue)),

			DEBUG_KEY_BITS_SHADER_ID = 0xB,
			DEBUG_KEY_BITS_PRIMITIVE_TYPE = 0x4,
			DEBUG_KEY_BITS_DEPTH_ENABLED = 0x1,
			DEBUG_KEY_BITS_TRANSLUCENCY = 0x2,
			DEBUG_KEY_BITS_DEPTH = 0xE,

			DEBUG_KEY_SHADER_ID_OFFSET = DEBUG_KEY_BITS_TOTAL - DEBUG_KEY_BITS_SHADER_ID,
			DEBUG_KEY_PRIMITIVE_TYPE_OFFSET = DEBUG_KEY_SHADER_ID_OFFSET - DEBUG_KEY_BITS_PRIMITIVE_TYPE,
			DEBUG_KEY_DEPTH_ENABLED_OFFSET = DEBUG_KEY_PRIMITIVE_TYPE_OFFSET - DEBUG_KEY_BITS_DEPTH_ENABLED,
			DEBUG_KEY_TRANSLUCENCY_OFFSET = DEBUG_KEY_DEPTH_ENABLED_OFFSET - DEBUG_KEY_BITS_TRANSLUCENCY,
			DEBUG_KEY_DEPTH_OFFSET = DEBUG_KEY_TRANSLUCENCY_OFFSET - DEBUG_KEY_BITS_DEPTH

		};
	};

	struct ShadowKey
	{
		/*
		+----1 bit-----+------1 bit-----+-14 bits-+
		|   Normal(1)  | Static/Dynamic |  Depth  |
		+--------------+----------------+---------+

		+----1 bit---+---3 bits---+--12 bits--+
		| Normal(0)  |Shader Order| Shader ID | //compute
		+------------+------------+-----------+

		*/

		u16 keyValue = 0;

		enum : u16
		{
			SHADOW_KEY_BITS_TOTAL = BytesToBits(sizeof(keyValue)),

			SHADOW_KEY_BITS_IS_NORMAL_PATH = 0x1,
			SHADOW_KEY_BITS_IS_DYNAMIC = 0x1,
			SHADOW_KEY_BITS_DEPTH = 0xE,
			SHADOW_KEY_BITS_SHADER_ORDER = 0x3,
			SHADOW_KEY_BITS_SHADER_ID = 0xC,

			SHADOW_KEY_IS_NORMAL_PATH_OFFSET = SHADOW_KEY_BITS_TOTAL - SHADOW_KEY_BITS_IS_NORMAL_PATH,
			SHADOW_KEY_IS_DYNAMIC_OFFSET = SHADOW_KEY_IS_NORMAL_PATH_OFFSET - SHADOW_KEY_BITS_IS_DYNAMIC,
			SHADOW_KEY_DEPTH_OFFSET = SHADOW_KEY_IS_DYNAMIC_OFFSET - SHADOW_KEY_BITS_DEPTH,
			SHADOW_KEY_SHADER_ORDER_OFFSET = SHADOW_KEY_IS_NORMAL_PATH_OFFSET - SHADOW_KEY_BITS_SHADER_ORDER,
			SHADOW_KEY_SHADER_ID_OFFSET = SHADOW_KEY_SHADER_ORDER_OFFSET - SHADOW_KEY_BITS_SHADER_ID
		};
	};

	struct DepthKey
	{
		/*
		+----1 bit-----+------1 bit-----+-14 bits-+
		|   Normal(1)  | Static/Dynamic |  Depth  |
		+--------------+----------------+---------+

		+----1 bit-----+--15 bits--+
		| Normal(0)    | Shader ID | //compute
		+--------------+-----------+

		*/

		u16 keyValue = 0;

		enum : u16
		{
			DEPTH_KEY_BITS_TOTAL = BytesToBits(sizeof(keyValue)),

			DEPTH_KEY_BITS_IS_NORMAL_PATH = 0x1,
			DEPTH_KEY_BITS_IS_DYNAMIC = 0x1,
			DEPTH_KEY_BITS_DEPTH = 0xE,
			DEPTH_KEY_BITS_SHADER_ORDER = 0x3,
			DEPTH_KEY_BITS_SHADER_ID = 0xC,

			DEPTH_KEY_IS_NORMAL_PATH_OFFSET = DEPTH_KEY_BITS_TOTAL - DEPTH_KEY_BITS_IS_NORMAL_PATH,
			DEPTH_KEY_IS_DYNAMIC_OFFSET = DEPTH_KEY_IS_NORMAL_PATH_OFFSET - DEPTH_KEY_BITS_IS_DYNAMIC,
			DEPTH_KEY_DEPTH_OFFSET = DEPTH_KEY_IS_DYNAMIC_OFFSET - DEPTH_KEY_BITS_DEPTH,
			DEPTH_KEY_SHADER_ORDER_OFFSET = DEPTH_KEY_IS_NORMAL_PATH_OFFSET - DEPTH_KEY_BITS_SHADER_ORDER,
			DEPTH_KEY_SHADER_ID_OFFSET = DEPTH_KEY_SHADER_ORDER_OFFSET - DEPTH_KEY_BITS_SHADER_ID
		};
	};

	//DEBUG
	bool CompareDebugKey(const DebugKey& a, const DebugKey& b);

	DebugKey GenerateDebugKey(u16 shaderID, PrimitiveType primitiveType, bool depthTest, u8 translucency, u16 depth);

	void DecodeDebugKey(const DebugKey& key);

	//Normal GBUFFER
	bool CompareBasicKey(const Basic& a, const Basic& b);

	Basic GenerateBasicKey(u8 fullscreenLayer, u8 viewport, DrawLayer viewportLayer, u8 translucency, u32 depth, r2::draw::ShaderHandle shaderID);
	
	void DecodeBasicKey(const Basic& key);

	//Shadows
	bool CompareShadowKey(const ShadowKey& a, const ShadowKey& b);
	ShadowKey GenerateShadowKey(bool normalPath, u8 shaderOrder, r2::draw::ShaderHandle, bool isDynamic, u16 depth);
	void DecodeShadowKey(const ShadowKey& key);

	//Depth
	bool CompareDepthKey(const DepthKey& a, const DepthKey& b);
	DepthKey GenerateDepthKey(bool normalPath, u8 shaderOrder, r2::draw::ShaderHandle, bool isDynamic, u16 depth);
	void DecodeDepthKey(const DepthKey& key);
}

#endif