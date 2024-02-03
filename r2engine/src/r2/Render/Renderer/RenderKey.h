#ifndef __RENDER_KEY_H__
#define __RENDER_KEY_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Model/Light.h"
#include "r2/Render/Model/Shader/Shader.h"

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

			//DON'T CHANGE THESE!!!! SWAPPING WILL CAUSE FORWARD CLUSTERED SHADING TO BREAK!!!
			KEY_DEPTH_OFFSET = KEY_TRANSLUCENCY_OFFSET - KEY_BITS_DEPTH,
			KEY_MATERIAL_ID_OFFSET = KEY_DEPTH_OFFSET - KEY_BITS_MATERIAL_ID,
			
			
			KEY_PASS_OFFSET = 0
		};

		static const u8 FSL_GAME;
		static const u8 FSL_EFFECT;
		static const u8 FSL_HUD;
		static const u8 FSL_OUTPUT;

		u64 keyValue = 0;

	};


	struct DebugKey
	{
		/*
		 
		+--6 bits--+-----1 bits-----+-----2 bit----+-----3 bits-----+--20 bits--+
		|   Layer  | Depth Enabled  | Translucency | Primitive Type |   Shader  |
		+----------+----------------+--------------+----------------+-----------+
		
		*/
		u32 keyValue = 0;

		enum : u32
		{
			DEBUG_KEY_BITS_TOTAL =  BytesToBits(sizeof(keyValue)),

			DEBUG_KEY_BITS_SHADER_ID = 0x14,
			DEBUG_KEY_BITS_PRIMITIVE_TYPE = 0x3,
			DEBUG_KEY_BITS_DEPTH_ENABLED = 0x1,
			DEBUG_KEY_BITS_TRANSLUCENCY = 0x2,
			DEBUG_KEY_BITS_LAYER = 0x6,

			DEBUG_KEY_LAYER_OFFSET = DEBUG_KEY_BITS_TOTAL - DEBUG_KEY_BITS_LAYER,
			DEBUG_KEY_DEPTH_ENABLED_OFFSET = DEBUG_KEY_LAYER_OFFSET - DEBUG_KEY_BITS_DEPTH_ENABLED,
			DEBUG_KEY_TRANSLUCENCY_OFFSET = DEBUG_KEY_DEPTH_ENABLED_OFFSET - DEBUG_KEY_BITS_TRANSLUCENCY,
			DEBUG_KEY_PRIMITIVE_TYPE_OFFSET = DEBUG_KEY_TRANSLUCENCY_OFFSET - DEBUG_KEY_BITS_PRIMITIVE_TYPE,
			DEBUG_KEY_SHADER_ID_OFFSET = DEBUG_KEY_PRIMITIVE_TYPE_OFFSET - DEBUG_KEY_BITS_SHADER_ID

		};
	};

	struct ShadowKey
	{
		/*

		+-----2 bits----+------30 bits----+
		|     Type      |    Ignored      | //Clear
		+---------------+-----------------+

		+----2 bit---+------1 bit-----+----5 bits----+-24 bits-+
		|    Type    | Static/Dynamic |  Light Type  |  Depth  | //Normal
		+------------+----------------+--------------+---------+

		+----2 bit---+---6 bits---+--24 bits--+
		|     Type   |Shader Order| Shader ID | //compute
		+------------+------------+-----------+

		*/

		u32 keyValue = 0;

		enum Type : u8
		{
			CLEAR = 0,
			COMPUTE,
			NORMAL,
			POINT_LIGHT
		};

		enum : u32
		{
			SHADOW_KEY_BITS_TOTAL = BytesToBits(sizeof(keyValue)),

			SHADOW_KEY_BITS_TYPE = 0x2,
			SHADOW_KEY_BITS_IS_DYNAMIC = 0x1,
			SHADOW_KEY_BITS_LIGHT_TYPE = 0x5,
			SHADOW_KEY_BITS_DEPTH = 0x18,
			SHADOW_KEY_BITS_SHADER_ORDER = 0x6,
			SHADOW_KEY_BITS_SHADER_ID = 0x18,

			SHADOW_KEY_TYPE_OFFSET = SHADOW_KEY_BITS_TOTAL - SHADOW_KEY_BITS_TYPE,
			SHADOW_KEY_IS_DYNAMIC_OFFSET = SHADOW_KEY_TYPE_OFFSET - SHADOW_KEY_BITS_IS_DYNAMIC,
			SHADOW_KEY_LIGHT_TYPE_OFFSET = SHADOW_KEY_IS_DYNAMIC_OFFSET - SHADOW_KEY_BITS_LIGHT_TYPE,
			SHADOW_KEY_DEPTH_OFFSET = SHADOW_KEY_LIGHT_TYPE_OFFSET - SHADOW_KEY_BITS_DEPTH,
			SHADOW_KEY_SHADER_ORDER_OFFSET = SHADOW_KEY_TYPE_OFFSET - SHADOW_KEY_BITS_SHADER_ORDER,
			SHADOW_KEY_SHADER_ID_OFFSET = SHADOW_KEY_SHADER_ORDER_OFFSET - SHADOW_KEY_BITS_SHADER_ID
		};
	};

	struct DepthKey
	{
		enum DepthType : u8
		{
			COMPUTE = 0,
			NORMAL,
			RESOLVE,
			UNUSED
		};

		/*
		+---2 bits-----+----6 bits----+---24 bits---+
		|  Resolve(2)  | Shader Order |  ShaderID   |
		+--------------+--------------+-------------+

		+---2 bits-----+------6 bit-----+--24 bits---+
		|   Normal(1)  |   Draw Layer   |  ShaderID  |
		+--------------+----------------+------------+

		+---2 bits-----+----6 bits----+---24 bits---+
		|   Compute(0) | Shader Order |  Shader ID  | //compute
		+--------------+--------------+-------------+

		*/

		u32 keyValue = 0;

		enum : u32
		{
			DEPTH_KEY_BITS_TOTAL = BytesToBits(sizeof(keyValue)),

			DEPTH_KEY_BITS_DEPTH_TYPE = 0x2,
			DEPTH_KEY_BITS_DRAW_LAYER = 0x6,
			DEPTH_KEY_BITS_SHADER_ORDER = 0x6,
			DEPTH_KEY_BITS_SHADER_ID = 0x18,

			DEPTH_KEY_DEPTH_TYPE_OFFSET = DEPTH_KEY_BITS_TOTAL - DEPTH_KEY_BITS_DEPTH_TYPE,
			DEPTH_KEY_DRAW_LAYER_OFFSET = DEPTH_KEY_DEPTH_TYPE_OFFSET - DEPTH_KEY_BITS_DRAW_LAYER,
			DEPTH_KEY_DEPTH_OFFSET = DEPTH_KEY_DRAW_LAYER_OFFSET - DEPTH_KEY_BITS_SHADER_ID,
			DEPTH_KEY_SHADER_ORDER_OFFSET = DEPTH_KEY_DEPTH_TYPE_OFFSET - DEPTH_KEY_BITS_SHADER_ORDER,
			DEPTH_KEY_SHADER_ID_OFFSET = DEPTH_KEY_SHADER_ORDER_OFFSET - DEPTH_KEY_BITS_SHADER_ID
		};
	};

	struct SortBatchKey
	{
		u64 keyValue = 0;

		enum : u32
		{
			SORT_BATCH_KEY_BITS_TOTAL = BytesToBits(sizeof(keyValue)),

			SORT_KEY_BITS_LAYER = 0x8,
			SORT_KEY_BITS_FORWARD_SHADER_ID = 0xC,
			SORT_KEY_BITS_DEPTH_SHADER_ID = 0xC,

			SORT_KEY_BITS_DRAW_STATE = 0x20,

			KEY_VIEWPORT_LAYER_OFFSET = SORT_BATCH_KEY_BITS_TOTAL - SORT_KEY_BITS_LAYER,
			KEY_FORWARD_SHADER_ID_OFFSET = KEY_VIEWPORT_LAYER_OFFSET - SORT_KEY_BITS_FORWARD_SHADER_ID,
			KEY_DEPTH_SHADER_ID_OFFSET = KEY_FORWARD_SHADER_ID_OFFSET - SORT_KEY_BITS_DEPTH_SHADER_ID,
			KEY_DRAW_STATE_OFFSET = KEY_DEPTH_SHADER_ID_OFFSET - SORT_KEY_BITS_DRAW_STATE,
		};
	};

	struct RenderSystemKey
	{
		u64 keyValue = 0;
		enum : u32
		{
			RENDER_SYSTEM_KEY_BITS_TOTAL = BytesToBits(sizeof(keyValue)),

			RENDER_SYSTEM_KEY_BITS_HAS_MATERIAL_OVERRIDES = 0x1,
			RENDER_SYSTEM_KEY_BITS_PRIMITIVE_TYPE = 0x8,
			RENDER_SYSTEM_KEY_BITS_DRAW_PARAMETERS = 0x20,

			RENDER_SYSTEM_KEY_HAS_MATERIAL_OVERRIDES_OFFSET = RENDER_SYSTEM_KEY_BITS_TOTAL - RENDER_SYSTEM_KEY_BITS_HAS_MATERIAL_OVERRIDES,
			RENDER_SYSTEM_KEY_PRIMITIVE_TYPE_OFFSET = RENDER_SYSTEM_KEY_HAS_MATERIAL_OVERRIDES_OFFSET - RENDER_SYSTEM_KEY_BITS_PRIMITIVE_TYPE,
			RENDER_SYSTEM_KEY_DRAW_PARAMATERS_OFFSET = RENDER_SYSTEM_KEY_PRIMITIVE_TYPE_OFFSET - RENDER_SYSTEM_KEY_BITS_DRAW_PARAMETERS,


		};
	};

	//DEBUG
	bool CompareDebugKey(const DebugKey& a, const DebugKey& b);
	DebugKey GenerateDebugKey(r2::draw::DrawLayer drawLayer, bool depthTest, u8 translucency, PrimitiveType primitiveType, r2::draw::ShaderHandle shaderID);
	void DecodeDebugKey(const DebugKey& key);

	//Normal GBUFFER
	bool CompareBasicKey(const Basic& a, const Basic& b);
	Basic GenerateBasicKey(u8 fullscreenLayer, u8 viewport, DrawLayer viewportLayer, u8 translucency, u32 depth, r2::draw::ShaderHandle shaderID, u8 pass = 0);
	void DecodeBasicKey(const Basic& key);

	//Shadows
	bool CompareShadowKey(const ShadowKey& a, const ShadowKey& b);
	ShadowKey GenerateShadowKey(ShadowKey::Type type, u8 shaderOrder, r2::draw::ShaderHandle shader, bool isDynamic, light::LightType lightType, u32 depth);
	void DecodeShadowKey(const ShadowKey& key);

	//Depth
	bool CompareDepthKey(const DepthKey& a, const DepthKey& b);
	DepthKey GenerateDepthKey(DepthKey::DepthType type, u8 shaderOrder, r2::draw::ShaderHandle, DrawLayer viewportLayer);
	void DecodeDepthKey(const DepthKey& key);

	//Sort Batch
	SortBatchKey GenerateSortBatchKey(u8 viewportLayer, r2::draw::ShaderHandle forwardShader, r2::draw::ShaderHandle depthShader, u32 drawState);

	//Render System Key
	RenderSystemKey GenerateRenderSystemKey(bool hasMaterialOverrides, PrimitiveType primitiveType, u32 drawParamaters);

	//Blending state helper
	u8 GetBlendingFunctionKeyValue(const BlendState& state);
}

#endif