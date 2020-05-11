#include "r2pch.h"
#include "r2/Render/Model/Material.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"

namespace r2::draw
{
	struct MaterialSystem
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mLinearArena = nullptr;
		r2::SArray<r2::draw::Material>* mMaterials = nullptr;
	};
}

namespace
{
	r2::draw::MaterialSystem* s_optrMaterialSystem = nullptr;
}

namespace r2::draw::mat
{
	const MaterialHandle InvalidMaterialHandle = -1;

	bool Init(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 capacity)
	{
		R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "Memory Area handle is invalid");
		R2_CHECK(s_optrMaterialSystem == nullptr, "Are you trying to initialize this system more than once?");

		if (memoryAreaHandle == r2::mem::MemoryArea::Invalid ||
			s_optrMaterialSystem != nullptr)
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
		u64 memoryNeeded = GetMemorySize(capacity);
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
		r2::mem::LinearArena* materialLinearArena = EMPLACE_LINEAR_ARENA(*noptrSubArea);
		
		if (!materialLinearArena)
		{
			R2_CHECK(materialLinearArena != nullptr, "linearArena is null");
			return false;
		}

		//allocate the MemorySystem
		s_optrMaterialSystem = ALLOC(r2::draw::MaterialSystem, *materialLinearArena);

		R2_CHECK(s_optrMaterialSystem != nullptr, "We couldn't allocate the material system!");

		s_optrMaterialSystem->mMemoryAreaHandle = memoryAreaHandle;
		s_optrMaterialSystem->mSubAreaHandle = subAreaHandle;
		s_optrMaterialSystem->mLinearArena = materialLinearArena;
		s_optrMaterialSystem->mMaterials = MAKE_SARRAY(*materialLinearArena, r2::draw::Material, capacity);
		R2_CHECK(s_optrMaterialSystem->mMaterials != nullptr, "we couldn't allocate the array for materials?");

		return s_optrMaterialSystem->mMaterials != nullptr;
	}

	MaterialHandle AddMaterial(const Material& mat)
	{
		if (s_optrMaterialSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the material system yet!");
			return InvalidMaterialHandle;
		}

		//see if we already have this material - if we do then return that
		const u64 numMaterials = r2::sarr::Size(*s_optrMaterialSystem->mMaterials);

		for (u64 i = 0; i < numMaterials; ++i)
		{
			const Material& nextMaterial = r2::sarr::At(*s_optrMaterialSystem->mMaterials, i);
			if (mat.shaderId == nextMaterial.shaderId &&
				mat.textureId == nextMaterial.textureId &&
				mat.color == nextMaterial.color)
			{
				return static_cast<MaterialHandle>(i);
			}
		}

		r2::sarr::Push(*s_optrMaterialSystem->mMaterials, mat);
		return static_cast<MaterialHandle>(numMaterials);
	}

	const Material* GetMaterial(MaterialHandle matID)
	{
		if (s_optrMaterialSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the material system yet!");
			return nullptr;
		}

		if (matID >= r2::sarr::Size(*s_optrMaterialSystem->mMaterials))
		{
			return nullptr;
		}

		return &r2::sarr::At(*s_optrMaterialSystem->mMaterials, matID);
	}

	void Shutdown()
	{
		if (s_optrMaterialSystem == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the material system yet!");
			return;
		}

		r2::mem::LinearArena* materialArena = s_optrMaterialSystem->mLinearArena;

		FREE(s_optrMaterialSystem->mMaterials, *materialArena);
		
		FREE(s_optrMaterialSystem, *materialArena);
		s_optrMaterialSystem = nullptr;

		FREE_EMPLACED_ARENA(materialArena);
	}

	u64 GetMemorySize(u64 numMaterials)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u64 memorySize =
			sizeof(r2::mem::LinearArena) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::draw::MaterialSystem), 64, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::Material>::MemorySize(numMaterials), alignof(r2::draw::Material), headerSize, boundsChecking);

		return r2::mem::utils::GetMaxMemoryForAllocation(memorySize, 64);
	}
}