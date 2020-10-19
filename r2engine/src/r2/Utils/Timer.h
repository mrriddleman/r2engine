#ifndef __TIMER_H__
#define __TIMER_H__

#include "r2/Utils/Utils.h"

#define PROFILE_SCOPE_FN r2::util::Timer timerScope(__FUNCTION__, true);
#define PROFILE_SCOPE(name) r2::util::Timer timerScopeName(name, true);

namespace r2::util
{
	class Timer
	{
	public:
		Timer(const std::string& name, bool printResult);
		~Timer();
		void Start();
		f64 Stop();

	private:
		std::string mName;
		f64 mStart;
		bool mStopped;
		bool mPrint;
	};
}


#endif // __TIMER_H__
