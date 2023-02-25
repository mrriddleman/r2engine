#ifndef __EDITOR_ACTION_H__
#define __EDITOR_ACTION_H__

namespace r2::edit
{
	class EditorAction
	{
	public:

		virtual ~EditorAction() {}
		virtual void Undo() = 0;
		virtual void Redo() = 0;


	protected:

	};
}


#endif // __EDITOR_ACTION_H__
