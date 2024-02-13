#ifndef __LEVEL_RENDER_SETTINGS_H__
#define __LEVEL_RENDER_SETTINGS_H__


#include "r2/Utils/Utils.h"
#include "r2/Render/Camera/Camera.h"
#include "r2/Core/Containers/SArray.h"

namespace r2::mem::utils
{
	struct MemoryProperties;
}

namespace r2
{
	struct LevelRenderSettings
	{

		r2::SArray<r2::Camera*>* mCameras = nullptr;

		static u64 MemorySize(const r2::mem::utils::MemoryProperties& memProperties);
	};
}

#endif