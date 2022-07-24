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
	using VertexConfigHandle = s32;
	using ConstantConfigHandle = s32;

    const VertexConfigHandle InvalidVertexConfigHandle = -1;
    const ConstantConfigHandle InvalidConstantConfigHandle = -1;

    static const u32 EMPTY_BUFFER = 0;

    struct VertexBuffer
    {
        u32 VBO = EMPTY_BUFFER;
        u64 size = 0;
    };

    struct IndexBuffer
    {
        u32 IBO = EMPTY_BUFFER;
        u64 size = 0;
    };

    struct VertexArrayBuffer
    {
        u32 VAO = EMPTY_BUFFER;
        std::vector<VertexBuffer> vertexBuffers;
        IndexBuffer indexBuffer;
        u32 vertexBufferIndex = 0;
    };

	enum class PrimitiveType : u8
	{
		POINTS,
		LINES,
		LINE_LOOP,
		LINE_STRIP,
		TRIANGLES,
		TRIANGLE_STRIP,
		TRIANGLE_FAN
	};

    enum eDrawFlags
    {
        DEPTH_TEST = 1 << 0,
        FILL_MODEL = 1 << 1 //for debug only I guess...
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
        NUM_DEBUG_DRAW_TYPES
    };

    enum DrawLayer : u8
    {
        DL_CLEAR = 0,
        DL_WORLD, //static models
        DL_CHARACTER, //dynamic models
        DL_EFFECT,
        DL_SKYBOX,
        DL_HUD,
        DL_SCREEN,

        DL_COMPUTE, //should be second to last (or last maybe?)
        DL_DEBUG //should always be last
    };

	enum RendererBackend : u8
	{
		OpenGL = 0,
		Vulkan,
		D3D11,
		D3D12,
        NUM_RENDERER_BACKEND_TYPES
	};

    const std::string GetRendererBackendName(r2::draw::RendererBackend backend);
}

#endif // !__RENDERER_TYPES_H__

