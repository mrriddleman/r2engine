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

		rendererimpl::SetRenderTargetMipLevel(
			realData->frameBufferID,
			realData->colorTextures,
			realData->toColorMipLevels,
			realData->colorTextureLayers,
			realData->numColorTextures,
			realData->depthTexture,
			realData->toDepthMipLevel,
			realData->depthTextureLayer,
			realData->xOffset,
			realData->yOffset,
			realData->width,
			realData->height,
			realData->colorUseLayeredRenderering,
			realData->depthUseLayeredRenderering);
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
}