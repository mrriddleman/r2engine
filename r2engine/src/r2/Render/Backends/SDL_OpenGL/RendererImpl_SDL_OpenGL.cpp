#include "r2pch.h"

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

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

namespace
{
	SDL_Window* s_optrWindow = nullptr;
	SDL_GLContext s_glContext;

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

		s_optrWindow = SDL_CreateWindow("r2engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, params.resolution.width, params.resolution.height, params.platformFlags);

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

	void SetupBufferLayoutConfiguration(const BufferLayoutConfiguration& config, BufferLayoutHandle layoutId, VertexBufferHandle vertexBufferId, IndexBufferHandle indexBufferId)
	{
		glBindVertexArray(layoutId);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
		glBufferData(GL_ARRAY_BUFFER, config.vertexBufferConfig.bufferSize, nullptr, config.vertexBufferConfig.drawType);
		//glBufferStorage(GL_ARRAY_BUFFER, config.vertexBufferConfig.bufferSize, nullptr, )
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
		for (u64 i = 0; i < numConfigs; i++)
		{
			const r2::draw::ConstantBufferLayoutConfiguration& config = r2::sarr::At(*configs, i);
			 
			if (config.layout.GetType() == ConstantBufferLayout::Type::Small)
			{
				glBindBuffer(GL_UNIFORM_BUFFER, handles[i]);
				//MAYBE USE PERSISTENT MAPPED BUFFER FOR THIS INSTEAD
				glBufferData(GL_UNIFORM_BUFFER, config.layout.GetSize(), nullptr, config.drawType);

				glBindBufferBase(GL_UNIFORM_BUFFER, uboIndex++, handles[i]);
			}
			else if (config.layout.GetType() == ConstantBufferLayout::Type::Big)
			{
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, handles[i]);
				glBufferData(GL_SHADER_STORAGE_BUFFER, config.layout.GetSize(), nullptr, config.drawType);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssboIndex++, handles[i]);
			}
		}

		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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
			r2::draw::shader::SetMat4(*shader, "model", glm::mat4(1.0f));
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

	void DrawIndexed(BufferLayoutHandle layoutId, VertexBufferHandle vBufferHandle, IndexBufferHandle iBufferHandle, u32 numIndices, u32 startingIndex)
	{
		//@TODO(Serge): make sure we don't set the buffers array/buffer/index if they are already set to the correct value!
		//We need a system to track all of the state changes!
		glBindVertexArray(layoutId);

		glBindBuffer(GL_ARRAY_BUFFER, vBufferHandle);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iBufferHandle);

		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, (const void*)(startingIndex * sizeof(u32)));
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

	void UpdateConstantBuffer(ConstantBufferHandle cBufferHandle, r2::draw::ConstantBufferLayout::Type type, u64 offset, void* data, u64 size)
	{
		if (type == ConstantBufferLayout::Small)
		{
			glBindBuffer(GL_UNIFORM_BUFFER, cBufferHandle);
			glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
		}
		else
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, cBufferHandle);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
		}
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
