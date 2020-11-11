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
	void MakeCone(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeCylinder(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeCylinderInternal(const char* name, const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir, const std::string& materialName, float baseRadius = 1.0f, float topRadius = 1.0f, float height = 1.0f,
		int sectorCount = 36, int stackCount = 1, bool smooth = true);
	void MakeSkybox(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);

	std::map<std::string, MakeModlFunc> s_makeModelsMap
	{
		{"Quad.modl", MakeQuad},
		{"Cube.modl", MakeCube},
		{"Sphere.modl", MakeSphere},
		{"Cone.modl", MakeCone},
		{"Cylinder.modl", MakeCylinder},
		{"Skybox.modl", MakeSkybox}
	};

	std::vector<MakeModlFunc> ShouldMakeEngineModels()
	{
		std::vector<MakeModlFunc> makeModelFuncs
		{ 
			MakeQuad,
			MakeCube,
			MakeSphere,
			MakeCone,
			MakeCylinder,
			MakeSkybox
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
		materials.push_back(r2::CreateMaterialID(fbb, STRING_ID("DebugMipMap"))); //@temporary

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
		positions.push_back(r2::Vertex3(1.0f, 1.0f, 1.0f));
		positions.push_back(r2::Vertex3(1.0f, -1.0f, 1.0f));
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, 1.0f));
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, 1.0f));

		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));

		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));

		//Right Face
		positions.push_back(r2::Vertex3(1.0f, 1.0f, 1.0f));
		positions.push_back(r2::Vertex3(1.0f, -1.0f, 1.0f));
		positions.push_back(r2::Vertex3(1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(1.0f, 1.0f, -1.0f));

		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Left Face
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, -1.0f));
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, 1.0f));
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, 1.0f));

		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Back Face
		positions.push_back(r2::Vertex3(1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(1.0f, 1.0f, -1.0f));
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, -1.0f));

		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Top Face
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, 1.0f));
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, -1.0f));
		positions.push_back(r2::Vertex3(1.0f, 1.0f, 1.0f));
		positions.push_back(r2::Vertex3(1.0f, 1.0f, -1.0f));

		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Bottom Face
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, 1.0f));
		positions.push_back(r2::Vertex3(1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(1.0f, -1.0f, 1.0f));

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

	void MakeSkybox(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		flatbuffers::FlatBufferBuilder fbb;
		std::vector<flatbuffers::Offset<r2::Mesh>> meshes;

		std::vector<r2::Vertex3> positions;
		std::vector<r2::Vertex3> normals;
		std::vector<r2::Vertex2> texCoords;

		//Front face
		positions.push_back(r2::Vertex3(1.0f, 1.0f, 1.0f));
		positions.push_back(r2::Vertex3(1.0f, -1.0f, 1.0f));
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, 1.0f));
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, 1.0f));

		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));

		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));

		//Right Face
		positions.push_back(r2::Vertex3(1.0f, 1.0f, 1.0f));
		positions.push_back(r2::Vertex3(1.0f, -1.0f, 1.0f));
		positions.push_back(r2::Vertex3(1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(1.0f, 1.0f, -1.0f));

		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(1.0f, 0.0f, 0.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Left Face
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, -1.0f));
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, 1.0f));
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, 1.0f));

		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(r2::Vertex3(-1.0f, 0.0f, 0.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Back Face
		positions.push_back(r2::Vertex3(1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(1.0f, 1.0f, -1.0f));
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, -1.0f));

		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Top Face
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, 1.0f));
		positions.push_back(r2::Vertex3(-1.0f, 1.0f, -1.0f));
		positions.push_back(r2::Vertex3(1.0f, 1.0f, 1.0f));
		positions.push_back(r2::Vertex3(1.0f, 1.0f, -1.0f));

		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(r2::Vertex3(0.0, 1.0f, 0.0f));

		texCoords.push_back(r2::Vertex2(0.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(0.0f, 1.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 0.0f));
		texCoords.push_back(r2::Vertex2(1.0f, 1.0f));

		//Bottom Face
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(-1.0f, -1.0f, 1.0f));
		positions.push_back(r2::Vertex3(1.0f, -1.0f, -1.0f));
		positions.push_back(r2::Vertex3(1.0f, -1.0f, 1.0f));

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
		materials.push_back(r2::CreateMaterialID(fbb, STRING_ID("Skybox"))); //@temporary

		flatbuffers::Offset<r2::Mesh> mesh = r2::CreateMeshDirect(fbb, positions.size(), faces.size(),
			&positions, &normals, &texCoords, &faces, &materials);

		meshes.push_back(mesh);

		auto model = r2::CreateModel(fbb, STRING_ID("Skybox"), fbb.CreateVector(meshes));
		fbb.Finish(model);
		const std::string name = "/Skybox";

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
		materials.push_back(r2::CreateMaterialID(fbb, STRING_ID("Face"))); //@temporary

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

	void MakeCone(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeCylinderInternal("Cone", schemaPath, binaryParentDir, jsonParentDir, "Bricks", 1.0f, 0.0f);

	}

	void MakeCylinder(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeCylinderInternal("Cylinder", schemaPath, binaryParentDir, jsonParentDir, "Mandelbrot");
	}

	void MakeCylinderInternal(const char* name, const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir, const std::string& materialName, float baseRadius, float topRadius, float height,
		int sectorCount, int stackCount, bool smooth)
	{
		//Copied from: https://www.songho.ca/opengl/gl_cylinder.html#example_pipe
		constexpr float PI = glm::pi<float>();
		flatbuffers::FlatBufferBuilder fbb;
		std::vector<flatbuffers::Offset<r2::Mesh>> meshes;

		std::vector<r2::Vertex3> positions;
		std::vector<r2::Vertex3> normals;
		std::vector<r2::Vertex2> texCoords;

		std::vector<flatbuffers::Offset<r2::Face>> faces;

		std::vector<uint32_t> indices;
		
		float sectorStep = 2 * PI / sectorCount;
		float sectorAngle;  // radian

		std::vector<float> unitCircleVertices;

		{
			std::vector<float>().swap(unitCircleVertices);
			for (int i = 0; i <= sectorCount; ++i)
			{
				sectorAngle = i * sectorStep;
				unitCircleVertices.push_back(cos(sectorAngle)); // x
				unitCircleVertices.push_back(sin(sectorAngle)); // y
				unitCircleVertices.push_back(0);                // z
			}
		}
		
		// compute the normal vector at 0 degree first
		float zAngle = atan2(baseRadius - topRadius, height);
		float x0 = cos(zAngle);     // nx
		float y0 = 0;               // ny
		float z0 = sin(zAngle);     // nz

		// rotate (x0,y0,z0) per sector angle
		std::vector<float> fnormals;
		for (int i = 0; i <= sectorCount; ++i)
		{
			sectorAngle = i * sectorStep;
			fnormals.push_back(cos(sectorAngle) * x0 - sin(sectorAngle) * y0);
			fnormals.push_back(sin(sectorAngle) * x0 + cos(sectorAngle) * y0);
			fnormals.push_back(z0);
		}

		float x, y, z;
		float radius;
		// put vertices of side cylinder to array by scaling unit circle
		for (int i = 0; i <= stackCount; ++i)
		{
			z = -(height * 0.5f) + (float)i / stackCount * height;      // vertex position z
			radius = baseRadius + (float)i / stackCount * (topRadius - baseRadius);     // lerp
			float t = 1.0f - (float)i / stackCount;   // top-to-bottom

			for (int j = 0, k = 0; j <= sectorCount; ++j, k += 3)
			{
				x = unitCircleVertices[k];
				y = unitCircleVertices[k + 1];

				positions.push_back(r2::Vertex3(x * radius, y * radius, z));
				normals.push_back(Vertex3(fnormals[k], fnormals[k+1], fnormals[k+2]));
				texCoords.push_back(r2::Vertex2((float)j / sectorCount, t));
			}
		}

		// remember where the base.top vertices start
		unsigned int baseVertexIndex = (unsigned int)positions.size();

		// put vertices of base of cylinder
		z = -height * 0.5f;
		positions.push_back(r2::Vertex3(0.0f, 0.0f, z));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));
		texCoords.push_back(r2::Vertex2( 0.5f, 0.5f));
		for (int i = 0, j = 0; i < sectorCount; ++i, j += 3)
		{
			x = unitCircleVertices[j];
			y = unitCircleVertices[j + 1];
			positions.push_back(r2::Vertex3(x * baseRadius, y * baseRadius, z));
			normals.push_back(r2::Vertex3(0.0f, 0.0f, -1.0f));
			texCoords.push_back(r2::Vertex2(-x * 0.5f + 0.5f, -y * 0.5f + 0.5f));
		}

		// remember where the base vertices start
		unsigned int topVertexIndex = (unsigned int)positions.size();

		// put vertices of top of cylinder
		z = height * 0.5f;
		positions.push_back(r2::Vertex3(0.0f, 0.0f, z));
		normals.push_back(r2::Vertex3(0.0f, 0.0f, 1));
		texCoords.push_back(r2::Vertex2(0.5f, 0.5f));
		for (int i = 0, j = 0; i < sectorCount; ++i, j += 3)
		{
			x = unitCircleVertices[j];
			y = unitCircleVertices[j + 1];
			positions.push_back(r2::Vertex3(x * topRadius, y * topRadius, z));
			normals.push_back(r2::Vertex3(0.0f, 0.0f, 1.0f));
			texCoords.push_back(r2::Vertex2(x * 0.5f + 0.5f, -y * 0.5f + 0.5f));
		}


		// put indices for sides
		unsigned int k1, k2;
		for (int i = 0; i < stackCount; ++i)
		{
			k1 = i * (sectorCount + 1);     // bebinning of current stack
			k2 = k1 + sectorCount + 1;      // beginning of next stack

			for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
			{
				// 2 trianles per sector
				indices.push_back(k1);
				indices.push_back(k1 + 1);
				indices.push_back(k2);
				
				faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();

				indices.push_back(k2);
				indices.push_back(k1 + 1);
				indices.push_back(k2 + 1);

				faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();

				/*
				// vertical lines for all stacks
				lineIndices.push_back(k1);
				lineIndices.push_back(k2);
				// horizontal lines
				lineIndices.push_back(k2);
				lineIndices.push_back(k2 + 1);
				if (i == 0)
				{
					lineIndices.push_back(k1);
					lineIndices.push_back(k1 + 1);
				}
				*/
			}
		}

		// remember where the base indices start
		//baseIndex = (unsigned int)indices.size();

		// put indices for base
		for (int i = 0, k = baseVertexIndex + 1; i < sectorCount; ++i, ++k)
		{
			if (i < (sectorCount - 1))
			{
				indices.push_back(baseVertexIndex);
				indices.push_back(k + 1);
				indices.push_back(k);

				faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();
			}
				
			else    // last triangle
			{
				indices.push_back(baseVertexIndex);
				indices.push_back(baseVertexIndex + 1);
				indices.push_back(k);

				faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();
			}
		}

		// remember where the base indices start
		//topIndex = (unsigned int)indices.size();

		for (int i = 0, k = topVertexIndex + 1; i < sectorCount; ++i, ++k)
		{
			if (i < (sectorCount - 1))
			{
				//addIndices(topVertexIndex, k, k + 1);

				indices.push_back(topVertexIndex);
				indices.push_back(k);
				indices.push_back(k + 1);

				faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();
			}
			else
			{
				indices.push_back(topVertexIndex);
				indices.push_back(k);
				indices.push_back(topVertexIndex + 1);

				faces.push_back(r2::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();
			}	
		}


		std::vector<flatbuffers::Offset<r2::MaterialID>> materials;
		materials.push_back(r2::CreateMaterialID(fbb, STRING_ID(materialName.c_str()))); //@temporary

		flatbuffers::Offset<r2::Mesh> mesh = r2::CreateMeshDirect(fbb, positions.size(), faces.size(),
			&positions, &normals, &texCoords, &faces, &materials);

		meshes.push_back(mesh);

		auto model = r2::CreateModel(fbb, STRING_ID(name), fbb.CreateVector(meshes));
		fbb.Finish(model);
		const std::string fileName = std::string("/") + name;

		byte* buf = fbb.GetBufferPointer();
		u32 size = fbb.GetSize();

		r2::asset::pln::flathelp::GenerateJSONAndBinary(
			buf, size,
			schemaPath, binaryParentDir + fileName + MODL_EXT,
			jsonParentDir + fileName + JSON_EXT);

	}
}
#endif