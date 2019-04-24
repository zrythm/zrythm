/*
 ===============================================================================
 
 LoudnessHistory.cpp
 
 
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


#include "LoudnessRangeHistory.h"


//==============================================================================
LoudnessRangeHistory::LoudnessRangeHistory (const Value & lowLoudnessToReferTo,
                                            const Value & highLoudnessToReferTo,
                                            const Value & minLoudnessToReferTo,
                                            const Value & maxLoudnessToReferTo)
: LoudnessHistory(highLoudnessToReferTo,
                  minLoudnessToReferTo,
                  maxLoudnessToReferTo),
  currentLowLoudnessValue (var(-300.0f)),
  mostRecentLowLoudnessInTheBuffer (circularLowLoudnessBuffer.begin())
{
    currentLowLoudnessValue.referTo(lowLoudnessToReferTo);
}

LoudnessRangeHistory::~LoudnessRangeHistory ()
{
}

void LoudnessRangeHistory::paint (Graphics& g)
{
    // The path which will be the border of the filled area.
    Path loudnessRangePath;
    
    // Create the top path for the high loudness
    // =========================================
    // from right to left.
    
    // Set the start-position of the path.
    const float startX = getWidth();
    const float mostRecentLoudnessHeightInPercent = stretch * *mostRecentLoudnessInTheBuffer + offset;
    const float startY = (1.0f - mostRecentLoudnessHeightInPercent) * getHeight();
    loudnessRangePath.startNewSubPath(startX, startY);
    
    // Add the additional points
    float nextX = startX;
    float nextY = 0;
    std::vector<float>::iterator nextLoudness = mostRecentLoudnessInTheBuffer;
    if (nextLoudness == circularLoudnessBuffer.begin())
    {
        nextLoudness = circularLoudnessBuffer.end();
    }
    --nextLoudness;
    
    while (nextLoudness != mostRecentLoudnessInTheBuffer)
    {
        // Prepare for the next iteration
        nextX -= numberOfPixelsBetweenTwoPoints;
        
        // Add the point
        const float loudnessHeightInPercent = stretch * *nextLoudness + offset;
        nextY = (1.0f - loudnessHeightInPercent) * getHeight();
        
        loudnessRangePath.lineTo(nextX, nextY);
        
        // Prepare for the next iteration
        if (nextLoudness == circularLoudnessBuffer.begin())
        {
            nextLoudness = circularLoudnessBuffer.end();
        }
        --nextLoudness;
    }

    // Create the bottom path for the low loudness
    // ===========================================
    // from left to right.
    
    // Preparation for the first point of the low loudness.
    nextLoudness = mostRecentLowLoudnessInTheBuffer;
    ++nextLoudness;
    if (nextLoudness == circularLowLoudnessBuffer.end())
    {
        nextLoudness = circularLowLoudnessBuffer.begin();
    }
    std::vector<float>::iterator oldestLowLoudness = nextLoudness;
    
    do
    {
        // Add the point
        const float loudnessHeightInPercent = stretch * *nextLoudness + offset;
        nextY = (1.0f - loudnessHeightInPercent) * getHeight();
        
        loudnessRangePath.lineTo(nextX, nextY);
        
        // Prepare for the next iteration
        nextX += numberOfPixelsBetweenTwoPoints;
        ++nextLoudness;
        if (nextLoudness == circularLowLoudnessBuffer.end())
        {
            nextLoudness = circularLowLoudnessBuffer.begin();
        }
    }
    while (nextLoudness != oldestLowLoudness);
    
    loudnessRangePath.closeSubPath();
    
    // Draw the graph.
    g.setColour (colour);
    //ColourGradient gradient (colour.withAlpha(0.8f), 0.0f, 0.0f, colour.withAlpha(0.4f), 0.0f, getHeight(), false);
    //g.setGradientFill(gradient);
    g.fillPath(loudnessRangePath);
    
    //g.setColour (colour);
    //g.strokePath (loudnessRangePath, PathStrokeType (lineThickness));
}

void LoudnessRangeHistory::refresh()
{
    // DEB("LoudnessRangeHistory refresh called.")
    
    // Store the low loudness
    // ----------------------
    // Adjust the position of the iterator...
    mostRecentLowLoudnessInTheBuffer++;
    if (mostRecentLowLoudnessInTheBuffer == circularLowLoudnessBuffer.end())
    {
        mostRecentLowLoudnessInTheBuffer = circularLowLoudnessBuffer.begin();
    }
    // ... and put the current low loudness into the circular buffer.
    *mostRecentLowLoudnessInTheBuffer = currentLowLoudnessValue.getValue();
    
    // Store the high loudness and invoke a repaint
    // --------------------------------------------
    LoudnessHistory::refresh();
}

void LoudnessRangeHistory::reset ()
{
    // Reset the circular buffer for the low loudness
    for (std::vector<float>::iterator i = circularLowLoudnessBuffer.begin();
         i != circularLowLoudnessBuffer.end();
         i++)
    {
        *i = minLoudnessToSet;
    }
    
    // Reset the circular buffer for the high loudness
    LoudnessHistory::reset();
}

void LoudnessRangeHistory::resized()
{
    // Resizing for the high loudness:
    // *******************************
    LoudnessHistory::resized();
    
    
    // Resizing for the low loudness:
    // *******************************
    // This is stupidly copied from the LoudnessHistory::resized()...
    
    // Rescaling, part 1
    // =================
    if (circularLowLoudnessBuffer.size() > 1)
    {
        // Dimensions changed -> Stretching of the current graph is needed.
        // Therefore this strategy:
        
        // Make a copy of the circularLoudnessBuffer and interpolate
        // ---------------------------------------------------------
        
        // Sort the buffer, such that the most recent value is at the end.
        // I.e. the oldest value is at the begin and the newest one at end()-1.
        if (mostRecentLowLoudnessInTheBuffer + 1 != circularLowLoudnessBuffer.end())
        {
            std::rotate(circularLowLoudnessBuffer.begin(),
                        mostRecentLowLoudnessInTheBuffer + 1,
                        circularLowLoudnessBuffer.end());
        }
    }
    // Copy this old (sorted) state.
    std::vector<float> oldCircularLowLoudnessBuffer (circularLowLoudnessBuffer);
    
    // Allocate memory needed for the circularLoudnessBuffer.
    std::vector<float>::size_type numberOfPoints = getWidth()/desiredNumberOfPixelsBetweenTwoPoints + 1;
    circularLowLoudnessBuffer.resize(numberOfPoints);
   
    // Rescaling, part 2
    // =================
    if (circularLowLoudnessBuffer.size() > 1
        && oldCircularLowLoudnessBuffer.size() > 1
        && getHeight() > 0)
    {
        double numberOfPixelsBetweenOldValues = double(getWidth())/double(oldCircularLowLoudnessBuffer.size() - 1);
        double xPosition = 0.0;
        
        // Set all the values but the last.
        for (std::vector<float>::iterator newLoudness = circularLowLoudnessBuffer.begin();
             newLoudness != circularLowLoudnessBuffer.end()-1;
             newLoudness++)
        {
            // Find the old value, that is closest to the new xPosition from
            // the left.
            int indexInTheOldBuffer = floor(xPosition/numberOfPixelsBetweenOldValues);
            double delta = xPosition/numberOfPixelsBetweenOldValues - indexInTheOldBuffer;
            // delta lies in the range [0.0, 1.0[ and describes the position
            // of the newYPosition between the neighbouring values of the
            // old circular buffer.
            jassert(indexInTheOldBuffer >= 0);
            if (indexInTheOldBuffer < (int)oldCircularLowLoudnessBuffer.size() - 1)
            {
                jassert(indexInTheOldBuffer < (int)oldCircularLowLoudnessBuffer.size() - 1);
                // Linear interpolation between two points.
                *newLoudness = (1.0 - delta) * oldCircularLowLoudnessBuffer[indexInTheOldBuffer] + delta * oldCircularLowLoudnessBuffer[indexInTheOldBuffer + 1];
            }
            else
            {
                // indexInTheOldBuffer >= oldCircularLoudnessBuffer.size() - 1
                // might happen if delta == 0.0.
                // In this case, the
                // oldCircularLoudnessBuffer[indexInTheOldBuffer + 1]
                // will point to a not allocated memory region.
                *newLoudness = oldCircularLowLoudnessBuffer[indexInTheOldBuffer];
            }
            
            xPosition += numberOfPixelsBetweenTwoPoints;
        }
        
        // Set the last value.
        *(circularLowLoudnessBuffer.end()-1) = *(oldCircularLowLoudnessBuffer.end()-1);
    }
    
    // Set the itator to the first value
    mostRecentLowLoudnessInTheBuffer = circularLowLoudnessBuffer.end() - 1;
    
}