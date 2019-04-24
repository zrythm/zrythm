#include "RefineDsp.h"

RefineDsp::RefineDsp()
: sampleRate (0),
  gainLow (0.f),
  gainMid (0.f),
  gainHigh (0.f),
  noise (-140.f),
  auxSize (4096),
  auxLowL (auxSize),
  auxHighL (auxSize),
  auxLowR (auxSize),
  auxHighR (auxSize),
  rms300 (static_cast<int> (44100*0.3)),
  rms5 (static_cast<int> (44100*0.005)),
  rms (400, 100, 0.025),
  colors (1),
  delayL (512),
  delayR (512)
{
	setSampleRate(44100);
	clear();
}

void RefineDsp::clear()
{
	lowL.clear();
	lowR.clear();
	highL.clear();
	highR.clear();
	levelSlow.clear();
	levelMid.clear();
	levelFast.clear();
	levelHold.clear();

	rms300.clear();
	rms5.clear();
	transient = 0;
	nonTransient = 0;
	level = 0;
}

void RefineDsp::setBlockSize (int newBlockSize)
{
    if (newBlockSize > auxSize)
    {
        auxSize = newBlockSize;
        auxLowL.realloc(auxSize);
        auxLowR.realloc(auxSize);
        auxHighL.realloc(auxSize);
        auxHighR.realloc(auxSize);
    }
}

void RefineDsp::setSampleRate (double newSampleRate)
{
	if (newSampleRate != sampleRate)
	{
		sampleRate = newSampleRate;

		rms.setSampleRate(sampleRate);
		colors.setSize(rms.getDataLength()+1);

		lowL.setFilter(BiquadType::kBandPass, 80.f, 0.5f);
		lowR.setFilter(BiquadType::kBandPass, 80.f, 0.5f);

		highL.setFilter(BiquadType::kHighPass6, 10e3f, 0.5f);
		highR.setFilter(BiquadType::kHighPass6, 10e3f, 0.5f);

		levelSlow.setFilter(BiquadType::kLowPass, 10, sqrt(0.5));
		levelMid.setFilter(BiquadType::kLowPass, 50, sqrt(0.5));
		levelFast.setFilter(BiquadType::kLowPass, 200, sqrt(0.5));

		rms300.setSize(int(0.3*sampleRate));
		rms5.setSize(int(0.02*sampleRate));
		trSmooth.setSampleRate(sampleRate, 0.3);

		delayL.setSize(512 * int(sampleRate / 44100));
		delayR.setSize(512 * int(sampleRate / 44100));
	}
}

void RefineDsp::setLow (float gain)
{
	gainLow = gain;
}

void RefineDsp::setMid (float gain)
{
	gainMid = gain;
}

void RefineDsp::setHigh (float gain)
{
	gainHigh = gain;
}

void RefineDsp::setX2Mode (bool enabled)
{
    x2Mode = enabled;
}

void RefineDsp::processBlock (float* dataL, float* dataR, int numSamples)
{
    const float x2gain = x2Mode ? 1.9475f : 1.f;

	if (dataR != nullptr)
		noise.processBlock(dataL, dataR, numSamples);
	else 
		noise.processBlock(dataL, numSamples);

	delayL.processBlock(dataL, numSamples);

	if (dataR != nullptr)
		delayR.processBlock(dataR, numSamples);

	{
		ScopedLock lock(processLock);

		if (dataR != nullptr)
			levelHold.processBlock(dataL, dataR, numSamples);
		else
			levelHold.processBlock(dataL, dataL, numSamples);

		const float release = 1.f - 1.f / float(sampleRate * 0.001 * 100);

		if (dataR == nullptr)
			dataR = dataL; 

		for (int i=0; i<numSamples; ++i)
		{
			if (std::abs(dataL[i]) < 1e-8f)
				dataL[i] = 0;

			if (std::abs(dataR[i]) < 1e-8f)
				dataR[i] = 0;

			float x = 0.5f * (dataL[i] + dataR[i]);

			if (x > -1e-4f && x < 1e-4f)
				x = 0;

			double r300 = rms300.process(x);	

			if (r300 != 0. && r300 < 1e-8f)
			{
				rms300.clear();
				r300 = 0;
			}

			double r5 = rms5.process(x);

			if (r5 != 0. && r5 < 1e-8f)
			{
				r5 = 0;
				rms5.clear();
			}

			const float y = r300 > 0 ? (float) jlimit(0., 1., (r5 / r300 - 1.)) : 0.f;

			trSmooth.processAttack(y*y);
		
			const float lHold = levelHold.getValue();			
			const float r300s = (float) sqrt(jmax(0., r300));
			level = lHold > 0 ? jlimit(0.f, 1.f, 1.4142f * r300s / lHold) : 0;
		
			jassert(level >= 0 && level < 1e8);

			const float newTrans = jmin(1.f, trSmooth.getValue()) * jmin<float>(1.f, sqrt(level) * 20.f);
			transient = newTrans > transient ? newTrans : transient * release;

			const float newNonTransient = (1 - pow(newTrans, 0.2f)) * jmin<float>(1.f, sqrt(level) * 20.f);
			nonTransient = newNonTransient > nonTransient ? newNonTransient : nonTransient * release;

			colorProc.add(transient, nonTransient, level);

			if (rms.process(dataL[i], dataR[i]))
			{
				colors.push(colorProc.getColor());
			}
		}
	}

	if (gainLow > 0)
	{
		if (dataL != nullptr)
		{
			memcpy(auxLowL, dataL, numSamples * sizeof(float));
			lowL.processBlock(auxLowL, numSamples);
		}

		if (dataR != nullptr)
		{
			memcpy(auxLowR, dataR, numSamples * sizeof(float));
			lowR.processBlock(auxLowR, numSamples);
		}
	}

	if (gainHigh > 0)
	{
		if (dataL != nullptr)
		{
			memcpy(auxHighL, dataL, numSamples * sizeof(float));
			highL.processBlock(auxHighL, numSamples);
		}

		if (dataR != nullptr)
		{
			memcpy(auxHighR, dataR, numSamples * sizeof(float));
			highR.processBlock(auxHighR, numSamples);
		}
	}

	if (gainMid > 0)
	{
		const float gain = pow(10.f, x2gain * 1.25f * sqrt(gainMid) / 20.f);

		if (dataL != nullptr)
		{
			for (int i=0; i<numSamples; ++i)
				dataL[i] *= gain;
		}

		if (dataR != nullptr)
		{
			for (int i=0; i<numSamples; ++i)
				dataR[i] *= gain;
		}
	}

	if (gainLow > 0)
	{
		const float gain = pow(10.f, x2gain * 0.95f * sqrt(gainLow) / 20.f) - 1.f;

		if (dataL != nullptr)
		{
			for (int i=0; i<numSamples; ++i)
				dataL[i] += auxLowL[i] * gain;
		}

		if (dataR != nullptr)
		{
			for (int i=0; i<numSamples; ++i)
				dataR[i] += auxLowR[i] * gain;
		}
	}

	if (gainHigh > 0)
	{
		const float gain = pow(10.f, x2gain * 1.28f * sqrt(gainHigh) / 20.f) - 1.f;

		if (dataL != nullptr)
		{
			for (int i=0; i<numSamples; ++i)
				dataL[i] += auxHighL[i] * gain;
		}

		if (dataR != nullptr)
		{
			for (int i=0; i<numSamples; ++i)
				dataR[i] += auxHighR[i] * gain;
		}
	}
}

float RefineDsp::getTransient() const
{
	return transient;
}

float RefineDsp::getNonTransient() const
{
	return nonTransient;
}

float RefineDsp::getLevel() const
{
	return level;
}


bool RefineDsp::getRmsData (Array<float>& d, Array<uint32>& c) const
{
	{
		ScopedTryLock lock(processLock);

		if (! lock.isLocked() || c.size() >= colors.getSize())
			return false;

		for (int i=0; i<c.size(); ++i)
			c.getReference(i) = colors[i];
	}

	return rms.getData(d);
}



