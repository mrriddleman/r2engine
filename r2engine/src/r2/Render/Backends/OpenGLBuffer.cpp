//
//  OpenGLBuffer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//

#include "OpenGLBuffer.h"
#include "glad/glad.h"

namespace r2::draw::opengl
{
    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Float:    return GL_FLOAT;
            case ShaderDataType::Float2:   return GL_FLOAT;
            case ShaderDataType::Float3:   return GL_FLOAT;
            case ShaderDataType::Float4:   return GL_FLOAT;
            case ShaderDataType::Mat3:     return GL_FLOAT;
            case ShaderDataType::Mat4:     return GL_FLOAT;
            case ShaderDataType::Int:      return GL_INT;
            case ShaderDataType::Int2:     return GL_INT;
            case ShaderDataType::Int3:     return GL_INT;
            case ShaderDataType::Int4:     return GL_INT;
            case ShaderDataType::Bool:     return GL_BOOL;
            case ShaderDataType::None:     break;
        }
        
        R2_CHECK(false, "Unknown ShaderDataType");
        return 0;
    }
    
    void Create(VertexArrayBuffer& buf)
    {
        glGenVertexArrays(1, &buf.VAO);
    }
    
    void Create(VertexBuffer& buf, const BufferLayout& layout, float * vertices, u64 size, u32 type)
    {
        glGenBuffers(1, &buf.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, buf.VBO);
        glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), vertices, (GLenum)type);
        
        buf.layout = layout;
        UnBind(buf);
        buf.size = size;
    }
    
    void Create(VertexBuffer& buf, const BufferLayout& layout, const std::vector<Vertex>& vertices, u32 type)
    {
        glGenBuffers(1, &buf.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, buf.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], (GLenum)type);
        
        buf.layout = layout;
        UnBind(buf);
        buf.size = vertices.size();
    }
    
    void Create(VertexBuffer& buf, const BufferLayout& layout, const Vertex* vertices, u64 size, u32 type)
    {
        glGenBuffers(1, &buf.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, buf.VBO);
        glBufferData(GL_ARRAY_BUFFER, size * sizeof(Vertex), &vertices[0], (GLenum)type);
        
        buf.layout = layout;
        UnBind(buf);
        buf.size = size;
    }
    
    void Create(VertexBuffer& buf, const BufferLayout& layout, const BoneData* boneData, u64 size, u32 type)
    {
        glGenBuffers(1, &buf.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, buf.VBO);
        glBufferData(GL_ARRAY_BUFFER, size * sizeof(BoneData), &boneData[0], (GLenum)type);
        
        buf.layout = layout;
        UnBind(buf);
        buf.size = size;
    }
    
    void Create(VertexBuffer& buf, const BufferLayout& layout, void* data, u64 size, u32 type)
    {
        glGenBuffers(1, &buf.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, buf.VBO);
        glBufferData(GL_ARRAY_BUFFER, size, data, (GLenum)type);
        
        buf.layout = layout;
        UnBind(buf);
        buf.size = size;
    }
    
    void Create(IndexBuffer& buf, const u32* indices, u64 size, u32 type)
    {
        glGenBuffers(1, &buf.IBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(u32), indices, (GLenum)type);
        UnBind(buf);
        buf.size = size;
    }
    
    void Create(FrameBuffer& buf, u32 width, u32 height)
    {
        glGenFramebuffers(1, &buf.FBO);
        buf.width = width;
        buf.height = height;
    }
    
    void Create(RenderBuffer& buf, u32 width, u32 height)
    {
        glGenRenderbuffers(1, &buf.RBO);
        buf.width = width;
        buf.height = height;
    }
    
    void Destroy(VertexArrayBuffer& buf)
    {
        glDeleteVertexArrays(1, &buf.VAO);
        buf.VAO = EMPTY_BUFFER;
        buf.vertexBufferIndex = 0;
        
        for(auto& v : buf.vertexBuffers)
        {
            Destroy(v);
        }
        
        Destroy(buf.indexBuffer);
        
        buf.vertexBuffers.clear();
        
    }
    
    void Destroy(VertexBuffer& buf)
    {
        glDeleteBuffers(1, &buf.VBO);
        buf.VBO = EMPTY_BUFFER;
        buf.size = 0;
    }
    
    void Destroy(IndexBuffer& buf)
    {
        glDeleteBuffers(1, &buf.IBO);
        buf.IBO = EMPTY_BUFFER;
        buf.size = 0;
    }
    
    void Destroy(FrameBuffer& buf)
    {
        glDeleteFramebuffers(1, &buf.FBO);
        buf.FBO = EMPTY_BUFFER;
        buf.width = buf.height = 0;
    }
    
    void Destroy(RenderBuffer& buf)
    {
        glDeleteRenderbuffers(1, &buf.RBO);
        buf.RBO = EMPTY_BUFFER;
        buf.width = buf.height = 0;
    }
    
    void Bind(const VertexArrayBuffer& buf)
    {
        glBindVertexArray(buf.VAO);
    }
    
    void UnBind(const VertexArrayBuffer& buf)
    {
        glBindVertexArray(0);
        if(buf.indexBuffer.IBO != EMPTY_BUFFER)
        {
            UnBind(buf.indexBuffer);
        }
    }
    
    void Bind(const VertexBuffer& buf)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buf.VBO);
    }
    
    void UnBind(const VertexBuffer& buf)
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    void Bind(const IndexBuffer& buf)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.IBO);
    }
    
    void UnBind(const IndexBuffer& buf)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    
    void Bind(const FrameBuffer& buf)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, buf.FBO);
    }
    
    void UnBind(const FrameBuffer& buf)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void Bind(const RenderBuffer& buf)
    {
        glBindRenderbuffer(GL_RENDERBUFFER, buf.RBO);
    }
    
    void UnBind(const RenderBuffer& buf)
    {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
    
    void AddBuffer(VertexArrayBuffer& arrayBuf, const VertexBuffer& vertexBuf)
    {
        glBindVertexArray(arrayBuf.VAO);
        Bind(vertexBuf);
        const auto& layout = vertexBuf.layout;
        for(const auto& element : layout)
        {
            glEnableVertexAttribArray(arrayBuf.vertexBufferIndex);
            
            if (element.type >= ShaderDataType::Float && element.type <= ShaderDataType::Mat4)
            {
                glVertexAttribPointer(arrayBuf.vertexBufferIndex,
                                      element.GetComponentCount(),
                                      ShaderDataTypeToOpenGLBaseType(element.type),
                                      element.normalized ? GL_TRUE : GL_FALSE,
                                      layout.GetStride(),
                                      (const void*)element.offset);
            }
            else if(element.type >= ShaderDataType::Int && element.type <= ShaderDataType::Int4)
            {
                glVertexAttribIPointer(arrayBuf.vertexBufferIndex,
                                       element.GetComponentCount(),
                                       ShaderDataTypeToOpenGLBaseType(element.type),
                                       layout.GetStride(),
                                       (const void*)element.offset);
            }
            
            if (vertexBuf.layout.GetVertexType() == VertexType::Instanced)
            {
                glVertexAttribDivisor(arrayBuf.vertexBufferIndex, 1);
            }
            
            ++arrayBuf.vertexBufferIndex;
        }
        UnBind(vertexBuf);

        
        arrayBuf.vertexBuffers.push_back(vertexBuf);
    }
    
    void SetIndexBuffer(VertexArrayBuffer& arrayBuf, const IndexBuffer& indexBuf)
    {
        glBindVertexArray(arrayBuf.VAO);
        Bind(indexBuf);
        arrayBuf.indexBuffer = indexBuf;
    }
    
    u32 AttachTextureToFrameBuffer(FrameBuffer& buf)
    {
        Bind(buf);
        
        u32 texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, buf.width, buf.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        
        //Attach the texture to the frame buffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)buf.colorAttachments.size(), GL_TEXTURE_2D, texture, 0);
        R2_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Failed to attach texture to frame buffer!");
        buf.colorAttachments.push_back(texture);
        UnBind(buf);
        return texture;
    }
    
    u32 AttachHDRTextureToFrameBuffer(FrameBuffer& buf)
    {
        Bind(buf);
        u32 texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, buf.width, buf.height, 0, GL_RGB, GL_FLOAT, NULL);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        //Attach the texture to the frame buffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)buf.colorAttachments.size(), GL_TEXTURE_2D, texture, 0);
        R2_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Failed to attach texture to frame buffer!");
        buf.colorAttachments.push_back(texture);
        UnBind(buf);
        
        return texture;
    }
    
    u32 AttachDepthToFrameBuffer(FrameBuffer& buf)
    {
        u32 depthMap;
        
        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, buf.width, buf.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        
        
        Bind(buf);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        UnBind(buf);
        
        buf.depthAttachment = depthMap;
        return depthMap;
    }
    
    void AttachDepthCubeMapToFrameBuffer(FrameBuffer& buf, u32 depthCubeMap)
    {
        Bind(buf);
        
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
        //we only care about depth values when generating a depth cubemap so we have to explicitly tell OpenGL this framebuffer object does not render to a color buffer
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        
        UnBind(buf);
    }
    
    u32 AttachDepthAndStencilForFrameBuffer(FrameBuffer& buf)
    {
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, buf.width, buf.height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
        Bind(buf);
        
        //Attach the texture to the frame buffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
        R2_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Failed to attach texture to frame buffer!");
        buf.depthAttachment = texture;
        buf.stencilAttachment = texture;
        return texture;
    }
    
    void AttachDepthAndStencilForRenderBufferToFrameBuffer(const FrameBuffer& frameBuf, const RenderBuffer& rBuf)
    {
        Bind(frameBuf);
        Bind(rBuf);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, rBuf.width, rBuf.height);
        UnBind(rBuf);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rBuf.RBO);
        R2_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Failed to attach texture to frame buffer!");
        UnBind(frameBuf);
    }
    
    u32 AttachMultisampleTextureToFrameBuffer(FrameBuffer& buf, u32 samples)
    {
        Bind(buf);
        u32 texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
        
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, buf.width, buf.height, GL_TRUE);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        
        
        //Attach the texture to the frame buffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)buf.colorAttachments.size(), GL_TEXTURE_2D_MULTISAMPLE, texture, 0);
        R2_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Failed to attach texture to frame buffer!");
        
        buf.colorAttachments.push_back(texture);
        return texture;
    }
    
    void AttachDepthAndStencilMultisampleForRenderBufferToFrameBuffer(const FrameBuffer& frameBuf, const RenderBuffer& rBuf, u32 samples)
    {
        Bind(frameBuf);
        Bind(rBuf);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, rBuf.width, rBuf.height);
        UnBind(rBuf);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rBuf.RBO);
        R2_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Failed to attach texture to frame buffer!");
        UnBind(frameBuf);
    }
}
