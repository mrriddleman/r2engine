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

	void CopyBuffer(const void* data)
	{
		const r2::draw::cmd::CopyBuffer* realData = static_cast<const r2::draw::cmd::CopyBuffer*>(data);
		R2_CHECK(realData != nullptr, "We don't have any of the real data?");

		rendererimpl::CopyBuffer(realData->readBuffer, realData->writeBuffer, realData->readOffset, realData->writeOffset, realData->size);
	}

	void DeleteBuffer(const void* data)
	{
		const r2::draw::cmd::DeleteBuffer* realData = static_cast<const r2::draw::cmd::DeleteBuffer*>(data);
		R2_CHECK(realData != nullptr, "We don't have any of the real data?");

		rendererimpl::DeleteBuffers(1, &realData->bufferHandle);
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

	void DispatchComputeIndirect(const void* data)
	{
		const cmd::DispatchComputeIndirect* realData = static_cast<const r2::draw::cmd::DispatchComputeIndirect*>(data);
		R2_CHECK(realData != nullptr, "We don't have any real data?");

		rendererimpl::DispatchComputeIndirect(realData->dispatchIndirectBuffer, realData->offset);
	}

	void DispatchCompute(const void* data)
	{
		const cmd::DispatchCompute* realData = static_cast<const r2::draw::cmd::DispatchCompute*>(data);
		R2_CHECK(realData != nullptr, "We don't have any real data?");

		rendererimpl::DispatchCompute(realData->numGroupsX, realData->numGroupsY, realData->numGroupsZ);
	}

	void Barrier(const void* data)
	{
		const cmd::Barrier* realData = static_cast<const r2::draw::cmd::Barrier*>(data);
		R2_CHECK(realData != nullptr, "We don't have any real data?");

		rendererimpl::Barrier(realData->flags);
	}

	void ConstantUint(const void* data)
	{
		const cmd::ConstantUint* realData = static_cast<const r2::draw::cmd::ConstantUint*>(data);
		R2_CHECK(realData != nullptr, "We don't have any real data?");

		rendererimpl::ConstantUint(realData->uniformLocation, realData->value);
	}

	void SetRenderTargetMipLevel(const void* data)
	{
		const cmd::SetRenderTargetMipLevel* realData = static_cast<const r2::draw::cmd::SetRenderTargetMipLevel*>(data);
		R2_CHECK(realData != nullptr, "We don't have any real data?");

		//@TODO(Serge): have SetRenderTargetMipLevel take in cmd::SetRenderTargetMipLevel command instead, this is getting silly...
		rendererimpl::SetRenderTargetMipLevel(
			realData->frameBufferID,
			realData->colorTextures,
			realData->toColorMipLevels,
			realData->colorTextureLayers,
			realData->numColorTextures,
			realData->depthTexture,
			realData->toDepthMipLevel,
			realData->depthTextureLayer,
			realData->stencilTexture,
			realData->toStencilMipLevel,
			realData->stencilTextureLayer,
			realData->depthStencilTexture,
			realData->toDepthStencilMipLevel,
			realData->depthStencilTextureLayer,
			realData->xOffset,
			realData->yOffset,
			realData->width,
			realData->height,
			realData->colorUseLayeredRenderering,
			realData->depthUseLayeredRenderering,
			realData->stencilUseLayeredRendering,
			realData->depthStencilUseLayeredRenderering,
			realData->colorIsMSAA,
			realData->depthStencilIsMSAA);
	}

	void CopyRenderTargetColorTexture(const void* data)
	{
		const cmd::CopyRenderTargetColorTexture* realData = static_cast<const r2::draw::cmd::CopyRenderTargetColorTexture*>(data);
		R2_CHECK(realData != nullptr, "We don't have any real data?");

		rendererimpl::CopyRenderTargetColorTexture(
			realData->frameBufferID,
			realData->attachment,
			realData->toTextureID,
			realData->mipLevel,
			realData->xOffset,
			realData->yOffset,
			realData->layer,
			realData->x, realData->y,
			realData->width, realData->height);
	}

	void SetTexture(const void* data)
	{
		const cmd::SetTexture* realData = static_cast<const r2::draw::cmd::SetTexture*>(data);
		R2_CHECK(realData != nullptr, "We don't have any real data?");

		rendererimpl::SetTexture(realData->textureContainerUniformLocation, realData->textureContainer, realData->texturePageUniformLocation, realData->texturePage, realData->textureLodUniformLocation, realData->textureLod);
	}

	void BindImageTexture(const void* data)
	{
		const cmd::BindImageTexture* realData = static_cast<const r2::draw::cmd::BindImageTexture*>(data);
		R2_CHECK(realData != nullptr, "We don't have any real data?");

		rendererimpl::BindImageTexture(realData->textureUnit, realData->texture, realData->mipLevel, realData->layered, realData->layer, realData->access, realData->format);
	}

	void BlitFramebuffer(const void* data)
	{
		const cmd::BlitFramebuffer* realData = static_cast<const r2::draw::cmd::BlitFramebuffer*>(data);
		R2_CHECK(realData != nullptr, "We don't have any real data?");
		//void BlitFramebuffer(u32 readFramebuffer, u32 drawFramebuffer, s32 srcX0, s32 srcY0, s32 srcX1, s32 srcY1, s32 dstX0, s32 dstY0, s32 dstX1, s32 dstY1, u32 mask, u32 filter);
		rendererimpl::BlitFramebuffer(realData->readFramebuffer, realData->drawFramebuffer, realData->srcX0, realData->srcY0, realData->srcX1, realData->srcY1, realData->dstX0, realData->dstY0, realData->dstX1, realData->dstY1, realData->mask, realData->filter);
	}
}