#ifndef __COMMAND_PACKET_H__
#define __COMMAND_PACKET_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/BackendDispatch.h"

namespace r2::draw
{
	using CommandPacket = void*;
	/*
	COMMAND PACKET STRUCTURE

	void* : a pointer to the next command packet (if any)
	BackendDispatchFunction : a pointer to the function responsible for dispatching the call to the backend
	T : the actual command
	u32 : the number of bytes for the aux memory
	char[] : auxiliary memory needed by the command (optional)

	*/

	namespace
	{
		static const size_t OFFSET_NEXT_COMMAND_PACKET = 0u;
		static const size_t OFFSET_BACKEND_DISPATCH_FUNCTION = OFFSET_NEXT_COMMAND_PACKET + sizeof(r2::draw::CommandPacket);
		static const size_t OFFSET_COMMAND = OFFSET_BACKEND_DISPATCH_FUNCTION + sizeof(r2::draw::dispatch::BackendDispatchFunction);
	}

	namespace cmdpkt
	{
		template <typename T, class ARENA> inline CommandPacket Create(ARENA& arena, u64 auxMemorySize);

		template <typename T> inline size_t GetSize(size_t auxMemorySize);

		inline CommandPacket* GetNextCommandPacket(CommandPacket packet);

		template <typename T> inline CommandPacket* GetNextCommandPacket(T* command);

		inline r2::draw::dispatch::BackendDispatchFunction* GetBackendDispatchFunction(CommandPacket packet);

		template <typename T> inline T* GetCommand(CommandPacket packet);

		template <typename T> inline char* GetAuxiliaryMemory(T* command);

		inline void StoreNextCommandPacket(CommandPacket packet, CommandPacket nextPacket);

		template <typename T> inline void StoreNextCommandPacket(T* command, CommandPacket nextPacket);

		inline void StoreBackendDispatchFunction(CommandPacket packet, r2::draw::dispatch::BackendDispatchFunction dispatchFunction);

		inline const CommandPacket LoadNextCommandPacket(const CommandPacket packet);

		inline const r2::draw::dispatch::BackendDispatchFunction LoadBackendDispatchFunction(const CommandPacket packet);

		inline const void* LoadCommand(const CommandPacket packet);
	}


	namespace cmdpkt
	{
		template <typename T, class ARENA>
		inline CommandPacket Create(ARENA& arena, u64 auxMemorySize)
		{
			void* cmdPacket = ALLOC_BYTESN(arena, GetSize<T>(auxMemorySize), 16);

			u32* auxMemorySizePtr = reinterpret_cast<u32*>(reinterpret_cast<char*>(cmdPacket) + OFFSET_COMMAND + sizeof(T));

			*auxMemorySizePtr = static_cast<u32>(auxMemorySize);

			return cmdPacket;
		}

		template <typename T>
		inline size_t GetSize(size_t auxMemorySize)
		{
			return OFFSET_COMMAND + sizeof(T) + sizeof(u32) + auxMemorySize;
		};

		inline CommandPacket* GetNextCommandPacket(CommandPacket packet)
		{
			return reinterpret_cast<CommandPacket*>(reinterpret_cast<char*>(packet) + OFFSET_NEXT_COMMAND_PACKET);
		}

		template <typename T>
		inline CommandPacket* GetNextCommandPacket(T* command)
		{
			return reinterpret_cast<CommandPacket*>(reinterpret_cast<char*>(command) - OFFSET_COMMAND + OFFSET_NEXT_COMMAND_PACKET);
		}

		inline r2::draw::dispatch::BackendDispatchFunction* GetBackendDispatchFunction(CommandPacket packet)
		{
			return reinterpret_cast<r2::draw::dispatch::BackendDispatchFunction*>(reinterpret_cast<char*>(packet) + OFFSET_BACKEND_DISPATCH_FUNCTION);
		}

		template <typename T>
		inline T* GetCommand(CommandPacket packet)
		{
			return reinterpret_cast<T*>(reinterpret_cast<char*>(packet) + OFFSET_COMMAND);
		}

		template <typename T>
		inline char* GetAuxiliaryMemory(T* command)
		{
			char* auxMemSizeCharPtr = reinterpret_cast<char*>(command) + sizeof(T);

			u32* auxMemSizePtr = reinterpret_cast<u32*>(auxMemSizeCharPtr);

			R2_CHECK(auxMemSizePtr != nullptr, "auxMemSizePtr should never be null");
			R2_CHECK(*auxMemSizePtr > 0, "This must be greater than 0 if you're trying to use aux memory. Otherwise you'll crash the driver if you write to it");

			return reinterpret_cast<char*>(command) + sizeof(T) + sizeof(u32);
		}

		inline void StoreNextCommandPacket(CommandPacket packet, CommandPacket nextPacket)
		{
			*cmdpkt::GetNextCommandPacket(packet) = nextPacket;
		}

		template <typename T>
		inline void StoreNextCommandPacket(T* command, CommandPacket nextPacket)
		{
			*cmdpkt::GetNextCommandPacket<T>(command) = nextPacket;
		}

		inline void StoreBackendDispatchFunction(CommandPacket packet, r2::draw::dispatch::BackendDispatchFunction dispatchFunction)
		{
			*cmdpkt::GetBackendDispatchFunction(packet) = dispatchFunction;
		}

		inline const CommandPacket LoadNextCommandPacket(const CommandPacket packet)
		{
			return *GetNextCommandPacket(packet);
		}

		inline const r2::draw::dispatch::BackendDispatchFunction LoadBackendDispatchFunction(const CommandPacket packet)
		{
			return *GetBackendDispatchFunction(packet);
		}

		inline const void* LoadCommand(const CommandPacket packet)
		{
			return reinterpret_cast<char*>(packet) + OFFSET_COMMAND;
		}
	}
}

#endif // __COMMAND_PACKET_H__
