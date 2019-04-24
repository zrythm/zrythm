/*
 ===============================================================================
 
 BackgroundHorizontalLinesAndCaption.cpp
 
 
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


#include "BackgroundVerticalLinesAndCaption.h"


//==============================================================================
BackgroundVerticalLinesAndCaption::BackgroundVerticalLinesAndCaption ()
  : specifiedTimeRange (20), // seconds
    textBoxWidth (40),
    distanceBetweenLeftBorderAndText (3),
    distanceBetweenGraphAndBottom (32)
{
}

BackgroundVerticalLinesAndCaption::~BackgroundVerticalLinesAndCaption ()
{
}


//==============================================================================
void BackgroundVerticalLinesAndCaption::resized()
{
    heightOfGraph = jmax(getHeight() - distanceBetweenGraphAndBottom, 0);
}

void BackgroundVerticalLinesAndCaption::paint (Graphics& g)
{
    // Draw the vertical lines and the time caption
    if (getWidth() > distanceBetweenLeftBorderAndText + textBoxWidth)
    {
        // Draw the left most vertical line behind the graph.
        g.setColour(Colour(40,40,40));
        const float xOfFirstLine = floor(distanceBetweenLeftBorderAndText + textBoxWidth/2.0) + 0.5;
            // "+0.5" causes the line to lie on a row of pixels and not between.
        g.drawLine(xOfFirstLine, 0, xOfFirstLine, heightOfGraph + 8);
        
        // Draw the text
        g.setColour(Colour(60,60,60));
        float fontHeight = 16.0f;
        const Font font (fontHeight);
        g.setFont(font);
        const int maximumNumberOfTextLines = 1;
        const float topLeftY = getHeight() - 1.5f * fontHeight;
        
        g.drawFittedText("-" + String(specifiedTimeRange) + "s", 
                         distanceBetweenLeftBorderAndText, 
                         topLeftY, 
                         textBoxWidth, 
                         fontHeight, 
                         juce::Justification::centred,
                         maximumNumberOfTextLines);
        
        // Draw the middle line and text
        int minimalDistanceBetweenText = 6; // pixels
        if (getWidth() > distanceBetweenLeftBorderAndText + 2*textBoxWidth + minimalDistanceBetweenText)
        {
            // Draw the vertical line behind the graph.
            g.setColour(Colour(40,40,40));
            const float xOfSecondLine = floor((getWidth() - xOfFirstLine)/2.0 + xOfFirstLine) + 0.5;
            // "+0.5" causes the line to lie on a row of pixels and not between.
            g.drawLine(xOfSecondLine, 0, xOfSecondLine, heightOfGraph + 8);
            
            // Draw the text
            g.setColour(Colour(60,60,60));
            g.setFont(font);
            g.drawFittedText("-" + String(specifiedTimeRange/2) + "s", 
                             xOfSecondLine - 0.5*textBoxWidth, 
                             topLeftY, 
                             textBoxWidth, 
                             fontHeight, 
                             juce::Justification::centred,
                             maximumNumberOfTextLines);
        }
    }
    
}