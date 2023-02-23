#ifdef R2_EDITOR
#ifndef __EDITOR_ASSET_PANEL_H__
#define __EDITOR_ASSET_PANEL_H__

#include "r2/Editor/EditorWidget.h"

namespace r2::edit
{
	class AssetPanel : public EditorWidget
	{
	public:
		AssetPanel();
		~AssetPanel();
		virtual void Init(Editor* noptrEditor) override;
		virtual void Shutdown() override;
		virtual void OnEvent(evt::Event& e) override;
		virtual void Update() override;
		virtual void Render(u32 dockingSpaceID) override;
	private:
	};
}


#endif
#endif // R2_EDITOR
