#ifndef __EDITOR_ACTION_H__
#define __EDITOR_ACTION_H__

#ifdef R2_EDITOR


namespace r2
{
	class Editor;
}


namespace r2::edit
{
	class EditorAction
	{
	public:

		virtual ~EditorAction() {}
		virtual void Undo() {};
		virtual void Redo() {};

	protected:
		EditorAction(r2::Editor* editor)
			:mnoptrEditor(editor) {}
		r2::Editor* mnoptrEditor;
	};
}
#endif

#endif // __EDITOR_ACTION_H__
