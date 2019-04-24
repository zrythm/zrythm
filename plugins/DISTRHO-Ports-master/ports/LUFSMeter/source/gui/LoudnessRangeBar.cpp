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


#include "LoudnessRangeBar.h"


//==============================================================================
LoudnessRangeBar::LoudnessRangeBar (const Value & startValueToReferTo,
                                    const Value & endValueToReferTo,
                                    const Value & minValueToReferTo,
                                    const Value & maxValueToReferTo)
:   colour (Colours::green)
{
    startValue.referTo(startValueToReferTo);
    startValue.addListener(this);
    endValue.referTo(endValueToReferTo);
    endValue.addListener(this);
    
    minLoudness.referTo(minValueToReferTo);
    minLoudness.addListener(this);
    maxLoudness.referTo(maxValueToReferTo);
    maxLoudness.addListener(this);

    determineStretchAndOffset();
}

LoudnessRangeBar::~LoudnessRangeBar()
{
    startValue.removeListener (this);
    endValue.removeListener (this);
}

Value & LoudnessRangeBar::getStartValueObject ()
{
    return startValue;
}

Value & LoudnessRangeBar::getEndValueObject ()
{
    return endValue;
}

void LoudnessRangeBar::valueChanged (Value & value)
{
    if (value.refersToSameSourceAs (startValue) || value.refersToSameSourceAs (endValue))
    {
        repaint();
    }

    else if (value.refersToSameSourceAs (minLoudness) || value.refersToSameSourceAs (maxLoudness))
    {
        determineStretchAndOffset();
        repaint();
    }
}

void LoudnessRangeBar::setColour (const Colour & newColour)
{
    colour = newColour;
}

//==============================================================================
void LoudnessRangeBar::paint (Graphics& g)
{
    const float width = (float) getWidth();
    const float height = (float) getHeight();
    
    g.setColour(colour);
    const float topLeftX = 0.0f;
    float barTop = stretch * float(endValue.getValue()) + offset;
    float topLeftY = (1.0f - barTop) * height;
    float barBottom = stretch * float(startValue.getValue()) + offset;
    float bottomY = (1.0f - barBottom) * height;
    g.fillRect(topLeftX,
               topLeftY,
               width,
               bottomY-topLeftY);
}

void LoudnessRangeBar::determineStretchAndOffset()
{
    // These two values define a linear mapping
    //    f(x) = stretch * x + offset
    // for which
    //    f(minimumLevel) = 0
    //    f(maximumLevel) = 1
    stretch = 1.0f/(double(maxLoudness.getValue()) - double(minLoudness.getValue()));
    offset = -double(minLoudness.getValue()) * stretch;
}