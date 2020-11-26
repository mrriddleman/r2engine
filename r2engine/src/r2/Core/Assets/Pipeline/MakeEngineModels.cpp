#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/MakeEngineModels.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Render/Model/Mesh_generated.h"
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
	const std::string MESH_SCHEMA_FBS = "Mesh.fbs";
	const std::string JSON_EXT = ".json";
	const std::string MODL_EXT = ".modl";
	const std::string MESH_EXT = ".mesh";

	void MakeQuad(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeCube(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeSphere(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeCone(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeCylinder(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeCylinderInternal(const char* name, const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir, const std::string& materialName, float baseRadius = 1.0f, float topRadius = 1.0f, float height = 1.0f,
		int sectorCount = 36, int stackCount = 1, bool smooth = true);
	
	void MakeQuadModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeCubeModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeSphereModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeConeModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeCylinderModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeSkyboxModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeModelInternal(const char* modelName, const char* meshName, const char* materialName, const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);


	std::map<std::string, MakeModlFunc> s_makeModelsMap
	{
		{"Quad.modl", MakeQuadModel},
		{"Cube.modl", MakeCubeModel},
		{"Sphere.modl", MakeSphereModel},
		{"Cone.modl", MakeConeModel},
		{"Cylinder.modl", MakeCylinderModel},
		{"Skybox.modl", MakeSkyboxModel}
	};

	std::map<std::string, MakeModlFunc> s_makeMeshesMap
	{
		{"QuadMesh.mesh", MakeQuad},
		{"CubeMesh.mesh", MakeCube},
		{"SphereMesh.mesh", MakeSphere},
		{"ConeMesh.mesh", MakeCone},
		{"CylinderMesh.mesh", MakeCylinder},
	};

	std::vector<MakeModlFunc> ShouldMakeEngineModels()
	{
		std::vector<MakeModlFunc> makeModlFuncs
		{
			MakeQuadModel,
			MakeCubeModel,
			MakeSphereModel,
			MakeConeModel,
			MakeCylinderModel,
			MakeSkyboxModel
		};

		for (const auto& modelFile : std::filesystem::directory_iterator(R2_ENGINE_INTERNAL_MODELS_BIN))
		{
			auto iter = s_makeModelsMap.find(modelFile.path().filename().string());
			if (iter != s_makeModelsMap.end())
			{
				makeModlFuncs.erase(std::remove(makeModlFuncs.begin(), makeModlFuncs.end(), iter->second));
			}
		}

		return makeModlFuncs;
	}

	std::vector<MakeModlFunc> ShouldMakeEngineMeshes()
	{
		std::vector<MakeModlFunc> makeMeshFuncs
		{
			MakeQuad,
			MakeCube,
			MakeSphere,
			MakeCone,
			MakeCylinder,
		};

		for (const auto& meshFile : std::filesystem::directory_iterator(R2_ENGINE_INTERNAL_MODELS_BIN))
		{
			auto iter = s_makeMeshesMap.find(meshFile.path().filename().string());
			if (iter != s_makeMeshesMap.end())
			{
				makeMeshFuncs.erase(std::remove(makeMeshFuncs.begin(), makeMeshFuncs.end(), iter->second));
			}
		}

		return makeMeshFuncs;
	}

	void MakeEngineModels(const std::vector<MakeModlFunc>& makeModels)
	{
		std::string schemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;
		char modelSchemaPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(schemaPath.c_str(), modelSchemaPath, MODEL_SCHEMA_FBS.c_str());

		for (auto makeModelFunc : makeModels)
		{
			makeModelFunc(modelSchemaPath, R2_ENGINE_INTERNAL_MODELS_BIN, R2_ENGINE_INTERNAL_MODELS_RAW + std::string("/"));
		}
	}

	void MakeEngineMeshes(const std::vector<MakeModlFunc>& makeMeshes)
	{
		std::string schemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;
		char meshSchemaPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::AppendSubPath(schemaPath.c_str(), meshSchemaPath, MESH_SCHEMA_FBS.c_str());

		for (auto makeMeshFunc : makeMeshes)
		{
			makeMeshFunc(meshSchemaPath, R2_ENGINE_INTERNAL_MODELS_BIN, R2_ENGINE_INTERNAL_MODELS_RAW + std::string("/"));
		}
	}

	void MakeQuad(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		//I guess what we're really making now is a .mesh file
		flatbuffers::FlatBufferBuilder fbb;

		std::vector<flat::Vertex3> positions;
		std::vector<flat::Vertex3> normals;
		std::vector<flat::Vertex2> texCoords;

		flat::Vertex3 p1 = flat::Vertex3(-0.5f, -0.5f, 0.0f);
		flat::Vertex3 p2 = flat::Vertex3(-0.5f, 0.5f, 0.0f);
		flat::Vertex3 p3 = flat::Vertex3(0.5f, -0.5f, 0.0f);
		flat::Vertex3 p4 = flat::Vertex3(0.5f, 0.5f, 0.0f);

		positions.push_back(p1);
		positions.push_back(p2);
		positions.push_back(p3);
		positions.push_back(p4);

		flat::Vertex3 n1 = flat::Vertex3(0.0f, 0.0f, 1.0f);
		flat::Vertex3 n2 = flat::Vertex3(0.0f, 0.0f, 1.0f);
		flat::Vertex3 n3 = flat::Vertex3(0.0f, 0.0f, 1.0f);
		flat::Vertex3 n4 = flat::Vertex3(0.0f, 0.0f, 1.0f);

		normals.push_back(n1);
		normals.push_back(n2);
		normals.push_back(n3);
		normals.push_back(n4);

		flat::Vertex2 t1 = flat::Vertex2(0.0f, 0.0f);
		flat::Vertex2 t2 = flat::Vertex2(0.0f, 1.0f);
		flat::Vertex2 t3 = flat::Vertex2(1.0f, 0.0f);
		flat::Vertex2 t4 = flat::Vertex2(1.0f, 1.0f);

		texCoords.push_back(t1);
		texCoords.push_back(t2);
		texCoords.push_back(t3);
		texCoords.push_back(t4);

		std::vector<flatbuffers::Offset<flat::Face>> faces;

		std::vector<uint32_t> indices;

		indices.push_back(0);
		indices.push_back(2);
		indices.push_back(1);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)) );

		indices.clear();
		
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(3);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		auto mesh = flat::CreateMeshDirect(fbb, STRING_ID("QuadMesh"), positions.size(), faces.size(), &positions, &normals, &texCoords, &faces);
		fbb.Finish(mesh);
		const std::string name = "QuadMesh";

		byte* buf = fbb.GetBufferPointer();
		u32 size = fbb.GetSize();

		r2::asset::pln::flathelp::GenerateJSONAndBinary(
			buf, size,
			schemaPath, binaryParentDir + name + MESH_EXT,
			jsonParentDir + name + JSON_EXT);
	}

	void MakeCube(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		flatbuffers::FlatBufferBuilder fbb;

		std::vector<flat::Vertex3> positions;
		std::vector<flat::Vertex3> normals;
		std::vector<flat::Vertex2> texCoords;

		//Front face
		positions.push_back(flat::Vertex3(1.0f, 1.0f, 1.0f));
		positions.push_back(flat::Vertex3(1.0f, -1.0f, 1.0f));
		positions.push_back(flat::Vertex3(-1.0f, -1.0f, 1.0f));
		positions.push_back(flat::Vertex3(-1.0f, 1.0f, 1.0f));

		normals.push_back(flat::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(flat::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(flat::Vertex3(0.0f, 0.0f, 1.0f));
		normals.push_back(flat::Vertex3(0.0f, 0.0f, 1.0f));

		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));

		//Right Face
		positions.push_back(flat::Vertex3(1.0f, 1.0f, 1.0f));
		positions.push_back(flat::Vertex3(1.0f, -1.0f, 1.0f));
		positions.push_back(flat::Vertex3(1.0f, -1.0f, -1.0f));
		positions.push_back(flat::Vertex3(1.0f, 1.0f, -1.0f));

		normals.push_back(flat::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(1.0f, 0.0f, 0.0f));

		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));

		//Left Face
		positions.push_back(flat::Vertex3(-1.0f, -1.0f, -1.0f));
		positions.push_back(flat::Vertex3(-1.0f, 1.0f, -1.0f));
		positions.push_back(flat::Vertex3(-1.0f, -1.0f, 1.0f));
		positions.push_back(flat::Vertex3(-1.0f, 1.0f, 1.0f));

		normals.push_back(flat::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(-1.0f, 0.0f, 0.0f));

		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));

		//Back Face
		positions.push_back(flat::Vertex3(1.0f, -1.0f, -1.0f));
		positions.push_back(flat::Vertex3(1.0f, 1.0f, -1.0f));
		positions.push_back(flat::Vertex3(-1.0f, -1.0f, -1.0f));
		positions.push_back(flat::Vertex3(-1.0f, 1.0f, -1.0f));

		normals.push_back(flat::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(flat::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(flat::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(flat::Vertex3(0.0f, 0.0f, -1.0f));

		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));

		//Top Face
		positions.push_back(flat::Vertex3(-1.0f, 1.0f, 1.0f));
		positions.push_back(flat::Vertex3(-1.0f, 1.0f, -1.0f));
		positions.push_back(flat::Vertex3(1.0f, 1.0f, 1.0f));
		positions.push_back(flat::Vertex3(1.0f, 1.0f, -1.0f));

		normals.push_back(flat::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0, 1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0, 1.0f, 0.0f));

		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));

		//Bottom Face
		positions.push_back(flat::Vertex3(-1.0f, -1.0f, -1.0f));
		positions.push_back(flat::Vertex3(-1.0f, -1.0f, 1.0f));
		positions.push_back(flat::Vertex3(1.0f, -1.0f, -1.0f));
		positions.push_back(flat::Vertex3(1.0f, -1.0f, 1.0f));

		normals.push_back(flat::Vertex3(0.0f, -1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0f, -1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0f, -1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0f, -1.0f, 0.0f));

		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));
		
		std::vector<flatbuffers::Offset<flat::Face>> faces;

		std::vector<uint32_t> indices;

		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(3);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(3);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(4);
		indices.push_back(6);
		indices.push_back(5);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(6);
		indices.push_back(4);
		indices.push_back(7);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(8);
		indices.push_back(9);
		indices.push_back(10);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(9);
		indices.push_back(11);
		indices.push_back(10);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(12);
		indices.push_back(13);
		indices.push_back(14);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(13);
		indices.push_back(15);
		indices.push_back(14);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(16);
		indices.push_back(17);
		indices.push_back(18);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(17);
		indices.push_back(19);
		indices.push_back(18);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(20);
		indices.push_back(21);
		indices.push_back(22);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(21);
		indices.push_back(23);
		indices.push_back(22);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		auto mesh= flat::CreateMeshDirect(fbb, STRING_ID("CubeMesh"), positions.size(), faces.size(),
			&positions, &normals, &texCoords, &faces);

		fbb.Finish(mesh);
		const std::string name = "CubeMesh";

		byte* buf = fbb.GetBufferPointer();
		u32 size = fbb.GetSize();

		r2::asset::pln::flathelp::GenerateJSONAndBinary(
			buf, size,
			schemaPath, binaryParentDir + name + MESH_EXT,
			jsonParentDir + name + JSON_EXT);
	}

	void MakeSphere(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		constexpr float PI =  glm::pi<float>();
		constexpr u32 segments = 32;
		flatbuffers::FlatBufferBuilder fbb;
	//	std::vector<flatbuffers::Offset<r2::Mesh>> meshes;

		std::vector<flat::Vertex3> positions;
		std::vector<flat::Vertex3> normals;
		std::vector<flat::Vertex2> texCoords;

		std::vector<flatbuffers::Offset<flat::Face>> faces;

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

				positions.push_back(flat::Vertex3(xPos, yPos, zPos));
				normals.push_back(flat::Vertex3(xPos, yPos, zPos));
				texCoords.push_back(flat::Vertex2(xSegment, ySegment));
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

					faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
					indices.clear();
				}

				// k1+1 => k2 => k2+1
				if (i != (segments - 1))
				{
					indices.push_back(k1 + 1);
					indices.push_back(k2);
					indices.push_back(k2 + 1);

					faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
					indices.clear();
				}
			}
		}

		auto mesh = flat::CreateMeshDirect(fbb, STRING_ID("SphereMesh"), positions.size(), faces.size(),
			&positions, &normals, &texCoords, &faces);

		fbb.Finish(mesh);
		const std::string name = "SphereMesh";

		byte* buf = fbb.GetBufferPointer();
		u32 size = fbb.GetSize();

		r2::asset::pln::flathelp::GenerateJSONAndBinary(
			buf, size,
			schemaPath, binaryParentDir + name + MESH_EXT,
			jsonParentDir + name + JSON_EXT);
	}

	void MakeCone(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeCylinderInternal("ConeMesh", schemaPath, binaryParentDir, jsonParentDir, "Bricks", 1.0f, 0.0f);

	}

	void MakeCylinder(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeCylinderInternal("CylinderMesh", schemaPath, binaryParentDir, jsonParentDir, "Mandelbrot");
	}

	void MakeCylinderInternal(const char* name, const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir, const std::string& materialName, float baseRadius, float topRadius, float height,
		int sectorCount, int stackCount, bool smooth)
	{
		//Copied from: https://www.songho.ca/opengl/gl_cylinder.html#example_pipe
		constexpr float PI = glm::pi<float>();
		flatbuffers::FlatBufferBuilder fbb;
		

		std::vector<flat::Vertex3> positions;
		std::vector<flat::Vertex3> normals;
		std::vector<flat::Vertex2> texCoords;

		std::vector<flatbuffers::Offset<flat::Face>> faces;

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

				positions.push_back(flat::Vertex3(x * radius, y * radius, z));
				normals.push_back(flat::Vertex3(fnormals[k], fnormals[k+1], fnormals[k+2]));
				texCoords.push_back(flat::Vertex2((float)j / sectorCount, t));
			}
		}

		// remember where the base.top vertices start
		unsigned int baseVertexIndex = (unsigned int)positions.size();

		// put vertices of base of cylinder
		z = -height * 0.5f;
		positions.push_back(flat::Vertex3(0.0f, 0.0f, z));
		normals.push_back(flat::Vertex3(0.0f, 0.0f, -1.0f));
		texCoords.push_back(flat::Vertex2( 0.5f, 0.5f));
		for (int i = 0, j = 0; i < sectorCount; ++i, j += 3)
		{
			x = unitCircleVertices[j];
			y = unitCircleVertices[j + 1];
			positions.push_back(flat::Vertex3(x * baseRadius, y * baseRadius, z));
			normals.push_back(flat::Vertex3(0.0f, 0.0f, -1.0f));
			texCoords.push_back(flat::Vertex2(-x * 0.5f + 0.5f, -y * 0.5f + 0.5f));
		}

		// remember where the base vertices start
		unsigned int topVertexIndex = (unsigned int)positions.size();

		// put vertices of top of cylinder
		z = height * 0.5f;
		positions.push_back(flat::Vertex3(0.0f, 0.0f, z));
		normals.push_back(flat::Vertex3(0.0f, 0.0f, 1));
		texCoords.push_back(flat::Vertex2(0.5f, 0.5f));
		for (int i = 0, j = 0; i < sectorCount; ++i, j += 3)
		{
			x = unitCircleVertices[j];
			y = unitCircleVertices[j + 1];
			positions.push_back(flat::Vertex3(x * topRadius, y * topRadius, z));
			normals.push_back(flat::Vertex3(0.0f, 0.0f, 1.0f));
			texCoords.push_back(flat::Vertex2(x * 0.5f + 0.5f, -y * 0.5f + 0.5f));
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
				
				faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();

				indices.push_back(k2);
				indices.push_back(k1 + 1);
				indices.push_back(k2 + 1);

				faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
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

				faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();
			}
				
			else    // last triangle
			{
				indices.push_back(baseVertexIndex);
				indices.push_back(baseVertexIndex + 1);
				indices.push_back(k);

				faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
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

				faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();
			}
			else
			{
				indices.push_back(topVertexIndex);
				indices.push_back(k);
				indices.push_back(topVertexIndex + 1);

				faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();
			}	
		}

		auto mesh = flat::CreateMeshDirect(fbb, STRING_ID(name), positions.size(), faces.size(),
			&positions, &normals, &texCoords, &faces);

		fbb.Finish(mesh);
		const std::string fileName = name;

		byte* buf = fbb.GetBufferPointer();
		u32 size = fbb.GetSize();

		r2::asset::pln::flathelp::GenerateJSONAndBinary(
			buf, size,
			schemaPath, binaryParentDir + fileName + MESH_EXT,
			jsonParentDir + fileName + JSON_EXT);

	}

	void MakeQuadModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("Quad", "QuadMesh", "Default", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeCubeModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("Cube", "CubeMesh", "Basic", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeSphereModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("Sphere", "SphereMesh", "Face", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeConeModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("Cone", "ConeMesh", "Bricks", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeCylinderModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("Cylinder", "CylinderMesh", "Mandelbrot", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeSkyboxModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("Skybox", "CubeMesh", "Skybox", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeModelInternal(const char* modelName, const char* meshName, const char* materialName, const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		flatbuffers::FlatBufferBuilder fbb;

		const std::string filename = std::string(modelName) ;
		const std::string filePath = std::string(meshName) + MESH_EXT;

		std::vector<flatbuffers::Offset<flatbuffers::String>> meshPaths = { fbb.CreateString(filePath) };

		std::vector<flatbuffers::Offset<flat::MaterialName>> materialNames = { flat::CreateMaterialName(fbb, STRING_ID(materialName)) };

		auto model = flat::CreateModelDirect(fbb, STRING_ID(filename.c_str()), &meshPaths, &materialNames);

		fbb.Finish(model);

		byte* buf = fbb.GetBufferPointer();
		u32 size = fbb.GetSize();

		r2::asset::pln::flathelp::GenerateJSONAndBinary(buf, size, schemaPath, binaryParentDir + filename + MODL_EXT, jsonParentDir + filename + JSON_EXT);
	}
}
#endif