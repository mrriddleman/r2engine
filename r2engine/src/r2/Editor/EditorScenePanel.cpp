#include "r2pch.h"

#if defined R2_EDITOR && defined R2_IMGUI

#include "r2/Editor/EditorScenePanel.h"
#include "r2/Editor/Editor.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Game/ECS/Components/EditorNameComponent.h"
#include "r2/Editor/EditorActions/CreateEntityEditorAction.h"
#include "r2/Editor/EditorActions/DestroyEntityEditorAction.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "imgui.h"


namespace r2::edit
{

	ScenePanel::ScenePanel()
		:mSceneGraphDataNeedsUpdate(true)
	{

	}

	ScenePanel::~ScenePanel()
	{

	}

	void ScenePanel::Init(Editor* noptrEditor)
	{
		mnoptrEditor = noptrEditor;
	}

	void ScenePanel::Shutdown()
	{

	}

	void ScenePanel::OnEvent(evt::Event& e)
	{
		r2::evt::EventDispatcher dispatcher(e);

		dispatcher.Dispatch<r2::evt::EditorEntityCreatedEvent>([this](const r2::evt::EditorEntityCreatedEvent& e)
		{
			mSceneGraphDataNeedsUpdate = true;
			return e.ShouldConsume();
		});

		dispatcher.Dispatch<r2::evt::EditorEntityDestroyedEvent>([this](const r2::evt::EditorEntityDestroyedEvent& e)
		{
			mSceneGraphDataNeedsUpdate = true;
			return e.ShouldConsume();
		});

		dispatcher.Dispatch<r2::evt::EditorEntityTreeDestroyedEvent>([this](const r2::evt::EditorEntityTreeDestroyedEvent& e)
			{
				mSceneGraphDataNeedsUpdate = true;
				return e.ShouldConsume();
			});
	}

	void ScenePanel::AddAllChildrenForEntity(SceneTreeNode& parent)
	{
		r2::SceneGraph& sceneGraph = mnoptrEditor->GetSceneGraph();

		r2::SArray<ecs::Entity>* children = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ecs::Entity, ecs::MAX_NUM_ENTITIES);

		sceneGraph.GetAllChildrenForEntity(parent.entity, *children);

		parent.numChildren = r2::sarr::Size(*children);

		for (u32 j = 0; j < parent.numChildren; ++j)
		{
			SceneTreeNode child;

			child.entity = r2::sarr::At(*children, j);

			AddAllChildrenForEntity(child);

			parent.children.push_back(child);
		}

		FREE(children, *MEM_ENG_SCRATCH_PTR);
	}

	void ScenePanel::Update()
	{
		if (mSceneGraphDataNeedsUpdate)
		{
			mSceneGraphData.clear();

			r2::SceneGraph& sceneGraph = mnoptrEditor->GetSceneGraph();

			r2::SArray<ecs::Entity>* rootEntities = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, ecs::Entity, ecs::MAX_NUM_ENTITIES);
			r2::SArray<u32>* rootIndices = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u32, ecs::MAX_NUM_ENTITIES);

			sceneGraph.GetAllTopLevelEntities(*rootEntities, *rootIndices);

			const auto numRoots = r2::sarr::Size(*rootEntities);

			for (u32 i = 0; i < numRoots; ++i)
			{
				ecs::Entity rootEntity = r2::sarr::At(*rootEntities, i);

				SceneTreeNode rootNode;
				rootNode.entity = rootEntity;
				rootNode.numChildren = 0;

				AddAllChildrenForEntity(rootNode);

				mSceneGraphData.push_back(rootNode);
			}

			FREE(rootIndices, *MEM_ENG_SCRATCH_PTR);
			FREE(rootEntities, *MEM_ENG_SCRATCH_PTR);

			mSceneGraphDataNeedsUpdate = false;
		}
	}

	void ScenePanel::DisplayNode(SceneTreeNode& node)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		const bool hasChildren = (node.numChildren > 0);
		const ecs::EditorNameComponent& editorNameComponent = mnoptrEditor->GetECSCoordinator()->GetComponent<ecs::EditorNameComponent>(node.entity);

		if (hasChildren)
		{
			bool open = ImGui::TreeNodeEx(editorNameComponent.editorName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
			ImGui::TableNextColumn();
			ImGui::Checkbox("", &node.enabled);
			ImGui::TableNextColumn();
			ImGui::Checkbox("", &node.show);
			ImGui::TableNextColumn();
			RemoveEntity(node.entity);

			if (open)
			{
				for (SceneTreeNode& childNode : node.children)
				{
					DisplayNode(childNode);
				}
				ImGui::TreePop();
			}
		}
		else
		{
			bool open = ImGui::TreeNodeEx(editorNameComponent.editorName.c_str(),  ImGuiTreeNodeFlags_SpanFullWidth );
			ImGui::TableNextColumn();
			ImGui::Checkbox("", &node.enabled);
			ImGui::TableNextColumn();
			ImGui::Checkbox("", &node.show);
			ImGui::TableNextColumn();
			RemoveEntity(node.entity);

			if (open)
			{
				AddNewEntityToTable(node.entity);
				ImGui::TreePop();
			}	


		}
	}

	void ScenePanel::AddNewEntityToTable(ecs::Entity parent)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		if (ImGui::Button("Add New Entity"))
		{
			mnoptrEditor->PostNewAction(std::make_unique<edit::CreateEntityEditorAction>(mnoptrEditor, parent));
		}
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();
	}

	void ScenePanel::RemoveEntity(ecs::Entity entity)
	{
		if (ImGui::Button("-"))
		{
			mnoptrEditor->PostNewAction(std::make_unique<edit::DestroyEntityEditorAction>(mnoptrEditor, entity, mnoptrEditor->GetSceneGraph().GetParent(entity)));
		}
	}

	void ScenePanel::Render(u32 dockingSpaceID)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("M").x;

		ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);

		bool open = true;

		r2::SceneGraph& sceneGraph = mnoptrEditor->GetSceneGraph();


		if (!ImGui::Begin("ScenePanel", &open))
		{
			ImGui::End();
			return;
		}

		if (ImGui::TreeNode("Level Name Goes Here!"))
		{
			static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

			if (ImGui::BeginTable("EntityTable", 4, flags))
			{
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
				ImGui::TableSetupColumn("Enable", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 4.0f);
				ImGui::TableSetupColumn("Show", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 4.0f);
				ImGui::TableSetupColumn("Delete", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 5.0f);
				ImGui::TableHeadersRow();

				for (SceneTreeNode& node : mSceneGraphData)
				{
					DisplayNode(node);
				}

				AddNewEntityToTable(ecs::INVALID_ENTITY);

				ImGui::EndTable();
			}

			ImGui::TreePop();
		}

		ImGui::End();

		//static bool show = false;
		//ImGui::ShowDemoWindow(&show);
	}

}

#endif