#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/MakeEngineModels.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Render/Model/Model_generated.h"
#include "r2/Utils/Hash.h"
#include "r2/Core/File/PathUtils.h"
#include <filesystem>
#include <map>
#include <algorithm>
#include <glm/gtc/constants.hpp>

namespace r2::asset::pln
{
	const std::string MODEL_SCHEMA_FBS = "Model.fbs";
	const std::string JSON_EXT = ".json";
	const std::string MODL_EXT = ".modl";

	void MakeQuad(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeCube(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeSphere(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);

	std::map<std::string, MakeModlFunc> s_makeModelsMap
	{
		{"Quad.modl", MakeQuad},
		{"Cube.modl", MakeCube},
		{"Sphere.modl", MakeSphere}
	};

	std::vector<MakeModlFunc> ShouldMakeEngineModels()
	{
		std::vector<MakeModlFunc> makeModelFuncs
		{ 
			MakeQuad,
			MakeCube,
			MakeSphere
		};

		for (const auto& modelFile : std::filesystem::directory_iterator(R2_ENGINE_INTERNAL_MODELS_BIN))
		{
			auto iter = s_makeModelsMap.find(modelFile.path().filename().string());
			if (iter != s_makeModelsMap.end())
			{
				makeModelFuncs.erase(std::remove(makeModelFuncs.begin(), makeModelFuncs.end(), iter->second));
			}
		}

		return makeModelFuncs;
	}

	void MakeEngineModels(const std::vector<MakeModlFunc>& makeModels)
	{
		std::string schemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;
		char modelSchemaPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(schemaPath.c_str(), modelSchemaPath, MODEL_SCHEMA_FBS.c_str());

		for (auto makeModelFunc : makeModels)
		{
			makeModelFunc(modelSchemaPath, R2_ENGINE_INTERNAL_MODELS_BIN, R2_ENGINE_INTERNAL_MODELS_RAW);
		}
	}

	void MakeQuad(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		flatbuffers::FlatBufferBuilder fbb;
		std::vector<flatbuffers::Offset<r2::Mesh>> meshes;

		std::vector<r2::Vertex3> positions;
		std::vector<r2::Vertex3> normals;
		std::vector<r2::Vertex2> texCoords;

		r2::Vertex3 p1 = r2::Vertex3(-0.5f, -0.5f, 0.0f);
		r2::Vertex3 p2 = r2::Vertex3(-0.5f, 0.5f, 0.0f);
		r2::Vertex3 p3 = r2::Vertex3(0.5f, -0.5f, 0.0f);
		r2::Vertex3 p4 = r2::Vertex3(0.5f, 0.5f, 0.0f);

		positions.push_back(p1);
		positions.push_back(p2);
		positions.push_back(p3);
		positions.push_back(p4);

		r2::Vertex3 n1 = r2::Vertex3(0.0f, 0.0f, 1.0f);
		r2::Vertex3 n2 = r2::Vertex3(0.0f, 0.0f, 1.0f);
		r2::Vertex3 n3 = r2::Vertex3(0.0f, 0.0f, 1.0f);
		r2::Vertex3 n4 = r2::Vertex3(0.0f, 0.0f, 1.0f);

		normals.push_back(n1);
		normals.push_back(n2);
		normals.push_back(n3);
		normals.push_back(n4);

		r2::Vertex2 t1 = r2::Vertex2(0.0f, 0.0f);
		r2::Vertex2 t2 = r2::Vertex2(0.0f, 1.0f);
		r2::Vertex2 t3 = r2::Vertex2(1.0f, 0.0f);
		r2::Vertex2 t4 = r2::Vertex2(1.0f, 1.0f);

		texCoords.push_back(t1);
		texCoords.push_back(t2);
		texCoords.push_back(t3);
		texCoords.push_back(t4);

		std::vector<flatbuffers::Offset<r2::Face>> faces;

		std::vector<uint32_t> indices;

		indices.push_back(0);
		indices.push_back(2);
		indices.push_back(1);

		faces.push_back( r2::CreateFace(fbb, 3, fbb.CreateVector(indices)) );

		indices.clear();
		
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(3);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		std::vector<flatbuffers::Offset<r2::MaterialID>> materials;
		materials.push_back(r2::CreateMaterialID(fbb, STRING_ID("Basic"))); //@temporary

		flatbuffers::Offset<r2::Mesh> mesh= r2::CreateMeshDirect(fbb, positions.size(), faces.size(), 
			&positions, &normals, &texCoords, &faces, &materials);

		meshes.push_back(mesh);

		auto model = r2::CreateModel(fbb, STRING_ID("Quad"), fbb.CreateVector(meshes));
		fbb.Finish(model);
		const std::string name = "/Quad";

		byte* buf = fbb.GetBufferPointer();
		u32 size = fbb.GetSize();

		r2::asset::pln::flathelp::GenerateJSONAndBinary(
			buf, size,
			schemaPath, binaryParentDir + name + MODL_EXT,
			jsonParentDir + name + JSON_EXT);
	}

	void MakeCube(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		flatbuffers::FlatBufferBuilder fbb;
		std::vector<flatbuffers::Offset<r2::Mesh>> meshes;

		std::vector<r2::Vertex3> positions;
		std::vector<r2::Vertex3> normals;
		std::vector<r2::Vertex2> texCoords;

		//Front face
		positions.push_back(r2::Vertex3(0.5f, 0.5f, 0.5f));
		positions.push_back(r2::Vertex3(0.5f, -0.5f, 0.5f));
		positions.push_back(r2::Vertex3(-0.5f, -0.5f, 0.5f));
		positions.push_back(r2::Vertex3(-0.5f, 0.5f, 0.5f));

		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));

		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));

		//Right Face
		positions.push_back(r2::Vertex3(0.5f, 0.5f, 0.5f));
		positions.push_back(r2::Vertex3(0.5f, -0.5f, 0.5f));
		positions.push_back(r2::Vertex3(0.5f, -0.5f, -0.5f));
		positions.push_back(r2::Vertex3(0.5f, 0.5f, -0.5f));

		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Left Face
		positions.push_back(r2::Vertex3(-0.5f, -0.5f, -0.5f));
		positions.push_back(r2::Vertex3(-0.5f, 0.5f, -0.5f));
		positions.push_back(r2::Vertex3(-0.5f, -0.5f, 0.5f));
		positions.push_back(r2::Vertex3(-0.5f, 0.5f, 0.5f));

		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Back Face
		positions.push_back(r2::Vertex3(0.5f, -0.5f, -0.5f));
		positions.push_back(r2::Vertex3(0.5f, 0.5f, -0.5f));
		positions.push_back(r2::Vertex3(-0.5f, -0.5f, -0.5f));
		positions.push_back(r2::Vertex3(-0.5f, 0.5f, -0.5f));

		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Top Face
		positions.push_back(r2::Vertex3(-0.5f, 0.5f, 0.5f));
		positions.push_back(r2::Vertex3(-0.5f, 0.5f, -0.5f));
		positions.push_back(r2::Vertex3(0.5f, 0.5f, 0.5f));
		positions.push_back(r2::Vertex3(0.5f, 0.5f, -0.5f));

		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Bottom Face
		positions.push_back(r2::Vertex3(-0.5f, -0.5f, -0.5f));
		positions.push_back(r2::Vertex3(-0.5f, -0.5f, 0.5f));
		positions.push_back(r2::Vertex3(0.5f, -0.5f, -0.5f));
		positions.push_back(r2::Vertex3(0.5f, -0.5f, 0.5f));

		normals.push_back(r2::Vertex3(0.0f, -1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0f, -1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0f, -1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0f, -1.0f, 0.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));
		
		std::vector<flatbuffers::Offset<r2::Face>> faces;

		std::vector<uint32_t> indices;

		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(3);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(3);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(4);
		indices.push_back(6);
		indices.push_back(5);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(6);
		indices.push_back(4);
		indices.push_back(7);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(8);
		indices.push_back(9);
		indices.push_back(10);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(9);
		indices.push_back(11);
		indices.push_back(10);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(12);
		indices.push_back(13);
		indices.push_back(14);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(13);
		indices.push_back(15);
		indices.push_back(14);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(16);
		indices.push_back(17);
		indices.push_back(18);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(17);
		indices.push_back(19);
		indices.push_back(18);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(20);
		indices.push_back(21);
		indices.push_back(22);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(21);
		indices.push_back(23);
		indices.push_back(22);

		faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();
		
		std::vector<flatbuffers::Offset<r2::MaterialID>> materials;
		materials.push_back(r2::CreateMaterialID(fbb, STRING_ID("Basic"))); //@temporary

		flatbuffers::Offset<r2::Mesh> mesh = r2::CreateMeshDirect(fbb, positions.size(), faces.size(),
			&positions, &normals, &texCoords, &faces, &materials);

		meshes.push_back(mesh);

		auto model = r2::CreateModel(fbb, STRING_ID("Cube"), fbb.CreateVector(meshes));
		fbb.Finish(model);
		const std::string name = "/Cube";

		byte* buf = fbb.GetBufferPointer();
		u32 size = fbb.GetSize();

		r2::asset::pln::flathelp::GenerateJSONAndBinary(
			buf, size,
			schemaPath, binaryParentDir + name + MODL_EXT,
			jsonParentDir + name + JSON_EXT);
	}

	void MakeSphere(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		constexpr float PI =  glm::pi<float>();
		constexpr u32 segments = 32;
		flatbuffers::FlatBufferBuilder fbb;
		std::vector<flatbuffers::Offset<r2::Mesh>> meshes;

		std::vector<r2::Vertex3> positions;
		std::vector<r2::Vertex3> normals;
		std::vector<r2::Vertex2> texCoords;

		std::vector<flatbuffers::Offset<r2::Face>> faces;

		std::vector<uint32_t> indices;

		for (u32 y = 0; y <= segments; ++y)
		{
			for (u32 x = 0; x <= segments; ++x)
			{
				float xSegment = (float)x / (float)segments;
				float ySegment = (float)y / (float)segments;
				float xPos = cos(xSegment * 2.0f * PI) * sin(ySegment * PI);
				float yPos = cos(ySegment * PI);
				float zPos = sin(xSegment * 2.0f * PI) * sin(ySegment * PI);

				positions.push_back(r2::Vertex3(xPos, yPos, zPos));
				normals.push_back(r2::Vertex3(xPos, yPos, zPos));
				texCoords.push_back(r2::Vertex2(xSegment, ySegment));
			}
		}

		// generate CCW index list of sphere triangles
		int k1, k2;
		for (int i = 0; i < segments; ++i)
		{
			k1 = i * (segments + 1);     // beginning of current stack
			k2 = k1 + segments + 1;      // beginning of next stack

			for (int j = 0; j < segments; ++j, ++k1, ++k2)
			{
				// 2 triangles per sector excluding first and last stacks
				// k1 => k2 => k1+1
				if (i != 0)
				{
					indices.push_back(k1);
					indices.push_back(k2);
					indices.push_back(k1 + 1);

					faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));
					indices.clear();
				}

				// k1+1 => k2 => k2+1
				if (i != (segments - 1))
				{
					indices.push_back(k1 + 1);
					indices.push_back(k2);
					indices.push_back(k2 + 1);

					faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));
					indices.clear();
				}
			}
		}

		std::vector<flatbuffers::Offset<r2::MaterialID>> materials;
		materials.push_back(r2::CreateMaterialID(fbb, STRING_ID("Basic"))); //@temporary

		flatbuffers::Offset<r2::Mesh> mesh = r2::CreateMeshDirect(fbb, positions.size(), faces.size(),
			&positions, &normals, &texCoords, &faces, &materials);

		meshes.push_back(mesh);

		auto model = r2::CreateModel(fbb, STRING_ID("Sphere"), fbb.CreateVector(meshes));
		fbb.Finish(model);
		const std::string name = "/Sphere";

		byte* buf = fbb.GetBufferPointer();
		u32 size = fbb.GetSize();

		r2::asset::pln::flathelp::GenerateJSONAndBinary(
			buf, size,
			schemaPath, binaryParentDir + name + MODL_EXT,
			jsonParentDir + name + JSON_EXT);
	}
}
#endif