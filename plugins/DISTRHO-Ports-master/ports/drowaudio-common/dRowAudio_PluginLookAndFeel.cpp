/*
 *  dRowLookAndFeel.cpp
 *  haydxn_tutorial
 *
 *  Created by David Rowland on 23/01/2009.
 *  Copyright 2009 dRowAudio. All rights reserved.
 *
 */

#include "dRowAudio_PluginLookAndFeel.h"

//=====================================================================================
void dRowLookAndFeel::drawRotarySlider (Graphics& g,
                                        int x, int y,
                                        int width, int height,
                                        float sliderPos,
                                        const float rotaryStartAngle,
                                        const float rotaryEndAngle,
                                        Slider& slider)
{
    const float radius = jmin (width / 2, height / 2) ;//- 2.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const bool isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();

	if (slider.isEnabled())
		g.setColour (slider.findColour (Slider::rotarySliderFillColourId).withAlpha (isMouseOver ? 1.0f : 0.8f));
	else
		g.setColour (Colour (0x80808080));

	//======================================================================
	// draw dial top
	ColourGradient bottomShaddowGradient ( slider.findColour(Slider::rotarySliderFillColourId).withBrightness(0.1f),
										  rx, ry+rw,
										  slider.findColour(Slider::rotarySliderFillColourId),
										  rx, (ry+rw)/2,
										  false);
	g.setGradientFill (bottomShaddowGradient);
	g.fillEllipse (rx, ry, rw, rw);

	// draw rounding highlight
	ColourGradient highlight (Colours::white.withAlpha(0.45f),
							  rx+(rw/2), ry+(rw * 0.2f),
							  Colours::transparentWhite,
							  rx+(rw/2), ry+(rw/2),
							  true);
	g.setGradientFill(highlight);
	g.fillEllipse (rx, ry, rw, rw/2);

	// draw rim
	g.setColour (Colours::black);
	g.drawEllipse (rx, ry, rw, rw, 0.5f);

	// draw thumb
	const float adj = (radius * 0.7f) * cos(angle);
	const float opp = (radius * 0.7f) * sin(angle);
	const float posX = rx + (rw/2) + opp;
	const float posY = ry + (rw/2) - adj;

	const float thumbW = rw * 0.15f;
	const float thumbX = posX - (thumbW * 0.5f);
	const float thumbY = posY - (thumbW * 0.5f);

	ColourGradient thumbGradient ( slider.findColour(Slider::rotarySliderFillColourId).withBrightness(0.05f),
								  thumbX, thumbY,
								  slider.findColour(Slider::rotarySliderFillColourId).withBrightness(0.75f),
								  thumbX, thumbY+thumbW,
								  false);
	g.setGradientFill(thumbGradient);
	g.fillEllipse (thumbX, thumbY, thumbW, thumbW);

	// draw thumb rim
	g.setColour (Colours::black);
	g.drawEllipse (thumbX, thumbY, thumbW, thumbW, thumbW * 0.02f);
}

//=====================================================================================
void dRowLookAndFeel::drawLabel (Graphics& g, Label& label)
{
	int innerBoxHeight = label.getHeight();
	int innerBoxWidth = label.getWidth();
	bool hasTransparentBackground = label.findColour(Label::backgroundColourId).isTransparent();

	// increase the border to allow the extra highlight
	if (! hasTransparentBackground)
	{
		innerBoxHeight -= 3;
		innerBoxWidth -= 2;
	}

	// fill background
	g.setColour(label.findColour (Label::backgroundColourId));
	g.fillRoundedRectangle(0, 0, label.getWidth(), label.getHeight()-1, 2);

    if (! label.isBeingEdited())
    {
        const float alpha = label.isEnabled() ? 1.0f : 0.5f;

        g.setColour (label.findColour (Label::textColourId).withMultipliedAlpha (alpha));
		if (! hasTransparentBackground)
		{
			Font font(label.getFont());
			g.setFont(font.getHeight()-2);
		}
		else
		{
			g.setFont (label.getFont());
		}
		g.drawFittedText (label.getText(),
                          label.getBorderSize().getLeftAndRight()+3,
                          label.getBorderSize().getTopAndBottom()+5,
                          innerBoxWidth - 2 * (label.getBorderSize().getLeftAndRight()+2),
                          innerBoxHeight - 2 * (label.getBorderSize().getTopAndBottom()+4),
                          label.getJustificationType(),
                          jmax (1, (int) (innerBoxHeight / label.getFont().getHeight())),
                          label.getMinimumHorizontalScale());

        g.setColour (label.findColour (Label::outlineColourId).withMultipliedAlpha (alpha));
        g.drawRect (1, 1, innerBoxWidth, innerBoxHeight);
    }
    else if (label.isEnabled())
    {
        g.setColour (label.findColour (Label::outlineColourId));
        g.drawRect (0, 0, innerBoxWidth, innerBoxHeight);
    }

	// draw the 3D aspects only if the background is not transparent
	if ( !hasTransparentBackground )
	{
		//draw inner shaddow and highlight
		ColourGradient innerShaddow(findColour(Label::backgroundColourId).withBrightness(1.0f).withAlpha(0.3f),
									0, 0,
									Colours::black.withAlpha(0.4f),
									0, label.getHeight(),
									false);
		innerShaddow.addColour(0.35, findColour(Label::backgroundColourId));
		innerShaddow.addColour(0.75, findColour(Label::backgroundColourId));

//		GradientBrush innerShaddowBrush(innerShaddow);
		g.setGradientFill(innerShaddow);

		g.fillRect(1, 1, innerBoxWidth, innerBoxHeight);

		// draw bottom edge highlight
		ColourGradient bottomHighlight(Colours::transparentWhite,
									   0, label.getHeight()-1,
									   Colours::transparentWhite,
									   label.getWidth(), label.getHeight()-1,
									   false);
		bottomHighlight.addColour(0.05f, Colours::white.withAlpha(0.7f));
		bottomHighlight.addColour(0.95f, Colours::white.withAlpha(0.7f));
//		GradientBrush bottomBrush(bottomHighlight);
		g.setGradientFill(bottomHighlight);
		g.drawLine(0, label.getHeight(), label.getWidth(), label.getHeight());
	}
}

//=====================================================================================
void dRowLookAndFeel::drawInsetLine (Graphics& g,
                                     const float startX,
                                     const float startY,
                                     const float endX,
                                     const float endY,
                                     const float lineThickness)
{
	Colour currentColour(Colours::grey);//(g.getCurrentColour());
	const float firstThickness = lineThickness * 0.5f;
	const float secondThickness = lineThickness * 0.25f;

	if (startX < endX)
	{
		g.setColour(currentColour.withBrightness(0.2f));
		g.drawLine(startX, startY, endX, endY, firstThickness);
		g.setColour(currentColour.withBrightness(1.0f).withAlpha(0.6f));
		g.drawLine(startX, startY+secondThickness, endX, endY+secondThickness, secondThickness);
	}
	else if (startY < endY)
	{
		g.setColour(currentColour.withBrightness(0.2f));
		g.drawLine(startX, startY, endX, endY, firstThickness);
		g.setColour(currentColour.withBrightness(1.0f).withAlpha(0.6f));
		g.drawLine(startX+secondThickness, startY, endX+secondThickness, endY, secondThickness);
	}
}
