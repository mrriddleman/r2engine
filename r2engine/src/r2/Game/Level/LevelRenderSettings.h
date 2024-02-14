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
		static const u32 MAX_NUM_LEVEL_CAMERAS;

		template<class ARENA>
		void Init(ARENA& arena)
		{
			mCameras = MAKE_SARRAY(arena, r2::Camera, MAX_NUM_LEVEL_CAMERAS);
		}

		template<class ARENA>
		void Shutdown(ARENA& arena)
		{
			FREE(mCameras, arena);
			mCameras = nullptr;
			mCurrentCameraIndex = -1;
		}

		bool AddCamera(const Camera& camera);
		bool SetCurrentCamera(s32 cameraIndex);


		Camera* GetCurrentCamera();

		static u32 MemorySize(const r2::mem::utils::MemoryProperties& memProperties);

	private:
		r2::SArray<r2::Camera>* mCameras = nullptr;
		s32 mCurrentCameraIndex = -1;
	};
}

#endif