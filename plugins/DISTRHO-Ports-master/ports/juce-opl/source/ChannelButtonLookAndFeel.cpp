/*
  ==============================================================================

    ChannelButtonLookAndFeel.cpp
    Created: 10 Oct 2016 9:38:20pm
    Author:  bruce

  ==============================================================================
*/

#include "ChannelButtonLookAndFeel.h"
#include "PluginGui.h"

ChannelButtonLookAndFeel::ChannelButtonLookAndFeel()
{


}

static void drawButtonShape(Graphics& g, const Path& outline, Colour baseColour, float height)
{
	const float mainBrightness = baseColour.getBrightness();
	const float mainAlpha = baseColour.getFloatAlpha();

	g.setFillType(FillType(baseColour));
	g.fillPath(outline);

	//g.setColour(Colours::white.withAlpha(0.4f * mainAlpha * mainBrightness * mainBrightness));
	//g.strokePath(outline, PathStrokeType(1.0f), AffineTransform::translation(0.0f, 1.0f)
	//	.scaled(1.0f, (height - 1.6f) / height));

	//g.setColour(Colours::black.withAlpha(0.4f * mainAlpha));
	//g.strokePath(outline, PathStrokeType(1.0f));
}

void ChannelButtonLookAndFeel::drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
	bool isMouseOverButton, bool isButtonDown)
{
	Colour baseColour(backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
		.withMultipliedAlpha(button.isEnabled() ? 0.9f : 0.5f));

	if (isButtonDown || isMouseOverButton)
		baseColour = baseColour.contrasting(isButtonDown ? 0.2f : 0.1f);

	const bool flatOnLeft = button.isConnectedOnLeft();
	const bool flatOnRight = button.isConnectedOnRight();
	const bool flatOnTop = button.isConnectedOnTop();
	const bool flatOnBottom = button.isConnectedOnBottom();

	const float width = button.getWidth() - 1.0f;
	const float height = button.getHeight() - 1.0f;

	if (width > 0 && height > 0)
	{
		const float cornerSize = 4.0f;

		Path outline;
		outline.addRoundedRectangle(0.5f, 0.5f, width, height, cornerSize, cornerSize,
			!(flatOnLeft || flatOnTop),
			!(flatOnRight || flatOnTop),
			!(flatOnLeft || flatOnBottom),
			!(flatOnRight || flatOnBottom));

		drawButtonShape(g, outline, baseColour, height);
	}
}
