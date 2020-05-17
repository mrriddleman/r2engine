#ifndef __DISK_FILE_UTILS_H__
#define __DISK_FILE_UTILS_H__

#include "r2/Core/File/FileTypes.h"

namespace r2::fs::utils
{
	void GetStringFileMode(char buf[4], FileMode mode);
}

#endif // 
