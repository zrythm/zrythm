/*
 ===============================================================================
 
 MultiChannelLoudnessBar.cpp
 
 
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


#include "MultiChannelLoudnessBar.h"


//==============================================================================
MultiChannelLoudnessBar::MultiChannelLoudnessBar (const Value & minLoudnessValueToReferTo,
                                                  const Value & maxLoudnessValueToReferTo)
 :  colour (Colours::green)
{
    minLoudness.referTo(minLoudnessValueToReferTo);
    minLoudness.addListener(this);
    maxLoudness.referTo(maxLoudnessValueToReferTo);
    maxLoudness.addListener(this);
    
    determineStretchOffsetAndWidthOfIndividualChannel();
}

MultiChannelLoudnessBar::~MultiChannelLoudnessBar ()
{
    minLoudness.removeListener(this);
    maxLoudness.removeListener(this);
}

void MultiChannelLoudnessBar::setLoudness (const vector<float>& multiChannelLoudness)
{
    if (multiChannelLoudness.size() != currentMultiChannelLoudness.size())
    {
        // If the number of channels has changed.
        currentMultiChannelLoudness = multiChannelLoudness;
        determineStretchOffsetAndWidthOfIndividualChannel();
    }
    else
    {
        currentMultiChannelLoudness = multiChannelLoudness;
    }
    
    repaint();
}

void MultiChannelLoudnessBar::valueChanged (Value & value)
{
    if (value == minLoudness || value == maxLoudness)
    {
        determineStretchOffsetAndWidthOfIndividualChannel();
        repaint();
    }
}

void MultiChannelLoudnessBar::setColour(const juce::Colour &newColour)
{
    colour = newColour;
}

//==============================================================================
void MultiChannelLoudnessBar::resized ()
{
    determineStretchOffsetAndWidthOfIndividualChannel();
}

void MultiChannelLoudnessBar::paint (Graphics& g)
{
    const float height = float (getHeight());
    
    g.setColour (colour);
    
    float topLeftX = 0.0f;
    for (size_t channel = 0; channel < currentMultiChannelLoudness.size(); ++channel)
    {
        float loudnessOfThisChannel = currentMultiChannelLoudness[channel];
        
        // It's only necessary to draw a bar for this channel, if its loudness
        // is inside the visible area of this component.
        if (loudnessOfThisChannel > float(minLoudness.getValue()))
        {
            // Don't draw the rectangle above (outside) of the visible area.
            loudnessOfThisChannel = jmin(loudnessOfThisChannel, float(maxLoudness.getValue()));
            
            float barHeightInPercent = stretch * loudnessOfThisChannel + offset;
            
            float topLeftY = (1.0f - barHeightInPercent) * height;
            float bottomY = height;
            g.fillRect(topLeftX,
                       topLeftY,
                       float (widthOfIndividualChannel),
                       bottomY-topLeftY);
        }
        
        topLeftX += widthOfIndividualChannel;
    }
}

void MultiChannelLoudnessBar::determineStretchOffsetAndWidthOfIndividualChannel()
{
    // These two values define a linear mapping
    //    f(x) = stretch * x + offset
    // for which
    //    f(minimumLevel) = 0
    //    f(maximumLevel) = 1
    stretch = 1.0f/(double(maxLoudness.getValue()) - double(minLoudness.getValue()));
    offset = -double(minLoudness.getValue()) * stretch;
    
    // Determine widthOfIndividualChannel.
    if (currentMultiChannelLoudness.size() != 0)
    {
        widthOfIndividualChannel = getWidth() / currentMultiChannelLoudness.size();
    }
    else
    {
        widthOfIndividualChannel = getWidth();
    }
    widthOfIndividualChannel = jmax(widthOfIndividualChannel, 1);
        // Should not be smaller than 1 pixel, such that an individual channel
        // is always clearly recognizable.
        // This also avoids division by zero.
}