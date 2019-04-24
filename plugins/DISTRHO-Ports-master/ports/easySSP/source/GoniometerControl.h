#ifndef GONIOMETERCONTROL_H_INCLUDED
#define GONIOMETERCONTROL_H_INCLUDED

#include "JuceHeader.h"
#include "TomatlImageType.h"
#include "dsp-utility.h"
#include "ILateInitComponent.h"
//==============================================================================
/*
*/
class GoniometerControl : public ILateInitComponent
{
private:
	Image mContent;
	Image mBackground;
	Image mSurface;
	AdmvAudioProcessor* mParentProcessor;

	void initLayers()
	{
		Graphics buffer(mContent);
		buffer.setColour(Colours::transparentBlack);
		buffer.fillAll();

		int margin = 4;

		float sqrt2 = std::sqrt(2);

		Point<float> center(getWidth() / 2, getHeight() / 2);

		float lineScaleX = (getWidth()) / 2 - margin;
		float lineScaleY = (getHeight()) / 2 - margin;

		
		Line<float> leftDiag(movePoint<float>(center, -lineScaleX / sqrt2, -lineScaleY / sqrt2), movePoint<float>(center, +lineScaleX / sqrt2, +lineScaleY / sqrt2));
		Line<float> rightDiag(movePoint<float>(center, -lineScaleX / sqrt2, +lineScaleY / sqrt2), movePoint<float>(center, +lineScaleX / sqrt2, -lineScaleY / sqrt2));

		Graphics background(mBackground);
		background.setImageResamplingQuality(Graphics::ResamplingQuality::highResamplingQuality);
		background.setColour(Colour::fromString("FF101010"));
		background.fillAll();
		background.setColour(Colour::fromString("FF202020"));
		background.drawEllipse(margin, margin, getWidth() - margin * 2, getHeight() - margin * 2, 1.5);
		
		
		background.drawLine(leftDiag);
		background.drawLine(rightDiag);
		background.setColour(Colours::darkgrey);
		background.drawRect(getLocalBounds(), 2.f);
		background.setColour(Colours::black);
		background.drawRect(getLocalBounds().expanded(-1.), 1.f);
	}
public:

	virtual void init(juce::Rectangle<int> bounds)
	{
		setOpaque(true);
		setPaintingIsUnclipped(false);
		setSize(bounds.getWidth(), bounds.getHeight());
		mContent = Image(Image::ARGB, getWidth(), getHeight(), true, TomatlImageType());
		mBackground = Image(Image::RGB, getWidth(), getHeight(), true, TomatlImageType());
		mSurface = Image(Image::RGB, getWidth(), getHeight(), true, TomatlImageType());

		initLayers();
	}

	template <typename T> Point<T> movePoint(Point<T> source, T x, T y)
	{
		Point<T> newPoint(source);

		newPoint.addXY(x, y);

		return newPoint;
	}

	GoniometerControl(AdmvAudioProcessor* parentPlugin)
	{
		mParentProcessor = parentPlugin;

		
		
	}

	~GoniometerControl()
	{
	}

	void paint (Graphics& g)
	{
		Graphics buffer(mContent);
		
		Image::BitmapData pixels(mContent, Image::BitmapData::ReadWriteMode::readWrite);

		// TODO: split to several smaller methods
		for (size_t cn = 0; cn < mParentProcessor->getMaxStereoPairCount(); ++cn)
		{
			if (mParentProcessor->mGonioSegments == NULL)
			{
				break;
			}

			GonioPoints<double> segment = mParentProcessor->mGonioSegments[cn];

			double totalDistance = 0;
			Path p;

			if (segment.mLength <= 0)
			{
				continue;
			}

			if (segment.mFramesRendered > TOMATL_FPS)
			{
				continue;
			}

			mParentProcessor->mGonioSegments[cn].mFramesRendered += 1;

			double x = TOMATL_BOUND_VALUE((int)((segment.mData[0].first + 1.) * getWidth() / 2.), 0, getWidth() - 1);
			double y = TOMATL_BOUND_VALUE((int)((segment.mData[0].second + 1.) * getHeight() / 2.), 0, getHeight() - 1);

			p.startNewSubPath(x, y);

			std::vector<std::pair<double, double>> points;

			points.push_back(std::pair<double, double>(x, y));

			for (size_t i = 1; i < segment.mLength - 1; ++i)
			{
				double x = TOMATL_BOUND_VALUE((segment.mData[i].first + 1.) * getWidth() / 2., 0., getWidth() - 1.);
				double y = TOMATL_BOUND_VALUE((segment.mData[i].second + 1.) * getHeight() / 2., 0., getHeight() - 1.);

				std::pair<double, double> prev = points[points.size() - 1];

				double distance = std::sqrt(std::pow(prev.first - x, 2) + std::pow(prev.second - y, 2));
				totalDistance += distance;

				if (distance > 20)
				{
					//p.quadraticTo(prev.first, prev.second, x, y);
					//p.addLineSegment(Line<float>(prev.first, prev.second, x, y), 0.7);
					points.push_back(std::pair<double, double>(x, y));
				}
			}

			juce::Colour srcColor = mParentProcessor->getStereoPairColor(segment.mIndex);
			tomatl::draw::ColorARGB color;
			color.fromColor(srcColor);

			if (points.size() > 4)
			{
				for (size_t i = 0; i < points.size() - 3; i += 3)
				{
					std::pair<double, double> p1 = points[i + 0];
					std::pair<double, double> p2 = points[i + 1];
					std::pair<double, double> p3 = points[i + 2];
					std::pair<double, double> p4 = points[i + 3];

					tomatl::draw::Util::cubic_bezier(
						p1.first, p1.second,
						p2.first, p2.second,
						p3.first, p3.second,
						p4.first, p4.second,
						pixels, color);

					//pixels.setPixelColour(points[i].first, points[i].second, Colours::green);
					//p.lineTo(points[i].first, points[i].second);
					//p.quadraticTo(points[i].first, points[i].second, points[i + 1].first, points[i + 1].second);
				}
			}

			p.closeSubPath();

			//buffer.setColour(mParentProcessor->getStereoPairColor(segment.mIndex));
			
			//mContent.multiplyAllAlphas(0.95);	
		}

		uint8 alpha = 242;
		uint16 reg;
		// Decay
		// TODO: move to CustomDrawing
		for (int i = 0; i < pixels.height; ++i)
		{
			uint8* line = pixels.getLinePointer(i);

			for (int j = 0; j < pixels.width * pixels.pixelStride; ++j)
			{
				reg = line[j] * alpha;

				line[j] = TOMATL_FAST_DIVIDE_BY_255(reg);
			}
		}

#if TOMATL_CUSTOM_BLENDING
		tomatl::draw::Util::blend(mBackground, mContent, mSurface);
		g.drawImageAt(mSurface, 0, 0, false);
#else
		g.drawImageAt(mBackground, 0, 0);
		g.drawImageAt(mContent, 0, 0);
#endif
	}

	void resized()
	{
		// This contol doesn't support resizing ATM
	}

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GoniometerControl)
};


#endif  // GONIOMETERCONTROL_H_INCLUDED
