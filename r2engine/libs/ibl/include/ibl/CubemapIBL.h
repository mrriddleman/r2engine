#ifndef __IBL_CUBEMAP_IBL_H__
#define __IBL_CUBEMAP_IBL_H__

#include <vector>
#include <functional>

namespace r2::ibl
{
	class Cubemap;

	class CubemapIBL
	{
	public:
		//typedef void (*Progress)(size_t, float, void*);
		using Progress = std::function<void(size_t, float, void*)>;


		static void DiffuseIrradiance(Cubemap& dst, const std::vector<Cubemap>& levels,
			size_t maxNumSamples = 1024, Progress updater = nullptr, void* userData = nullptr);
	};
}

#endif // __IBL_CUBEMAP_IBL_H__
