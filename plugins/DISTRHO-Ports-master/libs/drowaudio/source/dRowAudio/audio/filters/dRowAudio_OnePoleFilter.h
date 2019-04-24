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

#ifndef __DROWAUDIO_ONEPOLEFILTER_H__
#define __DROWAUDIO_ONEPOLEFILTER_H__

//==============================================================================
/**
    One-Pole Filter.
 
	This is a simple filter that uses only one pole. As such it is very
	computationaly efficient especially if used to process a buffer of samples.
	It has a slope of -6dB/octave.
 */
class OnePoleFilter
{
public:
    //==============================================================================
	/**	Create an unititialised filter.
		This will not perform any filtering yet, call a make... method
		to turn it into that particular type of filter.
	 */
	OnePoleFilter() noexcept;
	
	/** Destructor.
     */
	~OnePoleFilter() noexcept;
	
	/**	Process a number of samples in one go.
		This is the most effecient method of filtering.
		Note that the samples passed to it actually get changed.
	 */
	void processSamples (float* const samples,
						 const int numSamples) noexcept;
	
	/**	Process a single sample.
		Less efficient method but leaves the sample unchanged,
		returning a filtered copy of it.
	 */
	inline float processSingleSample (const float sampleToProcess) noexcept
	{
		return y1 = (b0 * sampleToProcess) + (a1 * y1);
	}
	
	/**	Turns the filter into a Low-pass.
	 */
	void makeLowPass (const double sampleRate,
					  const double frequency) noexcept;
	/**	Turns the filter into a High-pass.
	 */
	void makeHighPass (const double sampleRate,
					   const double frequency) noexcept;

private:
    //==============================================================================
	CriticalSection lock;
	float y1, b0, a1;
	
    //==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OnePoleFilter);
};

#endif // __DROWAUDIO_ONEPOLEFILTER_H__