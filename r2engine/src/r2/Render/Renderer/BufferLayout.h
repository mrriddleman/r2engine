//
//  BufferLayout.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-26.
//

#ifndef BufferLayout_h
#define BufferLayout_h

namespace r2::draw
{
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
        Bool
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
        
        inline u32 GetStride() const {return mStride;}
        inline const std::vector<BufferElement>& GetElements() const {return mElements;}
        inline VertexType GetVertexType() const {return mVertexType;}
        std::vector<BufferElement>::iterator begin() {return mElements.begin();}
        std::vector<BufferElement>::iterator end() {return mElements.end();}
        std::vector<BufferElement>::const_iterator begin() const {return mElements.begin();}
        std::vector<BufferElement>::const_iterator end() const {return mElements.end();}
        
    private:
        void CalculateOffsetAndStride();
        
        std::vector<BufferElement> mElements;
        u32 mStride = 0;
        VertexType mVertexType;
    };
}

#endif /* BufferLayout_h */