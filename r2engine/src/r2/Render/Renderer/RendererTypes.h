#ifndef __RENDERER_TYPES_H__
#define __RENDERER_TYPES_H__

#include "r2/Utils/Utils.h"
#include "glm/glm.hpp"

namespace r2::draw
{
	using BufferLayoutHandle = u32; //VBA
    using VertexBufferHandle = u32; //VBO
    using IndexBufferHandle = u32; //IBO
    using DrawIDHandle = u32;
    using ConstantBufferHandle = u32; //GL_SHADER_STORAGE_BUFFER or GL_UNIFORM_BUFFER
    using BatchBufferHandle = u32; //GL_DRAW_INDIRECT_BUFFER


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
}

#endif // !__RENDERER_TYPES_H__

