#include "r2pch.h"
#include "r2/Render/Renderer/Commands.h"
#include "r2/Render/Model/Mesh.h"

namespace r2::draw::cmd
{
	const r2::draw::dispatch::BackendDispatchFunction Clear::DispatchFunc = &r2::draw::dispatch::Clear;
	const r2::draw::dispatch::BackendDispatchFunction DrawIndexed::DispatchFunc = &r2::draw::dispatch::DrawIndexed;
	const r2::draw::dispatch::BackendDispatchFunction FillVertexBuffer::DispatchFunc = &r2::draw::dispatch::FillVertexBuffer;
	const r2::draw::dispatch::BackendDispatchFunction FillIndexBuffer::DispatchFunc = &r2::draw::dispatch::FillIndexBuffer;
	const r2::draw::dispatch::BackendDispatchFunction FillConstantBuffer::DispatchFunc = &r2::draw::dispatch::FillConstantBuffer;
	const r2::draw::dispatch::BackendDispatchFunction CompleteConstantBuffer::DispatchFunc = &r2::draw::dispatch::CompleteConstantBuffer;
	const r2::draw::dispatch::BackendDispatchFunction DrawBatch::DispatchFunc = &r2::draw::dispatch::DrawBatch;
	const r2::draw::dispatch::BackendDispatchFunction DrawDebugBatch::DispatchFunc = &r2::draw::dispatch::DrawDebugBatch;
	const r2::draw::dispatch::BackendDispatchFunction DispatchComputeIndirect::DispatchFunc = &r2::draw::dispatch::DispatchComputeIndirect;
	const r2::draw::dispatch::BackendDispatchFunction DispatchCompute::DispatchFunc = &r2::draw::dispatch::DispatchCompute;
	const r2::draw::dispatch::BackendDispatchFunction Barrier::DispatchFunc = &r2::draw::dispatch::Barrier;
	const r2::draw::dispatch::BackendDispatchFunction ConstantUint::DispatchFunc = &r2::draw::dispatch::ConstantUint;
	const r2::draw::dispatch::BackendDispatchFunction SetRenderTargetMipLevel::DispatchFunc = &r2::draw::dispatch::SetRenderTargetMipLevel;
	const r2::draw::dispatch::BackendDispatchFunction CopyRenderTargetColorTexture::DispatchFunc = &r2::draw::dispatch::CopyRenderTargetColorTexture;
	const r2::draw::dispatch::BackendDispatchFunction SetTexture::DispatchFunc = &r2::draw::dispatch::SetTexture;
	const r2::draw::dispatch::BackendDispatchFunction BindImageTexture::DispatchFunc = &r2::draw::dispatch::BindImageTexture;

	u64 LargestCommand()
	{
		return std::max({
			sizeof(r2::draw::cmd::Clear),
			sizeof(r2::draw::cmd::DrawIndexed),
			sizeof(r2::draw::cmd::FillVertexBuffer),
			sizeof(r2::draw::cmd::FillIndexBuffer),
			sizeof(r2::draw::cmd::FillConstantBuffer),
			sizeof(r2::draw::cmd::CompleteConstantBuffer),
			sizeof(r2::draw::cmd::DrawBatch),
			sizeof(r2::draw::cmd::DrawDebugBatch),
			sizeof(r2::draw::cmd::DispatchComputeIndirect),
			sizeof(r2::draw::cmd::DispatchCompute),
			sizeof(r2::draw::cmd::Barrier),
			sizeof(r2::draw::cmd::ConstantUint),
			sizeof(r2::draw::cmd::SetRenderTargetMipLevel),
			sizeof(r2::draw::cmd::CopyRenderTargetColorTexture),
			sizeof(r2::draw::cmd::SetTexture),
			sizeof(r2::draw::cmd::BindImageTexture),
			});
	}

	void SetDefaultStencilState(StencilState& stencilState)
	{
		stencilState.stencilEnabled = false;
		stencilState.stencilWriteEnabled = false;

		stencilState.op.stencilFail = KEEP;
		stencilState.op.depthFail = KEEP;
		stencilState.op.depthAndStencilPass = KEEP;

		stencilState.func.func = ALWAYS;
		stencilState.func.ref = 0;
		stencilState.func.mask = 0xFF;
	}
}