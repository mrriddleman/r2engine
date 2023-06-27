#include "assetlib/AssetUtils.h"
#include <cassert>
#include <string.h>
#include <algorithm>
#include <cctype>
#include "Hash.h"

namespace r2::asset
{
	const char PATH_SEPARATOR = '/';
	const int FILE_PATH_LENGTH = 256;

	bool CopyFileNameWithParentDirectories(const char* filePath, char* fileNameWithDirectories, uint32_t numParents)
	{
		if (!filePath || !fileNameWithDirectories)
		{
			return false;
		}

		strcpy_s(fileNameWithDirectories, FILE_PATH_LENGTH, "");

		const uint32_t subPathEndCounter = numParents + 1;
		size_t len = strlen(filePath);
		if (len == 0)
			return false;


		if (numParents == 0)
		{
			//we need to find the start of the string
			//which will be either the first encounter of a PATH_SEPARATOR or the start of the string
			int startIndex = static_cast<int>(len) - 1;

			for (int i = startIndex; i >= 0; --i)
			{
				startIndex = i;
				if (filePath[i] == PATH_SEPARATOR)
				{
					startIndex++;
					break;
				}
			}

			for (uint32_t i = 0; i < len; ++i)
			{
				fileNameWithDirectories[i] = filePath[i + startIndex];
			}

			fileNameWithDirectories[len] = '\0';

			return true;
		}

		uint32_t subPathCounter = 0;
		size_t startIndex = len - 1;


		bool finishedCorrectly = false;
		for (int i = static_cast<int>(startIndex); i >= 0; --i)
		{
			if (filePath[i] == PATH_SEPARATOR)
			{
				subPathCounter++;

				if (subPathCounter == subPathEndCounter)
				{
					finishedCorrectly = true;
					startIndex = static_cast<size_t>(i) + 1;
					break;
				}
			}
		}

		if (!finishedCorrectly)
			return false;

		size_t resultLen = len - startIndex;

		for (uint32_t i = 0; i < resultLen; ++i)
		{
			fileNameWithDirectories[i] = filePath[startIndex + i];
		}

		fileNameWithDirectories[resultLen] = '\0';

		return true;
	}

	bool SanitizeSubPath(const char* rawSubPath, char* result)
	{
		size_t startingIndex = 0;
		//@TODO(Serge): do in a loop
		if (strlen(rawSubPath) > 2) {
			if (rawSubPath[0] == '.' && rawSubPath[1] == '.')
			{
				startingIndex = 2;
			}
			else if (rawSubPath[0] == '.' && rawSubPath[1] != '.')
			{
				startingIndex = 1;
			}
		}

		size_t len = strlen(rawSubPath);
		size_t resultIndex = 0;
		for (; startingIndex < len; startingIndex++)
		{
			if (PATH_SEPARATOR != '\\' && rawSubPath[startingIndex] == '\\')
			{
				result[resultIndex] = PATH_SEPARATOR;
			}
			else
			{
				result[resultIndex] = rawSubPath[startingIndex];
			}

			++resultIndex;
		}

		result[resultIndex] = '\0';

		return true;
	}

	uint64_t GetAssetNameForFilePath(const char* filePath, AssetType assetType)
	{
		char assetName[FILE_PATH_LENGTH];
		
		MakeAssetNameStringForFilePath(filePath, assetName, assetType);

		return STRING_ID(assetName);
	}

	void MakeAssetNameStringForFilePath(const char* filePath, char* dst, AssetType assetType)
	{
		uint32_t numParentDirectoriesToInclude = GetNumberOfParentDirectoriesToIncludeForAssetType(assetType);

		char sanitizedPath[FILE_PATH_LENGTH];
		SanitizeSubPath(filePath, sanitizedPath);

		char assetName[FILE_PATH_LENGTH];
		CopyFileNameWithParentDirectories(sanitizedPath, assetName, numParentDirectoriesToInclude);

		std::transform(std::begin(assetName), std::end(assetName), std::begin(assetName), (int(*)(int))std::tolower);

		strcpy(dst, assetName);
	}

	uint32_t GetNumberOfParentDirectoriesToIncludeForAssetType(AssetType assetType)
	{
		switch (assetType)
		{
		case DEFAULT:
			return 0;
		case TEXTURE:
			return 2;
		case MODEL:
			return 1;
		case MESH:
			return 1;
		case CUBEMAP_TEXTURE:
			return 2;
		case MATERIAL_PACK_MANIFEST:
			return 0;
		case TEXTURE_PACK_MANIFEST:
			return 0;
		case RMODEL:
			return 0;
		case RANIMATION:
			return 0;
		case LEVEL:
			return 1;
		case LEVEL_PACK:
			return 0;
		case MATERIAL:
			return 0;
		case TEXTURE_PACK:
			return 0;
		case SOUND:
			return 1;
		default:
			//@TODO(Serge): may need to extend this using the Application
			assert(false && "Unsupported assettype!");
			break;
		}

		return 0;
	}
}