/*
 ===============================================================================
 
 BackgroundGrid.cpp
 
 
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


#include "BackgroundGrid.h"


//==============================================================================
BackgroundGrid::BackgroundGrid ()
{
}

BackgroundGrid::~BackgroundGrid ()
{
}

//==============================================================================
void BackgroundGrid::paint (Graphics& g)
{
    // TODO: Move them to a better place.
    const float topBorder = 0;
    const float bottomBorder = 0;
    int numberOfLines = 10;
    
    // Draw the background
//    g.setColour(Colours::black);
//    g.fillAll();
    
    // Draw the lines
    g.setColour(Colour(40,40,40));
    const float startX = 0.0f;
    const float endX = getWidth();
    float distanceBetweenTopAndBottomLine = getHeight() - topBorder - bottomBorder;
    // Draw the top line
    float startY = floor(float(int(topBorder))) + 0.5;
        // With - 0.5, this line would have not been drawn if topBorder = 0.
    float endY = startY;
    g.drawLine(startX, startY, endX, endY);
    // Draw the remaining lines
    for (int i=1; i<numberOfLines; i++)
    {
        float distanceFromTop = topBorder + float(i) * distanceBetweenTopAndBottomLine / (numberOfLines-1);
        // Draw a horizontal line
        float startY = floor(float(int(distanceFromTop))) - 0.5;
        //   float startY = getHeight()/2 + 0.5f;
        // Be aware: integer division.
        // Int. division - 0.5 ensures that the line will be drawn on a single
        // row of pixels. If only getHeight()/2.0f would have been written,
        // two rows of pixels would have been used for one line.
        float endY = startY;
        g.drawLine(startX, startY, endX, endY);  
    }
}