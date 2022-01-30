//
//  BufferLayout.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//

#ifndef BufferLayout_h
#define BufferLayout_h

#include "r2/Render/Renderer/RendererTypes.h"

namespace r2::draw
{

    extern u32 VertexDrawTypeStatic;
    extern u32 VertexDrawTypeStream;
    extern u32 VertexDrawTypeDynamic;

    enum class ShaderDataType
    {
        None = 0,
        Float,
        Float2,
        Float3,
        Float4,
        Mat3,
        Mat4,
        Int,
        Int2,
        Int3,
        Int4,
        Bool,
        UInt64,
        Struct,
        UInt
    };
    
    enum class VertexType
    {
        Vertex = 0,
        Instanced
    };

	struct Partition
	{
        glm::vec4 intervalBeginScale;
        glm::vec4 intervalEndBias;

		//float intervalBegin;
		//float intervalEnd;

		//glm::vec3 scale;
		//glm::vec3 bias;
	};

	struct UPartition
	{
        glm::uvec4 intervalBeginMinCoord;
        glm::uvec4 intervalEndMaxCoord;

		//u32 intervalBegin;
		//u32 intervalEnd;

		//glm::uvec3 minCoord;
		//glm::uvec3 maxCoord;
	};
    
    struct BufferElement
    {
        std::string name;
        ShaderDataType type;
        u32 size;
        size_t offset;
        u32 bufferIndex = 0;
        bool normalized;
        
        BufferElement() = default;
        BufferElement(ShaderDataType type, const std::string& name, u32 _bufferIndex = 0, bool normalized = false);
        u32 GetComponentCount() const;
        
    };

    
    class BufferLayout
    {
    public:
        BufferLayout();
        BufferLayout(const std::initializer_list<BufferElement>& elements, VertexType vertexType = VertexType::Vertex);
        

        //@TODO(Serge): make these not vectors
        u32 GetStride(u32 bufferIndex) const;
        inline u32 NumBuffers() const { return static_cast<u32>(mStrides.size()); }
        inline const std::vector<BufferElement>& GetElements() const {return mElements;}
        inline VertexType GetVertexType() const {return mVertexType;}
        std::vector<BufferElement>::iterator begin() {return mElements.begin();}
        std::vector<BufferElement>::iterator end() {return mElements.end();}
        std::vector<BufferElement>::const_iterator begin() const {return mElements.begin();}
        std::vector<BufferElement>::const_iterator end() const {return mElements.end();}
        
    private:
        void CalculateOffsetAndStride();
        //@TODO(Serge): make this not a vector
        std::vector<BufferElement> mElements;
        std::vector<u32> mStrides;
        VertexType mVertexType = VertexType::Vertex;
    };

    struct BufferConfig
    {
        u32 bufferSize = EMPTY_BUFFER; //are we going to have more than 4 gigs for this? if so change
        u32 drawType;
    };

    

    struct BufferLayoutConfiguration
    {
        static const size_t MAX_VERTEX_BUFFER_CONFIGS = 4;

        BufferLayout layout;
        BufferConfig vertexBufferConfigs[MAX_VERTEX_BUFFER_CONFIGS];
        BufferConfig indexBufferConfig;
        b32 useDrawIDs = false;
        u32 maxDrawCount = 0;
        u32 numVertexConfigs = 1;
    };

    class ConstantBufferElement
    {
    public:
		ShaderDataType type;
        u32 elementSize;
        u32 size;
		size_t offset;
        size_t typeCount;

        ConstantBufferElement() = default;
        ConstantBufferElement(ShaderDataType _type, const std::string& _name, size_t _typeCount = 1);
        //for structs only!
        ConstantBufferElement(const std::initializer_list<ConstantBufferElement>& elements, const std::string& name, size_t _typeCount = 1);
		u32 GetComponentCount() const;
    private:
        void InitializeConstantStruct(const std::vector<ConstantBufferElement>& elements);
    };
    
    extern u32 CB_FLAG_MAP_PERSISTENT;
    extern u32 CB_FLAG_MAP_COHERENT;
    extern u32 CB_FLAG_WRITE;
    extern u32 CB_FLAG_READ;
    extern u32 CB_CREATE_FLAG_DYNAMIC_STORAGE;
    using ConstantBufferFlags = r2::Flags<u32, u32>;
    using CreateConstantBufferFlags = r2::Flags<u32, u32>;

    class ConstantBufferLayout
    {
    public:
        enum Type
        {
            Small = 0, //ubo
            Big, //ssbo
            SubCommand, //GL_DRAW_INDIRECT_BUFFER
        };

        ConstantBufferLayout();
        ConstantBufferLayout(Type type, ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, const std::initializer_list<ConstantBufferElement>& elements);

        //Only For SubCommands!
        void InitForSubCommands(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numCommands);
        void InitForMaterials(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numMaterials);
        void InitForBoneTransforms(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numBoneTransforms);
        void InitForBoneTransformOffsets(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numBoneTransformOffsets);
        void InitForLighting();
        void InitForDebugSubCommands(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numCommands);
        void InitForSurfaces();
        void InitForDebugRenderConstants(ConstantBufferFlags flags, CreateConstantBufferFlags createFlags, u64 numBoneTransformOffsets);

        void InitForShadowData();


        inline const std::vector<ConstantBufferElement>& GetElements() const { return mElements; }
		std::vector<ConstantBufferElement>::iterator begin() { return mElements.begin(); }
		std::vector<ConstantBufferElement>::iterator end() { return mElements.end(); }
		std::vector<ConstantBufferElement>::const_iterator begin() const { return mElements.begin(); }
		std::vector<ConstantBufferElement>::const_iterator end() const { return mElements.end(); }
        size_t GetSize() const { return mSize; }
        Type GetType() const { return mType; }
        ConstantBufferFlags GetFlags() const { return mFlags; }
        CreateConstantBufferFlags GetCreateFlags() const { return mCreateFlags; }
    private:
        void CalculateOffsetAndSize();
        std::vector<ConstantBufferElement> mElements;
        size_t mSize;
        Type mType;
        ConstantBufferFlags mFlags;
        CreateConstantBufferFlags mCreateFlags;
    };

	struct ConstantBufferLayoutConfiguration
	{
        ConstantBufferLayout layout;
        u32 drawType;
	};
}

#endif /* BufferLayout_h */
