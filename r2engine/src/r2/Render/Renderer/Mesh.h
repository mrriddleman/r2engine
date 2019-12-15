//
//  Mesh.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#ifndef Mesh_h
#define Mesh_h

#include "r2/Render/Renderer/Vertex.h"

namespace r2::draw
{
    struct Mesh
    {
        std::vector<Vertex> vertices = {};
        std::vector<u32> indices  = {};
        std::vector<Texture> textures = {};
    };
    
    struct SkinnedMesh
    {
        std::vector<SkinnedVertex> vertices = {};
        std::vector<u32> indices = {};
        std::vector<Texture> textures = {};
    };
    
    void CreateMesh(Mesh& aMesh, const std::vector<Vertex>& vertices, const std::vector<u32>& indices, const std::vector<Texture>& textures);
    std::string TextureTypeToString(TextureType type);
}

#endif /* Mesh_h */
