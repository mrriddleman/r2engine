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
	const r2::draw::dispatch::BackendDispatchFunction SetRenderTarget::DispatchFunc = &r2::draw::dispatch::SetRenderTarget;

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
			sizeof(r2::draw::cmd::SetRenderTarget)
			});
	}


}