#include "r2pch.h"

#include <filesystem>

void MakeDirectoriesRecursively(const std::vector<std::string>& paths, bool startAtOne, bool startAtParent)
{
	std::vector<std::vector<std::filesystem::path>> pathsToBuild;

	u64 startingPoint = 0;
	if (startAtOne)
	{
		startingPoint = 1;
	}

	for (u64 i = startingPoint; i < paths.size(); ++i)
	{
		pathsToBuild.push_back({  });

		auto path = std::filesystem::path(paths[i]).parent_path();
		if (!startAtParent)
		{
			path = std::filesystem::path(paths[i]);
		}

		while (!std::filesystem::exists(path))
		{
			pathsToBuild[i - startingPoint].push_back(path);
			path = path.parent_path();
		}
	}

	for (u64 i = startingPoint; i < paths.size(); ++i)
	{
		auto rit = pathsToBuild[i - startingPoint].rbegin();
		for (; rit != pathsToBuild[i - startingPoint].rend(); rit++)
		{
			if (!std::filesystem::exists(*rit))
			{
				std::filesystem::create_directory(*rit);
			}
		}
	}
}