#include "r2pch.h"
#if defined(R2_PLATFORM_WINDOWS)

#pragma warning( disable : 4996 )

#include "r2/Core/File/FileDevices/Storage/Disk/DiskFile.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFileUtils.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::fs
{
	DiskFile::DiskFile()
		: mHandle(nullptr)
	{

	}

	DiskFile::~DiskFile()
	{
		if (IsOpen())
		{
			Close();
		}
	}

	bool DiskFile::Open(const char* path, FileMode mode)
	{
		R2_CHECK(path != nullptr, "Null Path!");
		R2_CHECK(path != "", "Empty path!");

		R2_CHECK(mHandle == nullptr, "We shouldn't have any handle yet?");
		if (IsOpen())
		{
			return true;
		}

		char strFileMode[4];
		utils::GetStringFileMode(strFileMode, mode);

		mHandle = fopen(path, strFileMode);

		if (mHandle)
		{
			SetFilePath(path);
			SetFileMode(mode);
		}

		return mHandle != nullptr;
	}

	void DiskFile::Close()
	{
		R2_CHECK(IsOpen(), "Trying to close a closed file?");

		fclose((FILE*)mHandle);

		mHandle = nullptr;
		SetFileDevice(nullptr);
	}

	u64 DiskFile::Read(void* buffer, u64 length)
	{
		R2_CHECK(IsOpen(), "The file isn't open?");
		R2_CHECK(buffer != nullptr, "The buffer is null?");
		R2_CHECK(length > 0, "You should want to read more than 0 bytes!");

		if (IsOpen() && buffer != nullptr && length > 0)
		{
			return fread(buffer, sizeof(byte), length, (FILE*)mHandle);
		}
		
		return 0;
	}

	u64 DiskFile::Read(void* buffer, u64 offset, u64 length)
	{
		R2_CHECK(IsOpen(), "The file isn't open?");
		R2_CHECK(buffer != nullptr, "The buffer is null?");
		R2_CHECK(length > 0, "You should want to read more than 0 bytes!");

		if (IsOpen() && buffer != nullptr && length > 0 && static_cast<s64>(offset + length) <= Size())
		{
			Seek(offset);
			return Read(buffer, length);
		}

		return 0;
	}

	u64 DiskFile::Write(const void* buffer, u64 length)
	{
		R2_CHECK(IsOpen(), "The file isn't open?");
		R2_CHECK(buffer != nullptr, "nullptr buffer?");
		R2_CHECK(length > 0, "The length is 0?");

		if (IsOpen() && buffer != nullptr && length > 0)
		{
			u64 numBytesWritten = fwrite(buffer, sizeof(byte), length, (FILE*)mHandle);
			R2_CHECK(numBytesWritten == length, "We couldn't write out all %llu bytes\n", length);
			return numBytesWritten;
		}

		return 0;
	}

	bool DiskFile::ReadAll(void* buffer)
	{
		R2_CHECK(IsOpen(), "The file isn't open?");
		R2_CHECK(buffer != nullptr, "nullptr buffer?");

		Seek(0);
		const u64 size = Size();
		u64 bytesRead = Read(buffer, size);

		R2_CHECK(bytesRead == size, "We should have read the whole file!");
		return bytesRead == size;
	}

	void DiskFile::Seek(u64 position)
	{
		R2_CHECK(IsOpen(), "The file should be open");
		int ret = fseek((FILE*)mHandle, (long)position, SEEK_SET);
		R2_CHECK(ret == 0, "We couldn't seek correctly!");
	}

	void DiskFile::SeekToEnd(void)
	{
		R2_CHECK(IsOpen(), "The file should be open");
		int ret = fseek((FILE*)mHandle, 0, SEEK_END);
		R2_CHECK(ret == 0, "We couldn't seek correctly!");
	}

	void DiskFile::Skip(u64 bytes)
	{
		R2_CHECK(IsOpen(), "The file should be open");
		int ret = fseek((FILE*)mHandle, (long)bytes, SEEK_CUR);
		R2_CHECK(ret == 0, "We couldn't seek correctly!");
	}

	s64 DiskFile::Tell(void) const
	{
		R2_CHECK(IsOpen(), "The file should be open");
		return ftell((FILE*)mHandle);
	}

	bool DiskFile::IsOpen() const
	{
		return mHandle != nullptr;
	}

	s64 DiskFile::Size() const
	{
		R2_CHECK(IsOpen(), "The file should be open");

		fpos_t pos;
		int ret = fgetpos((FILE*)mHandle, &pos);
		R2_CHECK(ret == 0, "We couldn't get the current position");
		
		ret = fseek((FILE*)mHandle, 0, SEEK_END);
		R2_CHECK(ret == 0, "We couldn't seek to the end of the file");

		u64 size = Tell();

		ret = fseek((FILE*)mHandle, (long)pos, SEEK_SET);
		R2_CHECK(ret == 0, "We couldn't seek to the original position of the file");

		return size;
	}
}


#endif