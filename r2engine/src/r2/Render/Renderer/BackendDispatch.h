#ifndef __BACKEND_DISPATCH_H__
#define __BACKEND_DISPATCH_H__

#include "r2/Render/Renderer/RendererTypes.h"

namespace r2::draw::dispatch
{
	typedef void (*BackendDispatchFunction)(const void*);
	static const size_t BACKEND_DISPATCH_OFFSET = 0u;

	void Clear(const void* data);
	void DrawIndexed(const void* data);
	void FillVertexBuffer(const void* data);
	void FillIndexBuffer(const void* data);
	void FillConstantBuffer(const void* data);
	void DrawBatch(const void* data);
}


#endif
