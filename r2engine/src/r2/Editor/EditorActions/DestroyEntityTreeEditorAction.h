#ifndef __DESTROY_ENTITY_TREE_EDITOR_ACTION_H__
#define __DESTROY_ENTITY_TREE_EDITOR_ACTION_H__
#ifdef R2_EDITOR

#include "r2/Editor/EditorActions/EditorAction.h"
#include "r2/Game/ECS/Entity.h"

#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/HierarchyComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"

namespace
{
	class Editor;
}

namespace r2::edit
{
	class DestroyEntityTreeEditorAction : public EditorAction
	{
	public:

		DestroyEntityTreeEditorAction(Editor* editor, ecs::Entity entityToDestroy, ecs::Entity parentOfEntityToDestroy);

		virtual void Undo() override;
		virtual void Redo() override;

	private:
		std::vector<ecs::Entity> mEntityTree;

		//@TODO(Serge): This is completely unsustainable since we don't usually know how many
		//				different components will exist for any given entity. Clearly this will
		//				need to change when we do serialization. May need to read it in from a
		//				file or something crazy like that. Or maybe save it to some kind of 
		//				flatbuffer format. The other issue is that we want the user of the engine
		//				to be able to create their own Components so we need a way to support that.
		//				This is very temporary code for now.
		std::vector<ecs::EditorComponent> mEntityTreeEditorNameComponents;
		std::vector<ecs::HierarchyComponent> mEntityTreeHeirarchyComponents;
		std::vector<ecs::TransformComponent> mEntityTreeTransformComponents;
		
	};
}
#endif
#endif // __DESTROY_ENTITY_TREE_EDITOR_ACTION_H__
