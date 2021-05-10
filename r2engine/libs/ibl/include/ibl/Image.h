#ifndef __IBL_IMAGE_H__
#define __IBL_IMAGE_H__

#include <cstdint>
#include <string>
#include <memory>
#include <cassert>

namespace r2::ibl
{
	class Image
	{
	public:
		Image() = default;
		Image(size_t w, size_t h, size_t stride = 0);

		Image(Image&& image) noexcept;
		Image& operator=(Image&& image) noexcept;
		Image(const Image& image) = delete;
		Image& operator=(const Image& image) = delete;


		~Image();

		void Reset();
		void Set(const Image& image);

		static int LoadImage(const std::string& path, Image& image);

		void SubImage(const Image& image, size_t x, size_t y, size_t w, size_t h);

		inline bool IsValid() const { return mImageData != nullptr; }
		inline size_t GetStride() const { return GetBytesPerRow() / GetBytesPerPixel(); }
		inline size_t GetWidth() const { return mWidth; }
		inline size_t GetHeight() const { return mHeight; }
		inline size_t GetBytesPerRow() const { return mBytesPerRow; }
		inline void* GetData() const { return mImageData; }
		inline size_t GetSize() const { return GetBytesPerRow() * mHeight; }
		inline size_t GetBytesPerPixel() const { return sizeof(float)*3; }
		inline void* GetPixelPtr(size_t x, size_t y) const {
			assert(IsValid());
			return static_cast<uint8_t*>(mImageData) + y * GetBytesPerRow() + x * GetBytesPerPixel();
		}
		
	private:

		void FreeImage();

		size_t mWidth = 0;
		size_t mHeight = 0;
		size_t mBytesPerRow = 0;
		std::unique_ptr<uint8_t[]> mOwnedData;
		void* mImageData = nullptr;
		bool mLoadedFromFile = false;
	};
}

#endif