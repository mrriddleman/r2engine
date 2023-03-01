#ifndef __EDITOR_LAYER_H__
#define __EDITOR_LAYER_H__

#ifdef R2_EDITOR
#include "r2/Core/Layer/Layer.h"
#include "r2/Editor/Editor.h"

namespace r2
{
	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual void Init() override;
		virtual void Shutdown() override;
		virtual void Update() override;
		virtual void Render(float alpha) override;
		virtual void ImGuiRender(u32 dockingSpaceID) override;
		virtual void OnEvent(evt::Event& event) override;
		bool IsEnabled() const;

	private:
		Editor editor;
		bool mEnabled;
	};
}

#endif
#endif
