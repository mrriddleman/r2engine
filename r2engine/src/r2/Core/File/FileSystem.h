//
//  FileSystem.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-15.
//

#ifndef FileSystem_h
#define FileSystem_h

#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/File/FileTypes.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/File/File.h"

namespace r2::fs
{
    class FileStorageArea;
    

    template <class ARENA>
    void* ReadFile(ARENA& arena, const char* filePath);
    
    class FileSystem
    {
    public:
        static const char* ALL_EXT;
        //Not to be used by application
        static bool Init(r2::mem::LinearArena& arena, u32 storageAreaCapacity);
        static void Mount(FileStorageArea& storageArea);
        static void Shutdown(r2::mem::LinearArena& arena);
        
        static File* Open(const DeviceConfig& config, const char* path, FileMode mode);
        static void Close(File* file);
        
        static bool FileExists(const char* path);
        static bool DeleteFile(const char* path);
        static bool RenameFile(const char* existingFile, const char* newName);
        static bool CopyFile(const char* existingFile, const char* newName);
        
        static bool DirExists(const char* path);
        static bool CreateDirectory(const char* path, bool recursive);
        static bool DeleteDirectory(const char* path);
        
        static void CreateFileListFromDirectory(const char* directory, const char* ext, r2::SArray<char[r2::fs::FILE_PATH_LENGTH]>* fileList);
        
    private:
        static void UnmountStorageAreas();
        static FileStorageArea* FindStorageArea(const char* path);
        static r2::SArray<FileStorageArea*>* mStorageAreas;
    };
}

namespace r2::fs
{
	template <class ARENA>
	void* ReadFile(ARENA& arena, const char* filePath, u64 &size)
	{
		r2::fs::File* theFile = r2::fs::FileSystem::Open(DISK_CONFIG, filePath, r2::fs::Read | r2::fs::Binary);

		if (theFile == nullptr)
		{
			R2_CHECK(false, "We couldn't open the file at the path: %s", filePath);
			return nullptr;
		}

		s64 fileSize = theFile->Size();

		if (fileSize <= 0)
		{
			R2_CHECK(false, "We have a zero byte file");
			r2::fs::FileSystem::Close(theFile);
			return nullptr;
		}

		char* buffer = (char*)ALLOC_BYTESN(arena, fileSize, 16);

		R2_CHECK(buffer != nullptr, "We couldn't allocate the buffer to read the file");

		bool readAllBytes = theFile->ReadAll(buffer);

		if (!readAllBytes)
		{
			R2_CHECK(false, "Failed to read all of the bytes!");
			FREE(buffer, arena);
			r2::fs::FileSystem::Close(theFile);
			return nullptr;
		}

		r2::fs::FileSystem::Close(theFile);

		return buffer;
	}

	template <class ARENA>
	void* ReadFile(ARENA& arena, const char* filePath)
	{
		u64 size = 0;
		return ReadFile<ARENA>(arena, filePath, size);
	}
}

#endif /* FileSystem_h */
