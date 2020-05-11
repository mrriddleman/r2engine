#ifndef __RENDER_TARGET_H__
#define __RENDER_TARGET_H__

#include "r2/Utils/Utils.h"

namespace r2::draw
{
	struct RenderTargetAttachment
	{

	};
}

namespace r2
{
	template <typename T = r2::draw::RenderTargetAttachment>
	struct SArray;
}

namespace r2::draw
{
	//@TODO(Serge): implement for real
	struct RenderTarget
	{
		static const u32 DEFAULT_RENDER_TARGET;
		u32 frameBufferID = DEFAULT_RENDER_TARGET;

		r2::SArray<RenderTargetAttachment>* attachments = nullptr;
	};
}


#endif // ! __RENDER_TARGET_H__
