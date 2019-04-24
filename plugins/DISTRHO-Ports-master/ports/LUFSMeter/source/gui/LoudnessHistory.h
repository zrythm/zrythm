/*
 ===============================================================================
 
 LoudnessHistory.h
 
 
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


#ifndef __LOUDNESS_HISTORY__
#define __LOUDNESS_HISTORY__

#include "../MacrosAndJuceHeaders.h"
#include <vector>
#include <algorithm> // to use std::rotate


//==============================================================================
/**
   A graph that shows the course of a value over time for a given time interval.
*/
class LoudnessHistory  : public Component,
                         public Value::Listener
{
public:
    LoudnessHistory (const Value & loudnessValueToReferTo,
                     const Value & minLoudnessToReferTo,
                     const Value & maxLoudnessToReferTo);
    
    ~LoudnessHistory ();
    
    Value & getLoudnessValueObject ();
    
    void setColour (const Colour & newColour);

    int getDesiredRefreshIntervalInMilliseconds ();

    void virtual paint (Graphics& g) override;
    
    /** Call this regularly to update and redraw the graph.
     */
    void virtual refresh();
    
    void virtual reset();
    
    void virtual resized() override;
    
protected:
    /** Called when minLoudnessToReferTo or maxLoudnessToReferTo
     has changed.
     */
    void valueChanged (Value & value);
    
    /** Recalculates the values 'stretch' and 'offset' by the values of
     'minLoudness' and 'maxLoudness'.
     
     The two values stretch and offset define a linear mapping
     f(x) = stretch * x + offset
     for which
     f(minimumLevel) = 0
     f(maximumLevel) = 1 .
     */
    void determineStretchAndOffset ();
    
    void stretchTheHistoryGraph ();

    float minLoudnessToSet;
    
    Value currentLoudnessValue;
    Value minLoudness;
    Value maxLoudness;
    
    float stretch;
    float offset;
    
    Colour colour;
    
    /** The time interval that is displayed.
     Measured in seconds.
     */
    int specifiedTimeRange;
    float lineThickness;
    
    /** The horizontal distance between two points of this line-segment-graph.
     
     Together with the size of the plugin window, this determines
     the refreshrate of the history graph.
     
     A small value will quite increase the CPU usage.
     */
    float desiredNumberOfPixelsBetweenTwoPoints;
    int textBoxWidth;
    int distanceBetweenLeftBorderAndText;
    
    float fullTimeRange;
    float numberOfPixelsBetweenTwoPoints;
    
    int desiredRefreshIntervalInMilliseconds;
    
    /** A circular buffer to hold the past loudness values.
     */
    std::vector<float> circularLoudnessBuffer;
    std::vector<float>::iterator mostRecentLoudnessInTheBuffer;
    
    int distanceBetweenGraphAndBottom;
};


#endif  // __LOUDNESS_HISTORY__
