#include "eqdsp.h"


EqDsp::EqDsp()
: highShelf(kHighOff),
	blockSize(0),
	analog(false),
	mastering(false),
	keepGain(false),
	sampleRate(CoeffCreator::k44100)
{
	for (int i=0; i<kNumTypes; ++i)
	{
		setGain((Type) i, 0);
		//gains[i] = 0.5f*56.2f/505.62f;
		biquads.add(new SimpleBiquad());
	}

	setBlockSize(2048);
	setSampleRate(44100);
}

void EqDsp::setGain(Type type, float newValue)
{
	const float maxError = 0.08f;

	jassert(newValue >= -10 && newValue <= 10);
	const bool changed = fabs(gains[type] - newValue) > maxError;

	gains[type] = newValue;

	if (analog && changed)
	{
		const float offs = (Random::getSystemRandom().nextFloat() - 0.5f) * maxError * (mastering ? 0.2f : 1.f);
		gains[type] += offs;
	}
}

float EqDsp::getGain(Type type)
{
	return gains[type];
}

void EqDsp::setHighShelf(HighShelf type)
{
	highShelf = type;

	for (int i=0; i<kNumTypes; ++i)
		setupFilter((Type) i);}

EqDsp::HighShelf EqDsp::getHighShelf()
{
	return highShelf;
}

void EqDsp::setBlockSize(int newBlockSize)
{
	if (newBlockSize > blockSize)
	{
		blockSize = newBlockSize;

		data10.realloc(blockSize);
		data40.realloc(blockSize);
		data160.realloc(blockSize);
		data640.realloc(blockSize);
		data2k5.realloc(blockSize);
		dataHi.realloc(blockSize);
		dataOut.realloc(blockSize);
	}
}

void EqDsp::setSampleRate(double newSR)
{
	sampleRate = newSR <=  44100 ? CoeffCreator::k44100 
		         : newSR <=  48000 ? CoeffCreator::k48000 
		         : newSR <=  88200 ? CoeffCreator::k88200
		         : newSR <=  96000 ? CoeffCreator::k96000 
		         : newSR <= 176400 ? CoeffCreator::k176400 
		         : newSR <= 192000 ? CoeffCreator::k192000 
						 : CoeffCreator::k192000;

	for (int i=0; i<kNumTypes; ++i)
		setupFilter((Type) i);
}

void EqDsp::processBlock(float* in, int numSamples)
{
	setBlockSize(numSamples);

	float g[kNumTypes];
	float pg[kNumTypes];

	for (int i=0; i<kShelfHi; ++i)
	{
		const float x = gains[i] /20.f + 0.5f;

		if (x > 0.5f)
		{
			g[i] = 0.5f * 56.2f / (5.56f + (4011.4f * exp(-7.4746f*x) - 0.54573f));
			pg[i] = 1.f;
		}
		else if (x >= 0.25f)
		{
			g[i] = 0.5f * 56.2f / (5.56f + (500 - 810.f*(x-0.25f)*2));
			pg[i] = 1.f;
		}
		else
		{
			g[i] = 0.5f * 56.2f / (5.56f + (500/* - 810.f*(x-0.25f)*2*/));
			pg[i] = (x*4);
		}
	}

	{
		const float x = gains[kShelfHi] / 10.f;
		g[kShelfHi] = 0.5f * 56.2f / (5.56f + (x <= 0.5f? 500 - 823.6f*x : 3258.2f * exp(-7.4126f*x) - 1.8466f));
		pg[kShelfHi] = 1.f;
		//g[kShelfHi] = x <= 0.5f? 500 - 823.6f*x : 3258.2f * exp(-7.4126f*x) - 1.8466f;
	}


	float* data[kNumTypes];
	data[kBand10] = data10;
	data[kBand40] = data40;
	data[kBand160] = data160;
	data[kBand640] = data640;
	data[kShelf2k5] = data2k5;
	data[kShelfHi] = dataHi;

	for (int i=0; i<kNumTypes; ++i)
		memcpy(data[i], in, sizeof(float)*numSamples);

	float dcGain = 0.f;

	for (int i=0; i<kNumTypes; ++i)
	{
		SimpleBiquad* biquad = biquads[i];
		jassert(biquad != nullptr);

		if (biquad != nullptr)
		{
			biquad->processBlock(data[i], numSamples);

			if (i != kShelfHi || highShelf != kHighOff)
				dcGain += g[i];
		}
	}

	if (highShelf != kHighOff)
	{
		for (int i=0; i<numSamples; ++i)
			dataOut[i] = (data[kShelfHi][i] * pg[kShelfHi] + in[i]) * g[kShelfHi];
	}
	else
	{
		for (int i=0; i<numSamples; ++i)
			dataOut[i] = 0.f;
	}

	for (int n=0; n<kShelfHi; ++n)
	{
		for (int i=0; i<numSamples; ++i)
			dataOut[i] += (data[n][i] * pg[n] + in[i]) * g[n];
	}

	const float globalGain = keepGain ? 0.398f/dcGain : 0.29f;

	if (analog)
	{
		Random& rnd(Random::getSystemRandom());

		for (int i=0; i<numSamples; ++i)
			in[i] = dataOut[i] * globalGain + float(1e-5) * (rnd.nextFloat() - 0.5f);
	}
	else
	{
		for (int i=0; i<numSamples; ++i)
			in[i] = dataOut[i] * globalGain;
	}
}

void EqDsp::setAnalog(bool newAnalog)
{
	analog = newAnalog;

	for (int i=0; i<biquads.size(); ++i)
		biquads.getUnchecked(i)->setAnalog(analog);
}

bool EqDsp::getAnalog()
{
	return analog;
}

void EqDsp::setMastering(bool newMastering)
{
	mastering = newMastering;
}

bool EqDsp::getMastering()
{
	return mastering;
}

void EqDsp::setKeepGain(bool keep)
{
	keepGain = keep;
}

bool EqDsp::getKeepGain()
{
	return keepGain;
}

void EqDsp::setupFilter(Type type)
{
	double b[3] = {0, 0, 0};
	double a[3] = {1, 0, 0};

	switch (type)
	{
	case kBand10:
		CoeffCreator::setCoeffs(CoeffCreator::kBand10, sampleRate, b, a);
		break;
	case kBand40:
		CoeffCreator::setCoeffs(CoeffCreator::kBand40, sampleRate, b, a);
		break;
	case kBand160:
		CoeffCreator::setCoeffs(CoeffCreator::kBand160, sampleRate, b, a);
		break;
	case kBand640:
		CoeffCreator::setCoeffs(CoeffCreator::kBand640, sampleRate, b, a);
		break;
	case kShelf2k5:
		CoeffCreator::setCoeffs(CoeffCreator::kShelf2k5, sampleRate, b, a);
		break;
	case kShelfHi:
		switch (highShelf)
		{
		case kHighOff:
			b[0] = 0; b[1] = 0; b[2] = 0;
			a[0] = 1; a[2] = 0; a[2] = 0;
			break;
		case kHigh2k5:
			CoeffCreator::setCoeffs(CoeffCreator::kA2k5, sampleRate, b, a);
			break;
		case kHigh5k:
			CoeffCreator::setCoeffs(CoeffCreator::kA5k, sampleRate, b, a);
			break;
		case kHigh10k:
			CoeffCreator::setCoeffs(CoeffCreator::kA10k, sampleRate, b, a);
			break;
		case kHigh20k:
			CoeffCreator::setCoeffs(CoeffCreator::kA20k, sampleRate, b, a);
			break;
		case kHigh40k:
			CoeffCreator::setCoeffs(CoeffCreator::kA40k, sampleRate, b, a);
			break;
		default:
			jassertfalse;
			break;
		}
		break;
	default:
		jassertfalse;
		break;
	}

	jassert(a[0] == 1);

	{
		SimpleBiquad* biquad = biquads[type];
		jassert(biquad != nullptr);

		if (biquad != nullptr)
			biquad->setBiquad(b[0], b[1], b[2], a[1], a[2]);
	}
}


// =================================================================================
MultiEq::MultiEq(int numChannels)
{
	jassert(numChannels > 0);

	for (int i=0; i<numChannels; ++i)
		eqs.add(new EqDsp());
}
	
void MultiEq::setGain(EqDsp::Type type, float newValue)
{
	for (int i=0; i<eqs.size(); ++i)
		eqs.getUnchecked(i)->setGain(type, newValue);
}

float MultiEq::getGain(EqDsp::Type type)
{
	return eqs.size() > 0 ? eqs.getUnchecked(0)->getGain(type) : 0;
}

void MultiEq::setHighShelf(EqDsp::HighShelf type)
{
	for (int i=0; i<eqs.size(); ++i)
		eqs.getUnchecked(i)->setHighShelf(type);
}

EqDsp::HighShelf MultiEq::getHighShelf()
{
	return eqs.size() > 0 ? eqs.getUnchecked(0)->getHighShelf() : EqDsp::kHighOff;
}

void MultiEq::setBlockSize(int newBlockSize)
{
	for (int i=0; i<eqs.size(); ++i)
		eqs.getUnchecked(i)->setBlockSize(newBlockSize);
}

void MultiEq::setSampleRate(double newSR)
{
	for (int i=0; i<eqs.size(); ++i)
		eqs.getUnchecked(i)->setSampleRate(newSR);
}

void MultiEq::processBlock(float** in, int numChannels, int numSamples)
{
	jassert (numChannels <= eqs.size());

	for (int i=0; i<jmin(numChannels, eqs.size()); ++i)
	{
		float* data = in[i];
		EqDsp* dsp = eqs.getUnchecked(i);
		dsp->processBlock(data, numSamples);
	}
}
void MultiEq::setAnalog(bool newAnalog)
{
	for (int i=0; i<eqs.size(); ++i)
		eqs.getUnchecked(i)->setAnalog(newAnalog);
}

bool MultiEq::getAnalog()
{
	return eqs.size() > 0 ? eqs.getUnchecked(0)->getAnalog() : false;
}

void MultiEq::setMastering(bool newMastering)
{
	for (int i=0; i<eqs.size(); ++i)
		eqs.getUnchecked(i)->setMastering(newMastering);
}

bool MultiEq::getMastering()
{
	return eqs.size() > 0 ? eqs.getUnchecked(0)->getMastering() : false;
}

void MultiEq::setKeepGain(bool keep)
{
	for (int i=0; i<eqs.size(); ++i)
		eqs.getUnchecked(i)->setKeepGain(keep);
}

bool MultiEq::getKeepGain()
{
	return eqs.size() > 0 ? eqs.getUnchecked(0)->getKeepGain() : false;
}