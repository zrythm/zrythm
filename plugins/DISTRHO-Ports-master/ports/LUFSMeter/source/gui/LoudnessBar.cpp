/*
 ===============================================================================
 
 LoudnessBar.cpp
 
 
 This file is part of the LUFS Meter audio measurement plugin.
 Copyright 2011-14 by Klangfreund, Samuel Gaehwiler.
 
 -------------------------------------------------------------------------------
 
 The LUFS Meter can be redistributed and/or modified under the terms of the GNU 
 General Public License Version 2, as published by the Free Software Foundation.
 A copy of the license is included with these source files. It can also be found
 at www.gnu.org/licenses.
 
 The LUFS Meter is distributed WITHOUT ANY WARRANTY.
 See the GNU General Public License for more details.
 
 -------------------------------------------------------------------------------
 
 To release a closed-source product which uses the LUFS Meter or parts of it,
 a commercial license is available. Visit www.klangfreund.com/lufsmeter for more
 information.
 
 ===============================================================================
 */


#include "LoudnessBar.h"


//==============================================================================
LoudnessBar::LoudnessBar (const Value & loudnessValueToReferTo,
                          const Value & minValueToReferTo,
                          const Value & maxValueToReferTo)
:   colour (Colours::green)
{
    loudnessValue.referTo(loudnessValueToReferTo);
    loudnessValue.addListener(this);
    
    minLoudness.referTo(minValueToReferTo);
    minLoudness.addListener(this);
    maxLoudness.referTo(maxValueToReferTo);
    maxLoudness.addListener(this);

    determineStretchAndOffset();
}

LoudnessBar::~LoudnessBar ()
{
    loudnessValue.removeListener(this);
}

Value & LoudnessBar::getLoudnessValueObject ()
{
    return loudnessValue;
}

void LoudnessBar::valueChanged (Value & value)
{
    if (value.refersToSameSourceAs (loudnessValue))
    {
        // Mesurements have shown that it is more CPU efficient to draw the whole
        // bar and not only the section that has changed.
        repaint();
    }
    else if (value.refersToSameSourceAs (minLoudness) || value.refersToSameSourceAs (maxLoudness))
    {
        determineStretchAndOffset();
        repaint();
    }
}

void LoudnessBar::setColour (const Colour & newColour)
{
    colour = newColour;
}

//==============================================================================
void LoudnessBar::paint (Graphics& g)
{
    const float width = (float) getWidth();
    const float height = (float) getHeight();
    
    // Draw a background
//    g.setColour(Colours::black);
//    float x = 0.0f;
//    float y = 0.0f;
//    float cornerSize = 3.0f;
//    g.fillRoundedRectangle(x, y, width, height, cornerSize);
    
    float barHeightInPercent = stretch * float (loudnessValue.getValue()) + offset;
    g.setColour (colour);
    const float topLeftX = 0.0f;
    float topLeftY = (1.0f - barHeightInPercent) * height;
    float bottomY = height;
    g.fillRect (topLeftX,
                topLeftY,
                width,
                bottomY - topLeftY);
}

void LoudnessBar::determineStretchAndOffset()
{
    // These two values define a linear mapping
    //    f(x) = stretch * x + offset
    // for which
    //    f(minimumLevel) = 0
    //    f(maximumLevel) = 1
    stretch = 1.0f/(double(maxLoudness.getValue()) - double(minLoudness.getValue()));
    offset = -double(minLoudness.getValue()) * stretch;
}