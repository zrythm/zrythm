#ifndef __SIMPLEDELAY__
#define __SIMPLEDELAY__

#include "JuceHeader.h"


class SampleDelay
{
public:
	SampleDelay(int lengthInSamples = 0)
		: length(0),
			position(0)
	{
		if (lengthInSamples > 0)
			setLength(lengthInSamples);
	}

	void setLength(int lengthInSamples)
	{		
		if (lengthInSamples != length)
		{
			dataL.realloc(lengthInSamples);
			dataR.realloc(lengthInSamples);
			length = lengthInSamples;
		}

		clearData();
	}

	void clearData()
	{
		for (int i=0; i<length; ++i)
		{
			dataL[i] = 0.f;
			dataR[i] = 0.f;
		}
	}


	void processBlock(float* proc, int numSamples)
	{
		if (length == 0)
			return;

		for (int i=0; i<numSamples; ++i)
		{
			const float x = proc[i];
			proc[i] = dataL[position];
			dataL[position] = x;

			if (++position >= length)
				position = 0;
		}

	}

	void processBlock(float* procL, float* procR, int numSamples)
	{
		if (length == 0)
			return;

		for (int i=0; i<numSamples; ++i)
		{
			const float l = procL[i];
			const float r = procR[i];
			procL[i] = dataL[position];
			procR[i] = dataR[position];
			dataL[position] = l;
			dataR[position] = r;

			if (++position >= length)
				position = 0;
		}

	}

private:
	HeapBlock<float> dataL;
	HeapBlock<float> dataR;
	int length;
	int position;

};



class SimpleDelay
{
public:

	SimpleDelay(double maxDelaySeconds = 4)
		: maxDelay(maxDelaySeconds),
			sampleRate(0),
			dataLength(0),
			delay(0)
	{
		setSampleRate(44100);
	}

	virtual ~SimpleDelay()
	{
	}

	void setSampleRate(double samplerate)
	{
		const int newDataLength = int(maxDelay * samplerate);

		if (sampleRate == samplerate && newDataLength == dataLength)
			return;

		sampleRate = samplerate;
		dataLength = newDataLength;

		jassert(dataLength > 0 && dataLength < 2.5e6);

		data.realloc(dataLength);
		clear();

		currentPos = 0;		
	}

	void clear()
	{
		for (int i=0; i<dataLength; ++i)
			data[i] = 0;
	}

	void processBlock(float* in, int numSamples)
	{
		for (int i=0; i<numSamples; ++i)
		{
			data[currentPos] = in[i];

			{
				int outPos = currentPos - delay;
				while (outPos < 0)
					outPos += dataLength;

				in[i] = data[outPos];

			}

			++currentPos;

			while (currentPos >= dataLength)
				currentPos -= dataLength;
		}
	}

	void setDelay(double time)
	{
		const int newDelay = int (time * sampleRate);
		jassert (newDelay>=0 && newDelay<dataLength-1);
		delay = jlimit(0, dataLength-1, newDelay);
	}

	void setDelaySamples(int samples)
	{
		jassert (samples>=0 && samples<dataLength-1);
		delay = jlimit(0, dataLength-1, samples);
	}

	double getSampleRate()
	{
		return sampleRate;
	}

	int getDataLength()
	{
		return dataLength;
	}

	double getDelayLengthSeconds()
	{
		return dataLength / sampleRate;
	}

	double getCurrentDelay()
	{
		return delay / sampleRate;
	}


private:

	double maxDelay;
	double sampleRate;

	HeapBlock<float> data;
	int dataLength;
	int currentPos;
	int delay;


};


#endif // __SIMPLEDELAY__