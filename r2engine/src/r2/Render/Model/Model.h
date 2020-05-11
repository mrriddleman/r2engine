//
//  Model.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#ifndef Model_h
#define Model_h

#include "r2/Render/Model/Mesh.h"
#include "r2/Render/Model/Material.h"

#define MAKE_MODEL(arena, numMeshes) r2::draw::MakeModel(arena, numMeshes, __FILE__, __LINE__, "")
#define FREE_MODEL(arena, modelPtr) r2::draw::FreeModel(arena, modelPtr, __FILE__, __LINE__, "")

namespace r2::draw
{
    struct Model
    {
        r2::SArray<Mesh>* optrMeshes = nullptr;
        MaterialHandle materialHandle = mat::InvalidMaterial;
        glm::mat4 globalInverseTransform = glm::mat4(1.0f);

        static u64 MemorySize(u64 numMeshes, u64 numVertices, u64 numIndices, u64 numTextures);
    };

    template<class ARENA>
    Model* MakeModel(ARENA& arena, u64 numMeshes, const char* file, s32 line, const char* description);

    template<class ARENA>
    void FreeModel(ARENA& arena, Model* modelPtr, const char* file, s32 line, const char* description);
}

namespace r2::draw
{
    template<class ARENA>
    Model* MakeModel(ARENA& arena, u64 numMeshes, const char* file, s32 line, const char* description)
    {
        Model* newModel = ALLOC_VERBOSE(Model, arena, file, line, description);
        if (!newModel)
        {
            R2_CHECK(false, "We couldn't allocate a new model!");
            return nullptr;
        }
            
        newModel->optrMeshes = MAKE_SARRAY_VERBOSE(arena, Mesh, numMeshes, file, line, description);

        if (!newModel->optrMeshes)
        {
            FREE_VERBOSE(newModel, arena, file, line, description);
            R2_CHECK(false, "Failed to allocate the array for all of the meshes");
            return false;
        }

        return newModel;
    }

    template<class ARENA>
    void FreeModel(ARENA& arena, Model* modelPtr, const char* file, s32 line, const char* description)
    {
        u64 numMeshes = r2::sarr::Size(*modelPtr->optrMeshes);

        for (u64 i = 0; i < numMeshes; ++i)
        {
            FREE_MESH(arena, r2::sarr::At(*modelPtr->optrMeshes, i));
        }

        FREE_VERBOSE(modelPtr->optrMeshes, arena, file, line, description);
        FREE_VERBOSE(modelPtr, arena, file, line, description);
    }

    
}

#endif /* Model_h */
