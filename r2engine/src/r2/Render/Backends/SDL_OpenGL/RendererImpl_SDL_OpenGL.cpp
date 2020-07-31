#include "r2pch.h"

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Render/Backends/SDL_OpenGL/OpenGLRingBuffer.h"
#include "r2/Render/Renderer/RendererImpl.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Camera/Camera.h"
#include "r2/Render/Model/Material.h"
#include "r2/Render/Renderer/Shader.h"
#include "r2/Render/Renderer/ShaderSystem.h"
#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Render/Renderer/Commands.h"

#include <SDL2/SDL.h>
#include "glad/glad.h"
#include "r2/Core/Containers/SHashMap.h"

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"

namespace
{
	SDL_Window* s_optrWindow = nullptr;
	SDL_GLContext s_glContext;

	struct ConstantBufferPostRenderUpdate
	{
		r2::draw::ConstantBufferHandle handle = 0;
		u64 size = 0;
	};

	struct RendererImplState
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;
		r2::SHashMap<r2::draw::rendererimpl::RingBuffer>* mRingBufferMap = nullptr;
		//@TODO(Serge): remove this
		r2::SArray<ConstantBufferPostRenderUpdate>* mConstantBuffersToUpdatePostRender = nullptr;
	};

	RendererImplState* s_optrRendererImpl = nullptr;

	static GLenum ShaderDataTypeToOpenGLBaseType(r2::draw::ShaderDataType type)
	{
		switch (type)
		{
		case r2::draw::ShaderDataType::Float:    return GL_FLOAT;
		case r2::draw::ShaderDataType::Float2:   return GL_FLOAT;
		case r2::draw::ShaderDataType::Float3:   return GL_FLOAT;
		case r2::draw::ShaderDataType::Float4:   return GL_FLOAT;
		case r2::draw::ShaderDataType::Mat3:     return GL_FLOAT;
		case r2::draw::ShaderDataType::Mat4:     return GL_FLOAT;
		case r2::draw::ShaderDataType::Int:      return GL_INT;
		case r2::draw::ShaderDataType::Int2:     return GL_INT;
		case r2::draw::ShaderDataType::Int3:     return GL_INT;
		case r2::draw::ShaderDataType::Int4:     return GL_INT;
		case r2::draw::ShaderDataType::Bool:     return GL_BOOL;
		case r2::draw::ShaderDataType::None:     break;
		}

		R2_CHECK(false, "Unknown ShaderDataType");
		return 0;
	}
}

namespace r2::draw
{
	u32 VertexDrawTypeStatic = GL_STATIC_DRAW;
	u32 VertexDrawTypeStream = GL_STREAM_DRAW;
	u32 VertexDrawTypeDynamic = GL_DYNAMIC_DRAW;

	u32 CB_FLAG_MAP_PERSISTENT = GL_MAP_PERSISTENT_BIT;
	u32 CB_FLAG_MAP_COHERENT = GL_MAP_COHERENT_BIT;
	u32 CB_FLAG_WRITE = GL_MAP_WRITE_BIT;
	u32 CB_FLAG_READ = GL_MAP_READ_BIT;

	u32 CB_CREATE_FLAG_DYNAMIC_STORAGE = GL_DYNAMIC_STORAGE_BIT;

	const u64 ALIGNMENT = 16;
	const f64 HASH_MULT = 1.5;
	const u32 RING_BUFFER_MULT = 3;
}

namespace r2::draw::cmd
{
	u32 CLEAR_COLOR_BUFFER = GL_COLOR_BUFFER_BIT;
	u32 CLEAR_DEPTH_BUFFER = GL_DEPTH_BUFFER_BIT;
}

namespace r2::draw::rendererimpl
{
	//basic stuff
	bool PlatformInit(const PlatformRendererSetupParams& params)
	{
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

		SDL_GL_LoadLibrary(nullptr);

		s_optrWindow = SDL_CreateWindow(params.windowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, params.resolution.width, params.resolution.height, params.platformFlags);

		R2_CHECK(s_optrWindow != nullptr, "We should have a window pointer!");

		s_glContext = SDL_GL_CreateContext(s_optrWindow);

		R2_CHECK(s_glContext != nullptr, "We should have an OpenGL context!");

		gladLoadGLLoader(SDL_GL_GetProcAddress);
		
		if (params.flags.IsSet(VSYNC))
		{
			SDL_GL_SetSwapInterval(1);
		}
		else
		{
			SDL_GL_SetSwapInterval(0);
		}

		return s_optrWindow && s_glContext;
	}

	void SetWindowName(const char* windowName)
	{
		if (s_optrWindow)
		{
			SDL_SetWindowTitle(s_optrWindow, windowName);
		}
	}

	bool RendererImplInit(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 numRingBuffers, u64 maxRingLocks, const char* systemName)
	{
		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = MemorySize(numRingBuffers * HASH_MULT, maxRingLocks);
		if (memoryArea->UnAllocatedSpace() < subAreaSize)
		{
			R2_CHECK(false, "We don't have enought space to allocate the renderer!");
			return false;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		if ((subAreaHandle = memoryArea->AddSubArea(subAreaSize, systemName)) == r2::mem::MemoryArea::SubArea::Invalid)
		{
			R2_CHECK(false, "We couldn't create a sub area for the engine model area");
			return false;
		}

		//emplace the linear arena
		r2::mem::LinearArena* linearArena = EMPLACE_LINEAR_ARENA(*memoryArea->GetSubArea(subAreaHandle));

		R2_CHECK(linearArena != nullptr, "We couldn't emplace the linear arena - no way to recover!");

		if (!linearArena)
		{
			R2_CHECK(linearArena != nullptr, "materialSystemsArena is null");
			return false;
		}

		s_optrRendererImpl = ALLOC(RendererImplState, *linearArena);

		R2_CHECK(s_optrRendererImpl != nullptr, "We couldn't allocate s_optrRendererImpl!");

		s_optrRendererImpl->mMemoryAreaHandle = memoryAreaHandle;
		s_optrRendererImpl->mSubAreaHandle = subAreaHandle;
		s_optrRendererImpl->mSubAreaArena = linearArena;
		s_optrRendererImpl->mRingBufferMap = MAKE_SHASHMAP(*linearArena, RingBuffer, numRingBuffers* HASH_MULT);
		s_optrRendererImpl->mConstantBuffersToUpdatePostRender = MAKE_SARRAY(*linearArena, ConstantBufferPostRenderUpdate, numRingBuffers);

		for (u64 i = 0; i < numRingBuffers; ++i)
		{
			ConstantBufferPostRenderUpdate update = { 0, 0 };
			r2::sarr::Push(*s_optrRendererImpl->mConstantBuffersToUpdatePostRender, update);
		}

		return s_optrRendererImpl->mRingBufferMap != nullptr;
	}

	u64 MemorySize(u64 numConstantBuffers, u64 maxNumRingLocks)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u64 memSize1 = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), ALIGNMENT, headerSize, boundsChecking);
		u64 memSize2 = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(RendererImplState), ALIGNMENT, headerSize, boundsChecking);
		u64 memSize3 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<RingBuffer>::MemorySize(numConstantBuffers), ALIGNMENT, headerSize, boundsChecking);
		u64 memSize4 = ringbuf::MemorySize(headerSize, boundsChecking, ALIGNMENT, maxNumRingLocks) * numConstantBuffers;
		u64 memSize5 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ConstantBufferPostRenderUpdate>::MemorySize(numConstantBuffers), ALIGNMENT, headerSize, boundsChecking);
		
		return memSize1 + memSize2 + memSize3 + memSize4 + memSize5;
	}

	void* GetRenderContext()
	{
		return s_glContext;
	}

	void* GetWindowHandle()
	{
		return s_optrWindow;
	}

	void SwapScreens()
	{
		SDL_GL_SwapWindow(s_optrWindow);
	}

	void Shutdown()
	{
		r2::mem::LinearArena* arena = s_optrRendererImpl->mSubAreaArena;

		auto hashMapEntryItr = r2::shashmap::Begin(*s_optrRendererImpl->mRingBufferMap);
		auto hashMapEnd = r2::shashmap::End(*s_optrRendererImpl->mRingBufferMap);
		for (; hashMapEntryItr != hashMapEnd; hashMapEntryItr++)
		{
			ringbuf::DestroyRingBuffer<r2::mem::LinearArena>(*arena, hashMapEntryItr->value);
		}

		FREE(s_optrRendererImpl->mRingBufferMap, *arena);
		FREE(s_optrRendererImpl->mConstantBuffersToUpdatePostRender, *arena);

		FREE(s_optrRendererImpl, *arena);

		FREE_EMPLACED_ARENA(arena);

		SDL_GL_DeleteContext(s_glContext);

		if (s_optrWindow)
		{
			SDL_DestroyWindow(s_optrWindow);
		}
	}

	s32 MaxNumberOfTextureUnits()
	{
		s32 texture_units;
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texture_units);
		return texture_units;
	}

	u32 MaxConstantBufferSize()
	{
		GLint cBufferSize;
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &cBufferSize);
		return cBufferSize;
	}

	u32 MaxConstantBufferPerShaderType()
	{
		GLint numVertexConstantBuffers;
		GLint numFragmentConstantBuffers;
		glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &numVertexConstantBuffers);
		glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &numFragmentConstantBuffers);

		R2_CHECK(numVertexConstantBuffers == numFragmentConstantBuffers, "These aren't the same?");

		return numFragmentConstantBuffers > numVertexConstantBuffers? numVertexConstantBuffers : numFragmentConstantBuffers;
	}

	u32 MaxConstantBindings()
	{
		GLint numBindings;
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &numBindings);
		return numBindings;
	}

	//Setup code
	void SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void GenerateBufferLayouts(u32 numBufferLayouts, u32* layoutIds)
	{
		glGenVertexArrays(numBufferLayouts, layoutIds);
	}

	void GenerateVertexBuffers(u32 numVertexBuffers, u32* bufferIds)
	{
		glGenBuffers(numVertexBuffers, bufferIds);
	}

	void GenerateIndexBuffers(u32 numIndexBuffers, u32* indexIds)
	{
		glGenBuffers(numIndexBuffers, indexIds);
	}

	void GenerateContantBuffers(u32 numConstantBuffers, u32* contantBufferIds)
	{
		glGenBuffers(numConstantBuffers, contantBufferIds);
	}

	void DeleteBuffers(u32 numBuffers, u32* bufferIds)
	{
		glDeleteBuffers(numBuffers, bufferIds);
	}

	void SetDepthTest(bool shouldDepthTest)
	{
		if (shouldDepthTest)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}

	void SetupBufferLayoutConfiguration(const BufferLayoutConfiguration& config, BufferLayoutHandle layoutId, VertexBufferHandle vertexBufferId, IndexBufferHandle indexBufferId, DrawIDHandle drawId)
	{
		glBindVertexArray(layoutId);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
		glBufferData(GL_ARRAY_BUFFER, config.vertexBufferConfig.bufferSize, nullptr, config.vertexBufferConfig.drawType);

		if (indexBufferId != 0 && config.indexBufferConfig.bufferSize != EMPTY_BUFFER)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, config.indexBufferConfig.bufferSize, nullptr, config.indexBufferConfig.drawType);
		
		}
		
		u32 vertexAttribId = 0;
		for (const auto& element : config.layout)
		{
			glEnableVertexAttribArray(vertexAttribId);

			if (element.type >= ShaderDataType::Float && element.type <= ShaderDataType::Mat4)
			{
				glVertexAttribPointer(
					vertexAttribId, 
					element.GetComponentCount(), 
					ShaderDataTypeToOpenGLBaseType(element.type), 
					element.normalized ? GL_TRUE : GL_FALSE, 
					config.layout.GetStride(), 
					(const void*)element.offset);
			}
			else if (element.type >= ShaderDataType::Int && element.type <= ShaderDataType::Int4)
			{
				glVertexAttribIPointer(
					vertexAttribId,
					element.GetComponentCount(),
					ShaderDataTypeToOpenGLBaseType(element.type),
					config.layout.GetStride(),
					(const void*)element.offset);
			}

			if (config.layout.GetVertexType() == VertexType::Instanced)
			{
				glVertexAttribDivisor(vertexAttribId, 1);
			}

			++vertexAttribId;
		}

		if (config.useDrawIDs && drawId != EMPTY_BUFFER)
		{
			r2::SArray<u32>* drawIdData = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u32, config.maxDrawCount);
			for (u32 i = 0; i < config.maxDrawCount; ++i)
			{
				r2::sarr::Push(*drawIdData, i);
			}

			glBindBuffer(GL_ARRAY_BUFFER, drawId);
			glBufferData(GL_ARRAY_BUFFER, config.maxDrawCount * sizeof(u32), drawIdData->mData, GL_STATIC_DRAW);
			glVertexAttribIPointer(vertexAttribId, 1, GL_UNSIGNED_INT, sizeof(u32), 0);
			glVertexAttribDivisor(vertexAttribId, 1);
			glEnableVertexAttribArray(vertexAttribId);

			FREE(drawIdData, *MEM_ENG_SCRATCH_PTR);
		}

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		if (config.indexBufferConfig.bufferSize != EMPTY_BUFFER)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	}

	void SetupConstantBufferConfigs(const r2::SArray<ConstantBufferLayoutConfiguration>* configs, ConstantBufferHandle* handles)
	{
		if (configs == nullptr)
		{
			R2_CHECK(configs != nullptr, "configs is nullptr!");
			return;
		}

		if (handles == nullptr)
		{
			R2_CHECK(handles != nullptr, "handles is nullptr!");
			return;
		}
		
		const u64 numConfigs = r2::sarr::Size(*configs);
		//@TODO(Serge): this is wrong since now we can have multiple types of constant buffers - re-think
		auto maxBindings = r2::draw::rendererimpl::MaxConstantBindings();

		if (maxBindings < numConfigs)
		{
			R2_CHECK(false, "You're trying to create more constant buffers than we have bindings!");
			return;
		}

		GLint uboIndex = 0;
		GLint ssboIndex = 0;
		GLint drawCMDIndex = 0;
		GLint* nextIndex = nullptr;
		GLint bufferType;

		for (u64 i = 0; i < numConfigs; i++)
		{
			const r2::draw::ConstantBufferLayoutConfiguration& config = r2::sarr::At(*configs, i);
			 
			if (config.layout.GetType() == ConstantBufferLayout::Type::Small)
			{
				bufferType = GL_UNIFORM_BUFFER;
				nextIndex = &uboIndex;
			}
			else if(config.layout.GetType() == ConstantBufferLayout::Type::Big)
			{
				bufferType = GL_SHADER_STORAGE_BUFFER;
				nextIndex = &ssboIndex;
			}
			else
			{
				bufferType = GL_DRAW_INDIRECT_BUFFER;
				nextIndex = &drawCMDIndex;
			}

			glBindBuffer(bufferType, handles[i]);
			
			
			if (config.layout.GetFlags().IsSet(CB_FLAG_MAP_PERSISTENT))
			{
			//	if (bufferType == GL_DRAW_INDIRECT_BUFFER)
				{

					glBufferStorage(bufferType, config.layout.GetSize() * RING_BUFFER_MULT, nullptr, config.layout.GetFlags().GetRawValue() | config.layout.GetCreateFlags().GetRawValue());
					void* ptr = glMapBufferRange(bufferType, 0, config.layout.GetSize() * RING_BUFFER_MULT, config.layout.GetFlags().GetRawValue());

					RingBuffer newRingBuffer;
					ringbuf::CreateRingBuffer<r2::mem::LinearArena>(*s_optrRendererImpl->mSubAreaArena, newRingBuffer, handles[i], bufferType, config.layout.begin()->elementSize, (*nextIndex)++, config.layout.GetFlags(), ptr, config.layout.begin()->typeCount * RING_BUFFER_MULT);
					r2::shashmap::Set(*s_optrRendererImpl->mRingBufferMap, handles[i], newRingBuffer);

					
				}
				//else
				//{
				//	glBufferData(bufferType, config.layout.GetSize(), nullptr, config.drawType);
				//	glBindBufferBase(bufferType, (*nextIndex)++, handles[i]);

				//}

				//@NOTE: right now we'll only support a constant buffer with 1 entry
				//		(could be an array though) for this type
			//	
				
			}
			else
			{
				glBufferData(bufferType, config.layout.GetSize(), nullptr, config.drawType);
				glBindBufferBase(bufferType, (*nextIndex)++, handles[i]);
			}
		}

		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	}
	
	void SetViewport(u32 viewport)
	{
		//@TODO(Serge): implement
	}

	void SetViewportLayer(u32 viewportLayer)
	{
		//@TODO(Serge): implement
	}

	void SetMaterialID(r2::draw::MaterialHandle materialID)
	{
		if (materialID.handle == r2::draw::mat::InvalidMaterial.handle)
		{
			R2_CHECK(false, "Passed in invalid material handle!");
		}

		r2::draw::MaterialSystem* matSystem = r2::draw::matsys::GetMaterialSystem(materialID.slot);
		R2_CHECK(matSystem != nullptr, "Failed to get the material system!");

		const Material* material = mat::GetMaterial(*matSystem, materialID);

		if (material)
		{			
			const Shader* shader = r2::draw::shadersystem::GetShader(material->shaderId);

			//@TODO(Serge): check current opengl state first
			r2::draw::shader::Use(*shader);
			r2::draw::shader::SetVec4(*shader, "material.color", material->color);

			const r2::SArray<r2::draw::tex::Texture>* textures = r2::draw::mat::GetTexturesForMaterial(*matSystem, materialID);
			if (textures)
			{
				u64 numTextures = r2::sarr::Size(*textures);

				const u32 numTextureTypes = r2::draw::tex::TextureType::NUM_TEXTURE_TYPES;
				u32 textureNum[numTextureTypes];
				for (u32 i = 0; i < numTextureTypes; ++i)
				{
					textureNum[i] = 1;
				}

				for (u64 i = 0; i < numTextures; ++i)
				{
					const r2::draw::tex::Texture& texture = r2::sarr::At(*textures, i);

					glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
					std::string number;
					std::string name = TextureTypeToString(texture.type);
					number = std::to_string(textureNum[texture.type]++);
					r2::draw::shader::SetInt(*shader, ("material." + name + number).c_str(), static_cast<s32>(i));
					glBindTexture(GL_TEXTURE_2D, r2::draw::texsys::GetGPUHandle(texture.textureAssetHandle));
				}
			}
		}
	}

	//draw functions
	void Clear(u32 flags)
	{
		glClear(flags);
	}

	void BindVertexArray(BufferLayoutHandle layoutId)
	{
		//@TODO(Serge): check the currently bound layoutId first
		glBindVertexArray(layoutId);
	}

	void DrawIndexed(BufferLayoutHandle layoutId, VertexBufferHandle vBufferHandle, IndexBufferHandle iBufferHandle, u32 numIndices, u32 startingIndex)
	{
		//@TODO(Serge): make sure we don't set the buffers array/buffer/index if they are already set to the correct value!
		//We need a system to track all of the state changes!
		BindVertexArray(layoutId);

	//	glBindBuffer(GL_ARRAY_BUFFER, vBufferHandle);

	//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iBufferHandle);

		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, (const void*)(startingIndex * sizeof(u32)));
	}

	void DrawIndexedCommands(BufferLayoutHandle layoutId, ConstantBufferHandle batchHandle, void* cmds, u32 count, u32 stride)
	{
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batchHandle);

		//glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batchHandle);
		//glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, count* sizeof(r2::draw::cmd::DrawBatchSubCommand), cmds);

		RingBuffer theDefault;
		RingBuffer& ringBuffer = r2::shashmap::Get(*s_optrRendererImpl->mRingBufferMap, batchHandle, theDefault);
		R2_CHECK(ringBuffer.dataPtr != theDefault.dataPtr, "Failed to get the ring buffer!");

		size_t totalSize = count * ringBuffer.typeSize;
		void* ptr = ringbuf::Reserve(ringBuffer, count);
		memcpy(ptr, cmds, totalSize);

		if (!ringBuffer.flags.IsSet(CB_FLAG_MAP_COHERENT))
		{
			glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		}

		BindVertexArray(layoutId);
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, ringbuf::GetHeadOffset(ringBuffer), count, stride);
		ringbuf::Complete(ringBuffer, count);
	}

	void UpdateVertexBuffer(VertexBufferHandle vBufferHandle, u64 offset, void* data, u64 size)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vBufferHandle);
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
	}

	void UpdateIndexBuffer(IndexBufferHandle iBufferHandle, u64 offset, void* data, u64 size)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iBufferHandle);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
	}

	void UpdateConstantBuffer(ConstantBufferHandle cBufferHandle, r2::draw::ConstantBufferLayout::Type type, b32 isPersistent, u64 offset, void* data, u64 size)
	{
		GLenum bufferType;

		if (type == ConstantBufferLayout::Small)
		{
			bufferType = GL_UNIFORM_BUFFER;
		}
		else if(type == ConstantBufferLayout::Big)
		{
			bufferType = GL_SHADER_STORAGE_BUFFER;
		}
		else
		{
			R2_CHECK(false, "GL_DRAW_INDIRECT_BUFFER updates should be through the DrawBatch command!");
		}

		if (isPersistent)
		{
			RingBuffer theDefault;
			RingBuffer& ringBuffer = r2::shashmap::Get(*s_optrRendererImpl->mRingBufferMap, cBufferHandle, theDefault);
			R2_CHECK(ringBuffer.dataPtr != theDefault.dataPtr, "Failed to get the ring buffer!");

			u64 count = size / ringBuffer.typeSize;

			void* ptr = ringbuf::Reserve(ringBuffer, count);
			memcpy(ptr, data, size);

			ringbuf::BindBufferRange(ringBuffer, cBufferHandle, count);

			if (!ringBuffer.flags.IsSet(CB_FLAG_MAP_COHERENT))
			{
				//@TODO(Serge): we should only do this once for all of the constant buffers so we'll need to track this
				glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
			}
		}
		else
		{
			glBindBuffer(bufferType, cBufferHandle);
			glBufferSubData(bufferType, offset, size, data);
		}
	}

	void CompleteConstantBuffer(ConstantBufferHandle cBufferHandle, u64 count)
	{
		RingBuffer theDefault;
		RingBuffer& ringBuffer = r2::shashmap::Get(*s_optrRendererImpl->mRingBufferMap, cBufferHandle, theDefault);
		R2_CHECK(ringBuffer.dataPtr != theDefault.dataPtr, "Failed to get the ring buffer!");

		ringbuf::Complete(ringBuffer, count);
	}

	//events
	void WindowResized(u32 width, u32 height)
	{
		glViewport(0, 0, width, height);
	}

	void MakeCurrent()
	{
		SDL_GL_MakeCurrent(s_optrWindow, s_glContext);
	}

	int SetFullscreen(int flags)
	{
		return SDL_SetWindowFullscreen(s_optrWindow, flags);
	}

	int SetVSYNC(bool vsync)
	{
		return SDL_GL_SetSwapInterval(static_cast<int>(vsync));
	}

	void SetWindowSize(u32 width, u32 height)
	{
		SDL_SetWindowSize(s_optrWindow, width, height);
	}
}

#endif // R2_PLATFORM_WINDOWS || R2_PLATFORM_MAC
