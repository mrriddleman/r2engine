#pragma once

#include "r2/Render/Model/Model_generated.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Model/Mesh.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Model/Material.h"
#include "r2/Render/Renderer/ShaderSystem.h"

namespace r2::draw::flat
{
	template<class ARENA>
	Model* LoadModel(ARENA& arena, const char* filePath);
}


namespace r2::draw::flat
{
	//@TODO(Serge): pass in the material system used for the model
	template<class ARENA>
	Model* LoadModel(ARENA& arena, const char* filePath)
	{
		//@NOTE: hopefully the file is less than 4 mb
		void* modelBuffer = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, filePath);

		R2_CHECK(modelBuffer != nullptr, "We couldn't get the model!");

		const r2::Model* flatModel = r2::GetModel(modelBuffer);

		R2_CHECK(flatModel != nullptr, "We couldn't get the flat buffer model!");

		const auto numMeshes = flatModel->meshes()->size();

		Model* theModel = MAKE_MODEL(arena, numMeshes);
		if (!theModel)
		{
			FREE(modelBuffer, *MEM_ENG_SCRATCH_PTR);
			return nullptr;
		}

		theModel->hash = flatModel->name();

		for (flatbuffers::uoffset_t i = 0; i < numMeshes; i++)
		{
			const u64 numVertices = flatModel->meshes()->Get(i)->numVertices();
			const u64 numIndices = flatModel->meshes()->Get(i)->numFaces() * 3; //* 3 for indices
			const u64 numMaterials = flatModel->meshes()->Get(i)->materials()->size();

			r2::draw::Mesh nextMesh;
			nextMesh = MAKE_MESH(arena, numVertices, numIndices, numMaterials); //@TODO(Serge): fix 0

			for (flatbuffers::uoffset_t v = 0; v < numVertices; ++v)
			{
				r2::draw::Vertex nextVertex;

				const auto* positions = flatModel->meshes()->Get(i)->positions();
				const auto* normals = flatModel->meshes()->Get(i)->normals();
				const auto* texCoords = flatModel->meshes()->Get(i)->textureCoords();

				R2_CHECK(positions != nullptr, "We should have positions always!");

				nextVertex.position = glm::vec3(positions->Get(v)->x(), positions->Get(v)->y(), positions->Get(v)->z());

				if (normals)
				{
					nextVertex.normal = glm::vec3(normals->Get(v)->x(), normals->Get(v)->y(), normals->Get(v)->z());
				}
				
				if (texCoords)
				{
					nextVertex.texCoords = glm::vec2(texCoords->Get(v)->x(), texCoords->Get(v)->y());
				}

				r2::sarr::Push(*nextMesh.optrVertices, nextVertex);
			}

			const auto numFaces = flatModel->meshes()->Get(i)->numFaces();

			for (flatbuffers::uoffset_t f = 0; f < numFaces; ++f)
			{
				for (flatbuffers::uoffset_t j = 0; j < flatModel->meshes()->Get(i)->faces()->Get(f)->numIndices(); ++j)
				{
					r2::sarr::Push(*nextMesh.optrIndices, flatModel->meshes()->Get(i)->faces()->Get(f)->indices()->Get(j));
				}
			}

			for (flatbuffers::uoffset_t m = 0; m < numMaterials; ++m)
			{
				//@NOTE: this assumes that the materials are already loaded so that needs to happen first
				u64 materialName = flatModel->meshes()->Get(i)->materials()->Get(m)->name();
				r2::sarr::Push(*nextMesh.optrMaterials, r2::draw::mat::GetMaterialHandleFromMaterialName(materialName));
			}

			r2::sarr::Push(*theModel->optrMeshes, nextMesh);
		}

		FREE(modelBuffer, *MEM_ENG_SCRATCH_PTR);

		return theModel;
	}
}