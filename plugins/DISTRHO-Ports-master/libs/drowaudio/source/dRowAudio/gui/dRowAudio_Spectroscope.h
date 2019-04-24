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

#ifndef __DROWAUDIO_SPECTROSCOPE_H__
#define __DROWAUDIO_SPECTROSCOPE_H__

#if JUCE_MAC || JUCE_IOS || DROWAUDIO_USE_FFTREAL

//==============================================================================
/** Creates a standard Spectroscope.
    This will display the amplitude of each frequency bin from an FFT in a
    continuous line which will decay with time.
    This is very simple to use, it is a GraphicalComponent so just register one
    with a TimeSliceThread, make sure its running and then continually call the
    copySamples() method. The FFT itself will be performed on a background thread.
 */
class Spectroscope : public GraphicalComponent
{
public:
    //==============================================================================
    /** Creates a spectroscope with a given FFT size.
        Note that the fft size given here is log2 of the FFT size so for example,
        a 1024 size fft use 10.
     */
	Spectroscope (int fftSizeLog2);
	
    /** Destructor. */
	~Spectroscope();
	
    /** @internal */
	void resized();
	
    /** @internal */
	void paint (Graphics &g);
	
    //==============================================================================
    /** Sets the scope to display in log or normal mode.
     */
	void setLogFrequencyDisplay (bool shouldDisplayLog);
	
    /** Returns true if the scope is being displayed in log mode.
     */
	inline bool getLogFrequencyDisplay() const      {   return logFrequency;	}

    //==============================================================================
	/** Copy a set of samples, ready to be processed.
        Your audio callback should continually call this method to pass it its
        audio data. When the scope has enough samples to perform an fft it will do
        so on a background thread and redraw itself.
     */
	void copySamples (const float* samples, int numSamples);

    /** @internal */
	void timerCallback();
	
    /** @internal */
	void process();
	
    /** @internal */
	void flagForRepaint();

private:
    //==============================================================================
	FFTEngine fftEngine;
	int numBins;
	bool needsRepaint;
	HeapBlock<float> tempBlock;			
	FifoBuffer<float> circularBuffer;
	
	bool logFrequency;
    Image scopeImage;
    
    void renderScopeImage();
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Spectroscope);
};

#endif
#endif  // __DROWAUDIO_SPECTROSCOPE_H__