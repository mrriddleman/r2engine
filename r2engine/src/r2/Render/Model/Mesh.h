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
#include "r2/Render/Renderer/Vertex.h"


namespace r2::draw
{
	struct Bounds
	{
		glm::vec3 origin;
		glm::vec3 extents;
		float radius;
	};

    struct Mesh
    {
        u64 hashName = 0;
        r2::SArray<r2::draw::Vertex>* optrVertices = nullptr;
        r2::SArray<u32>* optrIndices = nullptr;
        u32 materialIndex = 0;
        Bounds objectBounds; //in model space

        static u64 MemorySize(u64 numVertices, u64 numIndices, u64 alignment, u64 headerSize, u64 boundsChecking);
    };
}

#endif /* Mesh_h */