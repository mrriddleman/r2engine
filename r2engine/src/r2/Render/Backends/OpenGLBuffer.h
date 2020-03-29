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
#include "glad/glad.h"

namespace r2::draw::opengl
{
    const u32 EMPTY_BUFFER = 0;
    
    struct VertexBuffer
    {
        u32 VBO = EMPTY_BUFFER;
        u64 size = 0;
        BufferLayout layout;
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
    
    struct FrameBuffer
    {
        u32 FBO = EMPTY_BUFFER;
        u32 width = 0, height = 0;
        std::vector<u32> colorAttachments;
        u32 depthAttachment = EMPTY_BUFFER;
        u32 stencilAttachment = EMPTY_BUFFER;
    };
    
    struct RenderBuffer
    {
        u32 RBO = EMPTY_BUFFER;
        u32 width = 0, height = 0;
    };
    
    void Create(VertexArrayBuffer& buf);
    void Create(VertexBuffer& buf, const BufferLayout& layout, float * vertices, u64 size, u32 type);
    void Create(VertexBuffer& buf, const BufferLayout& layout, const std::vector<Vertex>& vertices, u32 type);
    void Create(VertexBuffer& buf, const BufferLayout& layout, const Vertex* vertices, u64 size, u32 type);
    void Create(VertexBuffer& buf, const BufferLayout& layout, const BoneData* boneData, u64 size, u32 type);
    
    void Create(VertexBuffer& buf, const BufferLayout& layout, void* data, u64 size, u32 type);
    
    void Create(IndexBuffer& buf, const u32* indices, u64 size, u32 type);
    
    void Create(FrameBuffer& buf, u32 width, u32 height);
    void Create(FrameBuffer& buf, const r2::util::Size& size);
    void Create(RenderBuffer& buf, u32 width, u32 height);
    
    void Destroy(VertexArrayBuffer& buf);
    void Destroy(VertexBuffer& buf);
    void Destroy(IndexBuffer& buf);
    void Destroy(FrameBuffer& buf);
    void Destroy(RenderBuffer& buf);
    
    void AddBuffer(VertexArrayBuffer& arrayBuf, const VertexBuffer& buf);
    void SetIndexBuffer(VertexArrayBuffer& arrayBuf, const IndexBuffer& buf);
    
    void Bind(const VertexArrayBuffer& buf);
    void UnBind(const VertexArrayBuffer& buf);
    
    void Bind(const VertexBuffer& buf);
    void UnBind(const VertexBuffer& buf);
    
    void Bind(const IndexBuffer& buf);
    void UnBind(const IndexBuffer& buf);
    
    void Bind(const FrameBuffer& buf);
    void UnBind(const FrameBuffer& buf);
    
    void Bind(const RenderBuffer& buf);
    void UnBind(const RenderBuffer& buf);
    
    u32 AttachTextureToFrameBuffer(FrameBuffer& buf, bool alpha = false, GLenum filter = GL_LINEAR);
    u32 AttachHDRTextureToFrameBuffer(FrameBuffer& buf, GLenum internalFormat, GLenum filter = GL_LINEAR, GLenum wrapMode = GL_REPEAT);
    
    
    
    u32 AttachDepthAndStencilForFrameBuffer(FrameBuffer& buf);
    void AttachDepthAndStencilForRenderBufferToFrameBuffer(const FrameBuffer& frameBuf, const RenderBuffer& rBuf);
    void AttachDepthBufferForRenderBufferToFrameBuffer(const FrameBuffer& frameBuf, const RenderBuffer& rBuf);
    u32 AttachDepthToFrameBuffer(FrameBuffer& buf);
    
    void AttachDepthCubeMapToFrameBuffer(FrameBuffer& buf, u32 depthCubeMap);
    
    u32 AttachMultisampleTextureToFrameBuffer(FrameBuffer& buf, u32 samples);
    void AttachDepthAndStencilMultisampleForRenderBufferToFrameBuffer(const FrameBuffer& frameBuf, const RenderBuffer& rBuf, u32 samples);
    
}

#endif /* OpenGLBuffer_h */
