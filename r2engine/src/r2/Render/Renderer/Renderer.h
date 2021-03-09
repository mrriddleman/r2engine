
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/Shader.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Renderer/Commands.h"

namespace r2
{
	struct Camera;
}

namespace r2::draw
{
	class BufferLayout;
	struct LightSystem;
	struct BufferLayoutConfiguration;

	struct BufferHandles
	{
		r2::SArray<BufferLayoutHandle>* bufferLayoutHandles = nullptr;
		r2::SArray<VertexBufferHandle>* vertexBufferHandles = nullptr;
		r2::SArray<IndexBufferHandle>* indexBufferHandles = nullptr;
		r2::SArray<DrawIDHandle>* drawIDHandles = nullptr;
	};

	//template <typename T = r2::draw::key::Basic>
	//struct CommandBucket;

	enum DefaultModel
	{
		QUAD = 0,
		CUBE,
		SPHERE,
		CONE,
		CYLINDER,
		SKYBOX,
		NUM_DEFAULT_MODELS
	};

	enum DebugModelType : u32
	{
		DEBUG_QUAD =0,
		DEBUG_CUBE,
		DEBUG_SPHERE,
		DEBUG_CONE,
		DEBUG_CYLINDER,

		DEBUG_ARROW,
		DEBUG_LINE,

		NUM_DEBUG_MODELS
	};

	struct Model;
}

namespace r2::draw::cmd
{
	struct Clear;
	struct DrawIndexed;
	struct FillIndexBuffer;
	struct FillVertexBuffer;
	struct FillConstantBuffer;
	struct DrawBatchSubCommand;
	struct DrawDebugBatchSubCommand;
}

namespace r2::draw
{

	struct MaterialBatch
	{
		struct Info
		{
			s32 start = -1;
			s32 numMaterials = 0;
		};

		r2::SArray<Info>* infos = nullptr;
		r2::SArray<MaterialHandle>* materialHandles = nullptr;
	};

	struct BatchConfig
	{
		r2::draw::key::Basic key;
		VertexConfigHandle vertexLayoutConfigHandle = InvalidVertexConfigHandle;

		//@TODO(Serge): make these into ConstantConfigHandles and look up the ConstantBufferHandle in the 
		ConstantBufferHandle subCommandsHandle;
		ConstantBufferHandle modelsHandle;
		ConstantBufferHandle materialsHandle;
		ConstantBufferHandle boneTransformOffsetsHandle;
		ConstantBufferHandle boneTransformsHandle;
		b32 clear = false;
		b32 clearDepth = true;
		u32 numDraws = 0;
		cmd::PrimitiveType primitiveType = cmd::PrimitiveType::TRIANGLES;
		b32 depthTest = true;

		r2::SArray<glm::mat4>* models = nullptr;

		
		r2::SArray<r2::draw::cmd::DrawBatchSubCommand>* subcommands = nullptr;
		MaterialBatch materials; //@NOTE: this assumes that the materials have already been uploaded

		//The boneTransforms can be of any size
		r2::SArray<r2::draw::ShaderBoneTransform>* boneTransforms = nullptr;
		r2::SArray<glm::ivec4>* boneTransformOffsets = nullptr;


		static u64 MemorySize(u64 numModels, u64 numSubcommands, u64 numInfos, u64 numMaterials, u64 numBoneTransforms, u64 numBoneOffsets, u64 alignment, u32 headerSize, u32 boundsChecking);

	};


}

namespace r2::draw::renderer
{
	//basic stuff
	bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle, const char* shaderManifestPath, const char* internalShaderManifestPath);
	void Update();
	void Render(float alpha);
	void Shutdown();
	u64 MemorySize(u64 materialSystemMemorySize);

	//Setup code
	void SetClearColor(const glm::vec4& color);
	bool GenerateLayouts();

#ifdef R2_DEBUG
	bool GenerateLayoutsWithDebug();
#endif
	
	VertexConfigHandle AddStaticModelLayout(const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize);
	VertexConfigHandle AddAnimatedModelLayout(const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize);

	ConstantConfigHandle AddConstantBufferLayout(ConstantBufferLayout::Type type, const std::initializer_list<ConstantBufferElement>& elements);
	ConstantConfigHandle AddModelsLayout(ConstantBufferLayout::Type type);
	ConstantConfigHandle AddMaterialLayout();
	ConstantConfigHandle AddSubCommandsLayout(); //we can use this for the debug 
	ConstantConfigHandle AddBoneTransformsLayout();
	ConstantConfigHandle AddLightingLayout();

	//Regular methods
	BufferHandles& GetVertexBufferHandles();
	const r2::SArray<r2::draw::ConstantBufferHandle>* GetConstantBufferHandles();

	const Model* GetDefaultModel(r2::draw::DefaultModel defaultModel);
	const r2::SArray<r2::draw::ModelRef>* GetDefaultModelRefs();
	r2::draw::ModelRef GetDefaultModelRef(r2::draw::DefaultModel defaultModel);

	void GetDefaultModelMaterials(r2::SArray<r2::draw::MaterialHandle>& defaultModelMaterials);
	r2::draw::MaterialHandle GetMaterialHandleForDefaultModel(r2::draw::DefaultModel defaultModel);

	void GetMaterialsAndBoneOffsetsForAnimModels(const r2::SArray<const r2::draw::AnimModel*>& models, MaterialBatch& materialBatch, r2::SArray<glm::ivec4>& boneOffsets);

	void UploadEngineModels(VertexConfigHandle vertexLayoutConfig);

	void LoadEngineTexturesFromDisk();
	void UploadEngineMaterialTexturesToGPUFromMaterialName(u64 materialName);
	void UploadEngineMaterialTexturesToGPU();

	ModelRef UploadModel(const Model* model, VertexConfigHandle vHandle);
	void UploadModels(const r2::SArray<const Model*>& models, VertexConfigHandle vHandle, r2::SArray<ModelRef>& modelRefs);
	ModelRef UploadAnimModel(const AnimModel* model, VertexConfigHandle vHandle);
	void UploadAnimModels(const r2::SArray<const AnimModel*>& models, VertexConfigHandle vHandle, r2::SArray<ModelRef>& modelRefs);

	void ClearVertexLayoutOffsets(VertexConfigHandle vHandle);
	void ClearAllVertexLayoutOffsets();

	void FillSubCommandsFromModelRefs(r2::SArray<r2::draw::cmd::DrawBatchSubCommand>& subCommands, const r2::SArray<ModelRef>& modelRefs);

	u64 AddFillConstantBufferCommandForData(ConstantBufferHandle handle, u64 elementIndex, void* data);
	
	void UpdateSceneLighting(const r2::draw::LightSystem& lightSystem);

	void AddDrawBatch(const BatchConfig& batch);

	
#ifdef R2_DEBUG
	void DrawDebugBones(
		const r2::SArray<DebugBone>& bones,
		const r2::SArray<u64>& numBonesPerModel,
		const r2::SArray<glm::mat4>& numModelMats,
		const glm::vec4& color);


	void DrawSphere(const glm::vec3& center, float radius, const glm::vec4& color, bool filled);
	void DrawCube(const glm::vec3& center, float scale, const glm::vec4& color, bool filled);
	void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, bool disableDepth);
	void DrawTangentVectors(DefaultModel model, const glm::mat4& transform);
#endif

	//events
	void WindowResized(u32 width, u32 height);
	void MakeCurrent();
	int SetFullscreen(int flags);
	int SetVSYNC(bool vsync);
	void SetWindowSize(u32 width, u32 height);
}

#endif