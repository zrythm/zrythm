/*
 *  dRowLookAndFeel.h
 *
 *  Created by David Rowland on 23/01/2009.
 *  Copyright 2009 dRowAudio. All rights reserved.
 *
 */

#ifndef _DROWLOOKANDFEEL_H_
#define _DROWLOOKANDFEEL_H_

#include "includes.h"

class dRowLookAndFeel : public LookAndFeel_V2
{
public:
    /**
        Draws a shiny, rounded-top knob rotary slider.
     */
    virtual void drawRotarySlider (Graphics& g,
                                   int x, int y,
                                   int width, int height,
                                   float sliderPosProportional,
                                   const float rotaryStartAngle,
                                   const float rotaryEndAngle,
                                   Slider& slider) override;

    /**
        Draws a label.
        If the label's background is not transparent then it will draw a 3D label.
    */
    virtual void drawLabel (Graphics& g, Label& label) override;

    /**
        Draws a line that will look like it is inset in its background.
    */
    static void drawInsetLine (Graphics& g,
                               const float startX,
                               const float startY,
                               const float endX,
                               const float endY,
                               const float lineThickness);
};

#endif
