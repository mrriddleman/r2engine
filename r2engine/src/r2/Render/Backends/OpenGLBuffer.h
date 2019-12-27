//
//  OpenGLBuffer.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//

#ifndef OpenGLBuffer_h
#define OpenGLBuffer_h

#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/Vertex.h"

namespace r2::draw::opengl
{
    const u32 EMPTY_BUFFER = 0;
    
    struct VertexBuffer
    {
        u32 VBO = EMPTY_BUFFER;
        BufferLayout layout;
        u64 size;
    };
    
    struct IndexBuffer
    {
        u32 IBO = EMPTY_BUFFER;
        u64 size;
    };
    
    struct VertexArrayBuffer
    {
        u32 VAO = EMPTY_BUFFER;
        std::vector<VertexBuffer> vertexBuffers;
        IndexBuffer indexBuffer;
        u32 vertexBufferIndex = 0;
    };
    
    
    void Create(VertexArrayBuffer& buf);
    void Create(VertexBuffer& buf, const BufferLayout& layout, float * vertices, u64 size, u32 type);
    void Create(VertexBuffer& buf, const BufferLayout& layout, const std::vector<Vertex>& vertices, u32 type);
    void Create(VertexBuffer& buf, const BufferLayout& layout, const Vertex* vertices, u64 size, u32 type);
    void Create(VertexBuffer& buf, const BufferLayout& layout, const BoneData* boneData, u64 size, u32 type);
    
    void Create(IndexBuffer& buf, const u32* indices, u64 size, u32 type);
    
    void Destroy(VertexArrayBuffer& buf);
    void Destroy(VertexBuffer& buf);
    void Destroy(IndexBuffer& buf);
    
    void AddBuffer(VertexArrayBuffer& arrayBuf, const VertexBuffer& buf);
    void SetIndexBuffer(VertexArrayBuffer& arrayBuf, const IndexBuffer& buf);
    
    void Bind(const VertexArrayBuffer& buf);
    void UnBind(const VertexArrayBuffer& buf);
    
    void Bind(const VertexBuffer& buf);
    void UnBind(const VertexBuffer& buf);
    
    void Bind(const IndexBuffer& buf);
    void UnBind(const IndexBuffer& buf);
}

#endif /* OpenGLBuffer_h */
