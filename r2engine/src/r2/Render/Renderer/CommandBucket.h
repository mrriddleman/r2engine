#ifndef __COMMAND_BUCKET_H__
#define __COMMAND_BUCKER_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/BackendDispatch.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/RenderTarget.h"
//
#include <algorithm>

#define MAKE_CMD_BUCKET(arena, key, keydecoder, capacity) r2::draw::cmdbkt::CreateCommandBucket<key>(arena, capacity, keydecoder, __FILE__, __LINE__, "")
#define FREE_CMD_BUCKET(arena, key, bkt) r2::draw::cmdbkt::DestroyCommandBucket<key>(arena, bkt, __FILE__, __LINE__, "")

namespace r2::draw
{
	template <typename T>
	struct CommandBucket
	{
	public:
		typedef T Key;
		using KeyDecoderFunc = void (*)(const T&);

		struct Entry
		{
			Key aKey;
			void* data = nullptr;
		};

		static u64 MemorySize(u64 capacity);

		r2::SArray<Entry>* entries = nullptr;
		r2::SArray<Entry*>* sortedEntries = nullptr;

	//	RenderTarget* noptrRenderTarget;
		KeyDecoderFunc KeyDecoder = nullptr;
	};

	namespace cmdbkt
	{
		//Make sure you use the same arena allocator for all commands -> we can only use Linear, Stack or free list arenas 
		//If we need something more flexible (like pools for different command types or something) then either
			//1) we store a pointer to the arena for each Entry
			//2) we circumvent the tracking of the allocation somehow
		template<typename T, class ARENA, class CMD> CMD* AddCommand(ARENA& arena, CommandBucket<T>& bkt, T key, u64 auxMem);
		template<class OLDCMD, class NEWCMD, class ARENA> NEWCMD* AppendCommand(ARENA& arena, OLDCMD* appendTo, u64 auxMem);
		template<typename T> void Close(CommandBucket<T>& bkt);


		template<typename T> void ClearAll(CommandBucket<T>& bkt);

		template<typename T> using CompareFunction = bool (*)(const T & a, const T & b);
		template<typename T> inline void Sort(CommandBucket<T>& bkt, CompareFunction<T> cmp);

		template<typename T> void Submit(CommandBucket<T>& bkt);

		//@TODO(Serge): implement methods for setting Render Targets

		template<typename T> inline u64 NumEntries(const CommandBucket<T>& bkt);
		template<typename T> inline u64 Capacity(const CommandBucket<T>& bkt);

		template<typename T, class ARENA> CommandBucket<T>* CreateCommandBucket(ARENA& arena, u64 capacity, typename CommandBucket<T>::KeyDecoderFunc decoderFunc, RenderTarget* rt, const char* file, s32 line, const char* description);
		template<typename T, class ARENA> void DestroyCommandBucket(ARENA& arena, CommandBucket<T>* cmdBkt, const char* file, s32 line, const char* description);
	}

	namespace cmdbkt
	{

		template<typename T, class ARENA, class CMD> CMD* AddCommand(ARENA& arena, CommandBucket<T>& bkt, T key, u64 auxMem)
		{
			CommandPacket packet = cmdpkt::Create<CMD, ARENA>(arena, auxMem);

			CommandBucket<T>::Entry newEntry;
			
			{
				newEntry.aKey = key;
				newEntry.data = packet;

				//this is where we would do any locking/atomics etc
				r2::sarr::Push(*bkt.entries, newEntry);
			}
			
			cmdpkt::StoreNextCommandPacket(packet, nullptr);
			cmdpkt::StoreBackendDispatchFunction(packet, CMD::DispatchFunc);

			return cmdpkt::GetCommand<CMD>(packet);
		}

		template<class OLDCMD, class NEWCMD, class ARENA> 
		NEWCMD* AppendCommand(ARENA& arena, OLDCMD* appendTo, u64 auxMem)
		{
			CommandPacket packet = cmdpkt::Create<NEWCMD, ARENA>(arena, auxMem);

			// append this command to the given one
			cmdpkt::StoreNextCommandPacket<OLDCMD>(appendTo, packet);

			cmdpkt::StoreNextCommandPacket(packet, nullptr);
			cmdpkt::StoreBackendDispatchFunction(packet, NEWCMD::DispatchFunc);

			return cmdpkt::GetCommand<NEWCMD>(packet);
		}

		template<typename T> void Close(CommandBucket<T>& bkt)
		{
			//nothing for now
		}

		template<typename T> inline void ClearAll(CommandBucket<T>& bkt)
		{
			R2_CHECK(bkt.entries != nullptr, "entries is nullptr!");

			const u64 numEntries = r2::sarr::Size(*bkt.entries);

			r2::sarr::Clear(*bkt.sortedEntries);
			r2::sarr::Clear(*bkt.entries);
		}

		void SubmitPacket(CommandPacket packet)
		{
			const dispatch::BackendDispatchFunction function = cmdpkt::LoadBackendDispatchFunction(packet);
			const void* command = cmdpkt::LoadCommand(packet);
			function(command);
		}

		template<typename T> void Submit(CommandBucket<T>& bkt)
		{
			//if (bkt.noptrRenderTarget)
			//{
			//	rt::SetRenderTarget(*bkt.noptrRenderTarget);
			//}

			const u64 numEntries = r2::sarr::Size(*bkt.entries);

			for (u64 i = 0; i < numEntries; ++i)
			{
				CommandPacket packet = r2::sarr::At(*bkt.sortedEntries, i)->data;

				R2_CHECK(packet != nullptr, "We don't have a valid command!");

				T theKey = r2::sarr::At(*bkt.sortedEntries, i)->aKey;
				
				R2_CHECK(bkt.KeyDecoder != nullptr, "We don't have a key decoder!");

				bkt.KeyDecoder(theKey);

				do 
				{
					SubmitPacket(packet);
					packet = cmdpkt::LoadNextCommandPacket(packet);
				} while (packet != nullptr);
			}
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

		template<typename T> inline void Sort(CommandBucket<T>& bkt, CompareFunction<T> cmp)
		{
			R2_CHECK(bkt.entries != nullptr, "entries is nullptr!");

			const u64 numEntries = r2::sarr::Size(*bkt.entries);

			for (u64 i = 0; i < numEntries; ++i)
			{
				r2::sarr::Push(*bkt.sortedEntries, & r2::sarr::At(*bkt.entries, i));
			}

			std::sort(r2::sarr::Begin(*bkt.sortedEntries), r2::sarr::End(*bkt.sortedEntries), [cmp](const CommandBucket<T>::Entry* a, const CommandBucket<T>::Entry* b)
				{
					return cmp(a->aKey, b->aKey);
				});
		}

		template<typename T, class ARENA> CommandBucket<T>* CreateCommandBucket(ARENA& arena, u64 capacity, typename CommandBucket<T>::KeyDecoderFunc decoderFunc, const char* file, s32 line, const char* description)
		{
			CommandBucket<T>* cmdBkt = new (ALLOC_BYTES(arena, CommandBucket<T>::MemorySize(capacity), alignof(u64), file, line, description)) CommandBucket<T>;

			r2::SArray<CommandBucket<T>::Entry>* startOfArray = new (r2::mem::utils::PointerAdd(cmdBkt, sizeof(CommandBucket<T>))) r2::SArray<CommandBucket<T>::Entry>();

			CommandBucket<T>::Entry* dataStart = (CommandBucket<T>::Entry*)r2::mem::utils::PointerAdd(startOfArray, sizeof(r2::SArray<CommandBucket<T>::Entry>));
			
			r2::SArray<CommandBucket<T>::Entry*>* startOfSortedArray = new (r2::mem::utils::PointerAdd(dataStart, sizeof(CommandBucket<T>::Entry) * capacity)) r2::SArray<CommandBucket<T>::Entry*>();

			CommandBucket<T>::Entry** startOfSortedData = (typename CommandBucket<T>::Entry**)r2::mem::utils::PointerAdd(startOfSortedArray, sizeof(r2::SArray<CommandBucket<T>::Entry*>));


			cmdBkt->entries = startOfArray;
			cmdBkt->entries->Create(dataStart, capacity);

			cmdBkt->sortedEntries = startOfSortedArray;
			cmdBkt->sortedEntries->Create(startOfSortedData, capacity);


			cmdBkt->KeyDecoder = decoderFunc;

			return cmdBkt;
		}

		template<typename T, class ARENA> void DestroyCommandBucket(ARENA& arena, CommandBucket<T>* cmdBkt, const char* file, s32 line, const char* description)
		{
			if (cmdBkt)
			{
				FREE(cmdBkt, arena);

			}
		}

	}

	template<typename T>
	u64 CommandBucket<T>::MemorySize(u64 capacity)
	{
		return sizeof(CommandBucket<T>) +
			r2::SArray<CommandBucket<T>::Entry>::MemorySize(capacity) +
			r2::SArray<CommandBucket<T>::Entry*>::MemorySize(capacity);
	}

}


#endif // !__COMMAND_BUCKET_H__

