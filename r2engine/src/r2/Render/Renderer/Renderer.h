
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/Shader.h"

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
	};

	namespace key
	{
		struct Basic;
	}

	//template <typename T = r2::draw::key::Basic>
	//struct CommandBucket;

	enum DefaultModel
	{
		QUAD = 0,
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

	//Regular methods
	BufferHandles& GetBufferHandles();
	Model* GetDefaultModel(r2::draw::DefaultModel defaultModel);
	//@Temporary
	void LoadEngineTexturesFromDisk();
	void UploadMaterialTexturesToGPUFromMaterialName(u64 materialName);


	u64 AddFillVertexCommandsForModel(Model* model, VertexBufferHandle handle, u64 offset = 0);
	u64 AddFillIndexCommandsForModel(Model* model, IndexBufferHandle handle, u64 offset = 0);
	r2::draw::cmd::Clear* AddClearCommand(r2::draw::key::Basic key);
	r2::draw::cmd::DrawIndexed* AddDrawIndexedCommand(r2::draw::key::Basic key);
	r2::draw::cmd::FillIndexBuffer* AddFillIndexBufferCommand(r2::draw::key::Basic key);
	r2::draw::cmd::FillVertexBuffer* AddFillVertexBufferCommand(r2::draw::key::Basic key);

	//@TODO(Serge): rethink this function
	void SetCameraPtrOnBucket(r2::Camera* cameraPtr);

	//events
	void WindowResized(u32 width, u32 height);
	void MakeCurrent();
	int SetFullscreen(int flags);
	int SetVSYNC(bool vsync);
	void SetWindowSize(u32 width, u32 height);
}

#endif