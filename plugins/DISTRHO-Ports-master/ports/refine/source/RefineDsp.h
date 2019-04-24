#ifndef SEPDSP_H_INCLUDED
#define SEPDSP_H_INCLUDED

#include "JuceHeader.h"
#include "Analyzer.h"
#include "Buffers.h"
#include "MiscDsp.h"

class LevelHold
{
public:
	LevelHold()
	{
		setSampleRate(44100);
		clear();
	}

	void setSampleRate (double sampleRate, double releaseTime = 1)
	{
		attack = 1 / (0.25 * releaseTime * sampleRate);
		release = 1 - 1 / (releaseTime * sampleRate);
	}

	void clear()
	{
		value = 0;
	}

	void process (double x)
	{
		value *= release;

		if (value < 1e-8)
			value = 0;

		if (x > value)
			value = x;
	}

	void processAttack (double x)
	{
		value *= release;

		if (value < 1e-8)
			value = 0;

		if (x > value)
			value = value + (x - value) * attack;
	}

	void processBlock (const float* inL, const float* inR, int numSamples)
	{
		for (int i=0; i<numSamples; ++i)
		{
			const double x = 0.5 * (inL[i] + inR[i]);
			process(x);
		}
	}

	float getValue()
	{
		return (float) value;
	}

private:

	double attack;
	double release;
	double value;

};



class RefineDsp
{
public:
	RefineDsp();

	void clear();

    void setBlockSize(int newBlockSize);
	void setSampleRate(double newSampleRate);	
	void setLow(float gain);
	void setMid(float gain);
	void setHigh(float gain);
    void setX2Mode(bool enabled);

	void processBlock(float* dataL, float* dataR, int numSamples);

	float getTransient() const;
	float getNonTransient() const;
	float getLevel() const;

	bool getRmsData(Array<float>& d, Array<uint32>& c) const;

private:

	struct ColorProc
	{
		ColorProc() 
            : transient (0), nonTransient (0), level (0) 
        {
        }

		void add (float tr, float ntr, float l)
		{
			transient += tr;
			nonTransient += ntr;
			level += l;
		}

		juce::uint32 getColor()
		{
			juce::uint32 ret = 0;

			const float sum = transient + nonTransient + level;

			if (sum > 0)
			{
				const juce::uint32 tr = juce::uint8(255 * transient / sum);
				const juce::uint32 ntr = juce::uint8(255 * nonTransient / sum);
				const juce::uint32 l = juce::uint8(255 * level / sum);

				ret = (tr << 16) | (ntr << 8) | l;
			}

			transient = 0;
			nonTransient = 0;
			level = 0;
			return ret;
		}

		float transient;
		float nonTransient;
		float level;
	};


	double sampleRate;

	float gainLow;
	float gainMid;
	float gainHigh;
    bool x2Mode;

	float transient;
	float nonTransient;
	float level;

	SimpleNoiseGen noise;

	int auxSize;
	juce::HeapBlock<float> auxLowL;
	juce::HeapBlock<float> auxHighL;
	juce::HeapBlock<float> auxLowR;
	juce::HeapBlock<float> auxHighR;

	StaticBiquad lowL;
	StaticBiquad lowR;
	StaticBiquad highL;
	StaticBiquad highR;

	StaticBiquad levelSlow;
	StaticBiquad levelMid;
	StaticBiquad levelFast;
	
	RmsBuffer rms300;
	RmsBuffer rms5;

	LevelHold trSmooth;

	LevelHold levelHold;

	RmsEnvelope rms;
	CircularBuffer<juce::uint32> colors;
	ColorProc colorProc;

	CircularBuffer<float> delayL;
	CircularBuffer<float> delayR;

	juce::CriticalSection processLock;
};

#endif  // SEPDSP_H_INCLUDED
