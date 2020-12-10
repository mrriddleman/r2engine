//
//  BufferLayout.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//
#include "r2pch.h"
#include "BufferLayout.h"
#include "r2/Utils/Utils.h"
#include "r2/Render/Renderer/Commands.h"
#include "r2/Render/Renderer/RendererImpl.h"
#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Render/Model/Model.h"

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
    
    BufferElement::BufferElement(ShaderDataType _type, const std::string& _name, u32 _bufferIndex, bool _normalized)
    : name(_name)
    , type(_type)
    , size(ShaderDataTypeSize(_type))
    , offset(0)
    , bufferIndex(_bufferIndex)
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

    u32 BufferLayout::GetStride(u32 bufferIndex) const
    {
        if (bufferIndex >= mStrides.size())
        {
            R2_CHECK(false, "Passed in a buffer index that's out of range! Index is: %u and we only have %zu buffers", bufferIndex, mStrides.size());
            return 0;
        }

        return mStrides[bufferIndex];
    }
    
    void BufferLayout::CalculateOffsetAndStride()
    {
        //find the max buffer index
        u32 maxIndex = 0;
        for (const auto& element : mElements)
        {
            if (maxIndex < element.bufferIndex)
            {
                maxIndex = element.bufferIndex;
            }
        }

        mStrides.resize(maxIndex + 1, 0);

        std::vector<size_t> offsets;
        offsets.resize(mStrides.size(), 0);

        for (auto& element : mElements)
        {
            element.offset = offsets[element.bufferIndex];
            offsets[element.bufferIndex] += element.size;
            mStrides[element.bufferIndex] += element.size;
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
		: type(_type)
        , elementSize(GetBaseAlignmentSize(_type))
        , size(elementSize * static_cast<u32>(_typeCount))
	    , offset(0)
        , typeCount(_typeCount)
    {

    }

    ConstantBufferElement::ConstantBufferElement(const std::initializer_list<ConstantBufferElement>& elements, const std::string& _name, size_t _typeCount)
        : type(ShaderDataType::Struct)
        , elementSize(0)
        , typeCount(_typeCount)
    {
        InitializeConstantStruct(elements);
    }

    u32 ConstantBufferElement::GetComponentCount() const
    {
        return ComponentCount(type);
    }

	void ConstantBufferElement::InitializeConstantStruct(const std::vector<ConstantBufferElement>& elements)
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
        , mFlags(0)
        , mCreateFlags(0)
    {
    }

    ConstantBufferLayout::ConstantBufferLayout(Type type, ConstantBufferFlags bufferFlags, CreateConstantBufferFlags createFlags, const std::initializer_list<ConstantBufferElement>& elements)
        : mElements(elements)
        , mSize(0)
        , mType(type)
        , mFlags(bufferFlags)
        , mCreateFlags(createFlags)
    {
        CalculateOffsetAndSize();
    }

    void ConstantBufferLayout::InitForSubCommands(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numCommands)
    {
        mElements.clear();
        mElements.emplace_back(ConstantBufferElement());
        mElements[0].offset = 0;
        mElements[0].typeCount = numCommands;
        mElements[0].elementSize = sizeof(r2::draw::cmd::DrawBatchSubCommand);
        mElements[0].size = numCommands * mElements[0].elementSize;
        mElements[0].type = ShaderDataType::Struct;

        mSize = mElements[0].size;
        mType = SubCommand;
        mFlags = flags;
        mCreateFlags = createFlags;
    }

    void ConstantBufferLayout::InitForMaterials(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numMaterials)
    {
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
        mElements[0].offset = 0;
        mElements[0].typeCount = 8;
        mElements[0].elementSize = sizeof(r2::draw::tex::TextureAddress);
        mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
        mElements[0].type = ShaderDataType::Struct;

        mSize = mElements[0].size * numMaterials;
        mType = Big;
		mFlags = flags;
		mCreateFlags = createFlags;
    }

    void ConstantBufferLayout::InitForBoneTransforms(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numBoneTransforms)
    {
        mElements.clear();
        mElements.emplace_back(ConstantBufferElement());
        mElements[0].offset = 0;
        mElements[0].typeCount = numBoneTransforms;
        mElements[0].elementSize = sizeof(glm::mat4) * 3;
        mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
        mElements[0].type = ShaderDataType::Struct;

        mSize = mElements[0].size;
        mType = Big;
        mFlags = flags;
        mCreateFlags = createFlags;
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
    }
}
