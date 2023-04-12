#include "r2pch.h"

#if defined R2_EDITOR && defined R2_IMGUI

#include "r2/Editor/EditorScenePanel.h"
#include "r2/Editor/Editor.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Editor/EditorActions/CreateEntityEditorAction.h"
#include "r2/Editor/EditorActions/DestroyEntityEditorAction.h"
#include "r2/Editor/EditorActions/DestroyEntityTreeEditorAction.h"
#include "r2/Editor/EditorActions/SelectedEntityEditorAction.h"
#include "r2/Editor/EditorActions/EntityEditorNameChangedEditorAction.h"
#include "r2/Editor/EditorActions/EntityEditorFlagChangedEditorAction.h"
#include "r2/Editor/EditorActions/AttachEntityEditorAction.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "imgui.h"


namespace r2::edit
{

	constexpr const char* DND_PAYLOAD_NAME = "SCENE_GRAPH_DND";

	ScenePanel::ScenePanel()
		:mSceneGraphDataNeedsUpdate(true)
		,mSelectedEntity(ecs::INVALID_ENTITY)
		,mPrevSelectedEntity(ecs::INVALID_ENTITY)
		,mRenameEntity(ecs::INVALID_ENTITY)
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

		dispatcher.Dispatch<r2::evt::EditorEntitySelectedEvent>([this](const r2::evt::EditorEntitySelectedEvent& e)
		{
			mSelectedEntity = e.GetEntity();
			mPrevSelectedEntity = mSelectedEntity;

			return e.ShouldConsume();
		});

		dispatcher.Dispatch<r2::evt::EditorEntityNameChangedEvent>([this](const r2::evt::EditorEntityNameChangedEvent& e) {
			mSceneGraphDataNeedsUpdate = true;
			return e.ShouldConsume();
		});

		dispatcher.Dispatch<r2::evt::EditorEntityAttachedToNewParentEvent>([this](const r2::evt::EditorEntityAttachedToNewParentEvent& e)
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
		const bool hasChildren = (node.numChildren > 0);
		ecs::EditorComponent* editorNameComponent = mnoptrEditor->GetECSCoordinator()->GetComponentPtr<ecs::EditorComponent>(node.entity);
		//HACK! Currently I don't know a good way to delete a whole subtree of the scene graph (while in the actual imgui render) without doing this
		if (!editorNameComponent)
		{
			return;
		}

		std::string editorName = editorNameComponent->editorName;

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowItemOverlap;

		bool isSelected = mSelectedEntity == node.entity;
		if (isSelected)
		{
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		}
		
		const char* nodeName = editorNameComponent->editorName.c_str();

		if (mRenameEntity == node.entity)
		{
			nodeName = "";
		}

		bool open = ImGui::TreeNodeEx(std::to_string(node.entity).c_str(), nodeFlags, "%s", "");
		ImGui::SameLine();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));


		bool originalSelected = editorNameComponent->flags.IsSet(ecs::EDITOR_FLAG_SELECTED_ENTITY);
		bool selected = originalSelected;

		if (ImGui::Selectable(nodeName, &selected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowItemOverlap))
		{
			mRenameEntity = ecs::INVALID_ENTITY;

			if (ImGui::IsMouseDoubleClicked(0))
			{
				mRenameEntity = node.entity;
			}
		}
		ImGui::PopStyleVar();

		if (selected != originalSelected)
		{
			mnoptrEditor->PostNewAction(std::make_unique<edit::EntityEditorFlagChangedEditorAction>(mnoptrEditor, node.entity, ecs::EDITOR_FLAG_SELECTED_ENTITY, selected));
		}

		if (node.entity == mRenameEntity)
		{
			char strName[1024];
			strName[0] = '\0';

			strcpy(strName, editorName.c_str());
			float spacing = ImGui::GetTreeNodeToLabelSpacing();

			ImGui::SameLine();
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
			if (ImGui::InputText("##rename", strName, IM_ARRAYSIZE(strName), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
			{
				mnoptrEditor->PostNewAction(std::make_unique<edit::EntityEditorNameChangedEditorAction>(mnoptrEditor, node.entity, std::string(strName), editorName));

				mRenameEntity = ecs::INVALID_ENTITY;
			}
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			mSelectedEntity = node.entity;
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload(DND_PAYLOAD_NAME, &node, sizeof(SceneTreeNode));

			ImGui::Text("Attach %s", editorName.c_str());

			ImGui::EndDragDropSource();
		}

		DropSceneNode(&node);

		ImGui::TableNextColumn();

		std::string enabledStrId = std::to_string(node.entity) + "enabledcheckbox";

		ImGui::PushID(enabledStrId.c_str());


		bool nodeEnabled = editorNameComponent->flags.IsSet(ecs::EDITOR_FLAG_ENABLE_ENTITY);

		if (ImGui::Checkbox("", &nodeEnabled))
		{
			mnoptrEditor->PostNewAction(std::make_unique<edit::EntityEditorFlagChangedEditorAction>(mnoptrEditor, node.entity, ecs::EDITOR_FLAG_ENABLE_ENTITY, nodeEnabled));
		}

		ImGui::PopID();
		ImGui::TableNextColumn();

		std::string showStrId = std::to_string(node.entity) + "showcheckbox";

		ImGui::PushID(showStrId.c_str());

		bool nodeShow = editorNameComponent->flags.IsSet(ecs::EDITOR_FLAG_SHOW_ENTITY);

		if (ImGui::Checkbox("", &nodeShow))
		{
			mnoptrEditor->PostNewAction(std::make_unique<edit::EntityEditorFlagChangedEditorAction>(mnoptrEditor, node.entity, ecs::EDITOR_FLAG_SHOW_ENTITY, nodeShow));
		}

		ImGui::PopID();
		ImGui::TableNextColumn();
		RemoveEntity(node.entity);

		if (open)
		{
			if (hasChildren)
			{
				for (SceneTreeNode& childNode : node.children)
				{
					DisplayNode(childNode);
				}
			}

			AddNewEntityToTable(node.entity);
			ImGui::TreePop();
		}
	}

	void ScenePanel::AddNewEntityToTable(ecs::Entity parent)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGui::PushID(parent);
		if (ImGui::Button("Add New Entity"))
		{
			mnoptrEditor->PostNewAction(std::make_unique<edit::CreateEntityEditorAction>(mnoptrEditor, parent));
		}
		ImGui::PopID();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();
	}

	void ScenePanel::RemoveEntity(ecs::Entity entity)
	{
		ImGui::PushID(entity);
		if (ImGui::Button("-"))
		{
			mnoptrEditor->PostNewAction(std::make_unique<edit::DestroyEntityEditorAction>(mnoptrEditor, entity, mnoptrEditor->GetSceneGraph().GetParent(entity)));
		}

		ImGui::SameLine();

		if (ImGui::Button(">"))
		{
			mnoptrEditor->PostNewAction(std::make_unique<edit::DestroyEntityTreeEditorAction>(mnoptrEditor, entity, mnoptrEditor->GetSceneGraph().GetParent(entity)));
		}
		ImGui::PopID();
	}

	void ScenePanel::DropSceneNode(SceneTreeNode* node)
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DND_PAYLOAD_NAME))
			{
				IM_ASSERT(payload->DataSize == sizeof(SceneTreeNode));
				SceneTreeNode* payload_n = (SceneTreeNode*)payload->Data;

				ecs::Entity newParent = ecs::INVALID_ENTITY;

				if (node)
				{
					newParent = node->entity;
				}

				//get the current parent of the entity
				ecs::Entity oldParent = mnoptrEditor->GetSceneGraph().GetParent(payload_n->entity);

				//post the action
				mnoptrEditor->PostNewAction(std::make_unique<edit::AttachEntityEditorAction>(mnoptrEditor, payload_n->entity, oldParent, newParent));

			}
			ImGui::EndDragDropTarget();
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

			DropSceneNode(nullptr);
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
				DropSceneNode(nullptr);

				ImGui::EndTable();
			}

			ImGui::TreePop();
		}

		ImGui::End();

		if (mPrevSelectedEntity != mSelectedEntity)
		{
			mnoptrEditor->PostNewAction(std::make_unique<edit::SelectedEntityEditorAction>(mnoptrEditor, mSelectedEntity, mPrevSelectedEntity));
		}

		//static bool show = false;
		//ImGui::ShowDemoWindow(&show);
	}

}

#endif