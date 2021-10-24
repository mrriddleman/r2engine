#include "ibl/CubemapUtils.h"
#include "ibl/Utils.h"
#include "glm/glm.hpp"
#include <cmath>
#include <algorithm>
#include "stb_image_write.h"

namespace
{
	const float PI = 3.14159f;
	constexpr const double F_1_PI = 0.318309886183790671537767526745028724;
}

namespace r2::ibl
{

	r2::ibl::Cubemap CubemapUtils::CreateCubemap(Image& image, size_t dim, bool horizontal /*= true*/)
	{
		Cubemap cm(dim);
		Image temp(CreateCubemapImage(dim, horizontal));
		SetAllFacesFromCross(cm, temp);
		std::swap(image, temp);
		return cm;
	}

	void CubemapUtils::Clamp(Image& src)
	{
		// See: http://graphicrants.blogspot.com/2013/12/tone-mapping.html
		// By Brian Karis
		auto compress = [](glm::vec3 color, float linear, float compressed)
		{
			float luma = glm::dot(color, glm::vec3(0.2126, 0.7152, 0.0722));
			return luma <= linear ? color :
				(color / luma) * ((linear * linear - compressed * luma) / (2 * linear - compressed - luma));
		};

		const size_t width = src.GetWidth();
		const size_t height = src.GetHeight();
		for (size_t y = 0; y < height; ++y)
		{
			for (size_t x = 0; x < width; ++x)
			{
				glm::vec3& c = *static_cast<glm::vec3*>(src.GetPixelPtr(x, y));
				c = compress(c, 4096.0f, 16384.0f);
			}
		}

	}

	void CubemapUtils::Highlight(Image& src)
	{
		/*const size_t width = src.GetWidth();
		const size_t height = src.GetHeight();
		for (size_t y = 0; y < height; ++y)
		{
			for (size_t x = 0; x < width; ++x)
			{
				glm::vec3& c = *static_cast<glm::vec3*>(src.GetPixelPtr(x, y));
				if (min(c) < 0.0f)
				{
					c = { 0, 0, 1 };
				}
				else if (max(c) > 64512.0f)
				{
					c = { 1, 0, 0 };
				}
			}
		}*/
	}

	void CubemapUtils::DownsampleBoxFilter(Cubemap& dst, const Cubemap& src)
	{
		size_t scale = src.GetDimensions() / dst.GetDimensions();
		const size_t dim = dst.GetDimensions();

		for (size_t faceIndex = 0; faceIndex < 6; faceIndex++)
		{
			const Cubemap::Face f = (Cubemap::Face)faceIndex;
			Image& image = dst.GetImageForFace(f);
			for (size_t y = 0; y < dim; ++y)
			{
				Cubemap::Texel* data = static_cast<Cubemap::Texel*>(image.GetPixelPtr(0, y));

				const Image& srcImage(src.GetImageForFace(f));

				for (size_t x = 0; x < dim; ++x, ++data)
				{
					Cubemap::WriteAt(data, Cubemap::FilterAtCenter(srcImage, x * scale, y * scale));
				}
			}
		}
	}

	const char* CubemapUtils::GetFaceName(Cubemap::Face face)
	{
		/*
			PX = 0,     // left            +----+
			NX,         // right           | PY |
			PY,         // bottom     +----+----+----+----+
			NY,         // top        | NX | PZ | PX | NZ |
			PZ,         // back       +----+----+----+----+
			NZ          // front           | NY |
						//                 +----+
						*/

		//@NOTE: we're remapping this to the internal engine layout. ie. reversing right and left, front and back, top and bottom (?)
		switch (face)
		{
		case r2::ibl::Cubemap::Face::PX:
			return "right";
			break;
		case r2::ibl::Cubemap::Face::NX:
			
			return "left";
			break;
		case r2::ibl::Cubemap::Face::PY:
			return "top";
			break; 
		case r2::ibl::Cubemap::Face::NY:
			return "bottom";
			break;
		case r2::ibl::Cubemap::Face::PZ:
			return "front";
			break;
		case r2::ibl::Cubemap::Face::NZ:
			 
			return "back";
			break;
		default:
			return "";
			break;
		}
	}

	static inline float SphereQuadrantArea(float x, float y)
	{
		return std::atan2(x * y, std::sqrt(x * x + y * y + 1));
	}


	float CubemapUtils::SolidAngle(size_t dim, size_t u, size_t v)
	{
		const float iDim = 1.0f / dim;
		float s = ((u + 0.5f) * 2 * iDim) - 1;
		float t = ((v + 0.5f) * 2 * iDim) - 1;
		const float x0 = s - iDim;
		const float y0 = t - iDim;
		const float x1 = s + iDim;
		const float y1 = t + iDim;

		float solidAngle = 
			SphereQuadrantArea(x0, y0) -
			SphereQuadrantArea(x0, y1) -
			SphereQuadrantArea(x1, y0) +
			SphereQuadrantArea(x1, y1);

		return solidAngle;
	}

	void CubemapUtils::SetAllFacesFromCross(Cubemap& cm, const Image& image)
	{
		CubemapUtils::SetFaceFromCross(cm, Cubemap::Face::NX, image);
		CubemapUtils::SetFaceFromCross(cm, Cubemap::Face::PX, image);
		CubemapUtils::SetFaceFromCross(cm, Cubemap::Face::NY, image);
		CubemapUtils::SetFaceFromCross(cm, Cubemap::Face::PY, image);
		CubemapUtils::SetFaceFromCross(cm, Cubemap::Face::NZ, image);
		CubemapUtils::SetFaceFromCross(cm, Cubemap::Face::PZ, image);
	}

	void CubemapUtils::CrossToCubemap(Cubemap& dst, const Image& src)
	{
		const size_t dimension = dst.GetDimensions();

		for (size_t faceIndex = 0; faceIndex < 6; faceIndex++)
		{
			const Cubemap::Face f = (Cubemap::Face)faceIndex;
			Image& image = dst.GetImageForFace(f);
			for (size_t y = 0; y < dimension; ++y)
			{
				Cubemap::Texel* data = static_cast<Cubemap::Texel*>(image.GetPixelPtr(0, y));
				for (size_t ix = 0; ix < dimension; ++ix, ++data)
				{
					size_t x = ix;
					
					size_t dx = 0;
					size_t dy = 0;
					size_t dim = std::max(src.GetHeight(), src.GetWidth()) / 4;

					switch (f)
					{
					case Cubemap::Face::NX:
						dx = 0, y = dim;
						break;
					case Cubemap::Face::PX:
						dx = 2 * dim, dy = dim;
						break;
					case Cubemap::Face::NY:
						dx = dim, dy = 2 * dim;
						break;
					case Cubemap::Face::PY:
						dx = dim, dy = 0;
						break;
					case Cubemap::Face::NZ:
						if (src.GetHeight() > src.GetWidth())
						{
							dx = dim, dy = 3 * dim;
							x = dimension - 1 - ix;
							y = dimension - 1 - y;
						}
						else
						{
							dx = 3 * dim, dy = dim;
						}
						break;
					case Cubemap::Face::PZ:
						dx = dim, dy = dim;
						break;
					}

					size_t sampleCount = std::max(size_t(1), dim / dimension);
					sampleCount = std::min(size_t(256), sampleCount * sampleCount);
					for (size_t i = 0; i < sampleCount; ++i)
					{
						const glm::vec2 h = Hammersley(uint32_t(i), 1.0f / sampleCount);
						size_t u = dx + size_t((x + h.x) * dim / dimension);
						size_t v = dy + size_t((y + h.y) * dim / dimension);
						Cubemap::WriteAt(data, Cubemap::SampleAt(src.GetPixelPtr(u, v)));
					}
				}
			}
		}
	}

	void CubemapUtils::EquirectangularToCubemap(Cubemap& dst, const Image& src)
	{
		const size_t width = src.GetWidth();
		const size_t height = src.GetHeight();

		auto toRectilinear = [width, height](glm::vec3 s) -> glm::vec2 {
			float xf = std::atan2(s.x, s.z) * F_1_PI; 
			float yf = std::asin(s.y) * (2.0f * F_1_PI);
			xf = (xf + 1.0f) * 0.5f * (width - 1);
			yf = (1.0f - yf) * 0.5f * (height - 1);
			
			return glm::vec2(xf, yf);
		};

		const size_t dim = dst.GetDimensions();

		for (size_t faceIndex = 0; faceIndex < 6; faceIndex++)
		{
			const Cubemap::Face f = (Cubemap::Face)faceIndex;
			Image& image( dst.GetImageForFace(f) );

			for (size_t y = 0; y < dim; ++y)
			{
				Cubemap::Texel* data = static_cast<Cubemap::Texel*>(image.GetPixelPtr(0, y));

				for (size_t x = 0; x < dim; ++x, ++data)
				{
					auto pos0 = toRectilinear(dst.GetDirectionFor(f, x + 0.0f, y + 0.0f));
					auto pos1 = toRectilinear(dst.GetDirectionFor(f, x + 1.0f, y + 0.0f));
					auto pos2 = toRectilinear(dst.GetDirectionFor(f, x + 0.0f, y + 1.0f));
					auto pos3 = toRectilinear(dst.GetDirectionFor(f, x + 1.0f, y + 1.0f));
					const float minx = std::min(pos0.x, std::min(pos1.x, std::min(pos2.x, pos3.x)));
					const float maxx = std::max(pos0.x, std::max(pos1.x, std::max(pos2.x, pos3.x)));
					const float miny = std::min(pos0.y, std::min(pos1.y, std::min(pos2.y, pos3.y)));
					const float maxy = std::max(pos0.y, std::max(pos1.y, std::max(pos2.y, pos3.y)));
					const float dx = std::max(1.0f, maxx - minx);
					const float dy = std::max(1.0f, maxy - miny);
					const size_t numSamples = 4;

					const float iNumSamples = 1.0f / numSamples;
					Cubemap::Texel c(0.0f);
					for (size_t sample = 0; sample < numSamples; ++sample)
					{
						const glm::vec2 h = Hammersley(uint32_t(sample), iNumSamples);
						const glm::vec3 s(dst.GetDirectionFor(f, x + h.x , y + h.y));

						auto pos = toRectilinear(s);

						//@TODO(Serge): filtering
						c += Cubemap::SampleAt(src.GetPixelPtr((size_t)pos.x, (size_t)pos.y));
					}

					c *= iNumSamples;

					Cubemap::WriteAt(data, c);
				}
			}			
		}
	}

	void CubemapUtils::GenerateUVGrid(Cubemap& cubemap, size_t gridFrequencyX, size_t gridFrequencyY)
	{
		Cubemap::Texel const colors[6] = {
			{ 1, 1, 1}, // +X / r - white
			{ 1, 0, 0}, // -X / l - red
			{ 0, 0, 1}, // +Y / t - blue
			{ 0, 1, 0}, // -Y / b - green
			{ 1, 1, 0}, // +Z / fr - yellow
			{ 1, 0, 1} // -Z / bk - magenta
		};

		const float uvGridHDRIntensity = 5.0f;
		size_t gridSizeX = cubemap.GetDimensions() / gridFrequencyX;
		size_t gridSizeY = cubemap.GetDimensions() / gridFrequencyY;

		const size_t dimension = cubemap.GetDimensions();

		for (size_t faceIndex = 0; faceIndex < 6; faceIndex++)
		{
			const Cubemap::Face f = (Cubemap::Face)faceIndex;
			Image& image = cubemap.GetImageForFace(f);
			for (size_t y = 0; y < dimension; ++y)
			{
				Cubemap::Texel* data = static_cast<Cubemap::Texel*>(image.GetPixelPtr(0, y));

				for (size_t x = 0; x < dimension; ++x, ++data)
				{
					bool grid = bool(((x / gridSizeX) ^ (y / gridSizeY)) & 1);
					Cubemap::Texel t = grid ? colors[(int)f] * uvGridHDRIntensity : Cubemap::Texel(0);
					Cubemap::WriteAt(data, t);
				}
			}
		}
	}

	void CubemapUtils::SetFaceFromCross(Cubemap& cm, Cubemap::Face face, const Image& image)
	{
		size_t dim = cm.GetDimensions() + 2; //2 extra per image, for seamless
		size_t x = 0;
		size_t y = 0;
		switch (face)
		{
		case Cubemap::Face::NX:
			x = 0, y = dim;
			break;
		case Cubemap::Face::PX:
			x = 2 * dim, y = dim;
			break;
		case Cubemap::Face::NY:
			x = dim, y = 2 * dim;
			break;
		case Cubemap::Face::PY:
			x = dim, y = 0;
			break;
		case Cubemap::Face::NZ:
			x = 3 * dim, y = dim;
			break;
		case Cubemap::Face::PZ:
			x = dim, y = dim;
			break;
		}

		Image subImage;
		subImage.SubImage(image, x + 1, y + 1, dim - 2, dim - 2);
		cm.SetImageForFace(face, subImage);
	}

	r2::ibl::Image CubemapUtils::CreateCubemapImage(size_t dim, bool horizontal /*= true*/)
	{
		size_t width = 4 * (dim + 2);
		size_t height = 3 * (dim + 2);

		if (!horizontal)
		{
			std::swap(width, height);
		}

		Image image(width, height);
		memset(image.GetData(), 0, image.GetBytesPerRow() * height);
		return image;
	}


	void CubemapUtils::MirrorCubemap(Cubemap& dst, const Cubemap& src)
	{
		const size_t dimension = dst.GetDimensions();

		for (size_t faceIndex = 0; faceIndex < 6; faceIndex++)
		{
			const Cubemap::Face f = (Cubemap::Face)faceIndex;
			Image& image = dst.GetImageForFace(f);

			for (size_t y = 0; y < dimension; ++y)
			{

				Cubemap::Texel* data = static_cast<Cubemap::Texel*>(image.GetPixelPtr(0, y));

				for (size_t x = 0; x < dimension; ++x, ++data)
				{
					const glm::vec3 N(dst.GetDirectionFor(f, x, y));
					Cubemap::WriteAt(data, src.SampleAt(glm::vec3(-N.x, N.y, N.z)));
				}
			}
		}
	}
}