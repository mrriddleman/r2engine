#include "r2pch.h"

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Render/Backends/SDL_OpenGL/OpenGLRingBuffer.h"
#include "r2/Render/Renderer/RendererImpl.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLGPUBuffer.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Camera/Camera.h"
#include "r2/Render/Model/Material.h"
#include "r2/Render/Renderer/Shader.h"
#include "r2/Render/Renderer/ShaderSystem.h"
#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Render/Model/Textures/TextureSystem.h"
#include "r2/Render/Renderer/Commands.h"

#include <SDL2/SDL.h>
#include "glad/glad.h"
#include "r2/Core/Containers/SHashMap.h"

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLUtils.h"

namespace
{
	SDL_Window* s_optrWindow = nullptr;
	SDL_GLContext s_glContext;

	struct ConstantBufferPostRenderUpdate
	{
		r2::draw::ConstantBufferHandle handle = 0;
		u64 size = 0;
	};

	struct ImplementationLimits
	{
		//Limits
		GLint mMaxFragmentShaderStorageBlocks = 0;
		GLint mMaxVertexShaderStorageBlocks = 0;
		GLint mMaxGeometryShaderStorageBlocks = 0;
		GLint mMaxComputeShaderStorageBlocks = 0;
		GLint mMaxComputeWorkGroupCountX = 0;
		GLint mMaxComputeWorkGroupCountY = 0;
		GLint mMaxComputeWorkGroupCountZ = 0;
	};

	struct RendererImplState
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;
		r2::SHashMap<r2::draw::rendererimpl::RingBuffer>* mRingBufferMap = nullptr;
		r2::SArray<r2::draw::rendererimpl::GPUBuffer>* mGPUBuffers = nullptr;

		
		ImplementationLimits mLimits;
		//@TODO(Serge): might be a good idea to have render targets here instead of how we do it now
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
		case r2::draw::ShaderDataType::UInt64:	 return GL_UNSIGNED_INT64_ARB;
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


	u32 LESS = GL_LESS;
	u32 LEQUAL = GL_LEQUAL;
	u32 EQUAL = GL_EQUAL;

	u32 GREATER = GL_GREATER;
	u32 GEQUAL = GL_GEQUAL;

	u32 KEEP = GL_KEEP;
	u32 REPLACE = GL_REPLACE;
	u32 ZERO = GL_ZERO;
	u32 NOTEQUAL = GL_NOTEQUAL;
	u32 ALWAYS = GL_ALWAYS;

	u32 CULL_FACE_FRONT = GL_FRONT;
	u32 CULL_FACE_BACK = GL_BACK;
	u32 NONE = GL_NONE;
	
}

namespace r2::draw::cmd
{
	u32 CLEAR_COLOR_BUFFER = GL_COLOR_BUFFER_BIT;
	u32 CLEAR_DEPTH_BUFFER = GL_DEPTH_BUFFER_BIT;
	u32 CLEAR_STENCIL_BUFFER = GL_STENCIL_BUFFER_BIT;

	u32 SHADER_STORAGE_BARRIER_BIT = GL_SHADER_STORAGE_BARRIER_BIT;
	u32 FRAMEBUFFER_BARRIER_BIT = GL_FRAMEBUFFER_BARRIER_BIT;
	u32 ALL_BARRIER_BITS = GL_ALL_BARRIER_BITS;
}

namespace r2::draw::rendererimpl
{

	void SetLimits(RendererImplState* rendererImpl);

	//basic stuff
	bool PlatformInit(const PlatformRendererSetupParams& params)
	{
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
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

	bool RendererImplInit(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 numConstantBuffers, u64 maxRingLocks, const char* systemName)
	{
		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = MemorySize(numConstantBuffers, maxRingLocks);
		u64 unallocatedSpace = memoryArea->UnAllocatedSpace();
		if (unallocatedSpace < subAreaSize)
		{
			R2_CHECK(false, "We don't have enough space to allocate the Impl Renderer! We requested: %llu and we only have %llu left in the memory area. Difference of: %llu", subAreaSize, unallocatedSpace, subAreaSize - unallocatedSpace);
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
		s_optrRendererImpl->mRingBufferMap = MAKE_SHASHMAP(*linearArena, RingBuffer, numConstantBuffers * HASH_MULT);
		//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		s_optrRendererImpl->mGPUBuffers = MAKE_SARRAY(*linearArena, GPUBuffer, numConstantBuffers); //+1 I THINK because we will never have a 0 for the constant buffer


		//Limits
		SetLimits(s_optrRendererImpl);

#if defined R2_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
#endif
		glEnable(GL_MULTISAMPLE);
		glClearStencil(0);

		return s_optrRendererImpl->mRingBufferMap != nullptr && s_optrRendererImpl->mGPUBuffers != nullptr;
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
		u64 memSize3 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<RingBuffer>::MemorySize(numConstantBuffers * HASH_MULT), ALIGNMENT, headerSize, boundsChecking);
		u64 memSize4 = ringbuf::MemorySize(headerSize, boundsChecking, ALIGNMENT, maxNumRingLocks * RING_BUFFER_MULT) * numConstantBuffers * HASH_MULT;
		u64 memSize5 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<ConstantBufferPostRenderUpdate>::MemorySize(numConstantBuffers * HASH_MULT), ALIGNMENT, headerSize, boundsChecking);
		u64 memSize6 = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<GPUBuffer>::MemorySize(numConstantBuffers + 1), ALIGNMENT, headerSize, boundsChecking);
		return memSize1 + memSize2 + memSize3 + memSize4 + memSize5 + memSize6;
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

		auto hashMapEntryItr = r2::shashmap::Begin(*s_optrRendererImpl->mRingBufferMap);
		auto hashMapEnd = r2::shashmap::End(*s_optrRendererImpl->mRingBufferMap);
		for (; hashMapEntryItr != hashMapEnd; hashMapEntryItr++)
		{
			ringbuf::Clear(hashMapEntryItr->value);
		}
	}

	void Shutdown()
	{
		r2::mem::LinearArena* arena = s_optrRendererImpl->mSubAreaArena;

		FREE(s_optrRendererImpl->mGPUBuffers, *arena);

		auto hashMapEntryItr = r2::shashmap::Begin(*s_optrRendererImpl->mRingBufferMap);
		auto hashMapEnd = r2::shashmap::End(*s_optrRendererImpl->mRingBufferMap);
		for (; hashMapEntryItr != hashMapEnd; hashMapEntryItr++)
		{
			ringbuf::DestroyRingBuffer<r2::mem::LinearArena>(*arena, hashMapEntryItr->value);
		}

		FREE(s_optrRendererImpl->mRingBufferMap, *arena);

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

		return numFragmentConstantBuffers > numVertexConstantBuffers ? numVertexConstantBuffers : numFragmentConstantBuffers;
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

	void SetDepthClearColor(float color)
	{
		glClearDepth(color);
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
		{
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}
			
	}

	void SetCullFace(u32 cullFace)
	{
		glCullFace(cullFace);
	}

	void SetDepthFunction(u32 depthFunc)
	{
		if (depthFunc != EQUAL && depthFunc != LESS && depthFunc != LEQUAL && depthFunc != ALWAYS)
		{
			R2_CHECK(false, "depthFunc not supported yet!");
		}
		
		
		glDepthFunc(depthFunc);
	}

	void SetDepthWriteEnabled(bool depthWriteEnabled)
	{
		if (depthWriteEnabled)
		{
			glDepthMask(GL_TRUE);
		}
		else
		{
			glDepthMask(GL_FALSE);
		}
	}

	void SetDepthClamp(bool shouldDepthClamp)
	{
		if (shouldDepthClamp)
		{
			glEnable(GL_DEPTH_CLAMP);
		}
		else
		{
			glDisable(GL_DEPTH_CLAMP);
		}
	}


	void EnablePolygonOffset(bool enabled)
	{
		if (enabled)
		{
			glEnable(GL_POLYGON_OFFSET_FILL);
		}
		else
		{
			glDisable(GL_POLYGON_OFFSET_FILL);
		}
	}

	void SetPolygonOffset(const glm::vec2& polygonOffset)
	{
		glPolygonOffset(polygonOffset.x, polygonOffset.y);
	}

	void SetStencilState(const StencilState& stencilState)
	{
		if (stencilState.stencilEnabled)
		{
			glEnable(GL_STENCIL_TEST);
		}
		else
		{
			glDisable(GL_STENCIL_TEST);
		}

		glStencilOpSeparate(GL_FRONT_AND_BACK, stencilState.op.stencilFail, stencilState.op.depthFail, stencilState.op.depthAndStencilPass);

		glStencilFuncSeparate(GL_FRONT_AND_BACK, stencilState.func.func, stencilState.func.ref, stencilState.func.mask);

		glStencilMask(0x00);

		if (stencilState.stencilWriteEnabled)
		{
			glStencilMask(0xFF);
		}
	}

	s32 GetConstantLocation(ShaderHandle shaderHandle, const char* name)
	{
		const Shader* shader = shadersystem::GetShader(shaderHandle);
		R2_CHECK(shader != nullptr, "We couldn't get the shader from the shader handle: %lu", shaderHandle);

		GLint location = glGetUniformLocation(shader->shaderProg, name);

		GLenum err = glGetError();
		
		R2_CHECK(location >= 0, "We couldn't get the uniform location of the uniform name passed in: %s", name);
		//if(err != GL_NO_ERROR)
		R2_CHECK(err == GL_NO_ERROR, "We got an OpenGL error from ConstantUint: %i", err);

		return location;
	}

	void SetupBufferLayoutConfiguration(const BufferLayoutConfiguration& config, BufferLayoutHandle layoutId, VertexBufferHandle vertexBufferId[], u32 numVertexBufferHandles, IndexBufferHandle indexBufferId, DrawIDHandle drawId)
	{

		glBindVertexArray(layoutId);

		for (u32 i = 0; i < numVertexBufferHandles; ++i)
		{
			glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId[i]);
			glBufferData(GL_ARRAY_BUFFER, config.vertexBufferConfigs[i].bufferSize, nullptr, config.vertexBufferConfigs[i].drawType);
		}

		u32 vertexAttribId = 0;
		for (const auto& element : config.layout)
		{
			glEnableVertexAttribArray(vertexAttribId);

			if (element.type >= ShaderDataType::Float && element.type <= ShaderDataType::Mat4)
			{
				//glVertexAttribPointer(
				//	vertexAttribId, 
				//	element.GetComponentCount(), 
				//	ShaderDataTypeToOpenGLBaseType(element.type), 
				//	element.normalized ? GL_TRUE : GL_FALSE, 
				//	config.layout.GetStride(), 
				//	(const void*)element.offset);

				glVertexAttribFormat(vertexAttribId, element.GetComponentCount(), ShaderDataTypeToOpenGLBaseType(element.type), element.normalized ? GL_TRUE : GL_FALSE, element.offset);

			}
			else if (element.type >= ShaderDataType::Int && element.type <= ShaderDataType::Int4)
			{
				glVertexAttribIFormat(vertexAttribId, element.GetComponentCount(), ShaderDataTypeToOpenGLBaseType(element.type), element.offset);

				//glVertexAttribIPointer(
				//	vertexAttribId,
				//	element.GetComponentCount(),
				//	ShaderDataTypeToOpenGLBaseType(element.type),
				//	config.layout.GetStride(),
				//	(const void*)element.offset);
			}

			glVertexAttribBinding(vertexAttribId, element.bufferIndex);

			if (config.layout.GetVertexType() == VertexType::Instanced)
			{
				glVertexAttribDivisor(vertexAttribId, 1);
			}

			//@TODO(Serge): this should be out of the loop?
			glBindVertexBuffer(element.bufferIndex, vertexBufferId[element.bufferIndex], 0, config.layout.GetStride(element.bufferIndex));

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


			glEnableVertexAttribArray(vertexAttribId);
			glVertexAttribIPointer(vertexAttribId, 1, GL_UNSIGNED_INT, sizeof(u32), 0);
			glVertexAttribDivisor(vertexAttribId, 1);


			FREE(drawIdData, *MEM_ENG_SCRATCH_PTR);
		}


		if (indexBufferId != 0 && config.indexBufferConfig.bufferSize != EMPTY_BUFFER)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, config.indexBufferConfig.bufferSize, nullptr, config.indexBufferConfig.drawType);

		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

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
			else if (config.layout.GetType() == ConstantBufferLayout::Type::Big)
			{
				bufferType = GL_SHADER_STORAGE_BUFFER;
				nextIndex = &ssboIndex;
			}
			else if (config.layout.GetType() == ConstantBufferLayout::SubCommand)
			{
				bufferType = GL_DRAW_INDIRECT_BUFFER;
				nextIndex = &drawCMDIndex;
			}
			else
			{
				R2_CHECK(false, "Unsupported buffer type!");
			}

			GLCall(glBindBuffer(bufferType, handles[i]));


			if (config.layout.GetFlags().IsSet(CB_FLAG_MAP_PERSISTENT))
			{

				//	if (config.layout.GetBufferMult() == 1)
				{
					//@TODO(Serge): we need to save the persistent mapped ptr so that we can use it later
				}
				//else

			//	if (bufferType == GL_DRAW_INDIRECT_BUFFER)
				{

					glBufferStorage(bufferType, config.layout.GetSize() * config.layout.GetBufferMult(), nullptr, config.layout.GetFlags().GetRawValue() | config.layout.GetCreateFlags().GetRawValue());
					void* ptr = glMapBufferRange(bufferType, 0, config.layout.GetSize() * config.layout.GetBufferMult(), config.layout.GetFlags().GetRawValue());

					if (config.layout.GetBufferMult() > 1)
					{
						RingBuffer newRingBuffer;
						ringbuf::CreateRingBuffer<r2::mem::LinearArena>(*s_optrRendererImpl->mSubAreaArena, newRingBuffer, handles[i], bufferType, config.layout.begin()->elementSize, (*nextIndex)++, config.layout.GetFlags(), ptr, config.layout.begin()->typeCount * config.layout.GetBufferMult());
						r2::shashmap::Set(*s_optrRendererImpl->mRingBufferMap, handles[i], newRingBuffer);
					}
					else
					{
						GPUBuffer newGPUBuffer;
						newGPUBuffer.bufferName = handles[i];
						newGPUBuffer.index = (*nextIndex)++;
						newGPUBuffer.bufferType = bufferType;
						newGPUBuffer.dataPtr = ptr;
						newGPUBuffer.flags = config.layout.GetFlags();

						r2::sarr::Push(*s_optrRendererImpl->mGPUBuffers, newGPUBuffer);

					}


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
				GLCall(glBufferData(bufferType, config.layout.GetSize(), nullptr, config.drawType));
				GLCall(glBindBufferBase(bufferType, (*nextIndex)++, handles[i]));
			}
		}

		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);
	}

	void SetViewportKey(u32 viewport)
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

		ShaderHandle shaderHandle = mat::GetShaderHandle(*matSystem, materialID);

		if (shaderHandle != InvalidShader)
		{
			const Shader* shader = r2::draw::shadersystem::GetShader(shaderHandle);

			//@TODO(Serge): check current opengl state first
			r2::draw::shader::Use(*shader);
		}
	}

	void SetShaderID(r2::draw::ShaderHandle shaderHandle)
	{

		if (shaderHandle != InvalidShader)
		{
			const Shader* shader = shadersystem::GetShader(shaderHandle);
			R2_CHECK(shader != nullptr, "Uh oh");
			r2::draw::shader::Use(*shader);
		}
		else
		{
			R2_CHECK(false, "Passed in an invalid shaderID");
		}
	}

	//draw functions
	void Clear(u32 flags)
	{
		if ((flags & GL_STENCIL_BUFFER_BIT) == GL_STENCIL_BUFFER_BIT)
		{
			glEnable(GL_STENCIL_TEST);
			glStencilMask(0xFF);
			glClearStencil(0);
		}

		if ((flags & GL_DEPTH_BUFFER_BIT) == GL_DEPTH_BUFFER_BIT)
		{
			glEnable(GL_DEPTH_TEST);
			glDepthMask(0xFF);
			glClearDepth(1.0f);
		}

		glClear(flags);

		glDisable(GL_STENCIL_TEST);
		glStencilMask(0x00);

		glDisable(GL_DEPTH_TEST);
		glDepthMask(0x00);
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

	void DrawIndexedCommands(BufferLayoutHandle layoutId, ConstantBufferHandle batchHandle, void* cmds, u32 count, u32 offset, u32 stride, PrimitiveType primitivetype)
	{

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batchHandle);

		RingBuffer theDefault;
		RingBuffer& ringBuffer = r2::shashmap::Get(*s_optrRendererImpl->mRingBufferMap, batchHandle, theDefault);
		R2_CHECK(ringBuffer.dataPtr != theDefault.dataPtr, "Failed to get the ring buffer!");


		if (!ringBuffer.flags.IsSet(CB_FLAG_MAP_COHERENT))
		{
			glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		}

		BindVertexArray(layoutId);

		GLenum primitiveMode;

		if (primitivetype == PrimitiveType::LINES)
		{
			primitiveMode = GL_LINES;
		}
		else if (primitivetype == PrimitiveType::TRIANGLES)
		{
			primitiveMode = GL_TRIANGLES;
		}
		else
		{
			R2_CHECK(false, "We don't support any other types yet!");
		}

		glMultiDrawElementsIndirect(primitiveMode, GL_UNSIGNED_INT, mem::utils::PointerAdd(ringbuf::GetHeadOffset(ringBuffer), sizeof(cmd::DrawBatchSubCommand) * offset), count, stride);

	}

	void DrawDebugCommands(BufferLayoutHandle layoutId, ConstantBufferHandle batchHandle, void* cmds, u32 count, u32 offset, u32 stride)
	{
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batchHandle);

		RingBuffer theDefault;
		RingBuffer& ringBuffer = r2::shashmap::Get(*s_optrRendererImpl->mRingBufferMap, batchHandle, theDefault);
		R2_CHECK(ringBuffer.dataPtr != theDefault.dataPtr, "Failed to get the ring buffer!");

		if (!ringBuffer.flags.IsSet(CB_FLAG_MAP_COHERENT))
		{
			glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		}

		BindVertexArray(layoutId);
		glMultiDrawArraysIndirect(GL_LINES, mem::utils::PointerAdd(ringbuf::GetHeadOffset(ringBuffer), sizeof(cmd::DrawDebugBatchSubCommand) * offset), count, stride);

	}

	void DispatchComputeIndirect(ConstantBufferHandle dispatchBufferHandle, u32 offset)
	{
		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, dispatchBufferHandle);
		glDispatchComputeIndirect(sizeof(cmd::DispatchSubCommand) * offset);
	}

	void DispatchCompute(u32 numGroupsX, u32 numGroupsY, u32 numGroupsZ)
	{
		glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
	}

	void Barrier(u32 flags)
	{
		glMemoryBarrier(flags);
	}

	void ConstantUint(u32 uniformLocation, u32 value)
	{
		glUniform1ui(uniformLocation, value);
	}

	void ApplyDrawState(const cmd::DrawState& state)
	{
		SetDepthTest(state.depthEnabled);
		SetDepthFunction(state.depthFunction);
		SetDepthWriteEnabled(state.depthWriteEnabled);
		
		SetCullFace(state.cullState);

		EnablePolygonOffset(state.polygonOffsetEnabled);
		SetPolygonOffset(state.polygonOffset);
		SetStencilState(state.stencilState);
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
		else if (type == ConstantBufferLayout::Big)
		{
			bufferType = GL_SHADER_STORAGE_BUFFER;
		}
		else if (type == ConstantBufferLayout::SubCommand)
		{
			bufferType = GL_DRAW_INDIRECT_BUFFER;
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cBufferHandle);
		}
		else
		{
			R2_CHECK(false, "Unsupported ConstantBufferLayout!");
		}

		if (isPersistent)
		{

			if (bufferType == GL_DRAW_INDIRECT_BUFFER)
			{
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cBufferHandle);
			}


			RingBuffer theDefault;
			RingBuffer& ringBuffer = r2::shashmap::Get(*s_optrRendererImpl->mRingBufferMap, cBufferHandle, theDefault);

			if (ringBuffer.dataPtr != theDefault.dataPtr)
			{
				u64 count = size / ringBuffer.typeSize;

				void* ptr = ringbuf::Reserve(ringBuffer, count);
				memcpy(ptr, data, size);

				if (bufferType != GL_DRAW_INDIRECT_BUFFER)
				{
					ringbuf::BindBufferRange(ringBuffer, cBufferHandle, count);
				}

				if (!ringBuffer.flags.IsSet(CB_FLAG_MAP_COHERENT))
				{
					//@TODO(Serge): we should only do this once for all of the constant buffers so we'll need to track this
					glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
				}
			}
			else
			{
				for (u32 i = 0; i < r2::sarr::Size(*s_optrRendererImpl->mGPUBuffers); ++i)
				{
					GPUBuffer& gpuBuffer = r2::sarr::At(*s_optrRendererImpl->mGPUBuffers, i);

					if (gpuBuffer.bufferName == cBufferHandle)
					{
						R2_CHECK(gpuBuffer.dataPtr != nullptr, "This shouldn't be nullptr!");

						void* ptr = gpubuffer::Reserve(gpuBuffer);
						memcpy((char*)ptr + offset, (char*)data, size);

						if (bufferType != GL_DRAW_INDIRECT_BUFFER)
						{
							gpubuffer::BindBuffer(gpuBuffer);
						}

						if (!gpuBuffer.flags.IsSet(CB_FLAG_MAP_COHERENT))
						{
							glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
						}

						break;
					}
				}
			}
		}
		else
		{
			//glBindBufferBase()
			glBindBuffer(bufferType, cBufferHandle);
			glBufferSubData(bufferType, offset, size, data);
		}
	}

	void CompleteConstantBuffer(ConstantBufferHandle cBufferHandle, u64 count)
	{
		RingBuffer theDefault;
		RingBuffer& ringBuffer = r2::shashmap::Get(*s_optrRendererImpl->mRingBufferMap, cBufferHandle, theDefault);
		if (ringBuffer.dataPtr != theDefault.dataPtr)
		{
			ringbuf::Complete(ringBuffer, count);
		}
		else
		{
			for (u32 i = 0; i < r2::sarr::Size(*s_optrRendererImpl->mGPUBuffers); ++i)
			{
				GPUBuffer& gpuBuffer = r2::sarr::At(*s_optrRendererImpl->mGPUBuffers, i);

				if (gpuBuffer.bufferName == cBufferHandle)
				{
					R2_CHECK(gpuBuffer.dataPtr != nullptr, "Shouldn't be nullptr!");

					gpubuffer::Complete(gpuBuffer);
				}
			}
		}
	}

	void SetRenderTargetMipLevel(
		u32 fboHandle,
		const s32 colorTextures[MAX_RENDER_TARGETS],
		const u32 colorMipLevels[MAX_RENDER_TARGETS],
		const u32 colorTextureLayers[MAX_RENDER_TARGETS],
		u32 numColorTextures,
		s32 depthTexture,
		u32 depthMipLevel,
		u32 depthTextureLayer,
		s32 stencilTexture,
		u32 stencilMipLevel,
		u32 stencilTextureLayer,
		s32 depthStencilTexture,
		u32 depthStencilMipLevel,
		u32 depthStencilTextureLayer,
		u32 xOffset,
		u32 yOffset,
		u32 width,
		u32 height,
		b32 colorUseLayeredRenderering,
		b32 depthUseLayeredRenderering,
		b32 stencilUseLayeredRenderering,
		b32 depthStencilUseLayeredRenderering,
		b32 colorIsMSAA,
		b32 depthStencilIsMSAA)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

		if (fboHandle != 0)
		{
			bool checkStatus = false;

			if (numColorTextures > 0)
			{
				R2_CHECK(numColorTextures <= MAX_RENDER_TARGETS, "Can't have more than %lu color buffers for now", MAX_RENDER_TARGETS);

				GLenum bufs[MAX_RENDER_TARGETS];

				for (u32 i = 0; i < numColorTextures; ++i)
				{
					bufs[i] = GL_COLOR_ATTACHMENT0 + i;
					if (!colorUseLayeredRenderering && !colorIsMSAA)
					{
						glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, colorTextures[i], colorMipLevels[i], colorTextureLayers[i]);
					}
					else if (!colorUseLayeredRenderering && colorIsMSAA)
					{
						glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D_MULTISAMPLE_ARRAY, colorTextures[i], colorMipLevels[i], colorTextureLayers[i]);
					}
					else
					{
						glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, colorTextures[i], colorMipLevels[i]);
					}
				}
				checkStatus = true;
				glDrawBuffers(numColorTextures, &bufs[0]);

			}

			if (depthTexture != -1)
			{
				if (!depthUseLayeredRenderering)
				{
					glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, depthMipLevel, depthTextureLayer);
				}
				else
				{
					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, depthMipLevel);
				}

				checkStatus = true;
				if (numColorTextures == 0)
				{
					glDrawBuffer(GL_NONE);
					glReadBuffer(GL_NONE);
				}
			}

			if (stencilTexture != -1)
			{
				if (!stencilUseLayeredRenderering)
				{
					glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, stencilTexture, stencilMipLevel, stencilTextureLayer);
				}
				else
				{
					glFramebufferTexture(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, stencilTexture, stencilMipLevel);
				}

				checkStatus = true;
			}

			if (depthStencilTexture != -1)
			{
				if (!depthStencilUseLayeredRenderering && !depthStencilIsMSAA)
				{
					glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthStencilTexture, depthStencilMipLevel, depthStencilTextureLayer);
				}
				else if (!depthStencilUseLayeredRenderering && depthStencilIsMSAA)
				{
					glFramebufferTexture3D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE_ARRAY, depthStencilTexture, depthStencilMipLevel, depthStencilTextureLayer);
				}
				else
				{
					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthStencilTexture, depthStencilMipLevel);
				}

				checkStatus = true;

				if (numColorTextures == 0)
				{
					glDrawBuffer(GL_NONE);
					glReadBuffer(GL_NONE);
				}
			}

			if (checkStatus)
			{
				//SO SLOW!!
//#ifdef R2_DEBUG
//			auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
//			R2_CHECK(result, "Failed to attach texture to frame buffer");
//#endif
			}
		}

		glViewport(xOffset, yOffset, width, height);
	}

	void CopyRenderTargetColorTexture(u32 fboHandle, u32 attachmentIndex, u32 textureID, u32 mipLevel, s32 xOffset, s32 yOffset, s32 layer, s32 x, s32 y, u32 width, u32 height)
	{
		glNamedFramebufferReadBuffer(fboHandle, GL_COLOR_ATTACHMENT0 + attachmentIndex);
		glCopyTextureSubImage3D(textureID, mipLevel, xOffset, yOffset, layer, x, y, width, height);
	}

	void SetTexture(u32 textureContainerUniformLocation, u64 textureContainer, u32 texturePageUniformLocation, float texturePage, u32 textureLodUniformLocation, float textureLod)
	{
		glUniform1ui64ARB(textureContainerUniformLocation, textureContainer);
		glUniform1f(texturePageUniformLocation, texturePage);
		glUniform1f(textureLodUniformLocation, textureLod);
	}

	void BindImageTexture(u32 textureUnit, u32 texture, u32 mipLevel, b32 layered, u32 layer, u32 access, u32 format)
	{
		glBindImageTexture(textureUnit, texture, mipLevel, layered ? GL_TRUE : GL_FALSE, layer, access, format);
	}

	//events
	void SetViewport(u32 xOffset, u32 yOffset, u32 width, u32 height)
	{
		glViewport(xOffset, yOffset, width, height);
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

	void SetWindowPosition(s32 xPos, s32 yPos)
	{
		SDL_SetWindowPosition(s_optrWindow, xPos, yPos);
	}

	void CenterWindow()
	{
		SDL_SetWindowPosition(s_optrWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}

	void SetLimits(RendererImplState* rendererImpl)
	{
		glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &rendererImpl->mLimits.mMaxFragmentShaderStorageBlocks);
		glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &rendererImpl->mLimits.mMaxVertexShaderStorageBlocks);
		glGetIntegerv(GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, &rendererImpl->mLimits.mMaxGeometryShaderStorageBlocks);
		glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &rendererImpl->mLimits.mMaxComputeShaderStorageBlocks);

		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &rendererImpl->mLimits.mMaxComputeWorkGroupCountX);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &rendererImpl->mLimits.mMaxComputeWorkGroupCountY);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &rendererImpl->mLimits.mMaxComputeWorkGroupCountZ);
	}
}

#endif // R2_PLATFORM_WINDOWS || R2_PLATFORM_MAC
