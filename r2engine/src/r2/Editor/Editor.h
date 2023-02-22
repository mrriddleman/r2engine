#ifdef R2_EDITOR
#ifndef __EDITOR_H__
#define __EDITOR_H__

namespace r2::evt
{
	class Event;
}

namespace r2
{
	class Editor
	{
	public:
		Editor();
		void Init();
		void Shutdown();
		void OnEvent(evt::Event& e);
		void Update();
		void Render();

	private:
	};
}

#endif // __EDITOR_H__
#endif