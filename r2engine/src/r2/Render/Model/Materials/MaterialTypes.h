#ifndef __MATERIAL_TYPES_H__
#define __MATERIAL_TYPES_H__

#include "r2/Utils/Utils.h"

namespace r2::draw 
{
	struct MaterialHandle
	{
		//forcing 8-byte alignment for free lists
		u64 handle = 0; //NEW VERSION - the name of the materialparams
		s64 slot = -1; //NEW VERSION - the index in the materialparamspack
		//@TODO(Serge): add the name of the cache here
	};
}

namespace r2::mat
{
	struct MaterialName
	{
		u64 name = 0;
		u64 packName = 0;

		bool operator==(const MaterialName& materialName) const
		{
			return name == materialName.name && packName == materialName.packName;
		}

		bool operator<(const MaterialName& materialName) const
		{
			return packName < materialName.packName || (name < materialName.name&& packName == materialName.packName);
		}
	};
}

#endif // __MATERIAL_TYPES_H__
