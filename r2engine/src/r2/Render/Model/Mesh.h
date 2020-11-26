//
//  Mesh.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#ifndef Mesh_h
#define Mesh_h

#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Model/Material.h"
#include "r2/Render/Renderer/Vertex.h"

#define MAKE_MESH(arena, numVertices, numIndices, numTextures) r2::draw::MakeMesh(arena, numVertices, numIndices, numTextures, __FILE__, __LINE__, "")
#define FREE_MESH(arena, mesh) r2::draw::FreeMesh(arena, mesh, __FILE__, __LINE__, "")

namespace r2::draw
{
    struct Mesh
    {
        u64 hashName = 0;
        r2::SArray<r2::draw::Vertex>* optrVertices = nullptr;
        r2::SArray<u32>* optrIndices = nullptr;
        //r2::SArray<r2::draw::MaterialHandle>* optrMaterials = nullptr;

        static u64 MemorySize(u64 numVertices, u64 numIndices, u64 alignment, u64 headerSize, u64 boundsChecking);
    };
    
    template<class ARENA>
    Mesh MakeMesh(ARENA& arena, u64 numVertices, u64 numIndices, u64 numMaterials, const char* file, s32 line, const char* description);

    template<class ARENA>
    void FreeMesh(ARENA& arena, Mesh& mesh, const char* file, s32 line, const char* description);
}


namespace r2::draw
{
    template<class ARENA>
    Mesh MakeMesh(ARENA& arena, u64 numVertices, u64 numIndices, u64 numMaterials, const char* file, s32 line, const char* description)
    {
        //@TODO(Serge): maybe we want to allocate the mesh struct too?
        Mesh newMesh;
        newMesh.optrVertices = MAKE_SARRAY_VERBOSE(arena, r2::draw::Vertex, numVertices, file, line, description);

        if(numIndices > 0)
            newMesh.optrIndices = MAKE_SARRAY_VERBOSE(arena, u32, numIndices, file, line, description);

        if(numMaterials > 0)
            newMesh.optrMaterials = MAKE_SARRAY_VERBOSE(arena, r2::draw::MaterialHandle, numMaterials, file, line, description);

        return newMesh;
    }

    template<class ARENA>
    void FreeMesh(ARENA& arena, Mesh& mesh, const char* file, s32 line, const char* description)
    {
        //reverse order in case this was a stack
        FREE_VERBOSE(mesh.optrMaterials, arena, file, line, description);
        FREE_VERBOSE(mesh.optrIndices, arena, file, line, description);
        FREE_VERBOSE(mesh.optrVertices, arena, file, line, description);
        

        //@TODO(Serge): maybe we want to free the mesh struct too?
    }
}


#endif /* Mesh_h */
