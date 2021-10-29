#include "r2pch.h"

#include "r2/Render/Renderer/RendererTypes.h"

namespace r2::draw
{
	const std::string GetRendererBackendName(r2::draw::RendererBackend backend)
	{
		switch (backend)
		{
		case draw::OpenGL:
			return "OpenGL";
		case draw::Vulkan:
			return "Vulkan";
		case draw::D3D11:
			return "D3D11";
		case draw::D3D12:
			return "D3D12";
		default:
			R2_CHECK(false, "Unsupported Renderer Backend!");
		}

		return "";
	}
}