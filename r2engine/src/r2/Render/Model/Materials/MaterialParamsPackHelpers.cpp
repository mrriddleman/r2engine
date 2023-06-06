#include "r2pch.h"
#include "r2/Render/Model/Materials/MaterialParams_generated.h"
#include "r2/Render/Model/Materials/MaterialParamsPack_generated.h"
#include "r2/Render/Model/Materials/MaterialParamsPackHelpers.h"

namespace r2::mat
{

	const flat::MaterialParams* GetMaterialParamsForMaterialName(const flat::MaterialParamsPack* materialPack, u64 materialName)
	{
		if (!materialPack)
		{
			return nullptr;
		}

		const auto numMaterialParams = materialPack->pack()->size();

		for (flatbuffers::uoffset_t i = 0; i < numMaterialParams; ++i)
		{
			if (materialPack->pack()->Get(i)->name() == materialName)
			{
				return materialPack->pack()->Get(i);
			}
		}

		return nullptr;
	}

	u64 GetShaderNameForMaterialName(const flat::MaterialParamsPack* materialPack, u64 materialName)
	{
		const flat::MaterialParams* material = GetMaterialParamsForMaterialName(materialPack, materialName);

		if (!material)
		{
			return 0;
		}

		const auto numShaderParams = material->ulongParams()->size();

		for (flatbuffers::uoffset_t i = 0; i < numShaderParams; ++i)
		{
			const auto* param = material->ulongParams()->Get(i);

			if (param->propertyType() == flat::MaterialPropertyType_SHADER)
			{
				return param->value();
			}
		}

		return 0;
	}

}