//
//  Mesh.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#include "Mesh.h"

namespace r2::draw
{
    void CreateMesh(Mesh& aMesh, const std::vector<Vertex>& vertices, const std::vector<u32>& indices, const std::vector<Texture>& textures)
    {
        aMesh.vertices = vertices;
        aMesh.indices = indices;
        aMesh.textures = textures;
    }
    
    std::string TextureTypeToString(TextureType type)
    {
        switch (type)
        {
            case TextureType::Diffuse:
                return "texture_diffuse";
            case TextureType::Specular:
                return "texture_specular";
            case TextureType::Ambient:
                return "texture_ambient";
            default:
                R2_CHECK(false, "Unsupported texture type!");
                return "";
        }
    }
}
