//
//  Mesh.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//
#include "r2pch.h"
#include "Mesh.h"

namespace r2::draw
{
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
            case TextureType::Normal:
                return "texture_normal";
            default:
                R2_CHECK(false, "Unsupported texture type!");
                return "";
        }
    }

    u64 Mesh::MemorySize(u64 numVertices, u64 numIndices, u64 numTextures)
    {
        return
            r2::SArray<glm::vec3>::MemorySize(numVertices) * 2 + //positions + normals
            r2::SArray<glm::vec2>::MemorySize(numVertices) + //texcoords
            r2::SArray<u32>::MemorySize(numIndices) +
            r2::SArray<Texture>::MemorySize(numTextures);
    }
}
