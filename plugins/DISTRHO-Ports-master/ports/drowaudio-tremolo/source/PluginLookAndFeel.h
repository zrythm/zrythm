/*
 *  dRowAudioLookAndFeel.h
 *  LookAndFeel_Test
 *
 *  Created by David Rowland on 27/12/2008.
 *  Copyright 2008 UWE. All rights reserved.
 *
 */

#ifndef _dRowAudioLookAndFeel_H_
#define _dRowAudioLookAndFeel_H_

#include "includes.h"

//==============================================================================
class PluginLookAndFeel :   public LookAndFeel_V2
{
public:
    //==============================================================================
    enum ColourIds
    {
        backgroundColourId = 0xd00001
    };

    //==============================================================================
    PluginLookAndFeel();

    //==============================================================================
    static void drawPluginBackgroundBase (Graphics& g, Component& editor);

    static void drawPluginBackgroundHighlights (Graphics& g, Component& editor);

    static void drawInsetLine (Graphics& g,
							   const float startX,
							   const float startY,
							   const float endX,
							   const float endY,
							   const float lineThickness);

    //==============================================================================
	virtual void drawRotarySlider (Graphics& g,
                                   int x, int y,
                                   int width, int height,
                                   float sliderPosProportional,
                                   const float rotaryStartAngle,
                                   const float rotaryEndAngle,
                                   Slider& slider) override;

	virtual void drawFaderKnob(Graphics& g,
							   const float x,
							   const float y,
							   const float width,
							   const float height);

	/**
     Draws a label.
     If the label's background is not transparent then it will draw a 3D label.
	 */
	virtual void drawLabel (Graphics& g, Label& label) override;
};

#endif