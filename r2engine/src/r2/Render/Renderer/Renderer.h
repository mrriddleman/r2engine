
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

	template <typename T = r2::draw::key::Basic>
	struct CommandBucket;
}

namespace r2::draw::renderer
{
	//basic stuff
	bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle, const char* shaderManifestPath);
	void Update();
	void Render(float alpha);
	void Shutdown();
	u64 MemorySize();

	//Setup code
	void SetClearColor(const glm::vec4& color);
	bool GenerateBufferLayouts(const r2::SArray<BufferLayoutConfiguration>* layouts);

	//Regular methods
	BufferHandles& GetBufferHandles();

	//@TODO(Serge): rethink this function
	template<class CMD>
	CMD* AddCommand(r2::draw::key::Basic key);

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