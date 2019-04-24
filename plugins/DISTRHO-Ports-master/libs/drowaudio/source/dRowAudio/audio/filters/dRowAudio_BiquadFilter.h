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

#ifndef __DROWAUDIO_BIQUADFILTER_H__
#define __DROWAUDIO_BIQUADFILTER_H__

//==============================================================================
/** A Biquad filter.
 
	This filter is a subclass of the Juce IIR filter but uses
	some additional methods to give more filter designs.
 */
class BiquadFilter : public IIRFilter
{
public:
    //==============================================================================
	/** Performs the filter operation on the given set of int samples.
	 */
    void processSamples (float* samples,
                         int numSamples) noexcept;
	
	/** Performs the filter operation on the given set of int samples.
	 */
    void processSamples (int* samples,
                         int numSamples) noexcept;
	
    //==============================================================================
	/**	Makes the filter a Low-pass filter. */
	static IIRCoefficients makeLowPass (const double sampleRate,
                                        const double frequency,
                                        const double Q) noexcept;
	
	/**	Makes the filter a High-pass filter. */
	static IIRCoefficients  makeHighPass (const double sampleRate,
                                          const double frequency,
                                          const double Q) noexcept;
	
	/**	Makes the filter a Band-pass filter. */
	static IIRCoefficients  makeBandPass (const double sampleRate,
                                          const double frequency,
                                          const double Q) noexcept;
	
	/**	Makes the filter a Band-stop filter. */
	static IIRCoefficients  makeBandStop (const double sampleRate,
                                          const double frequency,
                                          const double Q) noexcept;
	
	/**	Makes the filter a peak/notch filter. This type of filter
		adds or subtracts from the unfiltered signal.
	 */
	static IIRCoefficients  makePeakNotch (const double sampleRate,
                                           const double frequency,
                                           const double Q,
                                           const float gainFactor) noexcept;
	
	/**	Makes the filter an Allpass filter.
		This type of filter has a complex phase response so will give a comb 
		filtered effect when combined with an unfilterd copy of the signal.
	 */
	static IIRCoefficients  makeAllpass (const double sampleRate,
                                         const double frequency,
                                         const double Q) noexcept;
	
	/** Makes this filter duplicate the set-up of another one. */
    void copyOutputsFrom (const BiquadFilter& other) noexcept;
	
private:
    //==============================================================================
	JUCE_LEAK_DETECTOR (BiquadFilter);
};


//==============================================================================
/**	Primitive class to store the set-up info of a BiquadFilter
 */
class BiquadFilterSetup
{
public:
	enum FilterType
    {
		Lowpass = 0,
		Bandpass,
		Highpass,
		NoFilter
	};
	
	BiquadFilterSetup (FilterType filterType, double filterCf, double filterQ = 0.5)
	{
		type = filterType;
		cf = filterCf;
		q = filterQ;
	}
	
	void setUpFilter (BiquadFilter& filter, double sampleRate)
	{
        switch (type)
        {
            case Lowpass:   filter.setCoefficients (BiquadFilter::makeLowPass (sampleRate, cf, q));     break;
            case Bandpass:  filter.setCoefficients (BiquadFilter::makeBandPass (sampleRate, cf, q));    break;
            case Highpass:  filter.setCoefficients (BiquadFilter::makeHighPass (sampleRate, cf, q));    break;
            case NoFilter:  filter.makeInactive();                                                      break;
            default:                                                                                    break;
        }
		
		filter.reset();
	}
	
	FilterType type;
	double cf, q;
};

#endif //__DROWAUDIO_BIQUADFILTER_H__
