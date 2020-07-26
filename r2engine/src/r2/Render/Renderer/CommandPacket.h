#ifndef __COMMAND_PACKET_H__
#define __COMMAND_PACKET_H__

#include "r2/Render/Renderer/BackendDispatch.h"
#include "r2/Core/Memory/Memory.h"

using CommandPacket = void*;

namespace r2::draw::cmdpkt
{
	/*
	COMMAND PACKET STRUCTURE

	void* : a pointer to the next command packet (if any)
	BackendDispatchFunction : a pointer to the function responsible for dispatching the call to the backend
	T : the actual command
	char[] : auxiliary memory needed by the command (optional)

	*/

	static const size_t OFFSET_NEXT_COMMAND_PACKET = 0u;
	static const size_t OFFSET_BACKEND_DISPATCH_FUNCTION = OFFSET_NEXT_COMMAND_PACKET + sizeof(CommandPacket);
	static const size_t OFFSET_COMMAND = OFFSET_BACKEND_DISPATCH_FUNCTION + sizeof(r2::draw::dispatch::BackendDispatchFunction);

	template <typename T, class ARENA>
	CommandPacket Create(ARENA& arena, u64 auxMemorySize)
	{
		return ALLOC_BYTESN(arena, GetSize<T>(auxMemorySize), 16);
	}

	template <typename T>
	size_t GetSize(size_t auxMemorySize)
	{
		return OFFSET_COMMAND + sizeof(T) + auxMemorySize;
	};

	CommandPacket* GetNextCommandPacket(CommandPacket packet)
	{
		return reinterpret_cast<CommandPacket*>(reinterpret_cast<char*>(packet) + OFFSET_NEXT_COMMAND_PACKET);
	}

	template <typename T>
	CommandPacket* GetNextCommandPacket(T* command)
	{
		return reinterpret_cast<CommandPacket*>(reinterpret_cast<char*>(command) - OFFSET_COMMAND + OFFSET_NEXT_COMMAND_PACKET);
	}

	r2::draw::dispatch::BackendDispatchFunction* GetBackendDispatchFunction(CommandPacket packet)
	{
		return reinterpret_cast<r2::draw::dispatch::BackendDispatchFunction*>(reinterpret_cast<char*>(packet) + OFFSET_BACKEND_DISPATCH_FUNCTION);
	}

	template <typename T>
	T* GetCommand(CommandPacket packet)
	{
		return reinterpret_cast<T*>(reinterpret_cast<char*>(packet) + OFFSET_COMMAND);
	}

	template <typename T>
	char* GetAuxiliaryMemory(T* command)
	{
		return reinterpret_cast<char*>(command) + sizeof(T);
	}

	void StoreNextCommandPacket(CommandPacket packet, CommandPacket nextPacket)
	{
		*cmdpkt::GetNextCommandPacket(packet) = nextPacket;
	}

	template <typename T>
	void StoreNextCommandPacket(T* command, CommandPacket nextPacket)
	{
		*cmdpkt::GetNextCommandPacket<T>(command) = nextPacket;
	}

	void StoreBackendDispatchFunction(CommandPacket packet, r2::draw::dispatch::BackendDispatchFunction dispatchFunction)
	{
		*cmdpkt::GetBackendDispatchFunction(packet) = dispatchFunction;
	}

	const CommandPacket LoadNextCommandPacket(const CommandPacket packet)
	{
		return *GetNextCommandPacket(packet);
	}

	const r2::draw::dispatch::BackendDispatchFunction LoadBackendDispatchFunction(const CommandPacket packet)
	{
		return *GetBackendDispatchFunction(packet);
	}

	const void* LoadCommand(const CommandPacket packet)
	{
		return reinterpret_cast<char*>(packet) + OFFSET_COMMAND;
	}
}


#endif // __COMMAND_PACKET_H__
