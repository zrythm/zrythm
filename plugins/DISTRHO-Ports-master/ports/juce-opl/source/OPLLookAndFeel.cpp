/*
  ==============================================================================

    OPLLookAndFeel.cpp
    Created: 10 Oct 2016 9:38:20pm
    Author:  bruce

  ==============================================================================
*/

#include "OPLLookAndFeel.h"
#include "PluginGui.h"

const Colour OPLLookAndFeel::DOS_GREEN = Colour(0xff007f00);
const Colour OPLLookAndFeel::DOS_GREEN_DARK = Colour(0xff003f00);

OPLLookAndFeel::OPLLookAndFeel()
{
	toggleOff = ImageCache::getFromMemory(PluginGui::toggle_off_sq_png, PluginGui::toggle_off_sq_pngSize), 1.000f, Colour(0x00000000);
	toggleOn = ImageCache::getFromMemory(PluginGui::toggle_on_sq_png, PluginGui::toggle_on_sq_pngSize), 1.000f, Colour(0x00000000);
	toggleRect = Rectangle<float>((float)toggleOff.getWidth(), (float)toggleOn.getHeight());

	// Prevents an ugly white border from being drawn around a component with keyboard focus.
	setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::black);
	setColour(Slider::ColourIds::textBoxOutlineColourId, DOS_GREEN);

	setColour(TextButton::ColourIds::buttonColourId, DOS_GREEN);
	setColour(TextButton::ColourIds::buttonOnColourId, DOS_GREEN);
	setColour(TextButton::ColourIds::textColourOnId, Colours::black);
	setColour(TextButton::ColourIds::textColourOffId, Colours::black);

}

void OPLLookAndFeel::drawTickBox(Graphics &g,
	Component &c,
	float 	x,
	float 	y,
	float 	w,
	float 	h,
	bool 	ticked,
	bool 	isEnabled,
	bool 	isMouseOverButton,
	bool 	isButtonDown
) {
	g.drawImage(ticked ? toggleOn : toggleOff, toggleRect.withY(y + 2));
}

// From JuceLookAndFeel_V2
static Colour createBaseColour(Colour buttonColour,
	bool hasKeyboardFocus,
	bool isMouseOverButton,
	bool isButtonDown) noexcept
{
	const float sat = hasKeyboardFocus ? 1.3f : 0.9f;
	const Colour baseColour(buttonColour.withMultipliedSaturation(sat));

	if (isButtonDown)      return baseColour.contrasting(0.2f);
	if (isMouseOverButton) return baseColour.contrasting(0.1f);

	return baseColour;
}

int OPLLookAndFeel::getSliderThumbRadius(Slider& s) {
	return 10;
}

// Adapted rom JuceLookAndFeel_V2 - changed round thumb to plain filled rectangle and tweake size.
void OPLLookAndFeel::drawLinearSliderThumb(Graphics& g, int x, int y, int width, int height,
	float sliderPos, float minSliderPos, float maxSliderPos,
	const Slider::SliderStyle style, Slider& slider) {
	
	const float sliderRadius = (float)(getSliderThumbRadius(slider));

	Colour knobColour(createBaseColour(slider.findColour(Slider::thumbColourId),
		slider.hasKeyboardFocus(false) && slider.isEnabled(),
		slider.isMouseOverOrDragging() && slider.isEnabled(),
		slider.isMouseButtonDown() && slider.isEnabled()));

	const float outlineThickness = slider.isEnabled() ? 0.8f : 0.3f;

	if (style == Slider::LinearHorizontal || style == Slider::LinearVertical)
	{
		float kx, ky;
		float sw, sh;

		if (style == Slider::LinearVertical)
		{
			sw = sliderRadius * 2.0f;
			sh = sliderRadius;
			kx = x + width * 0.5f;
			ky = sliderPos + sh * 0.5f;
		}
		else
		{
			sw = sliderRadius;
			sh = sliderRadius * 2.0f;
			kx = sliderPos + sw * 0.5f;
			ky = y + height * 0.5f;
		}

		g.setColour(knobColour);
		g.fillRect(kx - sliderRadius, ky - sliderRadius, sw, sh);
	}
	else
	{
		LookAndFeel_V2::drawLinearSliderThumb(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
	}
}

// Adapted from JuceLookAndFeel_V3 - replace rounded rectangles with regular ones.
void OPLLookAndFeel::drawLinearSliderBackground(Graphics& g, int x, int y, int width, int height,
	float /*sliderPos*/,
	float /*minSliderPos*/,
	float /*maxSliderPos*/,
	const Slider::SliderStyle /*style*/, Slider& slider)
{
	const float sliderRadius = (float)(getSliderThumbRadius(slider) - 2);

	const Colour trackColour(slider.findColour(Slider::trackColourId));
	const Colour gradCol1(trackColour.overlaidWith(Colour(slider.isEnabled() ? 0x13000000 : 0x09000000)));
	const Colour gradCol2(trackColour.overlaidWith(Colour(0x06000000)));
	Path indent;

	if (slider.isHorizontal())
	{
		const float iy = y + height * 0.5f - sliderRadius * 0.5f;

		g.setGradientFill(ColourGradient(gradCol1, 0.0f, iy,
			gradCol2, 0.0f, iy + sliderRadius, false));

		indent.addRectangle(x - sliderRadius * 0.5f, iy, width + sliderRadius, sliderRadius);
	}
	else
	{
		const float ix = x + width * 0.5f - sliderRadius * 0.5f;

		g.setGradientFill(ColourGradient(gradCol1, ix, 0.0f,
			gradCol2, ix + sliderRadius, 0.0f, false));

		indent.addRectangle(ix, y - sliderRadius * 0.5f, sliderRadius, height + sliderRadius);
	}

	g.fillPath(indent);

	g.setColour(trackColour.contrasting(0.5f));
	g.strokePath(indent, PathStrokeType(0.5f));
}
