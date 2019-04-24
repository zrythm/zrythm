#include "Analyzer.h"


struct Analyzer::Data
{
    Data(int size_);
    void clear();
    void copyFrom (const Data& other);
    const juce::CriticalSection& getLock() const;

    const int size;
    juce::HeapBlock<float> mags;
    juce::HeapBlock<float> angles;

private:

    juce::CriticalSection lock;
    JUCE_DECLARE_NON_COPYABLE(Data)
};

Analyzer::Data::Data (int size_)
    : size (size_),
      mags (size),
      angles (size)
{
    clear();
}

void Analyzer::Data::clear()
{
    juce::zeromem(mags, sizeof(float) * size);
    juce::zeromem(angles, sizeof(float) * size);
}

void Analyzer::Data::copyFrom (const Data& other)
{
    if (other.size == size)
    {
        memcpy(mags, other.mags, sizeof(float) * size);
        memcpy(angles, other.angles, sizeof(float) * size);
    }
}

const juce::CriticalSection& Analyzer::Data::getLock() const
{
    return lock;
}


RmsEnvelope::RmsEnvelope (int envsize, double rmsLength, double updatetime)
: rms (rmsLength), rmsVals (envsize+1), updateTime (updatetime), sampleRate (44100)
{
	clear();
}

void RmsEnvelope::setSampleRate (double newSampleRate)
{
	sampleRate = newSampleRate;
	rms.setSampleRate(sampleRate);
	clear();
}

void RmsEnvelope::clear()
{
	rms.clear();
	rmsVals.clear();
	updateIndex = 0;
}

bool RmsEnvelope::process (const float inL, const float inR)
{
	rms.process(inL, inR);

	const int ovSize = int(updateTime * sampleRate);
	jassert(ovSize > 100);

	if (++updateIndex >= ovSize)
	{
		updateIndex = 0;
		const float rmsVal = rms.getRms();

		rmsVals.push(rmsVal);
		return true;
	}

	return false;
}

void RmsEnvelope::processBlock (const float* inL, const float* inR, int numSamples)
{
	ScopedLock lock(processLock);

	const int ovSize = int(updateTime * sampleRate);

	jassert(ovSize > 100);

	int idx = 0;

	while (numSamples > 0)
	{
		const int curNumSamples = jmin(ovSize-updateIndex, numSamples);

		rms.processBlock(&inL[idx], &inR[idx], curNumSamples);
		
		numSamples -= curNumSamples;
		idx += curNumSamples;
		updateIndex += curNumSamples;

		if (updateIndex >= ovSize)
		{
			updateIndex = 0;
			const float rmsVal = rms.getRms();

			rmsVals.push(rmsVal);
		}
	}
}

bool RmsEnvelope::getData (Array<float>& data) const
{
	const int dataLength = getDataLength();

	if (data.size() < dataLength)
	{
		ScopedTryLock lock(processLock);

		if (! lock.isLocked())
			return false;

		for (int i=0; i<data.size(); ++i)
			data.getReference(i) = rmsVals[i];

		return true;
	}

	return false;
}

int RmsEnvelope::getDataLength() const
{
	return rmsVals.getSize() - 1;
}

Analyzer::Analyzer()
: sampleRate (0)
{
	setSampleRate(44100);
}

void Analyzer::clear()
{
	fftIndex = 0;
	data->clear();
}

void Analyzer::processBlock (const float* inL, const float* inR, int numSamples)
{
	for (int i=0; i<numSamples; ++i)
	{
		x[fftIndex] = 0.5f * (inL[i] + inR[i]) * window[fftIndex];
		++fftIndex;

		if (fftIndex >= fftBlockSize)
		{
			processFFT();
			fftIndex = 0;
		}
	}
}

int Analyzer::getFFTSize() const
{
	return fftBlockSize;
}

int Analyzer::getNumBins() const
{
	return numBins;
}

bool Analyzer::getData (Data& d) const
{
	ScopedTryLock lock(data->getLock());

	if (lock.isLocked())
	{
		d.copyFrom(*data);
		return true;
	}

	return false;
}

void Analyzer::setSampleRate (double newSampleRate)
{
	if (sampleRate != newSampleRate)
	{
		sampleRate = newSampleRate;

		fftBlockSize = 512 * jmax(1, int(sampleRate / 44100));
		numBins = fftBlockSize / 2 + 1;

		fft = new ffft::FFTReal<float> (fftBlockSize);
		x.realloc(fftBlockSize);
		f.realloc(fftBlockSize);
		window.realloc(fftBlockSize);
		data = new Data(numBins);

		{
			const float alpha = 0.16f;
			const float a0 = 0.5f * (1-alpha);
			const float a1 = 0.5f;
			const float a2 = alpha*0.5f;

			for (int i=0; i<fftBlockSize; ++i)
				window[i] = a0 - a1*cos(2*float_Pi*i/(fftBlockSize-1)) + a2*cos(4*float_Pi*i/(fftBlockSize-1));
		}

		clear();
	}
}

double Analyzer::getSampleRate() const
{
	return sampleRate;
}

void Analyzer::processFFT()
{
	fft->do_fft(f, x);

	const int imagOffset = fftBlockSize / 2;
	const float weight = 1.f / fftBlockSize;

	{
		ScopedLock lock(data->getLock());

		data->mags[0] = f[0]*f[0] * weight;
		data->mags[imagOffset] = f[imagOffset]*f[imagOffset] * weight;
		data->angles[0] = 0;
		data->angles[numBins-1] = 0;

		for (int i=1; i<numBins-1; ++i)
		{
			const float re = f[i];
			const float im = f[imagOffset + i];
			data->mags[i] = (re*re + im*im) * weight;
			data->angles[i] = atan2(im, re);
		}
	}
}
