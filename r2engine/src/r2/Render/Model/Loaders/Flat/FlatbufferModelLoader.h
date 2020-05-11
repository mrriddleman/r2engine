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

		for (flatbuffers::uoffset_t i = 0; i < numMeshes; i++)
		{
			const u64 numVertices = flatModel->meshes()->Get(i)->numVertices();
			const u64 numIndices = flatModel->meshes()->Get(i)->numFaces() * 3; //* 3 for indices

			r2::draw::Mesh nextMesh;
			nextMesh = MAKE_MESH(arena, numVertices, numIndices, 0); //@TODO(Serge): fix

			//load the positions
			const auto numPositions = flatModel->meshes()->Get(i)->positions()->size();
			for (flatbuffers::uoffset_t p = 0; p < numPositions; ++p)
			{
				auto* pos = flatModel->meshes()->Get(i)->positions()->Get(p);
				r2::sarr::Push(*nextMesh.optrPositions, glm::vec3(pos->x(), pos->y(), pos->z()));
			}

			const auto numNormals = flatModel->meshes()->Get(i)->normals()->size();
			for (flatbuffers::uoffset_t n = 0; n < numNormals; ++n)
			{
				auto* normal = flatModel->meshes()->Get(i)->normals()->Get(n);
				r2::sarr::Push(*nextMesh.optrNormals, glm::vec3(normal->x(), normal->y(), normal->z()));
			}

			const auto numTexCoords = flatModel->meshes()->Get(i)->textureCoords()->size();
			for (flatbuffers::uoffset_t t = 0; t < numTexCoords; ++t)
			{
				auto* texCoord = flatModel->meshes()->Get(i)->textureCoords()->Get(t);
				r2::sarr::Push(*nextMesh.optrTexCoords, glm::vec2(texCoord->x(), texCoord->y()));
			}

			const auto numFaces = flatModel->meshes()->Get(i)->numFaces();

			for (flatbuffers::uoffset_t f = 0; f < numFaces; ++f)
			{
				for (flatbuffers::uoffset_t j = 0; j < flatModel->meshes()->Get(i)->faces()->Get(f)->numIndices(); ++j)
				{
					r2::sarr::Push(*nextMesh.optrIndices, flatModel->meshes()->Get(i)->faces()->Get(f)->indices()->Get(j));
				}
			}

			r2::sarr::Push(*theModel->optrMeshes, nextMesh);
		}

		//@TODO(Serge) - For now we're just going to evaluate the material and add a new one to the material system
		//We get back a material handle and we set that on the model
		//a better way would probably be to already have these defined somehow in the material system.
		//Then we'd just set the material id

		r2::draw::Material modelMaterial;
		modelMaterial.materialID = flatModel->material()->name();
		//find the shader to use
		modelMaterial.shaderId = r2::draw::shadersystem::FindShaderHandle(flatModel->material()->shader());

		R2_CHECK(modelMaterial.shaderId != InvalidShader, "We couldn't find the shader!");
		modelMaterial.color =
			glm::vec4(flatModel->material()->color()->r(),
				flatModel->material()->color()->g(),
				flatModel->material()->color()->b(),
				flatModel->material()->color()->a());

		theModel->materialHandle = r2::draw::mat::AddMaterial(modelMaterial);

		FREE(modelBuffer, *MEM_ENG_SCRATCH_PTR);

		return theModel;
	}
}