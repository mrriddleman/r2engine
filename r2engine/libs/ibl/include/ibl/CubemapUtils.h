#ifndef __IBL_CUBEMAP_UTILS_H__
#define __IBL_CUBEMAP_UTILS_H__

#include "ibl/Cubemap.h"
#include "ibl/Image.h"


namespace r2::ibl
{
	class CubemapUtils
	{
	public:

		static Cubemap CreateCubemap(Image& image, size_t dim, bool horizontal = true);

		static void Clamp(Image& src);
		static void Highlight(Image& src);
		static void DownsampleBoxFilter(Cubemap& dst, const Cubemap& src);
		static const char* GetFaceName(Cubemap::Face face);
		static float SolidAngle(size_t dim, size_t u, size_t v);
		static void SetAllFacesFromCross(Cubemap& cm, const Image& image);
		static void CrossToCubemap(Cubemap& dst, const Image& src);
		static void EquirectangularToCubemap(Cubemap& dst, const Image& src);
		static void GenerateUVGrid(Cubemap& cubemap, size_t gridFrequencyX, size_t gridFrequencyY);
		static void MirrorCubemap(Cubemap& dst, const Cubemap& src);

	private:
		static void SetFaceFromCross(Cubemap& cm, Cubemap::Face face, const Image& image);
		static Image CreateCubemapImage(size_t dim, bool horizontal = true);
	};
}

#endif // __IBL_CUBEMAP_UTILS_H__
