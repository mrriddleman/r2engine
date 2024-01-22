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
#include "r2/Render/Renderer/Renderer.h"

/*

Current overall layout

uniform buffers:

layout (std140, binding = 0) uniform Matrices
{
	mat4 projection;
	mat4 view;
	mat4 skyboxView;
	mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
	mat4 inverseProjection;
	mat4 inverseView;
	mat4 vpMatrix;
	mat4 prevProjection;
	mat4 prevView;
	mat4 prevVPMatrix;
};

layout (std140, binding = 1) uniform Vectors
{
	vec4 cameraPosTimeW;
	vec4 exposureNearFar;
	vec4 cascadePlanes; //depricated
	vec4 shadowMapSizes;
	vec4 fovAspectResXResY;
	uint64_t frame;
	vec2 clusterScaleBias;
	uvec4 clusterTileSizes; //{tileSizeX, tileSizeY, tileSizeZ, tileSizePx}
	vec4 jitter;
};

layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
	Tex2DAddress pointLightShadowsSurface;
	Tex2DAddress ambientOcclusionSurface;
	Tex2DAddress ambientOcclusionDenoiseSurface;
	Tex2DAddress zPrePassShadowsSurface[2];
	Tex2DAddress ambientOcclusionTemporalDenoiseSurface[2]; //current in 0
	Tex2DAddress normalSurface;
	Tex2DAddress specularSurface;
	Tex2DAddress ssrSurface;
	Tex2DAddress convolvedGBUfferSurface[2]; //@TODO(Serge): fix the name
	Tex2DAddress ssrConeTracedSurface;
	Tex2DAddress bloomDownSampledSurface;
	Tex2DAddress bloomBlurSurface;
	Tex2DAddress bloomUpSampledSurface;
	Tex2DAddress smaaEdgeDetectionSurface;
	Tex2DAddress smaaBlendingWeightSurface;
	Tex2DAddress smaaNeighborhoodBlendingSurface[2];
	Tex2DAddress msaa2XZPrePassSurface;
};

layout (std140, binding = 3) uniform SDSMParams
{
	vec4 lightSpaceBorder;
	vec4 maxScale;
	vec4 projMultSplitScaleZMultLambda;
	float dilationFactor;
	uint scatterTileDim;
	uint reduceTileDim;
	uint padding;
	vec4 splitScaleMultFadeFactor;
	Tex2DAddress blueNoiseTexture;
};

layout (std140, binding = 4) uniform SSRParams
{
	float ssr_stride;
	float ssr_zThickness;
	int ssr_rayMarchIterations;
	float ssr_strideZCutoff;

	Tex2DAddress ssr_ditherTexture;

	float ssr_ditherTilingFactor;
	int ssr_roughnessMips;
	int ssr_coneTracingSteps;
	float ssr_maxFadeDistance;

	float ssr_fadeScreenStart;
	float ssr_fadeScreenEnd;
	float ssr_maxDistance;
};

layout (std140, binding = 5) uniform BloomParams
{
	vec4 bloomFilter; //x - threshold, y = threshold - knee, z = 2.0f * knee, w = 0.25f / knee
	uvec4 bloomResolutions;
	vec4 bloomFilterRadiusIntensity;

	uint64_t textureContainerToSample;
	float texturePageToSample;
	float textureLodToSample;
};

layout (std140, binding=6) uniform AAParams
{
	Tex2DAddress inputTexture;
	float fxaa_lumaThreshold;
	float fxaa_lumaMulReduce;
	float fxaa_lumaMinReduce;
	float fxaa_maxSpan;
	vec2 fxaa_texelStep;
	float smaa_threshold;
	int smaa_maxSearchSteps;
	Tex2DAddress smaa_areaTexture;
	Tex2DAddress smaa_searchTexture;
	ivec4 smaa_subSampleIndices;
	int smaa_cornerRounding;
	int smaa_maxSearchStepsDiag;
	float smaa_cameraMovementWeight;
};

layout (std140, binding = 7) uniform ColorCorrectionValues
{
	float cc_contrast;
	float cc_brightness;
	float cc_saturation;
	float cc_gamma;
	float cc_filmGrainStrength;
};

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
	Partition gPartitions;
	UPartition gPartitionsU;

	vec4 gScale[MAX_NUM_LIGHTS][NUM_FRUSTUM_SPLITS];
	vec4 gBias[MAX_NUM_LIGHTS][NUM_FRUSTUM_SPLITS];

	mat4 gShadowMatrix[MAX_NUM_LIGHTS];

	float gSpotLightShadowMapPages[NUM_SPOTLIGHT_SHADOW_PAGES];
	float gPointLightShadowMapPages[NUM_POINTLIGHT_SHADOW_PAGES];
	float gDirectionLightShadowMapPages[NUM_DIRECTIONLIGHT_SHADOW_PAGES];
};

layout (std430, binding = 7) buffer MaterialOffsets
{
    uvec4 materialOffsets[];
	//x holds the actual material offset
	//in editor builds - y is the entity ID
	//in editor builds - z is the instance of the entity ID (-1 for the first one)
}

struct LightGrid {
	uint offset;
	uint count;
	uint pad0;
	uint pad1;
};

struct VolumeTileAABB
{
	vec4 minPoint;
	vec4 maxPoint;
};

#define MAX_CLUSTERS 4096

layout (std430, binding=8) buffer Clusters
{
	uint globalLightIndexCount;
	uint globalLightIndexList[MAX_NUM_LIGHTS];
	bool activeClusters[MAX_CLUSTERS];
	uint uniqueActiveClusters[MAX_CLUSTERS]; //compacted list of clusterIndices
	LightGrid lightGrid[MAX_CLUSTERS];
	VolumeTileAABB clusters[MAX_CLUSTERS];
};

struct DispatchIndirectCommand
{
	uint numGroupsX;
	uint numGroupsY;
	uint numGroupsZ;
	uint unused;
};

layout (std430, binding=9) buffer DispatchIndirectCommands
{
	DispatchIndirectCommand dispatchCMDs[];
};

*/

struct LightGrid {
	u32 offset;
    u32 count;
    u32 pad0;
    u32 pad1;
};

struct VolumeTileAABB
{
	glm::vec4 minPoint;
	glm::vec4 maxPoint;
};

namespace r2::draw
{
    const u32 ConstantBufferLayout::RING_BUFFER_MULTIPLIER = 3;

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
			case ShaderDataType::Int64:		return 8;
            case ShaderDataType::Bool:     return 1;
            case ShaderDataType::UInt:      return 4;
            case ShaderDataType::UInt2:     return 4 * 2;
            case ShaderDataType::UInt3:     return 4 * 3;
            case ShaderDataType::UInt4:     return 4 * 4;
            case ShaderDataType::UInt64:    return 8;
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
		case ShaderDataType::Int64:		return 1;
		case ShaderDataType::Bool:    return 1;
        case ShaderDataType::Struct:  return 1;
        case ShaderDataType::UInt:    return 1;
        case ShaderDataType::UInt2:   return 2;
        case ShaderDataType::UInt3:   return 3;
        case ShaderDataType::UInt4:   return 4;
		case ShaderDataType::None:    break;
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

	BufferLayout::BufferLayout(const BufferLayout& otherLayout)
	{
		mElements.clear();
		mElements.insert(mElements.end(), otherLayout.mElements.begin(), otherLayout.mElements.end());
		mStrides.clear();
		mStrides.insert(mStrides.end(), otherLayout.mStrides.begin(), otherLayout.mStrides.end());
		mVertexType = otherLayout.mVertexType;
	}

	BufferLayout& BufferLayout::operator=(const BufferLayout& otherLayout)
	{
		if (this == &otherLayout)
		{
			return *this;
		}

		mElements.clear();
		mElements.insert(mElements.end(), otherLayout.mElements.begin(), otherLayout.mElements.end());
		mStrides.clear();
		mStrides.insert(mStrides.end(), otherLayout.mStrides.begin(), otherLayout.mStrides.end());
		mVertexType = otherLayout.mVertexType;

		return *this;
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
		case ShaderDataType::Int64:		return 8;
		case ShaderDataType::Bool:     return 4;
        case ShaderDataType::Struct:   return 4 * 4;
        case ShaderDataType::UInt:      return 4;
        case ShaderDataType::UInt64:    return 8;
        case ShaderDataType::UInt4:     return 4 * 4;
        case ShaderDataType::UInt2:     return 4 * 2;
        case ShaderDataType::UInt3:     return 4 * 3;
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
        , mBufferMult(1)
    {
    }

    ConstantBufferLayout::ConstantBufferLayout(Type type, ConstantBufferFlags bufferFlags, CreateConstantBufferFlags createFlags, const std::initializer_list<ConstantBufferElement>& elements)
        : mElements(elements)
        , mSize(0)
        , mType(type)
        , mFlags(bufferFlags)
        , mCreateFlags(createFlags)
        , mBufferMult(1)
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

        mBufferMult = RING_BUFFER_MULTIPLIER;
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

        mBufferMult = RING_BUFFER_MULTIPLIER;
    }

    void ConstantBufferLayout::InitForMaterials(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numMaterials)
    {
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
        mElements[0].offset = 0;
        mElements[0].typeCount = numMaterials;
        mElements[0].elementSize = sizeof(r2::draw::RenderMaterialParams);
        mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
        mElements[0].type = ShaderDataType::Struct;

        mSize = mElements[0].size;
        mType = Big;
		mFlags = flags;
		mCreateFlags = createFlags;

        mBufferMult = RING_BUFFER_MULTIPLIER;
    }

    void ConstantBufferLayout::InitForBoneTransforms(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numBoneTransforms)
    {
        mElements.clear();
        mElements.emplace_back(ConstantBufferElement());
        mElements[0].offset = 0;
        mElements[0].typeCount = numBoneTransforms;
        mElements[0].elementSize = sizeof(r2::draw::ShaderBoneTransform);
        mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
        mElements[0].type = ShaderDataType::Struct;

        mSize = mElements[0].size;
        mType = Big;
        mFlags = flags;
        mCreateFlags = createFlags;

        mBufferMult = RING_BUFFER_MULTIPLIER;
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

        mBufferMult = RING_BUFFER_MULTIPLIER;
    }

    void ConstantBufferLayout::InitForLighting()
    {
        int index = 0;

		mElements.clear();

		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = 1;
		mElements[0].elementSize = sizeof(r2::draw::SceneLighting);
		mElements[0].size = mElements[0].typeCount * mElements[0].elementSize;
		mElements[0].type = ShaderDataType::Struct;

		mSize = mElements[0].size;
		mType = Big;
		mFlags = 0;
		mCreateFlags = 0;

        //Point lights
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = light::MAX_NUM_POINT_LIGHTS;
  //      mElements[index].type = ShaderDataType::Struct;
  //      mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(PointLight), GetBaseAlignmentSize(mElements[index].type)));
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

  //      index++;

  //      //Direction lights
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = light::MAX_NUM_DIRECTIONAL_LIGHTS;
		//mElements[index].type = ShaderDataType::Struct;
		//mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(DirectionLight), GetBaseAlignmentSize(mElements[index].type)));
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		//index++;

  //      //Spot lights
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = light::MAX_NUM_SPOT_LIGHTS;
		//mElements[index].type = ShaderDataType::Struct;
		//mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(SpotLight), GetBaseAlignmentSize(mElements[index].type)));
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

  //      index++;

  //      //Sky light
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = 1;
		//mElements[index].type = ShaderDataType::Struct;
		//mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(SkyLight), GetBaseAlignmentSize(mElements[index].type)));
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;
  //      
		//index++;

  //      //mNumPointLights
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = 1;
		//mElements[index].type = ShaderDataType::Int;
		//mElements[index].elementSize = GetBaseAlignmentSize(mElements[index].type);
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		//index++;

		////mNumDirectionLights
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = 1;
		//mElements[index].type = ShaderDataType::Int;
		//mElements[index].elementSize = GetBaseAlignmentSize(mElements[index].type);
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		//index++;

		////mNumSpotLights
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = 1;
		//mElements[index].type = ShaderDataType::Int;
		//mElements[index].elementSize = GetBaseAlignmentSize(mElements[index].type);
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		//index++;

		////numPrefilteredRoughnessMips
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = 1;
		//mElements[index].type = ShaderDataType::Int;
		//mElements[index].elementSize = GetBaseAlignmentSize(mElements[index].type);
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;
		//
		//index++;

		////useSDSMShadows
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = 1;
		//mElements[index].type = ShaderDataType::Int;
		//mElements[index].elementSize = GetBaseAlignmentSize(mElements[index].type);
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		//index++;

		//

		////numShadowCastingDirectionLights
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = 1;
		//mElements[index].type = ShaderDataType::Int;
		//mElements[index].elementSize = GetBaseAlignmentSize(mElements[index].type);
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		//index++;

		////numShadowCastingPointLights
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = 1;
		//mElements[index].type = ShaderDataType::Int;
		//mElements[index].elementSize = GetBaseAlignmentSize(mElements[index].type);
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		//index++;

		////numShadowCastingSpotLights
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = 1;
		//mElements[index].type = ShaderDataType::Int;
		//mElements[index].elementSize = GetBaseAlignmentSize(mElements[index].type);
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		//index++;

		////ShadowCastingLights mShadowCastingDirectionLights;
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = light::NUM_DIRECTIONLIGHT_SHADOW_PAGES;
		//mElements[index].type = ShaderDataType::Int64;
		//mElements[index].elementSize = GetBaseAlignmentSize(mElements[index].type);
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		//index++;

		////ShadowCastingLights mShadowCastingPointLights;
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = light::NUM_POINTLIGHT_SHADOW_PAGES;
		//mElements[index].type = ShaderDataType::Int64;
		//mElements[index].elementSize = GetBaseAlignmentSize(mElements[index].type);
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		//index++;

		////ShadowCastingLights mShadowCastingSpotLights;
		//mElements.emplace_back(ConstantBufferElement());
		//mElements[index].typeCount = light::NUM_SPOTLIGHT_SHADOW_PAGES;
		//mElements[index].type = ShaderDataType::Int64;
		//mElements[index].elementSize = GetBaseAlignmentSize(mElements[index].type);
		//mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		//mType = Big;
		//mFlags = 0;
		//mCreateFlags = 0;
  //      
  //      CalculateOffsetAndSize();
    }

    void ConstantBufferLayout::InitForShadowData()
    {
        mElements.clear();

        int index = 0;

        //gPartitions
        mElements.emplace_back(ConstantBufferElement());
        mElements[index].typeCount = 1;
        mElements[index].type = ShaderDataType::Struct;
        mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(Partition), GetBaseAlignmentSize(mElements[index].type)));
        mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

        ++index;

        //gPartitionsU
        mElements.emplace_back(ConstantBufferElement());
        mElements[index].typeCount = 1;
        mElements[index].type = ShaderDataType::Struct;
        mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(UPartition), GetBaseAlignmentSize(mElements[index].type)));
        mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

        ++index;

        //gScale
		mElements.emplace_back(ConstantBufferElement());
		mElements[index].typeCount = light::MAX_NUM_SHADOW_MAP_PAGES * cam::NUM_FRUSTUM_SPLITS;
		mElements[index].type = ShaderDataType::Float4;
		mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(glm::vec4), GetBaseAlignmentSize(mElements[index].type)));
		mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

        ++index;

        //gBias
		mElements.emplace_back(ConstantBufferElement());
		mElements[index].typeCount = light::MAX_NUM_SHADOW_MAP_PAGES * cam::NUM_FRUSTUM_SPLITS;
		mElements[index].type = ShaderDataType::Float4;
		mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(glm::vec4), GetBaseAlignmentSize(mElements[index].type)));
		mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		++index;

        //gShadowMatrix
        mElements.emplace_back(ConstantBufferElement());
        mElements[index].typeCount = light::MAX_NUM_SHADOW_MAP_PAGES;
        mElements[index].type = ShaderDataType::Mat4;
        mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(glm::mat4), GetBaseAlignmentSize(mElements[index].type)));
        mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

        ++index;


        //gSpotLightShadowMapPages
        mElements.emplace_back(ConstantBufferElement());
        mElements[index].typeCount = light::NUM_SPOTLIGHT_SHADOW_PAGES;
        mElements[index].type = ShaderDataType::Float;
        mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(float), GetBaseAlignmentSize(mElements[index].type)));
        mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

        ++index;

        //gPointLightShadowMapPages
		mElements.emplace_back(ConstantBufferElement());
		mElements[index].typeCount = light::NUM_POINTLIGHT_SHADOW_PAGES;
		mElements[index].type = ShaderDataType::Float;
		mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(float), GetBaseAlignmentSize(mElements[index].type)));
		mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

        ++index;

        //gDirectionLightShadowMapPages
		mElements.emplace_back(ConstantBufferElement());
		mElements[index].typeCount = light::NUM_DIRECTIONLIGHT_SHADOW_PAGES;
		mElements[index].type = ShaderDataType::Float;
		mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(float), GetBaseAlignmentSize(mElements[index].type)));
		mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

        ++index;



		mType = Big;
		mFlags = 0;
		mCreateFlags = 0;

        CalculateOffsetAndSize();
    }

    void ConstantBufferLayout::InitForMaterialOffsets(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numDraws)
    {
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = numDraws;
		mElements[0].elementSize = sizeof(glm::uvec4);
		mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
		mElements[0].type = ShaderDataType::UInt;

		mSize = mElements[0].size;
		mType = Big;
		mFlags = flags;
		mCreateFlags = createFlags;

        mBufferMult = RING_BUFFER_MULTIPLIER;
    }

    void ConstantBufferLayout::InitForClusterAABBs(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 size)
    {
		mElements.clear();
		
        u32 index = 0;

        mElements.emplace_back(ConstantBufferElement());
		mElements[index].typeCount = 1;
		mElements[index].elementSize = sizeof(glm::uvec2);
		mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;
		mElements[index].type = ShaderDataType::UInt2;

        index++;

		mElements.emplace_back(ConstantBufferElement());
		mElements[index].typeCount = light::MAX_NUMBER_OF_LIGHTS_PER_CLUSTER * size; //each cluster could have all of the lights in it I guess?
		mElements[index].elementSize = sizeof(glm::uvec2);
		mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;
		mElements[index].type = ShaderDataType::UInt2;

        index++;

        mElements.emplace_back(ConstantBufferElement());
		mElements[index].typeCount = size;
		mElements[index].elementSize = sizeof(bool);
		mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;
		mElements[index].type = ShaderDataType::Bool;

        index++;
		
		mElements.emplace_back(ConstantBufferElement());
		mElements[index].typeCount = size;
		mElements[index].elementSize = sizeof(u32);
		mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;
		mElements[index].type = ShaderDataType::UInt;

		index++;

		mElements.emplace_back(ConstantBufferElement());
		mElements[index].typeCount = size;
        mElements[index].type = ShaderDataType::Struct;
		mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(LightGrid), GetBaseAlignmentSize(mElements[index].type)));
		mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;
		

		index++;

		mElements.emplace_back(ConstantBufferElement());
		mElements[index].typeCount = size;
        mElements[index].type = ShaderDataType::Struct;
        mElements[index].elementSize = static_cast<u32>(r2::util::RoundUp(sizeof(VolumeTileAABB), GetBaseAlignmentSize(mElements[index].type)));
		mElements[index].size = mElements[index].elementSize * mElements[index].typeCount;

		index++;

		mType = Big;
		mFlags = flags;
		mCreateFlags = createFlags;
        mBufferMult = 1;

        CalculateOffsetAndSize();
    }

    void ConstantBufferLayout::InitForDispatchComputeIndirect(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u32 size)
    {
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = size;
		mElements[0].elementSize = sizeof(cmd::DispatchSubCommand);
		mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
		mElements[0].type = ShaderDataType::Struct;

		mSize = mElements[0].size;
		mType = Big;
		mFlags = flags;
		mCreateFlags = createFlags;

		mBufferMult = 1;
    }

	void ConstantBufferLayout::InitForMeshData(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u32 elementSize, u64 numDraws)
	{
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = numDraws;
		mElements[0].elementSize = elementSize;
		mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
		mElements[0].type = ShaderDataType::Struct;

		mSize = mElements[0].size;
		mType = Big;
		mFlags = flags;
		mCreateFlags = createFlags;

		mBufferMult = RING_BUFFER_MULTIPLIER;
	}

	void  ConstantBufferLayout::InitForSSR()
	{
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = 1;
		mElements[0].elementSize = sizeof(r2::draw::SSRShaderParams);
		mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
		mElements[0].type = ShaderDataType::Struct;

		mSize = mElements[0].size;
		mType = Small;
		mFlags = 0;
		mCreateFlags = 0;

		mBufferMult = 1;
	}

	void ConstantBufferLayout::InitForColorCorrection()
	{
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = 1;
		mElements[0].elementSize = sizeof(r2::draw::ColorCorrection);
		mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
		mElements[0].type = ShaderDataType::Struct;

		mSize = mElements[0].size;
		mType = Small;
		mFlags = 0;
		mCreateFlags = 0;

		mBufferMult = 1;
	}

	void ConstantBufferLayout::InitForAntiAliasing()
	{
		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = 1;
		mElements[0].elementSize = sizeof(r2::draw::AAShaderParams);
		mElements[0].size = mElements[0].elementSize * mElements[0].typeCount;
		mElements[0].type = ShaderDataType::Struct;

		mSize = mElements[0].size;
		mType = Small;
		mFlags = 0;
		mCreateFlags = 0;

		mBufferMult = 1;
	}

    void ConstantBufferLayout::InitForSurfaces(const rt::RenderTargetParams rtParams[])
    {
		u32 numSurfaces = 0;
		for (u32 i = 0; i < NUM_RENDER_TARGET_SURFACES; ++i)
		{
			numSurfaces += rtParams[i].numSurfacesPerTarget;
		}

		mElements.clear();
		mElements.emplace_back(ConstantBufferElement());
		mElements[0].offset = 0;
		mElements[0].typeCount = numSurfaces;
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

        mBufferMult = RING_BUFFER_MULTIPLIER;
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

	BufferLayoutConfiguration::BufferLayoutConfiguration()
	{

	}

	BufferLayoutConfiguration::BufferLayoutConfiguration(const BufferLayoutConfiguration& otherLayout)
	{
		layout = otherLayout.layout;

		for (u32 i = 0; i < otherLayout.numVertexConfigs; ++i)
		{
			vertexBufferConfigs[i] = otherLayout.vertexBufferConfigs[i];
		}
		
		indexBufferConfig = otherLayout.indexBufferConfig;
		useDrawIDs = otherLayout.useDrawIDs;
		maxDrawCount = otherLayout.maxDrawCount;
		numVertexConfigs = otherLayout.numVertexConfigs;
	}

	BufferLayoutConfiguration& BufferLayoutConfiguration::operator=(const BufferLayoutConfiguration& otherLayout)
	{
		if (this == &otherLayout)
		{
			return *this;
		}

		layout = otherLayout.layout;

		for (u32 i = 0; i < otherLayout.numVertexConfigs; ++i)
		{
			vertexBufferConfigs[i] = otherLayout.vertexBufferConfigs[i];
		}

		indexBufferConfig = otherLayout.indexBufferConfig;
		useDrawIDs = otherLayout.useDrawIDs;
		maxDrawCount = otherLayout.maxDrawCount;
		numVertexConfigs = otherLayout.numVertexConfigs;

		return *this;
	}

}
