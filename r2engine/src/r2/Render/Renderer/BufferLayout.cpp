//
//  BufferLayout.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//
#include "r2pch.h"
#include "BufferLayout.h"
#include "r2/Utils/Utils.h"
#include "r2/Render/Renderer/RendererImpl.h"

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

	u32 ComponentCount(ShaderDataType type)
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
        case ShaderDataType::Struct:  return 1;
		case ShaderDataType::None:      break;
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
        return ComponentCount(type);
    }
    
    BufferLayout::BufferLayout()
    :mVertexType(VertexType::Vertex)
    {
        
    }
    
    BufferLayout::BufferLayout(const std::initializer_list<BufferElement>& elements, VertexType vertexType)
    : mElements(elements)
    , mVertexType(vertexType)
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

    u32 GetBaseAlignmentSize(ShaderDataType type)
    {
		switch (type)
		{
		case ShaderDataType::Float:    return 4;
		case ShaderDataType::Float2:   return 4 * 2;
		case ShaderDataType::Float3:   return 4 * 4;
		case ShaderDataType::Float4:   return 4 * 4;
		case ShaderDataType::Mat3:     return 4 * 4 * 3;
		case ShaderDataType::Mat4:     return 4 * 4 * 4;
		case ShaderDataType::Int:      return 4;
		case ShaderDataType::Int2:     return 4 * 2;
		case ShaderDataType::Int3:     return 4 * 4;
		case ShaderDataType::Int4:     return 4 * 4;
		case ShaderDataType::Bool:     return 4;
        case ShaderDataType::Struct:   return 4 * 4;
		case ShaderDataType::None:
			break;
		}

		R2_CHECK(false, "Unknown ShaderDataType!");
		return 0;
    }

    ConstantBufferElement::ConstantBufferElement(ShaderDataType _type, const std::string& _name, size_t _typeCount)
		: name(_name)
        , type(_type)
        , elementSize(GetBaseAlignmentSize(_type))
        , size(elementSize * static_cast<u32>(_typeCount))
	    , offset(0)
        , typeCount(_typeCount)
    {

    }

    ConstantBufferElement::ConstantBufferElement(const std::initializer_list<BufferElement>& elements, const std::string& _name, size_t _typeCount)
        : name(_name)
        , type(ShaderDataType::Struct)
        , typeCount(_typeCount)
    {
        InitializeConstantStruct(elements);
    }

    u32 ConstantBufferElement::GetComponentCount() const
    {
        return ComponentCount(type);
    }

	void ConstantBufferElement::InitializeConstantStruct(const std::vector<BufferElement>& elements)
	{
        for (auto& element: elements)
        {
            elementSize += element.size;
        }

        elementSize = static_cast<u32>( r2::util::RoundUp(elementSize, GetBaseAlignmentSize(type)) );
        size = elementSize * static_cast<u32>(typeCount);
	}

    ConstantBufferLayout::ConstantBufferLayout()
        : mElements({})
        , mSize(0)
        , mType(Type::Small)
    {
    }

    ConstantBufferLayout::ConstantBufferLayout(const std::initializer_list<BufferElement>& elements)
        : mElements(elements)
        , mSize(0)
        , mType(Type::Small)
    {
        CalculateOffsetAndSize();
    }

    void ConstantBufferLayout::CalculateOffsetAndSize()
    {
        size_t offset = 0;
		for (auto& element : mElements)
		{
			element.offset = offset;
			offset += element.size;
            mSize += element.size;
		}

		auto maxConstantBufferSize = r2::draw::rendererimpl::MaxConstantBufferSize();

        if (mSize > maxConstantBufferSize)
        {
            mType = Type::Big;
        }
        else
        {
            mType = Type::Small;
        }
    }

    
}
