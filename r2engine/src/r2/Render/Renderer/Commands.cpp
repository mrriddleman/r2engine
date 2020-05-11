#include "r2pch.h"
#include "r2/Render/Renderer/Commands.h"

namespace r2::draw::cmd
{
	const r2::draw::dispatch::BackendDispatchFunction Clear::DispatchFunc = &r2::draw::dispatch::Clear;
	const r2::draw::dispatch::BackendDispatchFunction DrawIndexed::DispatchFunc = &r2::draw::dispatch::DrawIndexed;
	const r2::draw::dispatch::BackendDispatchFunction FillVertexBuffer::DispatchFunc = &r2::draw::dispatch::FillVertexBuffer;
	const r2::draw::dispatch::BackendDispatchFunction FillIndexBuffer::DispatchFunc = &r2::draw::dispatch::FillIndexBuffer;

	u64 LargestCommand()
	{
		return std::max({
			sizeof(r2::draw::cmd::Clear),
			sizeof(r2::draw::cmd::DrawIndexed),
			sizeof(r2::draw::cmd::FillVertexBuffer),
			sizeof(r2::draw::cmd::FillIndexBuffer)
			});
	}
}