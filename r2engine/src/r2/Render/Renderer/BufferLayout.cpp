//
//  BufferLayout.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//

#include "BufferLayout.h"

namespace r2::draw
{
    
    u32 ShaderDataTypeSize(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Float:    return 4;
            case ShaderDataType::Float2:   return 4 * 2;
            case ShaderDataType::Float3:   return 4 * 3;
            case ShaderDataType::Float4:   return 4 * 4;
            case ShaderDataType::Mat3:     return 4 * 3 * 3;
            case ShaderDataType::Mat4:     return 4 * 4 * 4;
            case ShaderDataType::Int:      return 4;
            case ShaderDataType::Int2:     return 4 * 2;
            case ShaderDataType::Int3:     return 4 * 3;
            case ShaderDataType::Int4:     return 4 * 4;
            case ShaderDataType::Bool:     return 1;
            case ShaderDataType::None:
                break;
        }
        
        R2_CHECK(false, "Unknown ShaderDataType!");
        return 0;
    }
    
    BufferElement::BufferElement(ShaderDataType _type, const std::string& _name, bool _normalized)
    : name(_name)
    , type(_type)
    , size(ShaderDataTypeSize(_type))
    , offset(0)
    , normalized(_normalized)
    {
        
    }
    
    u32 BufferElement::GetComponentCount() const
    {
        switch (type)
        {
            case ShaderDataType::Float:   return 1;
            case ShaderDataType::Float2:  return 2;
            case ShaderDataType::Float3:  return 3;
            case ShaderDataType::Float4:  return 4;
            case ShaderDataType::Mat3:    return 3 * 3;
            case ShaderDataType::Mat4:    return 4 * 4;
            case ShaderDataType::Int:     return 1;
            case ShaderDataType::Int2:    return 2;
            case ShaderDataType::Int3:    return 3;
            case ShaderDataType::Int4:    return 4;
            case ShaderDataType::Bool:    return 1;
            case ShaderDataType::None:      break;
        }
        
        R2_CHECK(false, "Unknown ShaderDataType!");
        return 0;
    }
    
    BufferLayout::BufferLayout()
    {
        
    }
    
    BufferLayout::BufferLayout(const std::initializer_list<BufferElement>& elements)
    : mElements(elements)
    {
        CalculateOffsetAndStride();
    }
    
    void BufferLayout::CalculateOffsetAndStride()
    {
        size_t offset = 0;
        mStride = 0;
        for (auto& element : mElements)
        {
            element.offset = offset;
            offset += element.size;
            mStride += element.size;
        }
    }
}
