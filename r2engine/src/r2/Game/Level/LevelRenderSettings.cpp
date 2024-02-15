#include "r2pch.h"

#include "r2/Game/Level/LevelRenderSettings.h"

namespace r2
{
	const u32 LevelRenderSettings::MAX_NUM_LEVEL_CAMERAS = 10;



	bool LevelRenderSettings::AddCamera(
		const Camera& camera
#ifdef R2_EDITOR
		, const std::string& cameraName
#endif
	)
	{
		if (mCameras && r2::sarr::Size(*mCameras) +1 < r2::sarr::Capacity(*mCameras))
		{
			r2::sarr::Push(*mCameras, camera);


#ifdef R2_EDITOR
			mCameraNames.push_back(cameraName);
#endif

			return true;
		}

		return false;
	}

	bool LevelRenderSettings::RemoveCamera(u32 cameraIndex)
	{
		if (mCameras && cameraIndex < r2::sarr::Size(*mCameras) && r2::sarr::Size(*mCameras) > 0)
		{

			if (cameraIndex == mCurrentCameraIndex)
			{
				mCurrentCameraIndex = 0;
			}

			r2::sarr::RemoveElementAtIndexShiftLeft(*mCameras, cameraIndex);

#ifdef R2_EDITOR
			mCameraNames.erase(mCameraNames.begin() + cameraIndex);
#endif
			return true;
		}

		return false;
	}

#ifdef R2_EDITOR
	const std::string& LevelRenderSettings::GetCameraName(u32 cameraIndex)
	{
		if (mCameraNames.size() > 0 && cameraIndex < mCameraNames.size())
		{
			return mCameraNames[cameraIndex];
		}
		
		return "Invalid";
	}
#endif

	bool LevelRenderSettings::SetCurrentCamera(s32 cameraIndex)
	{
		if (!mCameras)
		{
			return false;
		}

		if (cameraIndex >= 0 && cameraIndex < r2::sarr::Size(*mCameras))
		{
			mCurrentCameraIndex = cameraIndex;
			return true;
		}

		return false;
	}

	r2::Camera* LevelRenderSettings::GetCurrentCamera()
	{
		if (!mCameras || mCurrentCameraIndex == -1)
		{
			return nullptr;
		}

		if (r2::sarr::Size(*mCameras) == 0)
		{
			return nullptr;
		}

		return &r2::sarr::At(*mCameras, mCurrentCameraIndex);
	}

	u32 LevelRenderSettings::MemorySize(const r2::mem::utils::MemoryProperties& memProperties)
	{
		u32 totalMemorySize = 0;
		totalMemorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(LevelRenderSettings), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		totalMemorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Camera>::MemorySize(MAX_NUM_LEVEL_CAMERAS), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);


		return totalMemorySize;
	}
}