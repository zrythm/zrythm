/*
  ==============================================================================

    SpectroscopeProcessor.h
    Created: 27 Jul 2014 2:34:48pm
    Author:  Samuel Gaehwiler
 
    Heavily based on the dRowAudio Spectroscope class by David Rowland.

  ==============================================================================
*/

#ifndef SPECTRUM_PROCESSOR_H_INCLUDED
#define SPECTRUM_PROCESSOR_H_INCLUDED


#include "SpectrumAnalyserHeader.h"

#if JUCE_MAC || JUCE_IOS || DROWAUDIO_USE_FFTREAL

//==============================================================================
/** Provides the audio processing part for a spectrum analyser.
 
    Register it with a TimeSliceThread, make sure its running and then continually
    call the copySamples() method. The FFT itself will be performed on a background
    thread.
 */
class SpectrumProcessor : public TimeSliceClient
{
public:
    //==============================================================================
    /** Creates a spectroscope with a given FFT size.
        Note that the fft size given here is log2 of the FFT size so for example,
        a 1024 size fft use 10.
     */
	SpectrumProcessor (int fftSizeLog2);
    
    /** Destructor. */
    ~SpectrumProcessor();
    
    void setSampleRate (double sampleRate);
    
	/** Copy a set of samples, ready to be processed.
        Your audio callback should continually call this method to pass it its
        audio data. When the scope has enough samples to perform an fft it will do
        so on a background thread.
     */
	void copySamples (const float* samples, int numSamples);
    
	/** @internal */
	virtual int useTimeSlice() override;
    
    void process();
    
    Value& getRepaintViewerValue();
    
    Value& getDetectedFrequency();
    
    drow::Buffer& getMagnitudesBuffer();
    
private:
    drow::FFTEngine fftEngine;
	HeapBlock<float> tempBlock;
    drow::FifoBuffer<float> circularBuffer;
	bool needToProcess;
    
    double sampleRate;
    Value detectedFrequency;
    
    Value repaintViewer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumProcessor);
};

#endif
#endif  // SPECTRUM_PROCESSOR_H_INCLUDED
