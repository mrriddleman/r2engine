#ifndef __FRAME_H__
#define __FRAME_H__

namespace r2::anim
{
	template <unsigned int N>
	struct Frame
	{
		double mTime;
		float mValue[N];
		float mIn[N];
		float mOut[N];
	};

	typedef Frame<1> ScalarFrame;
	typedef Frame<3> VectorFrame;
	typedef Frame<4> QuatFrame;
}

#endif