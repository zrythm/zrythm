/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/

#ifndef __DROWAUDIO_ENVELOPEFOLLOWER_H__
#define __DROWAUDIO_ENVELOPEFOLLOWER_H__

#include "filters/dRowAudio_OnePoleFilter.h"

//==============================================================================
/**
    EnvelopeFollower.
    
    Envelope follower class that gives an overall amplitude response of a set of
    samples.
 */
class EnvelopeFollower
{
public:
    //==============================================================================
	/** Constructor. */
	EnvelopeFollower();

	/** Destructor. */
	~EnvelopeFollower();
	
    //==============================================================================
	/** Uses different exponential attack and release coefficients.
		Call setTimes to setup this method, ignoring the hold time.
	 */
	void processEnvelope (const float* inputBuffer, float* outputBuffer, int numSamples) noexcept;

	/** Sets the times for the vaious stages of the envelope.
        1 is an instant attack/release, 0 ill never change the value.
     */
	void setCoefficients (float attack, float release) noexcept;
	
private:
    //==============================================================================
	float envelope;
	float envAttack, envRelease;

    //==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeFollower);
};

#endif // __DROWAUDIO_ENVELOPEFOLLOWER_H__