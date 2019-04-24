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


#ifndef __LOUDNESS_RANGE_HISTORY__
#define __LOUDNESS_RANGE_HISTORY__

#include "../MacrosAndJuceHeaders.h"
#include "LoudnessHistory.h"
#include <vector>
#include <algorithm> // to use std::rotate


//==============================================================================
/**
   A graph that shows the course of a value over time for a given time interval.
*/
class LoudnessRangeHistory  : public LoudnessHistory
{
public:
    LoudnessRangeHistory (const Value & lowLoudnessToReferTo,
                          const Value & highLoudnessToReferTo,
                          const Value & minLoudnessToReferTo,
                          const Value & maxLoudnessToReferTo);
    
    ~LoudnessRangeHistory ();
    
    void virtual paint (Graphics& g) override;
    
    /** Call this regularly to update and redraw the graph.
     */
    void virtual refresh() override;
    
    void virtual reset() override;
    
    void virtual resized () override;
    
private:

    Value currentLowLoudnessValue;
    
    /** A circular buffer to hold the past loudness values.
     */
    std::vector<float> circularLowLoudnessBuffer;
    std::vector<float>::iterator mostRecentLowLoudnessInTheBuffer;
};


#endif  // __LOUDNESS_RANGE_HISTORY__
