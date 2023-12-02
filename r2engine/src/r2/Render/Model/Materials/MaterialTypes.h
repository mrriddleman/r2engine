#ifndef __MATERIAL_TYPES_H__
#define __MATERIAL_TYPES_H__

#include "r2/Utils/Utils.h"

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

		bool IsInavlid() const
		{
			return name == 0 || packName == 0;
		}
	};
}

#endif // __MATERIAL_TYPES_H__
