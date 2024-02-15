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

		bool AddCamera(
			const Camera& camera
#ifdef R2_EDITOR
			, const std::string& cameraName
#endif
		);

		bool RemoveCamera(u32 cameraIndex);

		bool SetCurrentCamera(s32 cameraIndex);

#ifdef R2_EDITOR
		const std::string& GetCameraName(u32 cameraIndex);
#endif

		Camera* GetCurrentCamera();

		r2::SArray<r2::Camera>* GetLevelCameras() { return mCameras; }
		inline s32 GetCurrentCameraIndex() {
			return mCurrentCameraIndex;
		}
		static u32 MemorySize(const r2::mem::utils::MemoryProperties& memProperties);

	private:
		r2::SArray<r2::Camera>* mCameras = nullptr;
#ifdef R2_EDITOR
		std::vector<std::string> mCameraNames;
#endif
		s32 mCurrentCameraIndex = -1;
	};
}

#endif