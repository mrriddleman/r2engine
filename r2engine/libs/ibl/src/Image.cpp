#include "ibl/Image.h"
#include "stb_image.h"

namespace r2::ibl
{
	Image::Image(size_t w, size_t h, size_t stride)
		: mWidth(w)
		, mHeight(h)
		, mBytesPerRow((stride ? stride : w) * sizeof(float) * 3)
		, mOwnedData(new uint8_t[mBytesPerRow * h] )
		, mImageData(mOwnedData.get())
		, mLoadedFromFile(false)
	{
	}

	Image::Image(Image&& image) noexcept
		: mWidth(image.mWidth)
		, mHeight(image.mHeight)
		, mBytesPerRow(image.mBytesPerRow)
		, mOwnedData(std::move(image.mOwnedData))
		, mImageData(image.mImageData)
		, mLoadedFromFile(image.mLoadedFromFile)
	{
		image.mOwnedData.release();
		image.mImageData = nullptr;

	}

	Image& Image::operator=(Image&& image) noexcept
	{

		if (this == &image)
		{
			return *this;
		}

		FreeImage();

		mWidth = image.mWidth;
		mHeight = image.mHeight;
		mBytesPerRow = image.mBytesPerRow;
		mOwnedData = std::move(image.mOwnedData);
		mImageData = image.mImageData;
		mLoadedFromFile = image.mLoadedFromFile;


		image.mOwnedData.release();
		image.mImageData = nullptr;

		return *this;
	}

	

	Image::~Image()
	{
		FreeImage();
	}

	void Image::Reset()
	{
		FreeImage();
	}

	void Image::Set(const Image& image)
	{
		FreeImage();

		mWidth = image.mWidth;
		mHeight = image.mHeight;
		mBytesPerRow = image.mBytesPerRow;
		
		mImageData = image.mImageData;
	}

	int Image::LoadImage(const std::string& path, Image& image)
	{
		if (image.IsValid())
		{
			image.Reset();
		}

		int isHDR = stbi_is_hdr(path.c_str());

		int w, h, nrComponents;
		void* imageData = nullptr;
		int bytesPerChannel = 0;

		if (isHDR)
		{
			imageData = stbi_loadf(path.c_str(), &w, &h, &nrComponents, 0);
			bytesPerChannel = sizeof(float);
		}
		else
		{
			imageData = stbi_load(path.c_str(), &w, &h, &nrComponents, 0);
			bytesPerChannel = sizeof(uint8_t);
		}

		if (imageData == nullptr)
		{
			assert(false && "We couldn't load the image!");
			return 0;
		}
			
		image.mWidth = w;
		image.mHeight = h;
		image.mBytesPerRow = w * bytesPerChannel * nrComponents;
		image.mOwnedData.reset(static_cast<uint8_t*>(imageData));
		image.mImageData = image.mOwnedData.get();
		image.mLoadedFromFile = true;

		return nrComponents;
	}

	void Image::SubImage(const Image& image, size_t x, size_t y, size_t w, size_t h)
	{
		FreeImage();
		mWidth = w;
		mHeight = h;
		mBytesPerRow = image.GetBytesPerRow();
		mImageData = static_cast<uint8_t*>(image.GetPixelPtr(x, y));
	}
	
	void Image::FreeImage()
	{
		if (mLoadedFromFile)
		{
			stbi_image_free(mImageData);
			mLoadedFromFile = false;
		}

		mOwnedData.release();
		 
		mWidth = 0;
		mHeight = 0;
		mBytesPerRow = 0;
		mImageData = nullptr;
	}
}

