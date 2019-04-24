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


#include "LoudnessHistory.h"


//==============================================================================
LoudnessHistory::LoudnessHistory (const Value & loudnessValueToReferTo,
                                  const Value & minLoudnessToReferTo,
                                  const Value & maxLoudnessToReferTo)
  : minLoudnessToSet {-300.0f},
    currentLoudnessValue {minLoudnessToSet},
    colour {Colours::green},
    specifiedTimeRange {20}, // seconds
    lineThickness {2.0f},
    desiredNumberOfPixelsBetweenTwoPoints {6.0f},
    textBoxWidth {40},
    distanceBetweenLeftBorderAndText {3},
    desiredRefreshIntervalInMilliseconds {1000},
    mostRecentLoudnessInTheBuffer {circularLoudnessBuffer.begin()},
    distanceBetweenGraphAndBottom {32}
{
    currentLoudnessValue.referTo (loudnessValueToReferTo);
    minLoudness.referTo (minLoudnessToReferTo);
    minLoudness.addListener(this);
    maxLoudness.referTo (maxLoudnessToReferTo);
    maxLoudness.addListener(this);
    
    determineStretchAndOffset();
    
    // Memory allocation for the circularLoudnessBuffer.
    resized();
}

LoudnessHistory::~LoudnessHistory ()
{
}

Value & LoudnessHistory::getLoudnessValueObject ()
{
    return currentLoudnessValue;
}

void LoudnessHistory::setColour (const Colour & newColour)
{
    colour = newColour;
}

int LoudnessHistory::getDesiredRefreshIntervalInMilliseconds ()
{
    return desiredRefreshIntervalInMilliseconds;
}

void LoudnessHistory::paint (Graphics& g)
{
    // Draw the graph.
    g.setColour (colour);
    float currentX = getWidth();
    const float mostRecentLoudnessHeightInPercent = stretch * *mostRecentLoudnessInTheBuffer + offset;
    float currentY = (1.0f - mostRecentLoudnessHeightInPercent) * getHeight();
    float nextX;
    float nextY;
    std::vector<float>::iterator nextLoudness = mostRecentLoudnessInTheBuffer;
    
    do
    {
        if (nextLoudness == circularLoudnessBuffer.begin())
        {
            nextLoudness = circularLoudnessBuffer.end();
        }
        --nextLoudness;
        
        nextX = floor(currentX - numberOfPixelsBetweenTwoPoints);
        // Without "floor", the line segments are not joined,
        // a little space between the segments is visible.
        // TODO: Report this to Jules.
        
        const float loudnessHeightInPercent = stretch * *nextLoudness + offset;
        nextY = (1.0f - loudnessHeightInPercent) * getHeight();
        
        g.drawLine(currentX, currentY, nextX, nextY, lineThickness);
        
        // Prepare for the next iteration.
        currentX = nextX;
        currentY = nextY;
    }
    while (nextLoudness != mostRecentLoudnessInTheBuffer);
}

void LoudnessHistory::refresh()
{
    // Adjust the position of the iterator...
    mostRecentLoudnessInTheBuffer++;
    if (mostRecentLoudnessInTheBuffer == circularLoudnessBuffer.end())
    {
        mostRecentLoudnessInTheBuffer = circularLoudnessBuffer.begin();
    }
    // ...and put the current loudness into the circular buffer.
    *mostRecentLoudnessInTheBuffer = currentLoudnessValue.getValue();
    
    // Mark this component as "dirty" to make the OS send a paint message asap.
    repaint();
}

void LoudnessHistory::reset ()
{
    for (std::vector<float>::iterator i = circularLoudnessBuffer.begin();
         i != circularLoudnessBuffer.end();
         i++)
    {
        *i = minLoudnessToSet;
    }
}

void LoudnessHistory::resized()
{
    // Figure out the full time range.
    if (getWidth() > distanceBetweenLeftBorderAndText + textBoxWidth)
    {
        const double borderToLeftVerticalLine = distanceBetweenLeftBorderAndText + textBoxWidth/2.0;
        fullTimeRange = getWidth()/(getWidth() - borderToLeftVerticalLine) * specifiedTimeRange;
    }
    else
    {
        fullTimeRange = specifiedTimeRange;
    }
    
    int numberOfPoints = getWidth()/desiredNumberOfPixelsBetweenTwoPoints + 1;
    numberOfPixelsBetweenTwoPoints = getWidth()/jmax(double(numberOfPoints-1.0),1.0);
    
    // Rescaling, part 1
    // =================
    if (circularLoudnessBuffer.size() > 1)
    {
        // Dimensions changed -> Stretching of the current graph is needed.
        // Therefore this strategy:
        
        // Make a copy of the circularLoudnessBuffer and interpolate
        // ---------------------------------------------------------
        
        // Sort the buffer, such that the most recent value is at the end.
        // I.e. the oldest value is at the begin and the newest one at end()-1.
        if (mostRecentLoudnessInTheBuffer + 1 != circularLoudnessBuffer.end())
        {
            std::rotate(circularLoudnessBuffer.begin(), 
                        mostRecentLoudnessInTheBuffer + 1, 
                        circularLoudnessBuffer.end());
        }
    }
    // Copy this old (sorted) state.
    std::vector<float> oldCircularLoudnessBuffer (circularLoudnessBuffer);
    
    // Allocate memory needed for the circularLoudnessBuffer.
    std::vector<float>::size_type numberOfValues = numberOfPoints;
    circularLoudnessBuffer.resize (numberOfValues);
    //reset();
     
    // Rescaling, part 2
    // =================
    if (circularLoudnessBuffer.size() > 1 
        && oldCircularLoudnessBuffer.size() > 1 
        && getHeight() > 0)
    {
        double numberOfPixelsBetweenOldValues = double(getWidth())/double(oldCircularLoudnessBuffer.size() - 1);
        double xPosition = 0.0;
        
        // Set all the values but the last.
        for (std::vector<float>::iterator newLoudness = circularLoudnessBuffer.begin();
             newLoudness != circularLoudnessBuffer.end()-1;
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
            if (indexInTheOldBuffer < (int)oldCircularLoudnessBuffer.size() - 1)
            {
                jassert(indexInTheOldBuffer < (int)oldCircularLoudnessBuffer.size() - 1);
                // Linear interpolation between two points.
                *newLoudness = (1.0 - delta) * oldCircularLoudnessBuffer[indexInTheOldBuffer] + delta * oldCircularLoudnessBuffer[indexInTheOldBuffer + 1];
            }
            else
            {
                // indexInTheOldBuffer >= oldCircularLoudnessBuffer.size() - 1
                // might happen if delta == 0.0.
                // In this case, the
                // oldCircularLoudnessBuffer[indexInTheOldBuffer + 1]
                // will point to a not allocated memory region.
                *newLoudness = oldCircularLoudnessBuffer[indexInTheOldBuffer];
            }
            
            xPosition += numberOfPixelsBetweenTwoPoints;
        }
        
        // Set the last value.
        *(circularLoudnessBuffer.end()-1) = *(oldCircularLoudnessBuffer.end()-1);
    }

    // Set the itator to the first value
    mostRecentLoudnessInTheBuffer = circularLoudnessBuffer.end() - 1;
    
    // Calculate the time interval, at which the timerCallback() will be called by a instance of LoudnessHistoryGroup.
    desiredRefreshIntervalInMilliseconds = 1000*fullTimeRange/numberOfPoints;
}

void LoudnessHistory::valueChanged (Value & value)
{
    // minLoudness or maxLoudness has changed.
    // Therefore:
    determineStretchAndOffset();
}

void LoudnessHistory::determineStretchAndOffset()
{
    // These two values define a linear mapping
    //    f(x) = stretch * x + offset
    // for which
    //    f(minimumLoudness) = 0
    //    f(maximumLoudness) = 1
    stretch = 1.0f/(double(maxLoudness.getValue()) - double(minLoudness.getValue()));
    offset = -double(minLoudness.getValue()) * stretch;
}