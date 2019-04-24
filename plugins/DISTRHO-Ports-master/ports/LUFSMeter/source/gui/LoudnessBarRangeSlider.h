/*
 ===============================================================================
 
 LoudnessBarRangeSlider.h
 
 
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


#ifndef __LOUDNESS_BAR_RANGE_SLIDER__
#define __LOUDNESS_BAR_RANGE_SLIDER__

#include "../MacrosAndJuceHeaders.h"



//==============================================================================
/** A two value horizontal slider where the two values have a given interval
 (or distance) between each other.
 
 This distance is hardcoded in the variable minimalIntervalBetweenMinAndMax
 in the method valueChanged().
*/
class LoudnessBarRangeSlider  : public Slider
{
public:
    LoudnessBarRangeSlider();
    
    ~LoudnessBarRangeSlider();
    
private:
    void valueChanged() override;

};


#endif  // __LOUDNESS_BAR_RANGE_SLIDER__
