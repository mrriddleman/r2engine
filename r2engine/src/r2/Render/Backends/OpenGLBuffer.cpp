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
    
    void Create(IndexBuffer& buf, const u32* indices, u64 size, u32 type)
    {
        glGenBuffers(1, &buf.IBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(u32), indices, (GLenum)type);
        UnBind(buf);
        buf.size = size;
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
    
    void AddBuffer(VertexArrayBuffer& arrayBuf, const VertexBuffer& vertexBuf)
    {
        glBindVertexArray(arrayBuf.VAO);
        Bind(vertexBuf);
        
        const auto& layout = vertexBuf.layout;
        for(const auto& element : layout)
        {
            glEnableVertexAttribArray(arrayBuf.vertexBufferIndex);
            
            if (element.type >= ShaderDataType::Float && element.type <= ShaderDataType::Float4)
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
            
            
            ++arrayBuf.vertexBufferIndex;
        }
        
        arrayBuf.vertexBuffers.push_back(vertexBuf);
    }
    
    void SetIndexBuffer(VertexArrayBuffer& arrayBuf, const IndexBuffer& indexBuf)
    {
        glBindVertexArray(arrayBuf.VAO);
        Bind(indexBuf);
        arrayBuf.indexBuffer = indexBuf;
    }
    
}
