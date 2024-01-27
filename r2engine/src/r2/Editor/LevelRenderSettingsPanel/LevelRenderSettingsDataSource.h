#ifndef __LEVEL_RENDER_SETTINGS_DATA_SOURCE_H__
#define __LEVEL_RENDER_SETTINGS_DATA_SOURCE_H__

#ifdef R2_EDITOR
#include <string>
#include "r2/Utils/Utils.h"

namespace r2::evt
{
	class Event;
}

class LevelRenderSettingsDataSource
{
public:
	LevelRenderSettingsDataSource(const std::string& title, s32 sortOrder)
		:mTitle(title)
		,mSortOrder(sortOrder)
	{
		
	}

	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual bool OnEvent(r2::evt::Event& e) = 0;
	virtual void Shutdown() = 0;

	virtual const std::string& GetTitle() const { return mTitle; }
	virtual void SetTitle(const std::string& title) { mTitle = title; }
	
	virtual s32 GetSortOrder() const { return mSortOrder; }
	virtual void SetSortOrder(s32 sortOrder) { mSortOrder = sortOrder; }
private:
	std::string mTitle;
	s32 mSortOrder;
};


#endif
#endif