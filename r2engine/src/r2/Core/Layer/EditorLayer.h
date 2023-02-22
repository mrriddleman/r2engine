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
		virtual void ImGuiRender() override;
		virtual void OnEvent(evt::Event& event) override;
	private:
		Editor editor;
		bool mEnabled;
	};
}

#endif
#endif
