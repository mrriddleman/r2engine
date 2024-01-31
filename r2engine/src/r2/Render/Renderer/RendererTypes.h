#ifndef __RENDERER_TYPES_H__
#define __RENDERER_TYPES_H__

#include "r2/Utils/Utils.h"
#include "r2/Utils/Flags.h"
#include <glm/glm.hpp>

namespace r2::draw
{
	using BufferLayoutHandle = u32; //VBA
    using VertexBufferHandle = u32; //VBO
    using IndexBufferHandle = u32; //IBO
    using DrawIDHandle = u32;
    using ConstantBufferHandle = u32; //GL_SHADER_STORAGE_BUFFER or GL_UNIFORM_BUFFER
    using BatchBufferHandle = u32; //GL_DRAW_INDIRECT_BUFFER
//	using VertexConfigHandle = s32;
	using ConstantConfigHandle = s32;

  //  const VertexConfigHandle InvalidVertexConfigHandle = -1;
    const ConstantConfigHandle InvalidConstantConfigHandle = -1;

    static const u32 EMPTY_BUFFER = 0;
    
    const u32 MAX_BLEND_TARGETS = 4;

	enum class PrimitiveType : u8
	{
		POINTS = 0,
		LINES,
		LINE_LOOP,
		LINE_STRIP,
		TRIANGLES,
		TRIANGLE_STRIP,
		TRIANGLE_FAN,
        NUM_PRIMITIVE_TYPES
	};

    enum eDrawFlags
    {
        DEPTH_TEST = 1 << 0,
        FILL_MODEL = 1 << 1, //for debug only I guess...
        USE_SAME_BONE_TRANSFORMS_FOR_INSTANCES = 1 << 2
    };

    using DrawFlags = r2::Flags<u32, u32>;

    enum DrawType
    {
        STATIC = 0,
        DYNAMIC,
        NUM_DRAW_TYPES
    };

    enum DebugDrawType
    {
        DDT_LINES = 0,
        DDT_MODELS,
        DDT_LINES_TRANSPARENT,
        DDT_MODELS_TRANSPARENT,
        NUM_DEBUG_DRAW_TYPES
    };

    enum DrawLayer : u8
    {
        DL_CLEAR = 0,
        DL_COMPUTE, 
        DL_WORLD, //static models
        DL_CHARACTER, //dynamic models
        DL_SKYBOX,
        DL_TRANSPARENT,
        DL_EFFECT,
        DL_HUD,
        DL_SCREEN,
       
        DL_DEBUG //should always be last
    };

	extern u32 LESS;
	extern u32 LEQUAL;
	extern u32 EQUAL;
    
    extern u32 NEVER;

    extern u32 GREATER;
    extern u32 GEQUAL;
    extern u32 NOTEQUAL;

	extern u32 KEEP;
	extern u32 REPLACE;
	extern u32 ZERO;
	
    extern u32 ALWAYS;

	extern u32 CULL_FACE_FRONT;
	extern u32 CULL_FACE_BACK;
    extern u32 NONE;

    extern u32 NEAREST;
    extern u32 LINEAR;

    extern u32 ONE;
    extern u32 ONE_MINUS_SRC_ALPHA;
    extern u32 ONE_MINUS_SRC_COLOR;
    extern u32 SRC_ALPHA;
    extern u32 DST_ALPHA;
    extern u32 ONE_MINUS_DST_ALPHA;
    extern u32 BLEND_EQUATION_ADD;
    extern u32 BLEND_EQUATION_SUBTRACT;
    extern u32 BLEND_EQUATION_REVERSE_SUBTRACT;
    extern u32 BLEND_EQUATION_MIN;
    extern u32 BLEND_EQUATION_MAX;
    extern u32 COLOR;

    extern u32 CW;
    extern u32 CCW;
    extern u32 UNSIGNED_BYTE;

	enum DefaultModel
	{
		QUAD = 0,
		CUBE,
		SPHERE,
		CONE,
		CYLINDER,
		FULLSCREEN_TRIANGLE,
		
		NUM_DEFAULT_MODELS,
	};

	enum DebugModelType : u32
	{
		DEBUG_QUAD = 0,
		DEBUG_CUBE,
		DEBUG_SPHERE,
		DEBUG_CONE,
		DEBUG_CYLINDER,

		DEBUG_ARROW,
		DEBUG_LINE,

		NUM_DEBUG_MODELS
	};


    struct CullState
    {
        b32 cullingEnabled;
        u32 cullFace;
        u32 frontFace;
    };

	struct StencilOp
	{
		u32 stencilFail;
		u32 depthFail;
		u32 depthAndStencilPass;
	};

	struct StencilFunc
	{
		u32 func;
		u32 ref;
		u32 mask;
	};

	struct StencilState
	{
		b32 stencilEnabled;
		b32 stencilWriteEnabled;

		StencilOp op;
		StencilFunc func;
	};

    struct BlendFunction
    {
        u32 blendDrawBuffer;

        u32 sfactor;
        u32 dfactor;
    };

    struct BlendState
    {
        b32 blendingEnabled;
        u32 blendEquation;

        BlendFunction blendFunctions[MAX_BLEND_TARGETS];
        u32 numBlendFunctions;
    };

    struct DrawParameters
    {
        DrawLayer layer;

        DrawFlags flags;

        StencilState stencilState;

        BlendState blendState;

        CullState cullState;
    };

	enum RendererBackend : u8
	{
		OpenGL = 0,
		Vulkan,
		D3D11,
		D3D12,
        NUM_RENDERER_BACKEND_TYPES
	};

    enum OutputMerger : u8
    {
        OUTPUT_NO_AA = 0,
        OUTPUT_FXAA,
        OUTPUT_SMAA_X1,
        OUTPUT_SMAA_T2X,
    };


    const std::string GetRendererBackendName(r2::draw::RendererBackend backend);
}

#endif // !__RENDERER_TYPES_H__

