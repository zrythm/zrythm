#ifndef TOMATL_WINDOWS_BITMAP_IMAGE
#define TOMATL_WINDOWS_BITMAP_IMAGE

#ifdef JUCE_WINDOWS
#include <Windows.h>
#endif

// The idea of this hack is to be able use the same pixelStride used internally in windows.
// This would significantly improve drawImageAt performance in case of RGB Image
class TomatlPixelData : public ImagePixelData
{
public:
	TomatlPixelData(const Image::PixelFormat format_, const int w, const int h, const bool clearImage, bool always32bit)
		: ImagePixelData(format_, w, h),
		pixelStride((format_ == Image::RGB && !always32bit) ? 3 : ((format_ == Image::ARGB || always32bit) ? 4 : 1)),
		lineStride((pixelStride * jmax(1, w) + 3) & ~3),
		mAlways32Bit(always32bit)
	{
		imageData.allocate((size_t)(lineStride * jmax(1, h)), clearImage);
	}

	LowLevelGraphicsContext* createLowLevelContext() override
	{
		sendDataChangeMessage();
		return new LowLevelGraphicsSoftwareRenderer(Image(this));
	}

	void initialiseBitmapData(Image::BitmapData& bitmap, int x, int y, Image::BitmapData::ReadWriteMode mode) override
	{
		bitmap.data = imageData + x * pixelStride + y * lineStride;
		bitmap.pixelFormat = pixelFormat;
		bitmap.lineStride = lineStride;
		bitmap.pixelStride = pixelStride;

		if (mode != Image::BitmapData::readOnly)
			sendDataChangeMessage();
	}

	Ptr clone() override
	{
		TomatlPixelData* s = new TomatlPixelData(pixelFormat, width, height, false, mAlways32Bit);
		memcpy(s->imageData, imageData, (size_t)(lineStride * height));
		return s;
	}

	ImageType* createType() const override    { return new SoftwareImageType(); }

private:
	HeapBlock<uint8> imageData;
	const int pixelStride, lineStride;
	bool mAlways32Bit;
	JUCE_LEAK_DETECTOR(TomatlPixelData)
};

class TomatlImageType : public ImageType
{
public:
	TomatlImageType() {}
	~TomatlImageType() {}

	static bool alwaysUse32bitPerPixel()
	{
#ifdef JUCE_WINDOWS
		HDC dc = GetDC(0);
		const int bitsPerPixel = GetDeviceCaps(dc, BITSPIXEL);
		ReleaseDC(0, dc);
		return bitsPerPixel > 24;
#else
		return false;
#endif
	}

	ImagePixelData::Ptr create(Image::PixelFormat format, int width, int height, bool clearImage) const override
	{
		return new TomatlPixelData(format, width, height, clearImage, alwaysUse32bitPerPixel());
	}

	int getTypeID() const override
	{
		return 100;
	}
};

#endif