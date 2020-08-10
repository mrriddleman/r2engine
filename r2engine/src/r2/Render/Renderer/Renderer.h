
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/Shader.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/RenderKey.h"

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
}

namespace r2::draw
{
	struct BatchConfig
	{
		r2::draw::key::Basic key;
		BufferLayoutHandle layoutHandle;
		ConstantBufferHandle subCommandsHandle;
		ConstantBufferHandle modelsHandle;
		ConstantBufferHandle materialsHandle;

		//@NOTE: the size of these arrays should always be the same
		const r2::SArray<glm::mat4>* models = nullptr;
		const r2::SArray<r2::draw::cmd::DrawBatchSubCommand>* subcommands = nullptr;
		//this assumes that the materials have already been uploaded
		const r2::SArray<r2::draw::MaterialHandle>* materials = nullptr;
	};
}

namespace r2::draw::renderer
{
	

	//basic stuff
	bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle, const char* shaderManifestPath);
	void Update();
	void Render(float alpha);
	void Shutdown();
	u64 MemorySize(u64 materialSystemMemorySize);

	//Setup code
	void SetClearColor(const glm::vec4& color);
	bool GenerateBufferLayouts(const r2::SArray<BufferLayoutConfiguration>* layouts);
	bool GenerateConstantBuffers(const r2::SArray<ConstantBufferLayoutConfiguration>* constantBufferConfigs);
	void SetDepthTest(bool shouldDepthTest);

	//Regular methods
	BufferHandles& GetBufferHandles();
	const r2::SArray<r2::draw::ConstantBufferHandle>* GetConstantBufferHandles();
	Model* GetDefaultModel(r2::draw::DefaultModel defaultModel);
	//@Temporary
	void LoadEngineTexturesFromDisk();
	void UploadMaterialTexturesToGPUFromMaterialName(u64 materialName);


	u64 AddFillVertexCommandsForModel(Model* model, VertexBufferHandle handle, u64 offset = 0);
	u64 AddFillIndexCommandsForModel(Model* model, IndexBufferHandle handle, u64 offset = 0);
	u64 AddFillConstantBufferCommandForData(ConstantBufferHandle handle, r2::draw::ConstantBufferLayout::Type type, b32 isPersistent, void* data, u64 size, u64 offset = 0);
	void FillSubCommandsFromModels(r2::SArray<r2::draw::cmd::DrawBatchSubCommand>& subCommands, const r2::SArray<Model*>& models);

	r2::draw::cmd::Clear* AddClearCommand(r2::draw::key::Basic key);
	r2::draw::cmd::DrawIndexed* AddDrawIndexedCommand(r2::draw::key::Basic key);
	r2::draw::cmd::FillIndexBuffer* AddFillIndexBufferCommand(r2::draw::key::Basic key);
	r2::draw::cmd::FillVertexBuffer* AddFillVertexBufferCommand(r2::draw::key::Basic key);
	r2::draw::cmd::FillConstantBuffer* AddFillConstantBufferCommand(r2::draw::key::Basic key, u64 auxMemory);
	
	void AddDrawBatch(const BatchConfig& batch);


	//events
	void WindowResized(u32 width, u32 height);
	void MakeCurrent();
	int SetFullscreen(int flags);
	int SetVSYNC(bool vsync);
	void SetWindowSize(u32 width, u32 height);
}

#endif