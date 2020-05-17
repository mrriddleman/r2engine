#include "r2pch.h"

#include "r2/Core/File/FileDevices/Storage/Disk/DiskFileUtils.h"


namespace r2::fs::utils
{
	void GetStringFileMode(char buf[4], FileMode mode)
	{
		u8 size = 0;

		if (mode.IsSet(Mode::Read) &&
			!mode.IsSet(Mode::Append) &&
			!mode.IsSet(Mode::Write) &&
			!mode.IsSet(Recreate))
		{
			buf[size++] = 'r';
		}
		else if ((mode.IsSet(Mode::Read) && mode.IsSet(Mode::Write)) && !mode.IsSet(Mode::Recreate) && !mode.IsSet(Mode::Append))
		{
			buf[size++] = 'r';
			buf[size++] = '+';
		}
		else if (mode.IsSet(Mode::Write) && !mode.IsSet(Mode::Read) && !mode.IsSet(Mode::Append))
		{
			buf[size++] = 'w';
		}
		else if (mode.IsSet(Mode::Write) && mode.IsSet(Mode::Read) && !mode.IsSet(Mode::Append))
		{
			buf[size++] = 'w';
			buf[size++] = '+';
		}
		else if (mode.IsSet(Mode::Append) && !mode.IsSet(Mode::Read))
		{
			buf[size++] = 'a';
		}
		else if (mode.IsSet(Mode::Append) && mode.IsSet(Mode::Read))
		{
			buf[size++] = 'a';
			buf[size++] = '+';
		}
		else
		{
			R2_CHECK(false, "Unknown FileMode combination!");
		}

		//Always add at the end
		if (mode.IsSet(Mode::Binary))
		{
			buf[size++] = 'b';
		}

		buf[size] = '\0';
	}
}
