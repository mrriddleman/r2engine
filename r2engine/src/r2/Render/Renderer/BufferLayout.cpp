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
#include "r2/Render/Model/Light.h"
#include "r2/Render/Renderer/RenderTarget.h"


/*

Current overall layout

uniform buffers:

layout (std140, binding = 0) uniform Matrices
{
	mat4 projection;
	mat4 view;
	mat4 skyboxView;
	mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
};

layout (std140, binding = 1) uniform Vectors
{
	vec4 cameraPosTimeW;
	vec4 exposureNearFar;
	vec4 cascadePlanes;
	vec4 shadowMapSizes;
};

layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
};

layout (std140, binding = 3) uniform SDSMParams
{
	vec4 lightSpaceBorder;
	vec4 maxScale;
	float dilationFactor;
	uint scatterTileDim;
	uint reduceTileDim;
}

ssbo buffers:


layout (std140, binding = 0) buffer Models
{
	mat4 models[];
};

layout (std430, binding = 1) buffer Materials
{
	Material materials[];
};

layout (std430, binding = 2) buffer BoneTransforms
{
	BoneTransform bonesXForms[];
};

layout (std140, binding = 3) buffer BoneTransformOffsets
{
	ivec4 boneOffsets[];
};

layout (std430, binding = 4) buffer Lighting
{
	PointLight pointLights[MAX_NUM_LIGHTS];
	DirLight dirLights[MAX_NUM_LIGHTS];
	SpotLight spotLights[MAX_NUM_LIGHTS];
	SkyLight skylight;

	int numPointLights;
	int numDirectionLights;
	int numSpotLights;
	int numPrefilteredRoughnessMips;
};

struct DebugRenderConstants
{
	vec4 color;
	mat4 modelMatrix;
};

layout (std430, binding = 5) buffer DebugConstants
{
	DebugRenderConstants constants[];
};


layout (std430, binding = 6) buffer ShadowData
{
	Partition gPartitions[NUM_FRUSTUM_SPLITS];
	UPartition gPartitionsU[NUM_FRUSTUM_SPLITS];
	BoundsUint gPartitionBoundsU[NUM_FRUSTUM_SPLITS];
}


*/













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
            case ShaderDataType::UInt:      return 4;
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
        case ShaderDataType::UInt: return 1;
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
        case ShaderDataType::UInt:      return 4;
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

    void ConstantBufferLayout::InitForDebugSubCommands(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numCommands)
    {
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = numCommands;
		mElements[0].elementSize = sizeof(r2::draw::cmd::DrawDebugBatchSubCommand);
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
        mElements[0].typeCount = numMaterials;
        mElements[0].elementSize = sizeof(r2::draw::RenderMaterial);
        mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
        mElements[0].type = ShaderDataType::Struct;

        mSize = mElements[0].size;
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

    void ConstantBufferLayout::InitForBoneTransformOffsets(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numBoneTransformOffsets)
    {
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = numBoneTransformOffsets;
		mElements[0].elementSize = sizeof(glm::ivec4);
		mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
		mElements[0].type = ShaderDataType::Int4;

		mSize = mElements[0].size;
		mType = Big;
		mFlags = flags;
		mCreateFlags = createFlags;
    }

    void ConstantBufferLayout::InitForLighting()
    {
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = 1;
        mElements[0].elementSize = sizeof(SceneLighting);
		mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
		mElements[0].type = ShaderDataType::Struct;

		mSize = mElements[0].size;
		mType = Big;
		mFlags = 0;
		mCreateFlags = 0;
    }

    

    void ConstantBufferLayout::InitForShadowData()
    {
        mElements.clear();

        mElements.emplace_back(ConstantBufferElement());
        mElements[0].typeCount = cam::NUM_FRUSTUM_SPLITS;
        mElements[0].type = ShaderDataType::Struct;
        mElements[0].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(Partition), GetBaseAlignmentSize(mElements[0].type)));
        mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;

        mElements.emplace_back(ConstantBufferElement());
        mElements[1].typeCount = cam::NUM_FRUSTUM_SPLITS;
        mElements[1].type = ShaderDataType::Struct;
        mElements[1].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(UPartition), GetBaseAlignmentSize(mElements[1].type)));
        mElements[1].size = mElements[1].elementSize * mElements[1].typeCount;

        //mElements.emplace_back(ConstantBufferElement());
        //mElements[2].typeCount = cam::NUM_FRUSTUM_SPLITS;
        //mElements[2].type = ShaderDataType::Struct;
        //mElements[2].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(UPartition), GetBaseAlignmentSize(mElements[2].type)));
        //mElements[2].size = mElements[2].elementSize * mElements[2].typeCount;

		mType = Big;
		mFlags = 0;
		mCreateFlags = 0;

        CalculateOffsetAndSize();
    }

    void ConstantBufferLayout::InitForSurfaces()
    {
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = NUM_RENDER_TARGET_SURFACES;
		mElements[0].elementSize = sizeof(tex::TextureAddress);
		mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
		mElements[0].type = ShaderDataType::Struct;

		mSize = mElements[0].size;
		mType = Small;
		mFlags = 0;
		mCreateFlags = 0;
    }

    void ConstantBufferLayout::InitForDebugRenderConstants(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numDebugRenderConstants)
    {
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = numDebugRenderConstants;
		mElements[0].elementSize = sizeof(glm::vec4) + sizeof(glm::mat4);
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
