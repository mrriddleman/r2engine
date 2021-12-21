#include "r2pch.h"
#include "r2/Render/Renderer/RendererImpl.h"
#include "r2/Render/Renderer/BackendDispatch.h"
#include "r2/Render/Renderer/Commands.h"

namespace r2::draw::dispatch
{
	void Clear(const void* data)
	{
		const r2::draw::cmd::Clear* realData = static_cast<const r2::draw::cmd::Clear*>(data);
		rendererimpl::Clear(realData->flags);
	}

	void DrawIndexed(const void* data)
	{
		const r2::draw::cmd::DrawIndexed* realData = static_cast<const r2::draw::cmd::DrawIndexed*>(data);

		R2_CHECK(realData != nullptr, "We don't have any of the real data?");
		rendererimpl::DrawIndexed(realData->bufferLayoutHandle, realData->vertexBufferHandle, realData->indexBufferHandle, realData->indexCount, realData->startIndex);
	}

	void FillVertexBuffer(const void* data)
	{
		const r2::draw::cmd::FillVertexBuffer* realData = static_cast<const r2::draw::cmd::FillVertexBuffer*>(data);
		R2_CHECK(realData != nullptr, "We don't have any of the real data?");

		rendererimpl::UpdateVertexBuffer(realData->vertexBufferHandle, realData->offset, realData->data, realData->dataSize);
	}

	void FillIndexBuffer(const void* data)
	{
		const r2::draw::cmd::FillIndexBuffer* realData = static_cast<const r2::draw::cmd::FillIndexBuffer*>(data);
		R2_CHECK(realData != nullptr, "We don't have any of the real data?");

		rendererimpl::UpdateIndexBuffer(realData->indexBufferHandle, realData->offset, realData->data, realData->dataSize);
	}

	void FillConstantBuffer(const void* data)
	{
		const r2::draw::cmd::FillConstantBuffer* realData = static_cast<const r2::draw::cmd::FillConstantBuffer*>(data);
		R2_CHECK(realData != nullptr, "We don't have any of the real data?");

		rendererimpl::UpdateConstantBuffer(realData->constantBufferHandle, realData->type, realData->isPersistent, realData->offset, realData->data, realData->dataSize);
	}

	void CompleteConstantBuffer(const void* data)
	{
		const r2::draw::cmd::CompleteConstantBuffer* realData = static_cast<const r2::draw::cmd::CompleteConstantBuffer*>(data);
		R2_CHECK(realData != nullptr, "We don't have any of the real data?");
	
		rendererimpl::CompleteConstantBuffer(realData->constantBufferHandle, realData->count);
	
	} 

	void DrawBatch(const void* data)
	{
		const r2::draw::cmd::DrawBatch* realData = static_cast<const r2::draw::cmd::DrawBatch*>(data);
		R2_CHECK(realData != nullptr, "We don't have any of the real data?");
		
		rendererimpl::ApplyDrawState(realData->state);
		rendererimpl::DrawIndexedCommands(realData->bufferLayoutHandle, realData->batchHandle, realData->subCommands, realData->numSubCommands, realData->startCommandIndex, 0, realData->primitiveType);
	}

	void DrawDebugBatch(const void* data)
	{
		const r2::draw::cmd::DrawDebugBatch* realData = static_cast<const r2::draw::cmd::DrawDebugBatch*>(data);
		R2_CHECK(realData != nullptr, "We don't have any of the real data?");
		
		rendererimpl::ApplyDrawState(realData->state);
		rendererimpl::DrawDebugCommands(realData->bufferLayoutHandle, realData->batchHandle, realData->subCommands, realData->numSubCommands, realData->startCommandIndex);
	}

	void SetRenderTarget(const void* data)
	{
		const cmd::SetRenderTarget* realData = static_cast<const cmd::SetRenderTarget*>(data);
		R2_CHECK(realData != nullptr, "We don't have any real data?");

		rendererimpl::SetRenderTarget(realData->framebufferID, realData->numColorAttachments, realData->numDepthAttachments, realData->xOffset, realData->yOffset, realData->width, realData->height, realData->depthTexture);
	}
}