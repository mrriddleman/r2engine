#ifdef R2_EDITOR
#ifndef __EDITOR_SCENE_PANEL_H__
#define __EDITOR_SCENE_PANEL_H__

#include "r2/Editor/EditorWidget.h"
#include "r2/Game/ECS/Entity.h"

namespace r2::edit
{
	class ScenePanel: public EditorWidget
	{
	public:
		ScenePanel();
		~ScenePanel();

		virtual void Init(Editor* noptrEditor) override;
		virtual void Shutdown() override;
		virtual void OnEvent(evt::Event& e) override;
		virtual void Update() override;
		virtual void Render(u32 dockingSpaceID) override;

	private:

		struct SceneTreeNode
		{
			ecs::Entity entity;
			size_t numChildren;
			std::vector<SceneTreeNode> children;
			bool enabled = true;
			bool show = true;
			bool selected = false;
		};

		std::vector<SceneTreeNode> mSceneGraphData;

		bool mSceneGraphDataNeedsUpdate;


		void AddAllChildrenForEntity(SceneTreeNode& parent);
		void DisplayNode(SceneTreeNode& node);
		void AddNewEntityToTable(ecs::Entity parent);
		void RemoveEntity(ecs::Entity entity);
		void DropSceneNode(SceneTreeNode* node);

		ecs::Entity mSelectedEntity;
		ecs::Entity mPrevSelectedEntity;
		ecs::Entity mRenameEntity;
	};
}

#endif
#endif