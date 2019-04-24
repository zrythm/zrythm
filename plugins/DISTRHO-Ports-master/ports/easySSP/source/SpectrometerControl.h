/*
  ==============================================================================

	SpectrometerControl.h
	Created: 7 Apr 2014 3:46:06pm
	Author:  Kykc

  ==============================================================================
*/

#ifndef SPECTROMETERCONTROL_H_INCLUDED
#define SPECTROMETERCONTROL_H_INCLUDED

#include "JuceHeader.h"
#include "TomatlImageType.h"
#include "ILateInitComponent.h"
#include <map>
//==============================================================================
/*
*/

class SpectrometerControl : public ILateInitComponent
{
private:
	AdmvAudioProcessor* mParentProcessor;
	tomatl::dsp::FrequencyDomainGrid mFreqGrid;
	size_t mRightBorder = 10;
	size_t mBottomBorder = 16;
	Image mBuffer;
	Image mBackground;
	Image mSurface;

	void redrawBackground()
	{
		Graphics g(mBackground);

		g.fillAll(LookAndFeel::getDefaultLookAndFeel().findColour(TomatlLookAndFeel::controlBackground));

		g.setColour(Colours::darkgrey);
		g.drawRect(getLocalBounds(), 2.f);
		
		g.setColour(Colours::darkgrey.withAlpha(0.3f));
		for (size_t i = 0; i < mFreqGrid.getFreqLineCount(); ++i)
		{
			auto line = mFreqGrid.getFreqLine(i);

			g.setColour(Colours::darkgrey.withAlpha(0.3f));
			g.drawLine(line.mLocation, 0, line.mLocation, mFreqGrid.getHeight());

			if (line.mLocation > 0)
			{
				g.setColour(LookAndFeel::getDefaultLookAndFeel().findColour(TomatlLookAndFeel::alternativeText1).withAlpha(0.6f));
				g.drawText(juce::String(line.mCaption.c_str()), juce::Rectangle<float>(line.mLocation - 12, mFreqGrid.getHeight(), 24, mBottomBorder), Justification::centred, false);
			}
		}

		for (size_t i = 0; i < mFreqGrid.getAmplLineCount(); ++i)
		{
			auto line = mFreqGrid.getAmplLine(i);

			g.setColour(Colours::darkgrey.withAlpha(0.3f));
			g.drawLine(juce::Line<float>(0, line.mLocation, mFreqGrid.getWidth(), line.mLocation));

			if (line.mLocation + 1 < (int)mFreqGrid.getHeight())
			{
				g.setColour(LookAndFeel::getDefaultLookAndFeel().findColour(TomatlLookAndFeel::alternativeText1).withAlpha(0.6f));
				g.drawText(juce::String(line.mCaption.c_str()), juce::Rectangle<float>(mFreqGrid.getWidth() - 14, line.mLocation - 8, 30, 14), Justification::centredLeft, false);
			}
		}

		g.setColour(Colours::black);
		g.drawRect(getLocalBounds().expanded(-1.), 1.f);
	}

	void clearArgbImage(Image& subject)
	{
		Image::BitmapData pixels(subject, Image::BitmapData::readWrite);

		for (int i = 0; i < pixels.height; ++i)
		{
			memset(pixels.getLinePointer(i), 0x0, pixels.width * pixels.pixelStride);
		}
	}

public:
	SpectrometerControl(AdmvAudioProcessor* parent) : 
		mFreqGrid(tomatl::dsp::Bound2D<double>(20., 30000., -72., 0.))
	{
		mParentProcessor = parent;
	}

	virtual void init(juce::Rectangle<int> bounds)
	{
		setSize(bounds.getWidth(), bounds.getHeight());
		mBuffer = Image(Image::ARGB, getWidth(), getHeight(), true, TomatlImageType());
		mBackground = Image(Image::RGB, getWidth(), getHeight(), true, TomatlImageType());
		mSurface = Image(Image::RGB, getWidth(), getHeight(), true, TomatlImageType());

		setOpaque(true);
		setPaintingIsUnclipped(true);

		mFreqGrid.updateSize(getWidth() - mRightBorder, getHeight() - mBottomBorder);
	}

	~SpectrometerControl()
	{
	}

	forcedinline void fillBoundsFromState(const AdmvPluginState& state, tomatl::dsp::Bound2D<double>& subject)
	{
		subject.X.mLow = mFreqGrid.fullScaleXToFreq(mFreqGrid.getWidth() / 100. * state.mSpectrometerFrequencyScale.first);
		subject.X.mHigh = mFreqGrid.fullScaleXToFreq(mFreqGrid.getWidth() / 100. * state.mSpectrometerFrequencyScale.second);

		subject.Y.mLow = mFreqGrid.fullScaleYToDb(mFreqGrid.getHeight() - mFreqGrid.getHeight() / 100. * state.mSpectrometerMagnitudeScale.first);
		subject.Y.mHigh = mFreqGrid.fullScaleYToDb(mFreqGrid.getHeight() - mFreqGrid.getHeight() / 100. * state.mSpectrometerMagnitudeScale.second);
	}

	bool shouldHoldLastFrame()
	{
		return mParentProcessor->getState().mSpectrometerReleaseSpeed == std::numeric_limits<double>::infinity();
	}

	void paint(Graphics& g)
	{
		clearArgbImage(mBuffer);
		bool backgroundRedraw = false;
		tomatl::dsp::Bound2D<double> bounds;
		fillBoundsFromState(mParentProcessor->getState(), bounds);

		backgroundRedraw = backgroundRedraw || mFreqGrid.updateBounds(bounds);

		Graphics buffer(mBuffer);

		std::vector<std::pair<Path, int>> paths;

		//LowLevelGraphicsSoftwareRenderer& c = dynamic_cast<LowLevelGraphicsSoftwareRenderer&>(g.getInternalContext());

		if (backgroundRedraw)
		{
			redrawBackground();
		}
		
		for (size_t pn = 0; pn < mParentProcessor->getMaxStereoPairCount(); ++pn)
		{
			if (mParentProcessor->mSpectroSegments == NULL)
			{
				break;
			}

			tomatl::dsp::SpectrumBlock block = mParentProcessor->mSpectroSegments[pn];

			if (block.mLength <= 0)
			{
				continue;
			}

			// TODO: implement correct last frame holding support (should work with resize)
			// maybe some local storage of spectro-blocks which can be drawn if no newer/eligible data
			if (block.mFramesRendered > TOMATL_FPS && !shouldHoldLastFrame())
			{
				continue;
			}

			mParentProcessor->mSpectroSegments[pn].mFramesRendered += 1;

			mFreqGrid.updateSampleRate(block.mSampleRate);
			mFreqGrid.updateBinCount(block.mLength);

			Path p;

			p.startNewSubPath(mFreqGrid.lowestVisibleFreqToX(), mFreqGrid.minusInfToY());			

			std::map<int, double> points;

			double lastX = mFreqGrid.lowestVisibleFreqToX();
			double lastY = mFreqGrid.minusInfToY();

			for (size_t i = 0; i < block.mLength; ++i)
			{
				double y0 = mFreqGrid.dbToY(TOMATL_TO_DB(block.mData[i].second));
				double f0 = mFreqGrid.binNumberToFrequency(block.mData[i].first);
				double x0 = mFreqGrid.freqToX(f0);

				if (!mFreqGrid.isFrequencyVisible(f0))
				{
					continue;
				}

				if (x0 - lastX >= 1 || x0 == mFreqGrid.lowestVisibleFreqToX())
				{
					points[x0] = y0;

					lastX = x0;
					lastY = y0;
				}
				else if (f0 > 0)
				{
					points[x0] = std::min(points[x0], y0);

					lastX = x0;
					lastY = y0;
				}
			}
			// unused
			(void)lastY;
			
			if (points.size() > 0)
			{
				if (points.begin()->first != mFreqGrid.lowestVisibleFreqToX())
				{
					p.lineTo(mFreqGrid.lowestVisibleFreqToX(), points.begin()->second);
				}

				for (auto i = points.begin(); i != points.end(); ++i)
				{
					p.lineTo(i->first, i->second);
				}
			}
			
			if (points.size() > 0)
			{
				p.lineTo((--points.end())->first, mFreqGrid.minusInfToY());
			}

			p.lineTo(mFreqGrid.getWidth(), mFreqGrid.getHeight() - 1);
			p.lineTo(0, mFreqGrid.getHeight() - 1);

			paths.push_back(std::pair<Path, int>(p.createPathWithRoundedCorners(5.f), block.mIndex));
		}

		for (size_t i = 0; i < paths.size(); ++i)
		{
			Path p = paths[i].first;

			buffer.setColour(mParentProcessor->getStereoPairColor(paths[i].second));
			buffer.strokePath(p, PathStrokeType(1.f));
			
			if (mParentProcessor->getState().mSpectrumFillMode == AdmvPluginState::spectrumFillWithTransparency)
			{
				buffer.setColour(mParentProcessor->getStereoPairColor(paths[i].second).withAlpha((float)0.1));
				buffer.fillPath(p);
			}
		}

		if (isMouseOver())
		{
			auto mouseLocation = getMouseXYRelative();
			
			if (mFreqGrid.containsPoint(mouseLocation.getX(), mouseLocation.getY()))
			{
				buffer.setColour(LookAndFeel::getDefaultLookAndFeel().findColour(TomatlLookAndFeel::alternativeText1).withAlpha(0.8f));
				buffer.setFont(Font(Font::getDefaultMonospacedFontName(), 12, Font::plain));
				buffer.drawFittedText(mFreqGrid.getPointNotation(mouseLocation.getX(), mouseLocation.getY()).c_str(), juce::Rectangle<int>(8, 8, 300, 20), Justification::topLeft, 1);
			}
		}
		
#if TOMATL_CUSTOM_BLENDING
		tomatl::draw::Util::blend(mBackground, mBuffer, mSurface);
		g.drawImageAt(mSurface, 0, 0, false);
#else
		g.drawImageAt(mBackground, 0, 0, false);
		g.drawImageAt(mBuffer, 0, 0, false);
#endif
	}

	void resized()
	{
		// This method is where you should set the bounds of any child
		// components that your component contains..
	}

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrometerControl)
};


#endif  // SPECTROMETERCONTROL_H_INCLUDED
