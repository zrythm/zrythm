#ifndef __ANALYZER_H__
#define __ANALYZER_H__

#include "JuceHeader.h"
#include "Buffers.h"
#include "ffft/FFTReal.h"

class RmsEnvelope
{
public:
	RmsEnvelope (int envsize, double rmsLength, double updatetime);
	
	void setSampleRate (double newSampleRate);
	void clear();

	bool process (const float inL, const float inR);
	void processBlock (const float* inL, const float* inR, int numSamples);

	bool getData (Array<float>& data) const;
	int getDataLength() const;

private:

	RmsLevel rms;
	CircularBuffer<float> rmsVals;
	double updateTime;
	double sampleRate;
	int updateIndex;

	juce::CriticalSection processLock;

	JUCE_DECLARE_NON_COPYABLE (RmsEnvelope)
};


class Analyzer
{
public:

	struct Data;

	Analyzer();

	void clear();
	void processBlock (const float* inL, const float* inR, int numSamples);

	int getFFTSize() const;
	int getNumBins() const;
	bool getData(Data& d) const;

	void setSampleRate (double newSampleRate);
	double getSampleRate() const;

private:

	void processFFT();

	juce::ScopedPointer<ffft::FFTReal<float> > fft;
	juce::HeapBlock<float> x;
	juce::HeapBlock<float> f;
	juce::HeapBlock<float> window;
	juce::ScopedPointer<Data> data;

	int fftIndex;

	int fftBlockSize;
	int numBins;

	double sampleRate;
};


#endif // __ANALYZER_H__