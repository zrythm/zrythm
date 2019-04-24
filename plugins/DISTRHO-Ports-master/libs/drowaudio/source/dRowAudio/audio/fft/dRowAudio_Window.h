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

#ifndef __DROWAUDIO_WINDOW_H__
#define __DROWAUDIO_WINDOW_H__

#include "../dRowAudio_Buffer.h"
#include "../../utility/dRowAudio_Constants.h"
#include "../../maths/dRowAudio_MathsUtilities.h"

//==============================================================================
/**
    A pre-calculated Window buffer used for audio processing.
    @see FFT
 */
class Window
{
public:
    //==============================================================================
    /** Window types supported. */
	enum WindowType
    {
		Rectangular,
		Hann,
		Hamming,
		Cosine,
		Lanczos,
		ZeroEndTriangle,
		NonZeroEndTriangle,
		Gaussian,
		BartlettHann,
		Blackman,
		Nuttall,
		BlackmanHarris,
		BlackmanNuttall,
		FlatTop
	};
	
    //==============================================================================
    /** Creates a default Hann Window with 0 size. */
    Window();

    /** Creates a Hann Window with a given size. */
	explicit Window (int windowSize);

    /** Creates a Window with a given size. */
	explicit Window (int windowSize, WindowType type);

    /** Destructor. */
	~Window();

    /** Sets the window type. */
	void setWindowType (WindowType newType);

    /** Sets the window size. */
	void setWindowSize (int newSize);

    /** Returns the window type. */
	WindowType getWindowType() const noexcept           { return windowType; }
	
    /** Returns the window factor. */
	float getWindowFactor() const noexcept              { return windowFactor; }

    /** Returns the reciprocal of the window factor. */
    float getOneOverWindowFactor()                      { return oneOverWindowFactor; }

	/** Applies this window to a set of samples.
        For speed, your the number of samples passed here should be the same as the window size.
     */
	void applyWindow (float* samples,  const int numSamples) const noexcept;
	
private:
    //==============================================================================
	void setUpWindowBuffer();
	
	void applyRectangularWindow (float *samples,  const int numSamples);
	void applyHannWindow (float *samples,  const int numSamples);
	void applyHammingWindow (float *samples,  const int numSamples);
	void applyCosineWindow (float *samples,  const int numSamples);
	void applyLanczosWindow (float *samples,  const int numSamples);
	void applyZeroEndTriangleWindow (float *samples,  const int numSamples);
	void applyNonZeroEndTriangleWindow (float *samples,  const int numSamples);
	void applyGaussianWindow (float *samples,  const int numSamples);
	void applyBartlettHannWindow (float *samples,  const int numSamples);
	void applyBlackmanWindow (float *samples,  const int numSamples);
	void applyNuttallWindow (float *samples,  const int numSamples);
	void applyBlackmanHarrisWindow (float *samples,  const int numSamples);
	void applyBlackmanNuttallWindow (float *samples,  const int numSamples);
	void applyFlatTopWindow (float *samples,  const int numSamples);
	
    //==============================================================================
	WindowType windowType;
	float windowFactor, oneOverWindowFactor;
	AudioSampleBuffer windowBuffer;
    
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Window)
};

#endif //__DROWAUDIO_WINDOW_H__