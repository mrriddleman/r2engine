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
#include <unordered_map>
#include <algorithm>
#include <glm/gtc/constants.hpp>
#include "r2/Core/Math/MathUtils.h"

namespace
{
	struct Triangle
	{
		uint32_t index[3];
	};
	
	bool Vertex3IsEqual(const flat::Vertex3& v1, const flat::Vertex3& v2)
	{
		return r2::math::NearEq(v1.x(), v2.x()) && r2::math::NearEq(v1.y(), v2.y()) && r2::math::NearEq(v1.z(), v2.z());
	}

	flat::Vertex3 Normalize(const flat::Vertex3& v)
	{
		float len = std::sqrtf(v.x() * v.x() + v.y() * v.y() + v.z() * v.z());
		
		if (len == 0)
		{
			return flat::Vertex3(0, 0, 0);
		}

	//	R2_CHECK(!r2::math::NearZero(len), "Len cannot be zero!");

		return flat::Vertex3(v.x() / len, v.y() / len, v.z() / len);
	}

	flat::Vertex3 Subtract(const flat::Vertex3& v1, const flat::Vertex3& v2)
	{
		return flat::Vertex3(v1.x() - v2.x(), v1.y() - v2.y(), v1.z() - v2.z());
	}

	flat::Vertex2 Subtract(const flat::Vertex2& v1, const flat::Vertex2& v2)
	{
		return flat::Vertex2(v1.x() - v2.x(), v1.y() - v2.y());
	}


	flat::Vertex3 Add(const flat::Vertex3& v1, const flat::Vertex3& v2)
	{
		return flat::Vertex3(v1.x() + v2.x(), v1.y() + v2.y(), v1.z() + v2.z());
	}

	float DotProduct(const flat::Vertex3& v1, const flat::Vertex3& v2)
	{
		return v1.x() * v2.x() + v1.y() * v2.y() + v1.z() * v2.z();
	}

	flat::Vertex3 CrossProduct(const flat::Vertex3& v1, const flat::Vertex3& v2)
	{
		return flat::Vertex3(
			v1.y() * v2.z() - v1.z() * v2.y(),
			v1.z() * v2.x() - v1.x() * v2.z(),
			v1.x() * v2.y() - v1.y() * v2.x()
		);
	}

	flat::Vertex3 Multiply(const flat::Vertex3& v, float s)
	{
		return flat::Vertex3(v.x() * s, v.y() * s, v.z() * s);
	}

	flat::Vertex3 Divide(const flat::Vertex3& v, float divisor)
	{
		if (r2::math::NearZero(divisor))
		{
			R2_CHECK(false, "Divisor is 0!");
			return flat::Vertex3(0, 0, 0);
		}

		return flat::Vertex3(v.x() / divisor, v.y() / divisor, v.z() / divisor);
	}

	flat::Vertex3 Average(const std::vector<flat::Vertex3>& vertices)
	{
		flat::Vertex3 result(0, 0, 0);
		for (size_t i = 0; i < vertices.size(); ++i)
		{
			result = Add(result, vertices[i]);
		}
		if (vertices.size() > 0)
		{
			return Divide(result, vertices.size());
		}
		return result;
	}

	void CalculateTangent(const flat::Vertex3& edge1, const flat::Vertex3& edge2, const flat::Vertex2& deltaUV1, const flat::Vertex2& deltaUV2, flat::Vertex3& tangent)
    {
        float denom = deltaUV1.x() * deltaUV2.y() - deltaUV2.x() * deltaUV1.y();
        R2_CHECK(!r2::math::NearEq( denom, 0.0), "denominator is 0!");
        
        float f = 1.0f / denom;

		tangent = flat::Vertex3(
			f * (deltaUV2.y() * edge1.x() - deltaUV1.y() * edge2.x()),
			f * (deltaUV2.y() * edge1.y() - deltaUV1.y() * edge2.y()),
			f * (deltaUV2.y() * edge1.z() - deltaUV1.y() * edge2.z())
			
		);

		tangent = Normalize(tangent);
    }

	//From: https://www.cs.upc.edu/~virtual/G/1.%20Teoria/06.%20Textures/Tangent%20Space%20Calculation.pdf
	void CalculateTangentArray(
		const std::vector<flat::Vertex3>& vertices,
		const std::vector<flat::Vertex3>& normals,
		const std::vector<flat::Vertex2>& texCoords,
		const std::vector<Triangle>& triangles,
		std::vector<flat::Vertex3>& tangents)
	{
		tangents.clear();

		flat::Vertex3* tan1 = new flat::Vertex3[vertices.size() * 2];
		flat::Vertex3* tan2 = tan1 + vertices.size();
		memset(tan1, 0, sizeof(flat::Vertex3) * vertices.size() * 2);

		for (size_t i = 0; i < triangles.size(); i ++)
		{
			const Triangle& triangle = triangles[i];
			uint32_t i1 = triangle.index[0];
			uint32_t i2 = triangle.index[1];
			uint32_t i3 = triangle.index[2];

			const flat::Vertex3& v1 = vertices[i1];
			const flat::Vertex3& v2 = vertices[i2];
			const flat::Vertex3& v3 = vertices[i3];

			const flat::Vertex2& w1 = texCoords[i1];
			const flat::Vertex2& w2 = texCoords[i2];
			const flat::Vertex2& w3 = texCoords[i3];

			float x1 = v2.x() - v1.x();
			float x2 = v3.x() - v1.x();
			float y1 = v2.y() - v1.y();
			float y2 = v3.y() - v1.y();
			float z1 = v2.z() - v1.z();
			float z2 = v3.z() - v1.z();

			float s1 = w2.x() - w1.x();
			float s2 = w3.x() - w1.x();
			float t1 = w2.y() - w1.y();
			float t2 = w3.y() - w1.y();

			float r = 1.0f / (s1 * t2 - s2 * t1);
			flat::Vertex3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
			flat::Vertex3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

			tan1[i1] = Add(tan1[i1], sdir);
			tan1[i2] = Add(tan1[i2], sdir);
			tan1[i3] = Add(tan1[i3], sdir);

			tan2[i1] = Add(tan2[i1], tdir);
			tan2[i2] = Add(tan2[i2], tdir);
			tan2[i3] = Add(tan2[i3], tdir);
		}

		for (size_t a = 0; a < vertices.size(); ++a)
		{
			const flat::Vertex3& n = normals[a];
			const flat::Vertex3& t = tan1[a];

			//Gram-Schmidt orthogonalize
			tangents.push_back(Normalize(Subtract(t, Multiply(n, DotProduct(n, t)))));

			float handedness = (DotProduct(CrossProduct(n, t), tan2[a]) < 0.0) ? -1.0f : 1.0f;
			
			if (handedness)
			{
				int k = 0;
			}
		}

		delete[] tan1;
	}
}

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
	void MakeFullscreenTriangle(const std::string& schemaPath, const std::string& bindaryParentDir, const std::string& jsonParentDir);

	void MakeQuadModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeCubeModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeSphereModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeConeModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeCylinderModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeSkyboxModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeFullscreenTriangleModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);
	void MakeModelInternal(const char* modelName, const char* meshName, const char* materialName, const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir);


	std::map<std::string, MakeModlFunc> s_makeModelsMap
	{
		{"Quad.modl", MakeQuadModel},
		{"Cube.modl", MakeCubeModel},
		{"Sphere.modl", MakeSphereModel},
		{"Cone.modl", MakeConeModel},
		{"Cylinder.modl", MakeCylinderModel},
		{"FullscreenTriangle.modl", MakeFullscreenTriangleModel},
		{"Skybox.modl", MakeSkyboxModel},
		
		
	};

	std::map<std::string, MakeModlFunc> s_makeMeshesMap
	{
		{"QuadMesh.mesh", MakeQuad},
		{"CubeMesh.mesh", MakeCube},
		{"SphereMesh.mesh", MakeSphere},
		{"ConeMesh.mesh", MakeCone},
		{"CylinderMesh.mesh", MakeCylinder},
		{"FullScreenTriangle.mesh", MakeFullscreenTriangle}
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
			MakeFullscreenTriangleModel,
			MakeSkyboxModel,
			
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
			MakeCylinder,
			MakeCone,
			MakeFullscreenTriangle
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
		std::vector<flat::Vertex3> tangents;


		flat::Vertex3 p1 = flat::Vertex3(-0.5f, 0.5f, 0.0f);
		flat::Vertex3 p2 = flat::Vertex3(-0.5f, -0.5f, 0.0f);
		flat::Vertex3 p3 = flat::Vertex3(0.5f, -0.5f, 0.0f);
		flat::Vertex3 p4 = flat::Vertex3(0.5f, 0.5f, 0.0f);

		positions.push_back(p1);
		positions.push_back(p2);
		positions.push_back(p3);
		positions.push_back(p4);

		flat::Vertex3 n1 = flat::Vertex3(0.0f, 0.0f, 1.0f);
	//	flat::Vertex3 n2 = flat::Vertex3(0.0f, 0.0f, 1.0f);
	//	flat::Vertex3 n3 = flat::Vertex3(0.0f, 0.0f, 1.0f);
	//	flat::Vertex3 n4 = flat::Vertex3(0.0f, 0.0f, 1.0f);

		normals.push_back(n1);
		normals.push_back(n1);
		normals.push_back(n1);
		normals.push_back(n1);

		flat::Vertex2 t1 = flat::Vertex2(0.0f, 1.0f);
		flat::Vertex2 t2 = flat::Vertex2(0.0f, 0.0f);
		flat::Vertex2 t3 = flat::Vertex2(1.0f, 0.0f);
		flat::Vertex2 t4 = flat::Vertex2(1.0f, 1.0f);

		texCoords.push_back(t1);
		texCoords.push_back(t2);
		texCoords.push_back(t3);
		texCoords.push_back(t4);


		//flat::Vertex3 tangent1;
		//flat::Vertex3 tangent2;


		//

		//flat::Vertex3 tri1Edge1 = Subtract(positions[1], positions[0]);
		//flat::Vertex3 tri1Edge2 = Subtract(positions[2], positions[0]);
		//flat::Vertex2 deltaUV1 = Subtract(t2, t1);
		//flat::Vertex2 deltaUV2 = Subtract(t3, t1);

		//CalculateTangent(tri1Edge1, tri1Edge2, deltaUV1, deltaUV2, tangent1);

		//flat::Vertex3 tri2Edge1 = Subtract(positions[2], positions[0]);
		//flat::Vertex3 tri2Edge2 = Subtract(positions[3], positions[0]);
		//deltaUV1 = Subtract(t3, t1);
		//deltaUV2 = Subtract(t4, t1);

		//CalculateTangent(tri2Edge1, tri2Edge2, deltaUV1, deltaUV2, tangent2);


		////tangent1 = Multiply(tangent1, -1.0f);
		////tangent2 = Multiply(tangent2, -1.0f);

		//std::vector<flat::Vertex3> temp{ tangent1, tangent2 };

		//flat::Vertex3 averagedTangent = Average(temp);

		//


		//tangents.push_back(tangent1);
		//tangents.push_back(averagedTangent);
		//tangents.push_back(averagedTangent);
		//tangents.push_back(tangent2);


		std::vector<flatbuffers::Offset<flat::Face>> faces;

		std::vector<uint32_t> indices;

		indices.push_back(2);
		indices.push_back(1);
		indices.push_back(0);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)) );

		indices.clear();
		
		indices.push_back(0);
		indices.push_back(3);
		indices.push_back(2);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		std::vector<Triangle> triangles;


		Triangle tri1;
		tri1.index[0] = 2;
		tri1.index[1] = 1;
		tri1.index[2] = 0;

		Triangle tri2;
		tri2.index[0] = 0;
		tri2.index[1] = 3;
		tri2.index[2] = 2;

		triangles.push_back(tri1);
		triangles.push_back(tri2);

		CalculateTangentArray(positions, normals, texCoords, triangles, tangents);


		auto mesh = flat::CreateMeshDirect(fbb, STRING_ID("QuadMesh"), positions.size(), faces.size(), &positions, &normals, &tangents, &texCoords, &faces);
		fbb.Finish(mesh);
		const std::string name = "QuadMesh";

		byte* buf = fbb.GetBufferPointer();
		u32 size = fbb.GetSize();

		r2::asset::pln::flathelp::GenerateJSONAndBinary(
			buf, size,
			schemaPath, binaryParentDir + name + MESH_EXT,
			jsonParentDir + name + JSON_EXT);
	}


	void MakeFullscreenTriangle(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		//I guess what we're really making now is a .mesh file
		flatbuffers::FlatBufferBuilder fbb;

		std::vector<flat::Vertex3> positions;
		std::vector<flat::Vertex3> normals;
		std::vector<flat::Vertex2> texCoords;
		std::vector<flat::Vertex3> tangents;


		flat::Vertex3 p1 = flat::Vertex3(-1.0f, -1.0f, 0.0f);
		flat::Vertex3 p2 = flat::Vertex3(3.0f, -1.0f, 0.0f);
		flat::Vertex3 p3 = flat::Vertex3(-1.0f, 3.0f, 0.0f);
		

		positions.push_back(p1);
		positions.push_back(p2);
		positions.push_back(p3);


		flat::Vertex3 n1 = flat::Vertex3(0.0f, 0.0f, 1.0f);
		//	flat::Vertex3 n2 = flat::Vertex3(0.0f, 0.0f, 1.0f);
		//	flat::Vertex3 n3 = flat::Vertex3(0.0f, 0.0f, 1.0f);
		//	flat::Vertex3 n4 = flat::Vertex3(0.0f, 0.0f, 1.0f);

		normals.push_back(n1);
		normals.push_back(n1);
		normals.push_back(n1);


		flat::Vertex2 t1 = flat::Vertex2(0.0f, 0.0f);
		flat::Vertex2 t2 = flat::Vertex2(2.0f, 0.0f);
		flat::Vertex2 t3 = flat::Vertex2(0.0f, 2.0f);


		texCoords.push_back(t1);
		texCoords.push_back(t2);
		texCoords.push_back(t3);


		std::vector<flatbuffers::Offset<flat::Face>> faces;

		std::vector<uint32_t> indices;

		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		

		std::vector<Triangle> triangles;


		Triangle tri1;
		tri1.index[0] = 0;
		tri1.index[1] = 1;
		tri1.index[2] = 2;



		triangles.push_back(tri1);


		CalculateTangentArray(positions, normals, texCoords, triangles, tangents);


		auto mesh = flat::CreateMeshDirect(fbb, STRING_ID("FullscreenTriangleMesh"), positions.size(), faces.size(), &positions, &normals, &tangents, &texCoords, &faces);
		fbb.Finish(mesh);
		const std::string name = "FullscreenTriangleMesh";

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
		std::vector<flat::Vertex3> tangents;

		//Front face
		constexpr float UNIT = 1.0f;
		constexpr float HUNIT = 0.5f;
		positions.push_back(flat::Vertex3(-UNIT, -UNIT, -UNIT));
		positions.push_back(flat::Vertex3(-UNIT, -UNIT, UNIT));
		positions.push_back(flat::Vertex3(UNIT, -UNIT, -UNIT));
		positions.push_back(flat::Vertex3(UNIT, -UNIT, UNIT));


	//	positions.push_back(flat::Vertex3(-1.0f, -1.0f, 1.0f));
	//	positions.push_back(flat::Vertex3(-1.0f, 1.0f, 1.0f));
	//	positions.push_back(flat::Vertex3(1.0f, -1.0f, 1.0f));
	//	positions.push_back(flat::Vertex3(1.0f, 1.0f, 1.0f));
		
		
		

		normals.push_back(flat::Vertex3(0.0f, -1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0f, -1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0f, -1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0f, -1.0f, 0.0f));


		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));
		
		flat::Vertex3 tangent1;
		flat::Vertex3 tangent2;

		flat::Vertex3 tri1Edge1;
		flat::Vertex3 tri1Edge2;

		flat::Vertex2 deltaUV1;
		flat::Vertex2 deltaUV2;

		flat::Vertex3 tri2Edge1;
		flat::Vertex3 tri2Edge2;

		tri1Edge1 = Subtract(positions[0], positions[1]);
		tri1Edge2 = Subtract(positions[2], positions[1]);

		deltaUV1 = Subtract(texCoords[0], texCoords[1]);
		deltaUV2 = Subtract(texCoords[2], texCoords[1]);

		CalculateTangent(tri1Edge1, tri1Edge2, deltaUV1, deltaUV2, tangent1);

		tri2Edge1 = Subtract(positions[2], positions[1]);
		tri2Edge2 = Subtract(positions[3], positions[1]);

		deltaUV1 = Subtract(texCoords[2], texCoords[1]);
		deltaUV2 = Subtract(texCoords[3], texCoords[1]);

		CalculateTangent(tri2Edge1, tri2Edge2, deltaUV1, deltaUV2, tangent2);

		std::vector<flat::Vertex3> temp{ tangent1, tangent2 };

		flat::Vertex3 averagedTangent = Average(temp);


		tangents.push_back(tangent1);
		tangents.push_back(averagedTangent);
		tangents.push_back(averagedTangent);
		tangents.push_back(tangent2);

		//Right Face
	//	positions.push_back(flat::Vertex3(1.0f, 1.0f, 1.0f));
	//	positions.push_back(flat::Vertex3(1.0f, -1.0f, 1.0f));
	//	positions.push_back(flat::Vertex3(1.0f, -1.0f, -1.0f));
	//	positions.push_back(flat::Vertex3(1.0f, 1.0f, -1.0f));

		positions.push_back(flat::Vertex3(UNIT, -UNIT, UNIT));
		positions.push_back(flat::Vertex3(UNIT, -UNIT, -UNIT));
		positions.push_back(flat::Vertex3(UNIT, UNIT, -UNIT));
		positions.push_back(flat::Vertex3(UNIT, UNIT, UNIT));

		normals.push_back(flat::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(1.0f, 0.0f, 0.0f));

		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));

		//tangent stuff
		{
			tri1Edge1 = Subtract(positions[5], positions[4]);
			tri1Edge2 = Subtract(positions[6], positions[4]);

			deltaUV1 = Subtract(texCoords[5], texCoords[4]);
			deltaUV2 = Subtract(texCoords[6], texCoords[4]);

			CalculateTangent(tri1Edge1, tri1Edge2, deltaUV1, deltaUV2, tangent1);

			tri2Edge1 = Subtract(positions[5], positions[4]);
			tri2Edge2 = Subtract(positions[7], positions[4]);

			deltaUV1 = Subtract(texCoords[5], texCoords[4]);
			deltaUV2 = Subtract(texCoords[7], texCoords[4]);

			CalculateTangent(tri2Edge1, tri2Edge2, deltaUV1, deltaUV2, tangent2);

			temp.clear();
			temp.push_back(tangent1);
			temp.push_back(tangent2);

			averagedTangent = Average(temp);

			tangents.push_back(averagedTangent);
			tangents.push_back(tangent1);
			tangents.push_back(averagedTangent);
			tangents.push_back(tangent2);
		}
		


		//Left Face
	//	positions.push_back(flat::Vertex3(-1.0f, -1.0f, -1.0f));
	//	positions.push_back(flat::Vertex3(-1.0f, 1.0f, -1.0f));
	//	positions.push_back(flat::Vertex3(-1.0f, -1.0f, 1.0f));
	//	positions.push_back(flat::Vertex3(-1.0f, 1.0f, 1.0f));

		positions.push_back(flat::Vertex3(-UNIT, UNIT, -UNIT));
		positions.push_back(flat::Vertex3(-UNIT, UNIT, UNIT));
		positions.push_back(flat::Vertex3(-UNIT, -UNIT, -UNIT));
		positions.push_back(flat::Vertex3(-UNIT, -UNIT, UNIT));

		normals.push_back(flat::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(-1.0f, 0.0f, 0.0f));
		normals.push_back(flat::Vertex3(-1.0f, 0.0f, 0.0f));

		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));

		{
			tri1Edge1 = Subtract(positions[8], positions[9]);
			tri1Edge2 = Subtract(positions[10], positions[9]);

			deltaUV1 = Subtract(texCoords[8], texCoords[9]);
			deltaUV2 = Subtract(texCoords[10], texCoords[9]);

			CalculateTangent(tri1Edge1, tri1Edge2, deltaUV1, deltaUV2, tangent1);

			tri2Edge1 = Subtract(positions[10], positions[9]);
			tri2Edge2 = Subtract(positions[11], positions[9]);

			deltaUV1 = Subtract(texCoords[10], texCoords[9]);
			deltaUV2 = Subtract(texCoords[11], texCoords[9]);

			CalculateTangent(tri2Edge1, tri2Edge2, deltaUV1, deltaUV2, tangent2);

			temp.clear();
			temp.push_back(tangent1);
			temp.push_back(tangent2);

			averagedTangent = Average(temp);

			tangents.push_back(tangent1);
			tangents.push_back(averagedTangent);

			tangents.push_back(averagedTangent);
			tangents.push_back(tangent2);
		}
		

		//Back Face
	//	positions.push_back(flat::Vertex3(1.0f, -1.0f, -1.0f));
	//	positions.push_back(flat::Vertex3(1.0f, 1.0f, -1.0f));
	//	positions.push_back(flat::Vertex3(-1.0f, -1.0f, -1.0f));
	//	positions.push_back(flat::Vertex3(-1.0f, 1.0f, -1.0f));

		positions.push_back(flat::Vertex3(UNIT, UNIT, -UNIT));
		positions.push_back(flat::Vertex3(UNIT, UNIT, UNIT));
		positions.push_back(flat::Vertex3(-UNIT, UNIT, -UNIT));
		positions.push_back(flat::Vertex3(-UNIT, UNIT, UNIT));


		normals.push_back(flat::Vertex3(0.0f, 1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0f, 1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0f, 1.0f, 0.0f));
		normals.push_back(flat::Vertex3(0.0f, 1.0f, 0.0f));

		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));


		//tangent stuff
		{
			tri1Edge1 = Subtract(positions[12], positions[13]);
			tri1Edge2 = Subtract(positions[14], positions[13]);

			deltaUV1 = Subtract(texCoords[12], texCoords[13]);
			deltaUV2 = Subtract(texCoords[14], texCoords[13]);

			CalculateTangent(tri1Edge1, tri1Edge2, deltaUV1, deltaUV2, tangent1);

			tri2Edge1 = Subtract(positions[14], positions[13]);
			tri2Edge2 = Subtract(positions[15], positions[13]);

			deltaUV1 = Subtract(texCoords[14], texCoords[13]);
			deltaUV2 = Subtract(texCoords[15], texCoords[13]);

			CalculateTangent(tri2Edge1, tri2Edge2, deltaUV1, deltaUV2, tangent2);

			temp.clear();
			temp.push_back(tangent1);
			temp.push_back(tangent2);

			averagedTangent = Average(temp);

			

			tangents.push_back(tangent1);
			tangents.push_back(averagedTangent);

			tangents.push_back(averagedTangent);
			tangents.push_back(tangent2);

		
		}

		//Top Face
		//positions.push_back(flat::Vertex3(-1.0f, 1.0f, 1.0f));
		//positions.push_back(flat::Vertex3(-1.0f, 1.0f, -1.0f));
		//positions.push_back(flat::Vertex3(1.0f, 1.0f, 1.0f));
		//positions.push_back(flat::Vertex3(1.0f, 1.0f, -1.0f));

		positions.push_back(flat::Vertex3(-UNIT, -UNIT, UNIT));
		positions.push_back(flat::Vertex3(-UNIT, UNIT, UNIT));
		positions.push_back(flat::Vertex3(UNIT, -UNIT, UNIT));
		positions.push_back(flat::Vertex3(UNIT, UNIT, UNIT));

		normals.push_back(flat::Vertex3(0.0, 0.0f, 1.0f));
		normals.push_back(flat::Vertex3(0.0, 0.0f, 1.0f));
		normals.push_back(flat::Vertex3(0.0, 0.0f, 1.0f));
		normals.push_back(flat::Vertex3(0.0, 0.0f, 1.0f));

		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));


		//tangent stuff
		{
			tri1Edge1 = Subtract(positions[16], positions[17]);
			tri1Edge2 = Subtract(positions[18], positions[17]);

			deltaUV1 = Subtract(texCoords[16], texCoords[17]);
			deltaUV2 = Subtract(texCoords[18], texCoords[17]);

			CalculateTangent(tri1Edge1, tri1Edge2, deltaUV1, deltaUV2, tangent1);

			tri2Edge1 = Subtract(positions[18], positions[17]);
			tri2Edge2 = Subtract(positions[19], positions[17]);

			deltaUV1 = Subtract(texCoords[18], texCoords[17]);
			deltaUV2 = Subtract(texCoords[19], texCoords[17]);

			CalculateTangent(tri2Edge1, tri2Edge2, deltaUV1, deltaUV2, tangent2);

			temp.clear();
			temp.push_back(tangent1);
			temp.push_back(tangent2);

			averagedTangent = Average(temp);

			tangents.push_back(tangent1);
			tangents.push_back(averagedTangent);

			tangents.push_back(averagedTangent);
			tangents.push_back(tangent2);
		}

		//Bottom Face
		//positions.push_back(flat::Vertex3(-1.0f, -1.0f, -1.0f));
		//positions.push_back(flat::Vertex3(-1.0f, -1.0f, 1.0f));
		//positions.push_back(flat::Vertex3(1.0f, -1.0f, -1.0f));
		//positions.push_back(flat::Vertex3(1.0f, -1.0f, 1.0f));

		positions.push_back(flat::Vertex3(-UNIT, UNIT, -UNIT));
		positions.push_back(flat::Vertex3(-UNIT, -UNIT, -UNIT));
		positions.push_back(flat::Vertex3(UNIT, UNIT, -UNIT));
		positions.push_back(flat::Vertex3(UNIT, -UNIT, -UNIT));

		normals.push_back(flat::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(flat::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(flat::Vertex3(0.0f, 0.0f, -1.0f));
		normals.push_back(flat::Vertex3(0.0f, 0.0f, -1.0f));

		texCoords.push_back(flat::Vertex2(0.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(0.0f, 1.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 0.0f));
		texCoords.push_back(flat::Vertex2(1.0f, 1.0f));

		//tangent stuff
		{
			tri1Edge1 = Subtract(positions[20], positions[21]);
			tri1Edge2 = Subtract(positions[22], positions[21]);

			deltaUV1 = Subtract(texCoords[20], texCoords[21]);
			deltaUV2 = Subtract(texCoords[22], texCoords[21]);

			CalculateTangent(tri1Edge1, tri1Edge2, deltaUV1, deltaUV2, tangent1);

			tri2Edge1 = Subtract(positions[22], positions[21]);
			tri2Edge2 = Subtract(positions[23], positions[21]);

			deltaUV1 = Subtract(texCoords[22], texCoords[21]);
			deltaUV2 = Subtract(texCoords[23], texCoords[21]);

			CalculateTangent(tri2Edge1, tri2Edge2, deltaUV1, deltaUV2, tangent2);

			temp.clear();
			temp.push_back(tangent1);
			temp.push_back(tangent2);

			averagedTangent = Average(temp);

			tangents.push_back(tangent1);
			tangents.push_back(averagedTangent);

			tangents.push_back(averagedTangent);
			tangents.push_back(tangent2);
		}
		
		std::vector<flatbuffers::Offset<flat::Face>> faces;

		std::vector<uint32_t> indices;

		indices.push_back(0);
		indices.push_back(2);
		indices.push_back(1);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(3);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(4);
		indices.push_back(5);
		indices.push_back(6);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(6);
		indices.push_back(7);
		indices.push_back(4);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(8);
		indices.push_back(10);
		indices.push_back(9);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(9);
		indices.push_back(10);
		indices.push_back(11);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(12);
		indices.push_back(14);
		indices.push_back(13);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(13);
		indices.push_back(14);
		indices.push_back(15);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(16);
		indices.push_back(18);
		indices.push_back(17);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(17);
		indices.push_back(18);
		indices.push_back(19);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(20);
		indices.push_back(22);
		indices.push_back(21);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		indices.push_back(21);
		indices.push_back(22);
		indices.push_back(23);

		faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));

		indices.clear();

		auto mesh= flat::CreateMeshDirect(fbb, STRING_ID("CubeMesh"), positions.size(), faces.size(),
			&positions, &normals, &tangents, &texCoords, &faces);

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
		constexpr u32 segments = 24;
		flatbuffers::FlatBufferBuilder fbb;

		std::vector<flat::Vertex3> positions;
		std::vector<flat::Vertex3> normals;
		std::vector<flat::Vertex2> texCoords;
		std::vector<flat::Vertex3> tangents;

		std::vector<Triangle> triangles;

		std::vector<flatbuffers::Offset<flat::Face>> faces;

		std::vector<uint32_t> indices;

		for (u32 y = 0; y <= segments; ++y)
		{
			for (u32 x = 0; x <= segments; ++x)
			{
				float xSegment = (float)x / (float)segments;
				float ySegment = (float)y / (float)segments;
				float xPos = cos(xSegment * 2.0f * PI) * sin(ySegment * PI);
				float zPos = cos(ySegment * PI);
				float yPos = sin(xSegment * 2.0f * PI) * sin(ySegment * PI);

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
					indices.push_back(k1 + 1);
					indices.push_back(k2);

					Triangle tri;
					tri.index[0] = k1;
					tri.index[1] = k1 + 1;
					tri.index[2] = k2;

					triangles.push_back(tri);

					faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
					indices.clear();
				}

				// k1+1 => k2 => k2+1
				if (i != (segments - 1))
				{
					indices.push_back(k1 + 1);
					indices.push_back(k2 + 1);
					indices.push_back(k2);

					Triangle tri;
					tri.index[0] = k1 + 1;
					tri.index[1] = k2 + 1;
					tri.index[2] = k2;

					triangles.push_back(tri);

					faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
					indices.clear();
				}
			}
		}
		
		CalculateTangentArray(positions, normals, texCoords, triangles, tangents);

		auto mesh = flat::CreateMeshDirect(fbb, STRING_ID("SphereMesh"), positions.size(), faces.size(),
			&positions, &normals, &tangents, &texCoords, &faces);

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
		MakeCylinderInternal("CylinderMesh", schemaPath, binaryParentDir, jsonParentDir, "Mandelbrot", 1.0f, 1.0f, 1.0f, 36, 8);
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
		std::vector<flat::Vertex3> tangents;

		std::vector<flatbuffers::Offset<flat::Face>> faces;

		std::vector<uint32_t> indices;

		std::vector<Triangle> triangles;
		
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
			texCoords.push_back(flat::Vertex2(-x * 0.5f + 0.5f, y * 0.5f + 0.5f));
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
			texCoords.push_back(flat::Vertex2(x * 0.5f + 0.5f, y * 0.5f + 0.5f));
		}


		// put indices for sides
		unsigned int k1, k2;
		for (int i = 0; i < stackCount; ++i)
		{
			k1 = i * (sectorCount + 1);     // beginning of current stack
			k2 = k1 + sectorCount + 1;      // beginning of next stack

			for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
			{
				// 2 triangles per sector
				indices.push_back(k1);
				indices.push_back(k1 + 1);
				indices.push_back(k2);

				faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();

				Triangle tri1;
				tri1.index[0] = k1;
				tri1.index[1] = k1 + 1;
				tri1.index[2] = k2;

				triangles.push_back(tri1);



				indices.push_back(k2);
				indices.push_back(k1 + 1);
				indices.push_back(k2 + 1);

				faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();

				tri1.index[0] = k2;
				tri1.index[1] = k1 + 1;
				tri1.index[2] = k2 + 1;

				triangles.push_back(tri1);

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

				Triangle tri1;
				tri1.index[0] = baseVertexIndex;
				tri1.index[1] = k1 + 1;
				tri1.index[2] = k;

				triangles.push_back(tri1);
			}
				
			else    // last triangle
			{
				indices.push_back(baseVertexIndex);
				indices.push_back(baseVertexIndex + 1);
				indices.push_back(k);

				faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();

				Triangle tri1;
				tri1.index[0] = baseVertexIndex;
				tri1.index[1] = baseVertexIndex + 1;
				tri1.index[2] = k;

				triangles.push_back(tri1);
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

				Triangle tri1;
				tri1.index[0] = topVertexIndex;
				tri1.index[1] = k;
				tri1.index[2] = k + 1;

				triangles.push_back(tri1);
			}
			else
			{
				indices.push_back(topVertexIndex);
				indices.push_back(k);
				indices.push_back(topVertexIndex + 1);

				faces.push_back(flat::CreateFace(fbb, 3, fbb.CreateVector(indices)));
				indices.clear();

				Triangle tri1;
				tri1.index[0] = topVertexIndex;
				tri1.index[1] = k;
				tri1.index[2] = topVertexIndex + 1;

				triangles.push_back(tri1);
			}	
		}

		CalculateTangentArray(positions, normals, texCoords, triangles, tangents);

		auto mesh = flat::CreateMeshDirect(fbb, STRING_ID(name), positions.size(), faces.size(),
			&positions, &normals, &tangents, &texCoords, &faces);

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
		MakeModelInternal("Quad", "QuadMesh", "StaticDefault", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeCubeModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("Cube", "CubeMesh", "StaticDefault", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeSphereModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("Sphere", "SphereMesh", "StaticDefault", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeConeModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("Cone", "ConeMesh", "StaticDefault", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeCylinderModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("Cylinder", "CylinderMesh", "StaticDefault", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeSkyboxModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("Skybox", "CubeMesh", "Skybox", schemaPath, binaryParentDir, jsonParentDir);
	}

	void MakeFullscreenTriangleModel(const std::string& schemaPath, const std::string& binaryParentDir, const std::string& jsonParentDir)
	{
		MakeModelInternal("FullscreenTriangle", "FullscreenTriangleMesh", "StaticDefault", schemaPath, binaryParentDir, jsonParentDir);
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