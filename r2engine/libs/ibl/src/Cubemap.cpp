#include "ibl/Cubemap.h"

namespace r2::ibl
{
	Cubemap::Cubemap(Cubemap&& other)
		:mDimensions(other.mDimensions)
		,mScale(other.mScale)
		,mUpperBound(other.mUpperBound)
	{
		for (size_t i = 0; i < 6; ++i)
		{
			mFaces[i] = std::move(other.mFaces[i]);
		}
	}

	Cubemap& Cubemap::operator=(Cubemap&& other)
	{
		if (&other == this)
		{
			return *this;
		}

		mDimensions = other.mDimensions;
		mScale = other.mScale;
		mUpperBound = other.mUpperBound;

		for (size_t i = 0; i < 6; ++i)
		{
			mFaces[i] = std::move(other.mFaces[i]);
		}

		return *this;
	}

	Cubemap::~Cubemap() = default;

	Cubemap::Cubemap(size_t dim)
	{
		ResetDimensions(dim);
	}

	size_t Cubemap::GetDimensions() const
	{
		return mDimensions;
	}

	void Cubemap::ResetDimensions(size_t dim)
	{
		mDimensions = dim;
		mScale = 2.0f / dim;
		mUpperBound = std::nextafter((float)mDimensions, 0.0f);
		for (auto& face : mFaces)
		{
			face.Reset();
		}
	}

	void Cubemap::SetImageForFace(Face face, const Image& image)
	{
		mFaces[size_t(face)].Set(image);
	}

	Image Cubemap::GetImageCopyForFace(Face face) const
	{
		const Image& faceImage = GetImageForFace(face);

		size_t dim = GetDimensions();

		Image newImage(dim, dim);

		for (size_t y = 0; y < dim; ++y)
		{
			//memcpy(newImage.GetPixelPtr(0, y), faceImage.GetPixelPtr(0, y), faceImage.GetBytesPerRow());

			for (size_t x = 0; x < dim; ++x)
			{
				Cubemap::Texel* t = static_cast<Cubemap::Texel*>(newImage.GetPixelPtr(x, y));

				*t = *static_cast<Cubemap::Texel*>(faceImage.GetPixelPtr(x, y));
			}
		}

		return newImage;
	}

	r2::ibl::Cubemap::Texel Cubemap::FilterAt(const Image& image, float x, float y)
	{
		const size_t x0 = size_t(x);
		const size_t y0 = size_t(y);

		size_t x1 = x0 + 1;
		size_t y1 = y0 + 1;

		const float u = float(x - x0);
		const float v = float(y - y0);
		const float one_minus_u = 1 - u;
		const float one_minus_v = 1 - v;
		const Texel& c0 = SampleAt(image.GetPixelPtr(x0, y0));
		const Texel& c1 = SampleAt(image.GetPixelPtr(x1, y0));
		const Texel& c2 = SampleAt(image.GetPixelPtr(x0, y1));
		const Texel& c3 = SampleAt(image.GetPixelPtr(x1, y1));

		return (one_minus_u * one_minus_v) * c0 + (u * one_minus_v) * c1 + (one_minus_u * v) * c2 + (u * v) * c3;
	}

	r2::ibl::Cubemap::Texel Cubemap::FilterAtCenter(const Image& image, size_t x, size_t y)
	{
		size_t x1 = x + 1;
		size_t y1 = y + 1;
		const Texel& c0 = SampleAt(image.GetPixelPtr(x, y));
		const Texel& c1 = SampleAt(image.GetPixelPtr(x1, y));
		const Texel& c2 = SampleAt(image.GetPixelPtr(x, y1));
		const Texel& c3 = SampleAt(image.GetPixelPtr(x1, y1));

		return (c0 + c1 + c2 + c3) * 0.25f;
	}

	r2::ibl::Cubemap::Texel Cubemap::TrilinearFilterAt(const Cubemap& l0, const Cubemap& l1, float lerp, const glm::vec3& direction)
	{
		Cubemap::Address addr(GetAddressFor(direction));
		const Image& i0 = l0.GetImageForFace(addr.face);
		const Image& i1 = l1.GetImageForFace(addr.face);

		float x0 = std::min(addr.s * l0.mDimensions, l0.mUpperBound);
		float y0 = std::min(addr.t * l0.mDimensions, l0.mUpperBound);
		float x1 = std::min(addr.s * l1.mDimensions, l1.mUpperBound);
		float y1 = std::min(addr.t * l1.mDimensions, l1.mUpperBound);

		Texel c0 = FilterAt(i0, x0, y0);
		c0 += lerp * (FilterAt(i1, x1, y1) - c0);
		return c0;
	}

	r2::ibl::Cubemap::Address Cubemap::GetAddressFor(const glm::vec3& direction)
	{
		Cubemap::Address addr;

		float sc, tc, ma;
		const float rx = std::abs(direction.x);
		const float ry = std::abs(direction.y);
		const float rz = std::abs(direction.z);

		if (rx >= ry && rx >= rz)
		{
			ma = 1.0f / rx;
			if (direction.x >= 0)
			{
				addr.face = Face::PX;
				sc = -direction.z;
				tc = -direction.y;
			}
			else
			{
				addr.face = Face::NX;
				sc = direction.z;
				tc = -direction.y;
			}
		}
		else if (ry >= rx && ry >= rz)
		{
			ma = 1.0f / ry;
			if (direction.y >= 0)
			{
				addr.face = Face::PY;
				sc = direction.x;
				tc = direction.z;
			}
			else
			{
				addr.face = Face::NY;
				sc = direction.x;
				tc = -direction.z;
			}
		}
		else
		{
			ma = 1.0f / rz;
			if (direction.z >= 0)
			{
				addr.face = Face::PZ;
				sc = direction.x;
				tc = -direction.y;
			}
			else
			{
				addr.face = Face::NZ;
				sc = -direction.x;
				tc = -direction.y;
			}
		}

		addr.s = (sc * ma + 1.0f) * 0.5f;
		addr.t = (tc * ma + 1.0f) * 0.5f;

		return addr;
	}

	void Cubemap::MakeSeamless()
	{
		size_t dim = GetDimensions();
		size_t D = dim;

		const size_t bpr = GetImageForFace(Face::NX).GetBytesPerRow();
		const size_t bpp = GetImageForFace(Face::NX).GetBytesPerPixel();

		auto GetTexel = [](Image& image, size_t x, size_t y) -> Texel* {
			return (Texel*)((uint8_t*)image.GetData() + x * image.GetBytesPerPixel() + y * image.GetBytesPerRow());
		};

		auto Stitch = [&] (
			Face faceDst, size_t xdst, size_t ydst, size_t incDst,
			Face faceSrc, size_t xsrc, size_t ysrc, size_t incSrc)
		{
			Image& imageDst = GetImageForFace(faceDst);
			Image& imageSrc = GetImageForFace(faceSrc);
			Texel* dst = GetTexel(imageDst, xdst, ydst);
			Texel* src = GetTexel(imageSrc, xsrc, ysrc);

			for (size_t i = 0; i < dim; ++i)
			{
				*dst = *src;
				dst = (Texel*)((uint8_t*)dst + incDst);
				src = (Texel*)((uint8_t*)src + incSrc);
			}
			
		};

		auto Corners = [&](Face face) {
			size_t L = D - 1;
			Image& image = GetImageForFace(face);
			*GetTexel(image, -1, -1) = (*GetTexel(image, 0, 0) + *GetTexel(image, -1, 0) + *GetTexel(image, 0, -1)) * float(1.0f/3.0f);
			*GetTexel(image, L+1, -1) = (*GetTexel(image, L, 0) + *GetTexel(image, L, -1) + *GetTexel(image, L+1, 0)) * float(1.0f / 3.0f);
			*GetTexel(image, -1, L+1) = (*GetTexel(image, 0, L) + *GetTexel(image, -1, L) + *GetTexel(image, 0, L+1)) * float(1.0f / 3.0f);
			*GetTexel(image, L+1, L+1) = (*GetTexel(image, L, L) + *GetTexel(image, L+1, L) + *GetTexel(image, L+1, L)) * float(1.0f / 3.0f);
		};


		// +Y / Top
		Stitch(Face::PY, -1, 0, bpr, Face::NX, 0, 0, bpp);      // left
		Stitch(Face::PY, 0, -1, bpp, Face::NZ, D - 1, 0, -bpp);      // top
		Stitch(Face::PY, D, 0, bpr, Face::PX, D - 1, 0, -bpp);      // right
		Stitch(Face::PY, 0, D, bpp, Face::PZ, 0, 0, bpp);      // bottom
		Corners(Face::PY);

		// -X / Left
		Stitch(Face::NX, -1, 0, bpr, Face::NZ, D - 1, 0, bpr);      // left
		Stitch(Face::NX, 0, -1, bpp, Face::PY, 0, 0, bpr);      // top
		Stitch(Face::NX, D, 0, bpr, Face::PZ, 0, 0, bpr);      // right
		Stitch(Face::NX, 0, D, bpp, Face::NY, 0, D - 1, -bpr);      // bottom
		Corners(Face::NX);

		// +Z / Front
		Stitch(Face::PZ, -1, 0, bpr, Face::NX, D - 1, 0, bpr);      // left
		Stitch(Face::PZ, 0, -1, bpp, Face::PY, 0, D - 1, bpp);      // top
		Stitch(Face::PZ, D, 0, bpr, Face::PX, 0, 0, bpr);      // right
		Stitch(Face::PZ, 0, D, bpp, Face::NY, 0, 0, bpp);      // bottom
		Corners(Face::PZ);

		// +X / Right
		Stitch(Face::PX, -1, 0, bpr, Face::PZ, D - 1, 0, bpr);      // left
		Stitch(Face::PX, 0, -1, bpp, Face::PY, D - 1, D - 1, -bpr);      // top
		Stitch(Face::PX, D, 0, bpr, Face::NZ, 0, 0, bpr);      // right
		Stitch(Face::PX, 0, D, bpp, Face::NY, D - 1, 0, bpr);      // bottom
		Corners(Face::PX);

		// -Z / Back
		Stitch(Face::NZ, -1, 0, bpr, Face::PX, D - 1, 0, bpr);      // left
		Stitch(Face::NZ, 0, -1, bpp, Face::PY, D - 1, 0, -bpp);      // top
		Stitch(Face::NZ, D, 0, bpr, Face::NX, 0, 0, bpr);      // right
		Stitch(Face::NZ, 0, D, bpp, Face::NY, D - 1, D - 1, -bpp);      // bottom
		Corners(Face::NZ);

		// -Y / Bottom
		Stitch(Face::NY, -1, 0, bpr, Face::NX, D - 1, D - 1, -bpp);      // left
		Stitch(Face::NY, 0, -1, bpp, Face::PZ, 0, D - 1, bpp);      // top
		Stitch(Face::NY, D, 0, bpr, Face::PX, 0, D - 1, bpp);      // right
		Stitch(Face::NY, 0, D, bpp, Face::NZ, D - 1, D - 1, -bpp);      // bottom
		Corners(Face::NY);
	}

}