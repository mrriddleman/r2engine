#ifndef __COMMAND_BUCKET_H__
#define __COMMAND_BUCKER_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/BackendDispatch.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/RenderTarget.h"
#include <algorithm>

#define MAKE_CMD_BUCKET(arena, key, keydecoder, cameraupdate, capacity) r2::draw::cmdbkt::CreateCommandBucket<key>(arena, capacity, keydecoder, cameraupdate, __FILE__, __LINE__, "")

namespace r2
{
	struct Camera;
}

namespace r2::draw
{
	template <typename T>
	struct CommandBucket
	{
	public:
		typedef T Key;
		using KeyDecoderFunc = void (*)(const T&);
		using CameraUpdateFunc = void(*)(const r2::Camera & cam);

		struct Entry
		{
			Key aKey;
			void* data = nullptr;
			r2::draw::dispatch::BackendDispatchFunction func = nullptr;
		};

		static u64 MemorySize(u64 capacity);

		r2::Camera* camera = nullptr;
		RenderTarget renderTarget;
		r2::SArray<Entry>* entries = nullptr;
		KeyDecoderFunc KeyDecoder = nullptr;
		CameraUpdateFunc CamUpdate = nullptr;
	};

	namespace cmdbkt
	{
		//Make sure you use the same arena allocator for all commands -> we can only use Linear, Stack or free list arenas 
		//If we need something more flexible (like pools for different command types or something) then either
			//1) we store a pointer to the arena for each Entry
			//2) we circumvent the tracking of the allocation somehow
		template<typename T, class ARENA, class CMD> CMD* AddCommand(ARENA& arena, CommandBucket<T>& bkt, T key, u64 auxMem);
		template<typename T, class ARENA> void ClearAll(ARENA& arena, CommandBucket<T>& bkt);

		template<typename T> using CompareFunction = bool (*)(const T & a, const T & b);
		template<typename T> inline void Sort(CommandBucket<T>& bkt, CompareFunction<T> cmp);

		template<typename T> void Submit(CommandBucket<T>& bkt);

		template<typename T> void SetCamera(CommandBucket<T>& bkt, Camera* camera);
		template<typename T> Camera* GetCamera(CommandBucket<T>& bkt);

		//@TODO(Serge): implement methods for setting Render Targets

		template<typename T> inline u64 NumEntries(const CommandBucket<T>& bkt);
		template<typename T> inline u64 Capacity(const CommandBucket<T>& bkt);

		template<typename T> inline T* Begin(CommandBucket<T>& arr);
		template<typename T> inline const T* Begin(const CommandBucket<T>& arr);
		template<typename T> inline T* End(CommandBucket<T>& arr);
		template<typename T> inline const T* End(const CommandBucket<T>& arr);

		template<typename T, class ARENA> CommandBucket<T>* CreateCommandBucket(ARENA& arena, u64 capacity, typename CommandBucket<T>::KeyDecoderFunc decoderFunc, typename CommandBucket<T>::CameraUpdateFunc camFunc, const char* file, s32 line, const char* description);
	}

	namespace cmdbkt
	{

		template<typename T, class ARENA, class CMD> CMD* AddCommand(ARENA& arena, CommandBucket<T>& bkt, T key, u64 auxMem)
		{
			CMD* newCmd = (CMD*)ALLOC(CMD, arena);

			CommandBucket<T>::Entry newEntry;

			newEntry.aKey = key;
			newEntry.data = newCmd;
			newEntry.func = CMD::DispatchFunc;

			r2::sarr::Push(*bkt.entries, newEntry);

			return newCmd;
		}

		template<typename T, class ARENA> inline void ClearAll(ARENA& arena, CommandBucket<T>& bkt)
		{
			R2_CHECK(bkt.entries != nullptr, "entries is nullptr!");

			const u64 numEntries = r2::sarr::Size(*bkt.entries);

			for (u64 i = numEntries-1; i >= 0; --i)
			{
				FREE(r2::sarr::At(*bkt.entries, i).data, arena);
			}

			r2::sarr::Clear(*bkt.entries);
		}

		template<typename T> void Submit(CommandBucket<T>& bkt)
		{
			const u64 numEntries = r2::sarr::Size(*bkt.entries);

			R2_CHECK(bkt.CamUpdate != nullptr, "We don't have a function to update the camera!");
			R2_CHECK(bkt.camera != nullptr, "We don't have a camera set!");

			bkt.CamUpdate(*bkt.camera);

			//@TODO(Serge): implement the render target

			for (u64 i = 0; i < numEntries; ++i)
			{
				void* cmd = r2::sarr::At(*bkt.entries, i).data;

				R2_CHECK(cmd != nullptr, "We don't have a valid command!");

				T theKey = r2::sarr::At(*bkt.entries, i).aKey;
				
				R2_CHECK(bkt.KeyDecoder != nullptr, "We don't have a key decoder!");

				bkt.KeyDecoder(theKey);

				r2::draw::dispatch::BackendDispatchFunction DispatchFunc = r2::sarr::At(*bkt.entries, i).func;

				DispatchFunc(cmd);
			}
		}

		template<typename T> void SetCamera(CommandBucket<T>& bkt, Camera* camera)
		{
			bkt.camera = camera;
		}

		template<typename T> Camera* GetCamera(CommandBucket<T>& bkt)
		{
			return bkt.camera;
		}

		template<typename T> inline u64 NumEntries(const CommandBucket<T>& bkt)
		{
			R2_CHECK(bkt.entries != nullptr, "entries is nullptr!");
			return r2::sarr::Size(*bkt.entries);
		}

		template<typename T> inline u64 Capacity(const CommandBucket<T>& bkt)
		{
			R2_CHECK(bkt.entries != nullptr, "entries is nullptr!");
			return r2::sarr::Capacity(*bkt.entries);
		}

		template<typename T> inline T* Begin(CommandBucket<T>& bkt)
		{
			R2_CHECK(bkt.entries != nullptr, "entries is nullptr!");
			return &r2::sarr::Begin(*bkt.entries)->aKey;
		}

		template<typename T> inline const T* Begin(const CommandBucket<T>& bkt)
		{
			R2_CHECK(bkt.entries != nullptr, "entries is nullptr!");
			return &r2::sarr::Begin(*bkt.entries)->aKey;
		}

		template<typename T> inline T* End(CommandBucket<T>& bkt)
		{
			R2_CHECK(bkt.entries != nullptr, "entries is nullptr!");
			return &r2::sarr::End(*bkt.entries)->aKey;
		}

		template<typename T> inline const T* End(const CommandBucket<T>& bkt)
		{
			R2_CHECK(bkt.entries != nullptr, "entries is nullptr!");
			return &r2::sarr::End(*bkt.entries)->aKey;
		}

		template<typename T> inline void Sort(CommandBucket<T>& bkt, CompareFunction<T> cmp)
		{
			R2_CHECK(bkt.entries != nullptr, "entries is nullptr!");
			std::sort(Begin(bkt), End(bkt), cmp);
		}

		template<typename T, class ARENA> CommandBucket<T>* CreateCommandBucket(ARENA& arena, u64 capacity, typename CommandBucket<T>::KeyDecoderFunc decoderFunc, typename CommandBucket<T>::CameraUpdateFunc camUpdateFunc, const char* file, s32 line, const char* description)
		{
			CommandBucket<T>* cmdBkt = new (ALLOC_BYTES(arena, CommandBucket<T>::MemorySize(capacity), alignof(u64), file, line, description)) CommandBucket<T>;

			r2::SArray<CommandBucket<T>::Entry>* startOfArray = new (r2::mem::utils::PointerAdd(cmdBkt, sizeof(CommandBucket<T>))) r2::SArray<CommandBucket<T>::Entry>();

			CommandBucket<T>::Entry* dataStart = (CommandBucket<T>::Entry*)r2::mem::utils::PointerAdd(startOfArray, sizeof(r2::SArray<CommandBucket<T>::Entry>));
			
			cmdBkt->entries = startOfArray;
			cmdBkt->entries->Create(dataStart, capacity);
			cmdBkt->KeyDecoder = decoderFunc;
			cmdBkt->CamUpdate = camUpdateFunc;
			return cmdBkt;
		}
	}

	template<typename T>
	u64 CommandBucket<T>::MemorySize(u64 capacity)
	{
		return sizeof(CommandBucket<T>) + r2::SArray<CommandBucket<T>::Entry>::MemorySize(capacity);
	}

}


#endif // !__COMMAND_BUCKET_H__

