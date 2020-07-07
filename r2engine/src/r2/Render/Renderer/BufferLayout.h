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
    };
    
    enum class VertexType
    {
        Vertex = 0,
        Instanced
    };
    
    struct BufferElement
    {
        std::string name;
        ShaderDataType type;
        u32 size;
        size_t offset;
        bool normalized;
        
        BufferElement() = default;
        BufferElement(ShaderDataType type, const std::string& name, bool normalized = false);
        u32 GetComponentCount() const;
        
    };
    
    class BufferLayout
    {
    public:
        BufferLayout();
        BufferLayout(const std::initializer_list<BufferElement>& elements, VertexType vertexType = VertexType::Vertex);
        

        //@TODO(Serge): make these not vectors
        inline u32 GetStride() const {return mStride;}
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
        u32 mStride = 0;
        VertexType mVertexType = VertexType::Vertex;
    };

    struct BufferConfig
    {
        u32 bufferSize = EMPTY_BUFFER; //are we going to have more than 4 gigs for this? if so change
        u32 drawType;
    };

    struct BufferLayoutConfiguration
    {
        BufferLayout layout;
        BufferConfig vertexBufferConfig;
        BufferConfig indexBufferConfig;
    };

    class ConstantBufferElement
    {
    public:
		std::string name;
		ShaderDataType type;
		u32 size;
		size_t offset;
        size_t typeCount;

        ConstantBufferElement() = default;
        ConstantBufferElement(ShaderDataType _type, const std::string& _name, size_t _typeCount = 1);
		u32 GetComponentCount() const;
    };
    
    class ConstantBufferLayout
    {
    public:
        ConstantBufferLayout();
        ConstantBufferLayout(const std::initializer_list<BufferElement>& elements);

        inline const std::vector<BufferElement>& GetElements() const { return mElements; }
		std::vector<BufferElement>::iterator begin() { return mElements.begin(); }
		std::vector<BufferElement>::iterator end() { return mElements.end(); }
		std::vector<BufferElement>::const_iterator begin() const { return mElements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return mElements.end(); }
        size_t GetSize() const { return mSize; }
    private:
        void CalculateOffsetAndSize();
        std::vector<BufferElement> mElements;
        size_t mSize;
    };

	struct ConstantBufferLayoutConfiguration
	{
        ConstantBufferLayout layout;
        u32 drawType;
	};
}

#endif /* BufferLayout_h */
