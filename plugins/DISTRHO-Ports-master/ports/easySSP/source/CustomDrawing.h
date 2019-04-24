#ifndef TOMATL_JUCE_CUSTOM_DRAWING
#define TOMATL_JUCE_CUSTOM_DRAWING

#include "JuceHeader.h"

namespace tomatl { namespace draw {

	struct ColorARGB
	{
		uint8 channel[4];

		void fromColor(const Colour& curveColor)
		{
			// TODO: byte order
			this->channel[0] = curveColor.getBlue();
			this->channel[1] = curveColor.getGreen();
			this->channel[2] = curveColor.getRed();
			this->channel[3] = curveColor.getAlpha();
		}
	};

class Util
{
public:

	static forcedinline void blend(Image& background, Image& subject, Image& surface)
	{
		Image::BitmapData srfpix(surface, Image::BitmapData::readWrite);
		Image::BitmapData backpix(background, Image::BitmapData::readWrite);
		Image::BitmapData subpix(subject, Image::BitmapData::readWrite);

		// TODO: crash or fallback
		if (!(srfpix.pixelStride == backpix.pixelStride &&
			backpix.pixelStride == subpix.pixelStride &&
			subpix.pixelStride == 4 &&
			subpix.pixelFormat == Image::ARGB))
		{
			return;
		}


		int h = std::min(subpix.height, std::min(backpix.height, srfpix.height));
		int w = std::min(subpix.width, std::min(backpix.width, srfpix.width));

		uint8 alphaOffset = 3;

		uint8 balpha;
		size_t offset /*, foffset = 0*/;

		uint8* srfline;
		uint8* subline;
		uint8* backline;

		uint16 reg;

		for (int i = 0; i < h; ++i)
		{
			srfline = srfpix.getLinePointer(i);
			subline = subpix.getLinePointer(i);
			backline = backpix.getLinePointer(i);

			offset = 0;

			for (int j = 0; j < w; ++j)
			{
				balpha = 255 - subline[offset + alphaOffset];

				//*(uint32*)(srfline + offset) = (*(uint32*)(subline + offset) & 0xFF00FF00) + TOMATL_FAST_DIVIDE_BY_255(balpha * (*(uint32*)(backline + offset) & 0xFF00FF00));

				reg = backline[offset + 0] * balpha;
				srfline[offset + 0] = TOMATL_FAST_DIVIDE_BY_255(reg);
				srfline[offset + 0] += subline[offset + 0];

				reg = backline[offset + 1] * balpha;
				srfline[offset + 1] = TOMATL_FAST_DIVIDE_BY_255(reg);
				srfline[offset + 1] += subline[offset + 1];

				reg = backline[offset + 2] * balpha;
				srfline[offset + 2] = TOMATL_FAST_DIVIDE_BY_255(reg);
				srfline[offset + 2] += subline[offset + 2];

				//srfline[offset + 0] = subline[offset + 0] + TOMATL_FAST_DIVIDE_BY_255(backline[offset + 0] * balpha);
				//srfline[offset + 1] = subline[offset + 1] + TOMATL_FAST_DIVIDE_BY_255(backline[offset + 1] * balpha);
				//srfline[offset + 2] = subline[offset + 2] + TOMATL_FAST_DIVIDE_BY_255(backline[offset + 2] * balpha);

				offset += subpix.pixelStride;
			}
		}
	}

	// TODO: will not decay to 0 properly
	static void multiplyAlphas(Image::BitmapData& pixels, float multiplier)
	{
		size_t pixelCount = pixels.height * pixels.width;
		uint8* start = pixels.getPixelPointer(0, 0);

		for (int i = 0; i < pixels.height; ++i)
		{
			start = pixels.getLinePointer(i);

			for (int j = 0; j < pixels.width; ++j)
			{
				//start[j * i + 1] = result
			}
		}

		for (size_t i = 0; i < pixelCount; ++i)
		{
			float result = start[pixels.pixelStride * i + 1] * multiplier;
			start[pixels.pixelStride * i + 1] = result < 1 ? 0 : result;
		}
	}

	static void setAlphas(Image::BitmapData& /*pixels*/, uint8 /*alpha*/)
	{
		/*
		size_t pixelCount = pixels.height * pixels.width;
		uint8* start = pixels.getPixelPointer(0, 0);

		for (int i = 0; i < pixelCount; ++i)
		{

		}
		*/
	}

#define N_SEG 10
	static void cubic_bezier(
		//image img,
		unsigned int x1, unsigned int y1,
		unsigned int x2, unsigned int y2,
		unsigned int x3, unsigned int y3,
		unsigned int x4, unsigned int y4,
		Image::BitmapData& pixels,
		tomatl::draw::ColorARGB& color)
	{
		unsigned int i;
		double pts[N_SEG + 1][2];
		for (i = 0; i <= N_SEG; ++i)
		{
			double t = (double)i / (double)N_SEG;

			double a = pow((1.0 - t), 3.0);
			double b = 3.0 * t * pow((1.0 - t), 2.0);
			double c = 3.0 * pow(t, 2.0) * (1.0 - t);
			double d = pow(t, 3.0);

			double x = a * x1 + b * x2 + c * x3 + d * x4;
			double y = a * y1 + b * y2 + c * y3 + d * y4;
			pts[i][0] = x;
			pts[i][1] = y;
		}

#if 0
		/* draw only points */
		for (i = 0; i <= N_SEG; ++i)
		{
			plot(pts[i][0],
				pts[i][1]);
		}
#else
		/* draw segments */
		for (i = 0; i < N_SEG; ++i)
		{
			int j = i + 1;
			tomatl::draw::Util::draw_line_antialias(pixels, pts[i][0], pts[i][1], pts[j][0], pts[j][1], color);
		}
#endif
	}
#undef N_SEG

	// TODO: optimize
	static inline void _dla_plot(Image::BitmapData& pixels, int x, int y, ColorARGB& col, float br) noexcept
	{
		if (x < 0 || x >= pixels.width || y < 0 || y >= pixels.height)
		{
			return;
		}

		uint8* pixel = pixels.getPixelPointer(x, y);

		for (int i = 0; i < 3; ++i)
		{
			pixel[i] = pixel[i] * (1 - br) + col.channel[i] * br;
		}
	}

#define plot_(X,Y,D) _dla_plot(pixels, (X), (Y), color, (D))
#define ipart_(X) ((int)(X))
#define round_(X) ((int)(((double)(X))+0.5))
#define fpart_(X) (((double)(X))-(double)ipart_(X))
#define rfpart_(X) (1.0-fpart_(X))
#define swap_(a, b) std::swap(a, b)

	static void inline draw_line_antialias(
		Image::BitmapData& pixels,
		unsigned int x1, unsigned int y1,
		unsigned int x2, unsigned int y2,
		ColorARGB& color)
	{

		double dx = (double)x2 - (double)x1;
		double dy = (double)y2 - (double)y1;
		if (fabs(dx) > fabs(dy)) {
			if (x2 < x1) {
				swap_(x1, x2);
				swap_(y1, y2);
			}
			double gradient = dy / dx;
			double xend = round_(x1);
			double yend = y1 + gradient*(xend - x1);
			double xgap = rfpart_(x1 + 0.5);
			int xpxl1 = xend;
			int ypxl1 = ipart_(yend);
			plot_(xpxl1, ypxl1, rfpart_(yend)*xgap);
			plot_(xpxl1, ypxl1 + 1, fpart_(yend)*xgap);
			double intery = yend + gradient;

			xend = round_(x2);
			yend = y2 + gradient*(xend - x2);
			xgap = fpart_(x2 + 0.5);
			int xpxl2 = xend;
			int ypxl2 = ipart_(yend);
			plot_(xpxl2, ypxl2, rfpart_(yend) * xgap);
			plot_(xpxl2, ypxl2 + 1, fpart_(yend) * xgap);

			int x;
			for (x = xpxl1 + 1; x <= (xpxl2 - 1); x++) {
				plot_(x, ipart_(intery), rfpart_(intery));
				plot_(x, ipart_(intery) + 1, fpart_(intery));
				intery += gradient;
			}
		}
		else {
			if (y2 < y1) {
				swap_(x1, x2);
				swap_(y1, y2);
			}
			double gradient = dx / dy;
			double yend = round_(y1);
			double xend = x1 + gradient*(yend - y1);
			double ygap = rfpart_(y1 + 0.5);
			int ypxl1 = yend;
			int xpxl1 = ipart_(xend);
			plot_(xpxl1, ypxl1, rfpart_(xend)*ygap);
			plot_(xpxl1, ypxl1 + 1, fpart_(xend)*ygap);
			double interx = xend + gradient;

			yend = round_(y2);
			xend = x2 + gradient*(yend - y2);
			ygap = fpart_(y2 + 0.5);
			int ypxl2 = yend;
			int xpxl2 = ipart_(xend);
			plot_(xpxl2, ypxl2, rfpart_(xend) * ygap);
			plot_(xpxl2, ypxl2 + 1, fpart_(xend) * ygap);

			int y;
			for (y = ypxl1 + 1; y <= (ypxl2 - 1); y++) {
				plot_(ipart_(interx), y, rfpart_(interx));
				plot_(ipart_(interx) + 1, y, fpart_(interx));
				interx += gradient;
			}
		}
	}

	static void DrawPixel(Image::BitmapData& pixels, short x0, short y0, short color)
	{
		//std::size_t color = 0xFFFF40;
		std::size_t red = (color & 0xff0000) >> 16;
		std::size_t green = (color & 0x00ff00) >> 8;
		std::size_t blue = (color & 0x0000ff);

		uint8* pixel = pixels.getPixelPointer(x0, y0);
		//((PixelARGB*)pixel)->set(oc);
		pixel[1] = red;
		pixel[2] = green;
		pixel[3] = blue;
	}

	static short colorToShort(Colour& c)
	{
		return c.getRed() * 16 * 16 * 16 * 16 + c.getGreen() * 16 * 16 + c.getBlue();
	}

#undef swap_
#undef plot_
#undef ipart_
#undef fpart_
#undef round_
#undef rfpart_
};
} }
#endif