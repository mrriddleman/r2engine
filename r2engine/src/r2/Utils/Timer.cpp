#include "r2pch.h"
#include "r2/Utils/Timer.h"
#include "r2/Platform/Platform.h"

namespace r2::util
{
	Timer::Timer(const std::string& name, bool printResult)
		: mName(name)
		, mStart(0.0)
		, mStopped(true)
		, mPrint(printResult)
	{
		Start();
	}

	Timer::~Timer()
	{
		if (!mStopped)
			Stop();
	}

	void Timer::Start()
	{
		mStopped = false;
		mStart = CENG.GetTicks();
	}

	f64 Timer::Stop()
	{
		mStopped = true;
		f64 result = CENG.GetTicks() - mStart;

		if (mPrint)
		{
			printf("%s took %f ms\n", mName.c_str(), result);
		}
		
		return result;
	}
}