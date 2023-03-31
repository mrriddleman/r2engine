#ifndef __ASSET_WRITER_H__
#define __ASSET_WRITER_H__

#include "r2/Core/File/FileTypes.h"

namespace r2::asset
{
	class AssetWriter
	{
	public:
		virtual ~AssetWriter(){}
		virtual const char* GetPattern() = 0;
		virtual AssetType GetType() const = 0;
		virtual bool WriteAsset(const char* filePath, const void* rawBuffer, u32 rawSize, u32 offset, r2::fs::FileMode mode) = 0;
	};
}


#endif // __ASSET_WRITER_H__
