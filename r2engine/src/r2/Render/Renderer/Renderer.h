
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/Shader.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Model/Model.h"

namespace r2
{
	struct Camera;
}

namespace r2::draw
{
	class BufferLayout;

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
	struct BatchConfig
	{
		r2::draw::key::Basic key;
		VertexConfigHandle vertexLayoutConfigHandle;
		ConstantBufferHandle subCommandsHandle;
		ConstantBufferHandle modelsHandle;
		ConstantBufferHandle materialsHandle;
		ConstantBufferHandle boneTransformOffsetsHandle;
		ConstantBufferHandle boneTransformsHandle;
		b32 clear = false;

		//@NOTE: the size of these arrays should always be the same
		const r2::SArray<glm::mat4>* models = nullptr;
		const r2::SArray<r2::draw::cmd::DrawBatchSubCommand>* subcommands = nullptr;
		//@NOTE: this assumes that the materials have already been uploaded
		const r2::SArray<r2::draw::MaterialHandle>* materials = nullptr;

		//The boneTransforms can be of any size
		const r2::SArray<glm::mat4>* boneTransforms = nullptr;
		//boneTransformOffsets' size should be exactly the same as the number of subcommands
		const r2::SArray<glm::ivec4>* boneTransformOffsets = nullptr;

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

	
	void SetDepthTest(bool shouldDepthTest);

	VertexConfigHandle AddStaticModelLayout(const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize, u64 numDraws, bool generateDrawIDs = true);
	VertexConfigHandle AddAnimatedModelLayout(const std::initializer_list<u64>& vertexLayoutSizes, u64 indexSize, u64 numDraws, bool generateDrawIDs = true);
	VertexConfigHandle AddDebugDrawLayout(u64 maxDraws);
	ConstantConfigHandle AddConstantBufferLayout(ConstantBufferLayout::Type type, const std::initializer_list<ConstantBufferElement>& elements);
	ConstantConfigHandle AddMaterialLayout(u64 maxDraws);
	ConstantConfigHandle AddSubCommandsLayout(u64 maxDraws);


	//Regular methods
	BufferHandles& GetVertexBufferHandles();
	const r2::SArray<r2::draw::ConstantBufferHandle>* GetConstantBufferHandles();

	const Model* GetDefaultModel(r2::draw::DefaultModel defaultModel);
	const r2::SArray<r2::draw::ModelRef>* GetDefaultModelRefs();
	r2::draw::ModelRef GetDefaultModelRef(r2::draw::DefaultModel defaultModel);

	void GetDefaultModelMaterials(r2::SArray<r2::draw::MaterialHandle>& defaultModelMaterials);
	r2::draw::MaterialHandle GetMaterialHandleForDefaultModel(r2::draw::DefaultModel defaultModel);

	void GetMaterialsAndBoneOffsetsForAnimModels(const r2::SArray<const r2::draw::AnimModel*>& models, r2::SArray<r2::draw::MaterialHandle>& materialHandles, r2::SArray<glm::ivec4>& boneOffsets);

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

	void FillSubCommandsFromModels(r2::SArray<r2::draw::cmd::DrawBatchSubCommand>& subCommands, const r2::SArray<const Model*>& models);
	void FillSubCommandsFromModelRefs(r2::SArray<r2::draw::cmd::DrawBatchSubCommand>& subCommands, const r2::SArray<ModelRef>& modelRefs);
	void FillSubCommandsForDebugBones(r2::SArray<r2::draw::cmd::DrawDebugBatchSubCommand>& subCommands, const r2::SArray<const DebugBone>& debugBones);


	u64 AddFillConstantBufferCommandForData(ConstantBufferHandle handle, u64 elementIndex, void* data);

	void AddDrawBatch(const BatchConfig& batch);

	//@NOTE: maybe these handles should be set by the renderer?
	void AddDebugBatch(
		const r2::SArray<DebugBone>& bones,
		const r2::SArray<u64>& numBonesPerModel,
		const r2::SArray<glm::mat4>& numModelMats,
		r2::draw::ConstantBufferHandle modelMatsHandle,
		r2::draw::ConstantBufferHandle subCommandsHandle);

	//events
	void WindowResized(u32 width, u32 height);
	void MakeCurrent();
	int SetFullscreen(int flags);
	int SetVSYNC(bool vsync);
	void SetWindowSize(u32 width, u32 height);
}

#endif