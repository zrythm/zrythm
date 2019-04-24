#include "pitcheddelay.h"

//#include "pitchdiracle.h"
#include "simpledetune.h"

PitchedDelay::PitchedDelay(float samplerate)
	: pitch(1.f),
		sampleRate(samplerate),
		feedback(0.f),
		pingpong(false),
		preDelayPitch(false),
		enablePitch(false),
		latency(0),
		currentTime(2),
		delayL(MAXDELAYSECONDS),
		delayR(MAXDELAYSECONDS),
		sizeLastData(0)
{
#if 0
	pitcher.addPitchProc(new PitchDiracLE(PitchDiracLE::kPreview));
	pitcher.addPitchProc(new PitchDiracLE(PitchDiracLE::kGood));
	pitcher.addPitchProc(new PitchDiracLE(PitchDiracLE::kBetter));
	pitcher.addPitchProc(new PitchDiracLE(PitchDiracLE::kBest));
#endif
	pitcher.addPitchProc(new Detune("Detune (low-latency)", 256));
	pitcher.addPitchProc(new Detune("Detune (compromise)", 1024));
	pitcher.addPitchProc(new Detune("Detune (best)", 4096));

	prepareToPlay(44100, 512);
}

PitchedDelay::~PitchedDelay()
{

}

void PitchedDelay::prepareToPlay(double samplerate, int blockSize)
{
	if (blockSize != sizeLastData)
	{
		sizeLastData = blockSize;
		lastDataL.realloc(sizeLastData);
		lastDataR.realloc(sizeLastData);
	}

	jassert(samplerate == 44100 || samplerate == 48000);

	if (samplerate != 44100 && samplerate != 44800)
		samplerate = 44100;

	sampleRate = samplerate;

	pitcher.prepareToPlay(sampleRate, blockSize);
	dcBlock.setSampleRate(sampleRate);

	latency = pitcher.getLatency();

	delayL.setSampleRate(sampleRate);
	delayR.setSampleRate(sampleRate);

	updateLatency(latency);

	clearLastData();
}


int PitchedDelay::getLatency()
{
	return latency;
}

void PitchedDelay::setPitch(double newPitch)
{
	// max 3 octave up or down.
	pitch = jlimit(0.125, 8., newPitch);
}

void PitchedDelay::setPitchSemitones(double semitones)
{
	pitch = jlimit(0.125, 8., pow(2., semitones / 12.));
}


double PitchedDelay::getPitchSemitones()
{
	return 12 * log(pitch) / log(2.);
}

void PitchedDelay::setDelay(double time, bool prePitch)
{
	preDelayPitch = prePitch;

	time = jlimit(0., MAXDELAYSECONDS - 2./sampleRate, time);

	currentTime = time;

	const int latency = pitcher.getLatency();

	DBG("setDelay("+String(time*1000, 1)+" ms, "+String(prePitch?"prePitch)":"postPitch)") +" pitcherlatency="+String(latency));

	const int delaySamplesPrePitch = jlimit(0, delayL.getDataLength(), int(time*sampleRate));
	const int delaySamples = jmax(0, preDelayPitch ? delaySamplesPrePitch : int(delayRange.clipValue(time)*sampleRate) - latency);

	delayL.setDelaySamples(delaySamples);
	delayR.setDelaySamples(delaySamples);
}

double PitchedDelay::getDelay()
{
	const int latency = pitcher.getLatency();
	return preDelayPitch ? delayL.getCurrentDelay() : delayL.getCurrentDelay() + latency/sampleRate;
}

bool PitchedDelay::isPrePitch()
{
	return preDelayPitch;
}

Range<double> PitchedDelay::getDelayRange()
{
	return delayRange;
}

Range<double> PitchedDelay::getDelayRangePrePitch()
{
	return Range<double> (0, delayL.getDelayLengthSeconds());
}


Range<double> PitchedDelay::getCurrentDelayRange()
{
	return preDelayPitch ? getDelayRangePrePitch() : getDelayRange();
}

int PitchedDelay::getLatencyWhenPrePitched()
{
	return pitcher.getLatency();
}

void PitchedDelay::processBlock(float* data, int numSamples)
{
	if (preDelayPitch)
	{
		if (enablePitch)
		{
			pitcher.setPitch((float) pitch);
			pitcher.processBlock(data, numSamples);
		}
		else
		{
			unpitchedDelay.processBlock(data, numSamples);
		}

		for (int i=0; i<numSamples; ++i)
			data[i] = data[i] + lastDataL[i]*feedback;

		delayL.processBlock(data, numSamples);

		filterL.processBlock(data, numSamples);

		for (int i=0; i<numSamples; ++i)
		{
			if (data[i] < 1e-8 && data[i] > -1e-8)
				data[i] = 0;

			lastDataL[i] = data[i];
		}
		dcBlock.processBlock(lastDataL, numSamples);
	}
	else
	{
		latencyCompensation.processBlock(data, numSamples);

		for (int i=0; i<numSamples; ++i)
			data[i] = data[i] + lastDataL[i]*feedback;

		if (enablePitch)
		{
			pitcher.setPitch((float) pitch);
			pitcher.processBlock(data, numSamples);
		}
		else
		{
			unpitchedDelay.processBlock(data, numSamples);
		}

		delayL.processBlock(data, numSamples);

		filterL.processBlock(data, numSamples);

		for (int i=0; i<numSamples; ++i)
		{
			if (data[i] < 1e-8 && data[i] > -1e-8)
				data[i] = 0;

			lastDataL[i] = data[i];
			jassert(fabs(data[i]) < 1e3);
		}
		dcBlock.processBlock(lastDataL, numSamples);
	}
}

void PitchedDelay::processBlock(float* dataL, float* dataR, int numSamples)
{
	if (preDelayPitch)
	{
		if (enablePitch)
		{
			pitcher.setPitch((float) pitch);
			pitcher.processBlock(dataL, dataR, numSamples);
		}
		else
		{
			unpitchedDelay.processBlock(dataL, dataR, numSamples);
		}

		if (pingpong)		
		{
			for (int i=0; i<numSamples; ++i)
			{
				const float l = dataL[i];
				const float r = dataR[i];
				dataL[i] = r + lastDataL[i] * feedback;
				dataR[i] = l + lastDataR[i] * feedback;
			}
		}
		else
		{
			for (int i=0; i<numSamples; ++i)
			{
				dataL[i] = dataR[i] + lastDataL[i] * feedback;
				dataR[i] = dataL[i] + lastDataR[i] * feedback;
			}
		}

		delayL.processBlock(dataL, numSamples);
		delayR.processBlock(dataR, numSamples);

		filterL.processBlock(dataL, numSamples);
		filterR.processBlock(dataR, numSamples);

		if (pingpong)
		{
			for (int i=0; i<numSamples; ++i)
			{
				if (dataL[i] < 1e-8 && dataL[i] > -1e-8)
					dataL[i] = 0;

				if (dataR[i] < 1e-8 && dataR[i] > -1e-8)
					dataR[i] = 0;

				lastDataL[i] = dataR[i];
				lastDataR[i] = dataL[i];
			}
		}
		else
		{
			for (int i=0; i<numSamples; ++i)
			{
				if (dataL[i] < 1e-8 && dataL[i] > -1e-8)
					dataL[i] = 0;

				if (dataR[i] < 1e-8 && dataR[i] > -1e-8)
					dataR[i] = 0;

				lastDataL[i] = dataL[i];
				lastDataR[i] = dataR[i];
			}
		}
		dcBlock.processBlock(lastDataL, lastDataR, numSamples);
	}
	else // ! preDelayPitch
	{
		latencyCompensation.processBlock(dataL, dataR, numSamples);


		if (pingpong)
		{
			for (int i=0; i<numSamples; ++i)
			{
				const float l = dataL[i];
				const float r = dataR[i];
				dataL[i] = r + lastDataL[i] * feedback;
				dataR[i] = l + lastDataR[i] * feedback;
			}
		}
		else
		{
			for (int i=0; i<numSamples; ++i)
			{
				dataL[i] = dataL[i] + lastDataL[i] * feedback;
				dataR[i] = dataR[i] + lastDataR[i] * feedback;
			}
		}


		if (enablePitch)
		{
			pitcher.setPitch((float) pitch);
			pitcher.processBlock(dataL, dataR, numSamples);

		}
		else
		{
			unpitchedDelay.processBlock(dataL, dataR, numSamples);
		}

		delayL.processBlock(dataL, numSamples);
		delayR.processBlock(dataR, numSamples);

		filterL.processBlock(dataL, numSamples);
		filterR.processBlock(dataR, numSamples);

		if (pingpong)
		{
			for (int i=0; i<numSamples; ++i)
			{
				if (dataL[i] < 1e-8 && dataL[i] > -1e-8)
					dataL[i] = 0;

				if (dataR[i] < 1e-8 && dataR[i] > -1e-8)
					dataR[i] = 0;

				lastDataL[i] = dataR[i];
				lastDataR[i] = dataL[i];
			}
		}
		else
		{
			for (int i=0; i<numSamples; ++i)
			{
				if (dataL[i] < 1e-8 && dataL[i] > -1e-8)
					dataL[i] = 0;

				if (dataR[i] < 1e-8 && dataR[i] > -1e-8)
					dataR[i] = 0;

				lastDataL[i] = dataL[i];
				lastDataR[i] = dataR[i];
			}
		}
		dcBlock.processBlock(lastDataL, lastDataR, numSamples);
	}
}

void PitchedDelay::setFeedback(float newFeedback)
{
	jassert(newFeedback>=0 && newFeedback <=1);

	feedback = newFeedback;
}

float PitchedDelay::getFeedback()
{
	return feedback;
}

void PitchedDelay::setGain(double gain)
{
	filterL.setGain(gain);
	filterR.setGain(gain);
}

double PitchedDelay::getGain()
{
	return filterL.getGain();
}

void PitchedDelay::setQ(double Q)
{
	filterL.setQ(Q);
	filterR.setQ(Q);
}

double PitchedDelay::getQ()
{
	return filterL.getQ();
}

void PitchedDelay::setFreq(double freq)
{
	filterL.setFreq(freq);
	filterR.setFreq(freq);
}

double PitchedDelay::getFreq()
{
	return filterL.getFreq();
}

void PitchedDelay::setType(BasicFilters::Type type)
{
	filterL.setType(type);
	filterR.setType(type);
}

BasicFilters::Type PitchedDelay::getType()
{
	return filterL.getType();
}

void PitchedDelay::setPingPong(bool enable)
{
	pingpong = enable;
}

bool PitchedDelay::getPingPong()
{
	return pingpong;
}

void PitchedDelay::setCurrentPitch(int index)
{
	if (index < 0 || index >= pitcher.getNumPitches())
		index = -1;

	pitcher.setPitchProc(index);
	enablePitch = index >= 0;
	updateLatency(enablePitch ? pitcher.getLatency() : 0);
}

void PitchedDelay::clearLastData()
{
	for (int i=0; i<sizeLastData; ++i)
	{
		lastDataL[i] = 0;
		lastDataR[i] = 0;
	}
}

void PitchedDelay::updateLatency(int latency)
{
	latencyCompensation.setLength(latency);
	unpitchedDelay.setLength(latency);

	const double minDelay = (latency + 10) / sampleRate;
	const double maxDelay = (delayL.getDataLength() + latency - 10) / sampleRate;
	delayRange = Range<double> (minDelay, maxDelay);

	setDelay(currentTime, preDelayPitch);
}



