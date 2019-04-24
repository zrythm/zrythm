#ifndef __EQDSP_H_C44CCF6F__
#define __EQDSP_H_C44CCF6F__

#include "JuceHeader.h"
#include "coeffcreator.h"

class SimpleBiquad
{
public:
	SimpleBiquad()
		: analog(false), b0(1), b1(0), b2(0), a1(0), a2(0)
	{
		clear();
	}

	void clear()
	{
		x1 = 0;
		x2 = 0;
		y1 = 0;
		y2 = 0;
	}

	void setBiquad(double b0_, double b1_, double b2_, double a1_, double a2_)
	{
		b0 = b0_;
		b1 = b1_;
		b2 = b2_;
		a1 = a1_;
		a2 = a2_;
	}

	void processBlock(float* in, int numSamples)
	{
		Random& rnd(Random::getSystemRandom());

		if (analog)
		{
			for (int i=0; i<numSamples; ++i)
			{
				const double x0 = in[i] + 1e-5 * (rnd.nextDouble() - 0.5);
				double y0 = x0*b0 + x1*b1 + x2*b2 - y1*a1 - y2*a2;

				in[i] = (float) y0;
				x2 = x1; x1 = x0;
				y2 = y1; y1 = y0;
			}
		}
		else
		{
			for (int i=0; i<numSamples; ++i)
			{
				const double x0 = in[i];
				double y0 = x0*b0 + x1*b1 + x2*b2 - y1*a1 - y2*a2;

				if (y0 > -1e-10 && y0 < 1e-10)
					y0 = 0;

				in[i] = (float) y0;
				x2 = x1; x1 = x0;
				y2 = y1; y1 = y0;
			}
		}
	}

	void setAnalog(bool newAnalog)
	{
		analog = newAnalog;
	}

	float getDcGainWithLinGain(float filterGain, float linGain)
	{
		const double den = ( (b0+b1+b2)*filterGain + (1+a1+a2) )*linGain;
		const double nom = 1 + a1 + a2;
		return float(den/nom);
	}

private:

	bool analog;

	double b0, b1, b2;
	double a1, a2;
	
	double x1, x2;
	double y1, y2;
};

class EqDsp
{
public:

	enum Type
	{
		kBand10,
		kBand40,
		kBand160,
		kBand640,
		kShelf2k5,
		kShelfHi,

		kNumTypes
	};
	
	enum HighShelf
	{
		kHighOff,
		kHigh2k5,
		kHigh5k,
		kHigh10k,
		kHigh20k,
		kHigh40k,

		kNumHighSelves
	};

	EqDsp();

	void setGain(Type type, float newValue);
	float getGain(Type type);
	void setHighShelf(HighShelf type);
	HighShelf getHighShelf();

	void setBlockSize(int newBlockSize);
	void setSampleRate(double newSR);
	void processBlock(float* in, int numSamples);

	void setAnalog(bool newAnalog);
	bool getAnalog();

	void setMastering(bool newMastering);
	bool getMastering();

	void setKeepGain(bool keep);
	bool getKeepGain();

private:
	void setupFilter(Type type);

	float gains[kNumTypes];
	//float passGains[kNumTypes];
	HighShelf highShelf;

	HeapBlock<float> data10;
	HeapBlock<float> data40;
	HeapBlock<float> data160;
	HeapBlock<float> data640;
	HeapBlock<float> data2k5;
	HeapBlock<float> dataHi;
	HeapBlock<float> dataOut;
	int blockSize;

	bool analog;
	bool mastering;
	bool keepGain;

	CoeffCreator::SampleRates sampleRate;

	OwnedArray<SimpleBiquad> biquads;
};


class MultiEq
{
public:

	MultiEq(int numChannels);
	
	void setGain(EqDsp::Type type, float newValue);
	float getGain(EqDsp::Type type);
	void setHighShelf(EqDsp::HighShelf type);
	EqDsp::HighShelf getHighShelf();

	void setBlockSize(int newBlockSize);
	void setSampleRate(double newSR);
	void processBlock(float** in, int numChannels, int numSamples);

	void setAnalog(bool newAnalog);
	bool getAnalog();
	void setMastering(bool newMastering);
	bool getMastering();
	void setKeepGain(bool keep);
	bool getKeepGain();

private:
	OwnedArray<EqDsp> eqs;
};



#endif  // __EQDSP_H_C44CCF6F__
