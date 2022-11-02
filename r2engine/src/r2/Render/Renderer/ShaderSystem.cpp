#include "r2pch.h"
#include "r2/Render/Renderer/ShaderSystem.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Assets/Pipeline/ShaderManifest_generated.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/File.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::draw
{
    struct ShaderSystem
    {
        r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
        r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
        r2::mem::LinearArena* mLinearArena = nullptr;
        r2::SArray<r2::draw::Shader>* mShaders = nullptr;
        r2::mem::StackArena* mShaderLoadingArena = nullptr;

        void* mInternalShaderManifestsData = nullptr;
        void* mAppShaderManifestsData = nullptr;

        const r2::ShaderManifests* mInternalShaderManifests = nullptr;
        const r2::ShaderManifests* mAppShaderManifests = nullptr;

#ifdef R2_ASSET_PIPELINE
        r2::SArray<ShaderHandle>* mShadersToReload = nullptr;
        r2::SArray<std::string>* mReloadShaderManifests = nullptr;
#endif
    };
}

namespace
{
    r2::draw::ShaderSystem* s_optrShaderSystem = nullptr;

    const u64 ALIGNMENT = 16;

    const u32 SHADER_LOADING_SIZE = Kilobytes(512);

    const u32 NUM_MANIFESTS_TO_LOAD = 2;
}

namespace r2::draw::shadersystem
{

    void* LoadShadersFromManifestFile(const char* manifestFilePath, bool assertOnFailure);
    void DeleteLoadedShaders();
    ShaderHandle MakeShaderHandleFromIndex(u64 index);
    u64 GetIndexFromShaderHandle(ShaderHandle handle);

    bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 capacity, const char* shaderManifestPath, const char* internalShaderManifestPath)
    {
        R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "Memory Area handle is invalid");
        R2_CHECK(s_optrShaderSystem == nullptr, "Are you trying to initialize this system more than once?");
        R2_CHECK(shaderManifestPath != nullptr && shaderManifestPath != "", "We have a null or empty shader manifest path");
        R2_CHECK(internalShaderManifestPath != nullptr && internalShaderManifestPath != "", "We have a null or empty internal shader manifest path");


        if (memoryAreaHandle == r2::mem::MemoryArea::Invalid ||
            s_optrShaderSystem != nullptr)
        {
            return false;
        }

        //Get the memory area
        r2::mem::MemoryArea* noptrMemArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
        R2_CHECK(noptrMemArea != nullptr, "noptrMemArea is null?");
        if (!noptrMemArea)
        {
            return false;
        }

        u64 unallocatedSpace = noptrMemArea->UnAllocatedSpace();
        u64 memoryNeeded = shadersystem::GetMemorySize(capacity);
        if (memoryNeeded > unallocatedSpace)
        {
            R2_CHECK(false, "We don't have enough space to allocate a new sub area for this system");
            return false;
        }

        r2::mem::MemoryArea::SubArea::Handle subAreaHandle = noptrMemArea->AddSubArea(memoryNeeded, "Material System");

        R2_CHECK(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid, "We have an invalid sub area");

        if (subAreaHandle == r2::mem::MemoryArea::SubArea::Invalid)
        {
            return false;
        }

        r2::mem::MemoryArea::SubArea* noptrSubArea = noptrMemArea->GetSubArea(subAreaHandle);
        R2_CHECK(noptrSubArea != nullptr, "noptrSubArea is null");
        if (!noptrSubArea)
        {
            return false;
        }

        //Emplace the linear arena in the subarea
        r2::mem::LinearArena* shaderLinearArena = EMPLACE_LINEAR_ARENA(*noptrSubArea);

        if (!shaderLinearArena)
        {
            R2_CHECK(shaderLinearArena != nullptr, "linearArena is null");
            return false;
        }

        //allocate the MemorySystem
        s_optrShaderSystem = ALLOC(r2::draw::ShaderSystem, *shaderLinearArena);

        R2_CHECK(s_optrShaderSystem != nullptr, "We couldn't allocate the material system!");

        s_optrShaderSystem->mMemoryAreaHandle = memoryAreaHandle;
        s_optrShaderSystem->mSubAreaHandle = subAreaHandle;
        s_optrShaderSystem->mLinearArena = shaderLinearArena;
        s_optrShaderSystem->mShaders = MAKE_SARRAY(*shaderLinearArena, r2::draw::Shader, capacity);
        
#ifdef R2_ASSET_CACHE_DEBUG
        s_optrShaderSystem->mShadersToReload = MAKE_SARRAY(*shaderLinearArena, ShaderHandle, capacity);
        s_optrShaderSystem->mReloadShaderManifests = MAKE_SARRAY(*shaderLinearArena, std::string, capacity);
#endif
        //r2::util::PathCpy(s_optrShaderSystem->mShaderManifestPath, shaderManifestPath);

        s_optrShaderSystem->mShaderLoadingArena = MAKE_STACK_ARENA(*shaderLinearArena, SHADER_LOADING_SIZE * NUM_MANIFESTS_TO_LOAD);

        R2_CHECK(s_optrShaderSystem->mShaderLoadingArena != nullptr, "We couldn't make the loading arena");
        R2_CHECK(s_optrShaderSystem->mShaders != nullptr, "we couldn't allocate the array for materials?");

        constexpr bool assertOnFailure = true;

        s_optrShaderSystem->mAppShaderManifestsData = LoadShadersFromManifestFile(shaderManifestPath, assertOnFailure);
        s_optrShaderSystem->mAppShaderManifests = r2::GetShaderManifests(s_optrShaderSystem->mAppShaderManifestsData);

        s_optrShaderSystem->mInternalShaderManifestsData = LoadShadersFromManifestFile(internalShaderManifestPath, assertOnFailure);
        s_optrShaderSystem->mInternalShaderManifests = r2::GetShaderManifests(s_optrShaderSystem->mInternalShaderManifestsData);

        return s_optrShaderSystem->mAppShaderManifestsData && s_optrShaderSystem->mInternalShaderManifestsData;
    }

    void Update()
    {
#ifdef R2_ASSET_PIPELINE

        if (s_optrShaderSystem == nullptr)
        {
            R2_CHECK(false, "We haven't initialized the shader system yet!");
            return;
        }

        u64 numShaderManifests = r2::sarr::Size(*s_optrShaderSystem->mReloadShaderManifests);
        if (numShaderManifests > 0)
        {
            R2_CHECK(false, "Not actually sure if this is correct since we're appending more shaders to the end of the shader array without removing the originals");

            for (u64 i = 0; i < numShaderManifests; ++i)
            {
                const std::string& manifestFile = r2::sarr::At(*s_optrShaderSystem->mReloadShaderManifests, i);
				bool loaded = LoadShadersFromManifestFile(manifestFile.c_str(), false);

				R2_CHECK(loaded, "We couldn't load the shaders from the file: %s\n", manifestFile.c_str());

            }

            r2::sarr::Clear(*s_optrShaderSystem->mReloadShaderManifests);
        }

        const u64 numShadersToReload = r2::sarr::Size(*s_optrShaderSystem->mShadersToReload);
        if (numShadersToReload > 0)
        {
            for (u64 i = 0; i < numShadersToReload; ++i)
            {
                ShaderHandle shaderHandle = r2::sarr::At(*s_optrShaderSystem->mShadersToReload, i);

                Shader& shaderToReload = r2::sarr::At(*s_optrShaderSystem->mShaders, shaderHandle);

                shader::ReloadShaderProgramFromRawFiles(
                    &shaderToReload.shaderProg,
                    shaderToReload.manifest.hashName,
                    shaderToReload.manifest.vertexShaderPath.c_str(),
                    shaderToReload.manifest.fragmentShaderPath.c_str(),
                    shaderToReload.manifest.geometryShaderPath.c_str(),
                    shaderToReload.manifest.computeShaderPath.c_str());
            }

            r2::sarr::Clear(*s_optrShaderSystem->mShadersToReload);
        }
#endif
    }

    ShaderHandle AddShader(const Shader& shader)
    {
        if (s_optrShaderSystem == nullptr)
        {
            R2_CHECK(false, "We haven't initialized the shader system yet!");
            return InvalidShader;
        }

        const u64 numShaders = r2::sarr::Size(*s_optrShaderSystem->mShaders);

        for (u64 i = 0; i < numShaders; ++i)
        {
            const Shader& nextShader = r2::sarr::At(*s_optrShaderSystem->mShaders, i);
            if (nextShader.shaderProg == shader.shaderProg)
            {
                return MakeShaderHandleFromIndex(i);
            }
        }

        r2::sarr::Push(*s_optrShaderSystem->mShaders, shader);
        return MakeShaderHandleFromIndex(numShaders);
    }

    ShaderHandle FindShaderHandle(const Shader& shader)
    {
        if (s_optrShaderSystem == nullptr)
        {
            R2_CHECK(false, "We haven't initialized the shader system yet!");
            return InvalidShader;
        }

        const u64 numShaders = r2::sarr::Size(*s_optrShaderSystem->mShaders);

        for (u64 i = 0; i < numShaders; ++i)
        {
            const Shader& nextShader = r2::sarr::At(*s_optrShaderSystem->mShaders, i);
            if (nextShader.shaderProg == shader.shaderProg)
            {
                return MakeShaderHandleFromIndex(i);
            }
        }

        return InvalidShader;
    }

    ShaderHandle FindShaderHandle(u64 shaderName)
    {
        if (s_optrShaderSystem == nullptr)
        {
            R2_CHECK(false, "We haven't initialized the shader system yet!");
            return InvalidShader;
        }

        const u64 numShaders = r2::sarr::Size(*s_optrShaderSystem->mShaders);

        for (u64 i = 0; i < numShaders; ++i)
        {
            const Shader& nextShader = r2::sarr::At(*s_optrShaderSystem->mShaders, i);
            if (nextShader.shaderID == shaderName)
            {
                return MakeShaderHandleFromIndex(i);
            }
        }

        return InvalidShader;
    }

    const Shader* GetShader(ShaderHandle handle)
    {
        if (s_optrShaderSystem == nullptr)
        {
            R2_CHECK(false, "We haven't initialized the shader system yet!");
            return nullptr;
        }

        u64 index = GetIndexFromShaderHandle(handle);

        if (index >= r2::sarr::Size(*s_optrShaderSystem->mShaders))
        {
            return nullptr;
        }

        return &r2::sarr::At(*s_optrShaderSystem->mShaders, index);
    }

    const char* FindShaderPathInManifest(const r2::ShaderManifests* shaderManifest, const char* name)
    {
        const auto numAppManifests = shaderManifest->manifests()->size();

		char shaderFileName[fs::FILE_PATH_LENGTH];

		for (flatbuffers::uoffset_t i = 0; i < numAppManifests; ++i)
		{
			const auto* manifest = shaderManifest->manifests()->Get(i);

			bool success = fs::utils::CopyFileNameWithExtension(manifest->vertexPath()->c_str(), shaderFileName);

			R2_CHECK(success, "Couldn't copy the filename of the shader!");

			if (strcmp(shaderFileName, name) == 0)
			{
				return manifest->vertexPath()->c_str();
			}

			success = fs::utils::CopyFileNameWithExtension(manifest->fragmentPath()->c_str(), shaderFileName);

			R2_CHECK(success, "Couldn't copy the filename of the shader!");

			if (strcmp(shaderFileName, name) == 0)
			{
				return manifest->fragmentPath()->c_str();
			}

			success = fs::utils::CopyFileNameWithExtension(manifest->geometryPath()->c_str(), shaderFileName);

			R2_CHECK(success, "Couldn't copy the filename of the shader!");

			if (strcmp(shaderFileName, name) == 0)
			{
				return manifest->geometryPath()->c_str();
			}

			success = fs::utils::CopyFileNameWithExtension(manifest->computePath()->c_str(), shaderFileName);

			R2_CHECK(success, "Couldn't copy the filename of the shader!");

			if (strcmp(shaderFileName, name) == 0)
			{
				return manifest->computePath()->c_str();
			}

            success = fs::utils::CopyFileNameWithExtension(manifest->partPath()->c_str(), shaderFileName);

            R2_CHECK(success, "Couldn't copy the filename of the shader!");

			if (strcmp(shaderFileName, name) == 0)
			{
				return manifest->partPath()->c_str();
			}
		}

        return nullptr;
    }


    const char* FindShaderPathByName(const char* name)
    {
        const char* result = FindShaderPathInManifest(s_optrShaderSystem->mAppShaderManifests, name);

        if (result == nullptr)
        {
            result = FindShaderPathInManifest(s_optrShaderSystem->mInternalShaderManifests, name);
        }

        return result;
    }

    void Shutdown()
    {
        if (s_optrShaderSystem == nullptr)
        {
            R2_CHECK(false, "We haven't initialized the shader system yet!");
            return;
        }

        DeleteLoadedShaders();

        FREE(s_optrShaderSystem->mInternalShaderManifestsData, *s_optrShaderSystem->mShaderLoadingArena);
        FREE(s_optrShaderSystem->mAppShaderManifestsData, *s_optrShaderSystem->mShaderLoadingArena);

        r2::mem::LinearArena* shaderArena = s_optrShaderSystem->mLinearArena;

        FREE(s_optrShaderSystem->mShaderLoadingArena, *shaderArena);

#ifdef R2_ASSET_PIPELINE
        FREE(s_optrShaderSystem->mShadersToReload, *shaderArena);
        FREE(s_optrShaderSystem->mReloadShaderManifests, *shaderArena);
#endif

        FREE(s_optrShaderSystem->mShaders, *shaderArena);

        FREE(s_optrShaderSystem, *shaderArena);
        s_optrShaderSystem = nullptr;

        FREE_EMPLACED_ARENA(shaderArena);
    }

    u64 GetMemorySize(u64 numShaders)
    {
        u32 boundsChecking = 0;
#ifdef R2_DEBUG
        boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
        u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

        u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();

        u64 memorySize =
            r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), ALIGNMENT, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(SHADER_LOADING_SIZE, ALIGNMENT, stackHeaderSize, boundsChecking) * NUM_MANIFESTS_TO_LOAD + //for the shader manifest loading
            r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::draw::ShaderSystem), ALIGNMENT, headerSize, boundsChecking) +

#ifdef R2_ASSET_PIPELINE
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ShaderHandle>::MemorySize(numShaders), ALIGNMENT, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<std::string>::MemorySize(numShaders), ALIGNMENT, headerSize, boundsChecking) +
#endif

            r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::Shader>::MemorySize(numShaders), ALIGNMENT, headerSize, boundsChecking);

        return r2::mem::utils::GetMaxMemoryForAllocation(memorySize, ALIGNMENT);
    }

    void DeleteLoadedShaders()
    {
        if (s_optrShaderSystem == nullptr)
        {
            R2_CHECK(false, "We haven't initialized the shader system yet!");
            return;
        }

        u64 numShaders = r2::sarr::Size(*s_optrShaderSystem->mShaders);

        for (u64 i = 0; i < numShaders; ++i)
        {
            Shader& nextShader = r2::sarr::At(*s_optrShaderSystem->mShaders, i);
            r2::draw::shader::Delete(nextShader);
        }

        r2::sarr::Clear(*s_optrShaderSystem->mShaders);

    }

    void* LoadShadersFromManifestFile(const char* shaderManifestPath, bool assertOnFailure)
    {
        if (s_optrShaderSystem == nullptr)
        {
            R2_CHECK(false, "We haven't initialized the shader system yet!");
            return nullptr;
        }

        void* shaderManifestBuffer = r2::fs::ReadFile<r2::mem::StackArena>(*s_optrShaderSystem->mShaderLoadingArena, shaderManifestPath);

        R2_CHECK(shaderManifestBuffer != nullptr, "Shader manifest should not be null!");

        auto shaderManifestsBuf = r2::GetShaderManifests(shaderManifestBuffer);

        flatbuffers::uoffset_t numManifests = shaderManifestsBuf->manifests()->size();

        for (flatbuffers::uoffset_t i = 0; i < numManifests; ++i)
        {
            ShaderHandle shaderHandle = FindShaderHandle(shaderManifestsBuf->manifests()->Get(i)->shaderName());

            if (shaderHandle == InvalidShader)
            {
                //@NOTE: this assumes that we can just compile all the shader programs at startup
                r2::draw::Shader nextShader = r2::draw::shader::CreateShaderProgramFromRawFiles(
                    shaderManifestsBuf->manifests()->Get(i)->shaderName(),
                    shaderManifestsBuf->manifests()->Get(i)->vertexPath()->str().c_str(),
                    shaderManifestsBuf->manifests()->Get(i)->fragmentPath()->str().c_str(),
                    shaderManifestsBuf->manifests()->Get(i)->geometryPath()->str().c_str(),
                    shaderManifestsBuf->manifests()->Get(i)->computePath()->str().c_str(),
                    assertOnFailure);

                r2::sarr::Push(*s_optrShaderSystem->mShaders, nextShader);
            }
        }

        //FREE(shaderManifestBuffer, *s_optrShaderSystem->mShaderLoadingArena);

        return shaderManifestBuffer;
    }

#ifdef R2_ASSET_PIPELINE

    void ReloadManifestFile(const std::string& manifestFilePath)
    {

        if (s_optrShaderSystem == nullptr)
        {
        //    R2_CHECK(false, "We haven't initialized the shader system yet!");
            return;
        }

        r2::sarr::Push(*s_optrShaderSystem->mReloadShaderManifests, manifestFilePath);
    }

    void ReloadShader(const r2::asset::pln::ShaderManifest& manifest)
    {
        if (s_optrShaderSystem == nullptr)
        {
            R2_CHECK(false, "We haven't initialized the shader system yet!");
            return;
        }

        const u64 numShaders = r2::sarr::Size(*s_optrShaderSystem->mShaders);
        for (u64 i = 0; i < numShaders; ++i)
        {
            const auto& nextShader = r2::sarr::At(*s_optrShaderSystem->mShaders, i);

            if (nextShader.manifest.hashName == manifest.hashName)
            {
                r2::sarr::Push(*s_optrShaderSystem->mShadersToReload, static_cast<ShaderHandle>(i));
            }
        }
    }
#endif // R2_ASSET_PIPELINE

    ShaderHandle MakeShaderHandleFromIndex(u64 index)
    {
        if (s_optrShaderSystem == nullptr)
        {
            R2_CHECK(false, "shader system hasn't been initialized yet!");
            return InvalidShader;
        }

        const u64 numShaders = r2::sarr::Size(*s_optrShaderSystem->mShaders);
        if (index >= numShaders)
        {
            R2_CHECK(false, "the index is greater than the number of shaders we have!");
            return InvalidShader;
        }

        return static_cast<ShaderHandle>(index + 1);
    }
    
    u64 GetIndexFromShaderHandle(ShaderHandle handle)
    {
        if (handle == InvalidShader)
        {
            R2_CHECK(false, "You passed in an invalid shader handle!");
            return 0;
        }

        return static_cast<u64>(handle - 1);
    }
}
