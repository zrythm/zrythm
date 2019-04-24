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

#ifndef __DROWAUDIO_SPECTROGRAPH_H__
#define __DROWAUDIO_SPECTROGRAPH_H__

#if JUCE_MAC || JUCE_IOS || DROWAUDIO_USE_FFTREAL

//==============================================================================
/**
    Creates a standard right-left greyscale Spectrograph.
 */
class Spectrograph
{
public:
    //==============================================================================
    /** Creates a Spectrograph with a given FFT size.
        Note that the fft size given here is log2 of the FFT size so for example,
        for a 1024 size FFT use 10 as the argument.
     */
    Spectrograph (int fftSizeLog2);
	
    /** Destructor. */
	~Spectrograph();
	
    //==============================================================================
    /** Creates a Spetrograph based on the whole set of samples provided.
        This effectively calls reset, preAllocateStorage, processSamples and then getImage.
     */
    Image generateImage (const float* samples, int numSamples);
    
    /** Clears all the internal buffers ready for a new set of samples. */
    void reset() noexcept;

    /** Pre-allocates the internal storage required for a number of samples.
        If you are creating a graph of an existing buffer it is more efficient to call this 
        first. Other wise there may be many re-allocations goinf on as data is added to be processed.
     */
    void ensureStorageAllocated (int numSamples);
    
	/** Processes a set of samples, to be added to the graph.
        Once enough samples have been gathered to perform an FFT operation they will 
        do so. Once you have finished prcessing all your samples use getImage to retrieve 
        the Spectrograph.
     */
	void processSamples (const float* samples, int numSamples);
    
    /** Returns the graph of the current set of processed samples.
        Note that this actually generates a new Image based on the internal buffers so if 
        you need to take copies etc. don't repeatedly call this method.
     */
    Image createImage() const;
    
    //==============================================================================
    /** Sets the scope to display in log or normal mode. */
	void setLogFrequencyDisplay (bool shouldDisplayLog);
	
    /** Returns true if the scope is being displayed in log mode. */
	bool getLogFrequencyDisplay() const             { return logFrequency; }
    
    /** Sets the size for one bin of fft data. This must be greater than 0.
        Higher values will effectively cause the graph to be wider and taller.
     */
    void setBinSize (const Rectangle<float>& size) noexcept;

    /** Returns the current bin size. */
    Rectangle<float> getBinSize() const             { return binSize; }
    
private:
    //==============================================================================
	FFTEngine fftEngine;
	int numBins;
	FifoBuffer<float> circularBuffer, fftMagnitudesData;
	HeapBlock<float> tempBlock;
    Array<float*> fftMagnitudesBlocks;
	bool logFrequency;
    Rectangle<float> binSize;

    void addMagnitudesBlock (const float* data, int size);
    void renderScopeLine();
    
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Spectrograph);
};

#endif
#endif  // __DROWAUDIO_SPECTROGRAPH_H__
