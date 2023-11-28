#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include <fstream>
#include <filesystem>

namespace r2::asset::pln::utils
{
	bool WriteFile(const std::string& filePath, char* data, size_t size)
	{
		std::fstream fs;
		fs.open(filePath, std::fstream::out | std::fstream::binary);
		if (fs.good())
		{
			fs.write((const char*)data, size);
			
			R2_CHECK(fs.good(), "Failed to write out the buffer!");
			if (!fs.good())
			{
				return false;
			}
		}

		fs.close();
		return true;
	}

	char* ReadFile(const std::string& filePath)
	{
		char* data = nullptr;
		std::fstream fs;
		fs.open(filePath, std::fstream::in | std::fstream::binary);
		if (fs.good())
		{
			std::streampos fsize = fs.tellg();
			fs.seekg(0, std::ios::end);
			fsize = fs.tellg() - fsize;
			fs.seekg(std::ios::beg);

			data = new char[fsize];

			fs.read(data, fsize);

			R2_CHECK(data != nullptr, "We couldn't read the data!");

			R2_CHECK(fs.good(), "Failed to write out the buffer!");
			if (!fs.good())
			{
				delete[] data;
				return false;
			}
		}

		fs.close();

		return data;
	}
}

namespace r2::asset::pln
{
	static const std::string JSON_EXT = ".json";

	bool FindManifestFile(const std::string& directory, const std::string& stemName, const std::string& binEXT, std::string& outPath, bool isBinary)
	{
		for (auto& file : std::filesystem::recursive_directory_iterator(directory))
		{
			//UGH MAC - ignore .DS_Store
			if (std::filesystem::file_size(file.path()) <= 0 ||
				(std::filesystem::path(file.path()).extension().string() != JSON_EXT &&
					std::filesystem::path(file.path()).extension().string() != binEXT &&
					file.path().stem().string() != stemName))
			{
				continue;
			}

			if (file.path().stem().string() == stemName && ((isBinary && std::filesystem::path(file.path()).extension().string() == binEXT) ||
				(!isBinary && std::filesystem::path(file.path()).extension().string() == JSON_EXT)))
			{
				outPath = file.path().string();
				return true;
			}
		}

		return false;
	}
}

#endif