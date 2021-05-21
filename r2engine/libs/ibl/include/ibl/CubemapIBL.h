#ifndef __IBL_CUBEMAP_IBL_H__
#define __IBL_CUBEMAP_IBL_H__

#include <vector>
#include <functional>
#include <glm/glm.hpp>

namespace r2::ibl
{
	class Cubemap;
	class Image;

	class CubemapIBL
	{
	public:
		//typedef void (*Progress)(size_t, float, void*);
		using Progress = std::function<void(size_t, float, void*)>;

		//Implement roughness filter of the specular term
		static void RoughnessFilter(
			Cubemap& dst,
			const std::vector<Cubemap>& levels,
			float linearRoughness,
			size_t maxNumSamples,
			glm::vec3 mirror, 
			bool prefilter,
			Progress updater = nullptr,
			void* userData = nullptr);

		static void DFG(Image& dst, bool multiscatter, bool cloth);

		static void DiffuseIrradiance(Cubemap& dst, const std::vector<Cubemap>& levels,
			size_t maxNumSamples = 1024, Progress updater = nullptr, void* userData = nullptr);
	};
}

#endif // __IBL_CUBEMAP_IBL_H__
