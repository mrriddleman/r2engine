#ifndef __IBL_CUBEMAP_H__
#define __IBL_CUBEMAP_H__

#include "ibl/Image.h"
#include "glm/glm.hpp"
#include <cmath>

namespace r2::ibl
{
	class Cubemap
	{
	public:

		enum class Face : uint8_t {
			PX = 0,     // left            +----+
			NX,         // right           | NY |
			PY,         // bottom     +----+----+----+----+
			NY,         // top        | NX | PZ | PX | NZ |
			PZ,         // back       +----+----+----+----+
			NZ          // front           | PY |
						//                 +----+
		};

		struct Texel
		{
			float r = 0.0f;
			float g = 0.0f;
			float b = 0.0f;

			Texel()
				:r(0.0f)
				,g(0.0f)
				,b(0.0f)
			{}

			Texel(float val)
				:r(val)
				,g(val)
				,b(val)
			{}

			Texel(float inR, float inG, float inB)
				:r(inR)
				,g(inG)
				,b(inB)
			{}

			Texel(const glm::vec3& vec)
				:r(vec.x)
				,g(vec.y)
				,b(vec.z)
			{}

			Texel(const Texel& t) = default;
			Texel& operator=(const Texel& t) = default;


			Texel& operator*=(float c)
			{
				r *= c;
				g *= c;
				b *= c;

				return *this;
			}

			Texel operator*(float c) const
			{
				Texel result;

				result.r = r * c;
				result.g = g * c;
				result.b = b * c;

				return result;
			}

			Texel operator+(const Texel& t) const
			{
				Texel result;

				result.r = r + t.r;
				result.g = g + t.g;
				result.b = b + t.b;

				return result;
			}

			Texel& operator+=(const Texel& t)
			{
				r += t.r;
				g += t.g;
				b += t.b;

				return *this;
			}

			Texel operator-(const Texel& t)
			{
				Texel result;

				result.r = r - t.r;
				result.g = g - t.g;
				result.b = b - t.b;

				return result;
			}

			friend Texel operator*(float c, const Texel& t)
			{
				Texel result;

				result = t * c;
				return result;
			}



		};

		explicit Cubemap(size_t dim);
		Cubemap(Cubemap&&);
		Cubemap& operator=(Cubemap&&);

		Cubemap(const Cubemap&) = delete;
		Cubemap& operator=(const Cubemap&) = delete;

		~Cubemap();

		size_t GetDimensions() const;

		void ResetDimensions(size_t dim);

		void SetImageForFace(Face face, const Image& image);

		inline const Image& GetImageForFace(Face face) const;

		inline Image& GetImageForFace(Face face);

		Image GetImageCopyForFace(Face face) const;

		static inline glm::vec2 Center(size_t x, size_t y);

		inline glm::vec3 GetDirectionFor(Face face, size_t x, size_t y) const;

		inline glm::vec3 GetDirectionFor(Face face, float x, float y) const;

		inline const Texel& SampleAt(const glm::vec3& direction) const;

		inline Texel FilterAt(const glm::vec3& direction) const;

		static Texel FilterAt(const Image& image, float x, float y);

		static Texel FilterAtCenter(const Image& image, size_t x, size_t y);

		static Texel TrilinearFilterAt(const Cubemap& c0, const Cubemap& c1, float lerp, const glm::vec3& direction);

		inline static const Texel& SampleAt(void const* data) {
			return *static_cast<Texel const*>(data);
		}

		inline static void WriteAt(void* data, const Texel& texel) {
			*static_cast<Texel*>(data) = texel;
		}

		void MakeSeamless();

		struct Address {
			Face face;
			float s = 0;
			float t = 0;
		};

		static Address GetAddressFor(const glm::vec3& direction);

	private:
		size_t mDimensions = 0;
		float mScale = 1.0f;
		float mUpperBound = 0;
		Image mFaces[6];
	};

	

	inline const Image& Cubemap::GetImageForFace(Face face) const
	{
		return mFaces[int(face)];
	}

	inline Image& Cubemap::GetImageForFace(Face face)
	{
		return mFaces[int(face)];
	}

	inline glm::vec2 Cubemap::Center(size_t x, size_t y)
	{
		return { x + 0.5f, y + 0.5f };
	}

	inline glm::vec3 Cubemap::GetDirectionFor(Face face, size_t x, size_t y) const 
	{
		return GetDirectionFor(face, x + 0.5f, y + 0.5f);
	}

	inline glm::vec3 Cubemap::GetDirectionFor(Face face, float x, float y) const
	{
		float cx = (x * mScale) - 1.0f;
		float cy = 1.0f - (y * mScale);

		glm::vec3 dir;
		const float l = std::sqrt(cx * cx + cy * cy + 1.0f);
		switch (face)
		{
		case Face::PX: dir = {  1.f, cy, -cx }; break;
		case Face::NX: dir = { -1.f, cy, cx }; break;
		case Face::PY: dir = { cx, 1.f, -cy }; break;
		case Face::NY: dir = { cx, -1.f, cy }; break;
		case Face::PZ: dir = { cx, cy, 1.f }; break;
		case Face::NZ: dir = { -cx, cy, -1.f }; break;

		}

		return dir * (1.0f / l);
	}

	inline const Cubemap::Texel&  Cubemap::SampleAt(const glm::vec3& direction) const
	{
		Cubemap::Address addr(GetAddressFor(direction));
		const size_t x = std::min(size_t(addr.s * mDimensions), mDimensions - 1);
		const size_t y = std::min(size_t(addr.t * mDimensions), mDimensions - 1);

		return SampleAt(GetImageForFace(addr.face).GetPixelPtr(x, y));
	}

	inline Cubemap::Texel Cubemap::FilterAt(const glm::vec3& direction) const
	{
		Cubemap::Address addr(GetAddressFor(direction));
		addr.s = std::min(addr.s * mDimensions, mUpperBound);
		addr.t = std::min(addr.t * mDimensions, mUpperBound);
		return FilterAt(GetImageForFace(addr.face), addr.s, addr.t);
	}
}


#endif // __IBL_CUBEMAP_H__
