/*
 ===============================================================================
 
 MultiChannelLoudnessBar.h
 
 
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


#ifndef __MULTI_CHANNEL_LOUDNESS_BAR__
#define __MULTI_CHANNEL_LOUDNESS_BAR__

#include "../MacrosAndJuceHeaders.h"

using std::vector;


//==============================================================================
/**
 A vertical loudness meter for multiple values.
 
 Usage: Create a Value object v to which this level bar can "listen" to.
 Every time v is changed it will update itself. You don't have to add the
 loudness bar directly to the listeners of v (v.addListener(&MultiChannelLoudnessBar)).
 But you need to ensure that its internal value referes to the same
 value source as your value object v:
 
 -  MultiChannelLoudnessBar.getLevelValueObject().referTo(v);
*/
class MultiChannelLoudnessBar  : public Component,
                                 public Value::Listener
{
public:
    MultiChannelLoudnessBar (const Value & minLoudnessValueToReferTo,
                             const Value & maxLoudnessValueToReferTo);
    
    ~MultiChannelLoudnessBar ();
    
    void setLoudness (const vector<float>& multiChannelLoudness);
    
    void valueChanged (Value & value) override;
    
    void setColour (const Colour & newColour);

    void resized () override;
    void paint (Graphics& g) override;
    
private:
    /** Recalculates the values 'stretch' and 'offset' by the values of
     'minLoudness' and 'maxLoudness'.
     
     The two values stretch and offset define a linear mapping
        f(x) = stretch * x + offset
     for which
        f(minimumLevel) = 0
        f(maximumLevel) = 1 .
     */
    void determineStretchOffsetAndWidthOfIndividualChannel();
    
    float stretch;
    float offset;
    int widthOfIndividualChannel;
    
    Colour colour;
    
    vector<float> currentMultiChannelLoudness;
    Value minLoudness;
    Value maxLoudness;
};


#endif  // __MULTI_CHANNEL_LOUDNESS_BAR__
