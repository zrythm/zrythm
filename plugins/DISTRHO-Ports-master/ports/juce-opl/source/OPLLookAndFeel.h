/*
  ==============================================================================

    OPLLookAndFeel.h
    Created: 10 Oct 2016 9:38:20pm
    Author:  bruce

  ==============================================================================
*/

#ifndef OPLLOOKANDFEEL_H_INCLUDED
#define OPLLOOKANDFEEL_H_INCLUDED

#include "JuceHeader.h"

class OPLLookAndFeel : public LookAndFeel_V3
{
private:
	Image toggleOff;
	Image toggleOn;
	Rectangle<float> toggleRect;

public:
	static const Colour DOS_GREEN;
	static const Colour DOS_GREEN_DARK;

	OPLLookAndFeel();

	void drawTickBox(Graphics &g,
		Component &c,
		float 	x,
		float 	y,
		float 	w,
		float 	h,
		bool 	ticked,
		bool 	isEnabled,
		bool 	isMouseOverButton,
		bool 	isButtonDown
	);

	int getSliderThumbRadius(Slider& s);
	void drawLinearSliderThumb(Graphics& g, int x, int y, int width, int height,
		float sliderPos, float minSliderPos, float maxSliderPos,
		const Slider::SliderStyle style, Slider& slider);
	void drawLinearSliderBackground(Graphics& g, int x, int y, int width, int height,
		float /*sliderPos*/,
		float /*minSliderPos*/,
		float /*maxSliderPos*/,
		const Slider::SliderStyle /*style*/, Slider& slider);
};




#endif  // OPLLOOKANDFEEL_H_INCLUDED
