//
//  Mesh.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#ifndef Mesh_h
#define Mesh_h

#include "glm/glm.hpp"

namespace r2::draw
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
    };
    
    enum TextureType
    {
        Diffuse = 0,
        Specular,
        NUM_TEXTURE_TYPES
    };
    
    struct Texture
    {
        u32 texID;
        TextureType type;
        char path[r2::fs::FILE_PATH_LENGTH];
    };
    
    struct Mesh
    {
        std::vector<Vertex> vertices = {};
        std::vector<u32> indices  = {};
        std::vector<Texture> textures = {};
    };
    
    void CreateMesh(Mesh& aMesh, const std::vector<Vertex>& vertices, const std::vector<u32>& indices, const std::vector<Texture>& textures);
    std::string TextureTypeToString(TextureType type);
}

#endif /* Mesh_h */
