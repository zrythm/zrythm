#include "delaytabdsp.h"



DelayTabDsp::DelayTabDsp(const String& id)
: Parameters(id),
	preDelayL(MAXDELAYSECONDS),
	preDelayR(MAXDELAYSECONDS),
	volume(0),
	volumeLin(1.f),
	panning(0.f),
	preVolume(0),
	enabled(false),
	mode(kMono),
	sync(0),
	dataSize(0)
{
	addParameter(kPitch, "Pitch", -12, 12, 0, 0);
	addParameter(kSync, "Sync", 0, 9, 0, 0);
	addParameter(kPitchType, "PitchType", 0, (int) delay.getNumPitches() + 1, 0, 0);
	addParameter(kPrePitch, "PrePitch", 0, 1, 0, 0);
	addParameter(kPreDelay, "Predelay", 0, MAXDELAYSECONDS, 0, 0);
	addParameter(kPreDelayVol, "Pre-Volume", -60, 0, 0, 0);
	addParameter(kDelay, "Delay", 0, MAXDELAYSECONDS, 0, MAXDELAYSECONDS/2);
	addParameter(kFeedback, "Feedback", 0, 100, 0, 0);

	addParameter(kFilterType, "EQ-Type", 0, 7, 0, 0);
	addParameter(kFilterFreq, "EQ-Freq", 20, 20e3, 1000, 1000);
	addParameter(kFilterQ, "EQ-Q", 0.3, 10, 3, 0.707);
	addParameter(kFilterGain, "EQ-Gain", -FILTERGAIN, FILTERGAIN, 0, 0);

	addParameter(kMode, "Mode", 0, (double) kNumModes - 1, 0, 0);

	addParameter(kVolume, "Volume", -60, 0, 0, 0);
	addParameter(kPan, "Pan", -100, 100, 1, 0);

	addParameter(kEnabled, "Enabled", 0, 1, 0, 0);

}

void DelayTabDsp::setParam(int index, double val)
{
	jassert(getParamRange(index).clipValue(val) == val);

	switch(index)
	{
	case kPitch:
		delay.setPitchSemitones(val);
		break;
	case kSync:
		sync = val;
		break;
	case kPreDelay:
		preDelayL.setDelay(val);
		preDelayR.setDelay(val);
		break;
	case kPreDelayVol:
		preVolume = val;
		break;
	case kDelay:
		delay.setDelay(val, delay.isPrePitch());
		break;
	case kPitchType:
		delay.setCurrentPitch(int(val - 1));
		break;
	case kPrePitch:
		delay.setDelay(delay.getDelay(), val > 0.5);
		break;
	case kFeedback:
		delay.setFeedback((float) val/100);
		break;

	case kFilterType:
		delay.setType((BasicFilters::Type) (int) (val+0.5));
		break;
	case kFilterFreq:
		delay.setFreq(val);
		break;
	case kFilterQ: 
		delay.setQ(val);
		break;
	case kFilterGain:
		delay.setGain(val);
		break;

	case kMode:
		mode = (Mode) jlimit(0, kNumModes - 1, int(val +0.5));
		delay.setPingPong(mode == kPingpong);
		break;

	case kVolume:
		volume = val;
		volumeLin = pow(10.f, (float) volume/20.f);
		break;
	case kPan:
		panning = (float) val;
		break;

	case kEnabled:
		enabled = val > 0.5;

		if (! enabled && dataSize > 0)
			clearData();
		break;
	default:
		jassertfalse;
	}


}

double DelayTabDsp::getParam(int index)
{
	double tmp = 0;

	switch(index)
	{
	case kPitch:
		tmp = delay.getPitchSemitones();
		break;
	case kSync:
		tmp = sync;
		break;
	case kPreDelay:
		tmp = preDelayL.getCurrentDelay();
		break;
	case kPreDelayVol:
		tmp = preVolume;
		break;
	case kDelay:
		tmp = delay.getDelay();
		break;
	case kPitchType:
		tmp = delay.getCurrentPitch() + 1;
		break;
	case kPrePitch:
		tmp = delay.isPrePitch() ? 1 : 0;
		break;
	case kFeedback:
		tmp = 100 * delay.getFeedback();
		break;

	case kFilterType:
		tmp = (double) (int) delay.getType();
		break;
	case kFilterFreq:
		tmp = delay.getFreq();
		break;
	case kFilterQ: 
		tmp = delay.getQ();
		break;
	case kFilterGain:
		tmp = delay.getGain();
		break;

	case kMode:
		return (double) (int) mode;
		break;

	case kVolume:
		tmp = volume;
		break;
	case kPan:
		tmp = panning;
		break;

	case kEnabled:
		tmp = enabled ? 1 : 0;
		break;
	default:
		jassertfalse;
	}

	return tmp;

}


void DelayTabDsp::prepareToPlay(double sampleRate, int numSamples)
{
	preDelayL.setSampleRate(sampleRate);
	preDelayR.setSampleRate(sampleRate);
	delay.prepareToPlay(sampleRate, numSamples);
	checkDataSize(numSamples);
}

void DelayTabDsp::checkDataSize(int numSamples)
{
	if (dataSize < numSamples)
	{
		dataL.realloc(numSamples);
		dataR.realloc(numSamples);
		dataPreL.realloc(numSamples);
		dataPreR.realloc(numSamples);
		dataSize = numSamples;
		clearData();
	}
}

void DelayTabDsp::clearData()
{
	jassert(dataSize > 0);

	for (int i=0; i<dataSize; ++i)
	{
		dataL[i] = 0;
		dataR[i] = 0;
		dataPreL[i] = 0;
		dataPreR[i] = 0;
	}
}

void DelayTabDsp::processBlock(const float* inL, const float* inR, int numSamples)
{

	checkDataSize(numSamples);

	if (! enabled)
		return;

	if (mode != kMono)
		processStereo(inL, inR, numSamples);
	else
		processMono(inL, inR, numSamples);
}

void DelayTabDsp::processMono(const float* inL, const float* inR, int numSamples)
{
	const float gainLeft = cos(float_Pi * (panning + 100.f)/400.f) * sqrt(2.f);
	const float gainRight = sin(float_Pi * (panning + 100.f)/400.f) * sqrt(2.f);

	for (int i=0; i<numSamples; ++i)
		dataL[i] = 0.5f * (inL[i] + inR[i]);

	if (preDelayL.getDelayLengthSeconds() > 0)
	{
		preDelayL.processBlock(dataL, numSamples);

		for (int i=0; i<numSamples; ++i)
			dataPreL[i] = dataL[i];
	}

	delay.processBlock(dataL, numSamples);

	if (preDelayL.getDelayLengthSeconds() > 0)
	{
		const float preGain = Decibels::decibelsToGain((float) preVolume);

		for (int i=0; i<numSamples; ++i)
		{
			const float x = dataL[i] * volumeLin;
			const float preX = dataPreL[i];
			dataL[i] = (x + preX * preGain) * gainLeft;
			dataR[i] = (x + preX * preGain) * gainRight;
		}
	}
	else
	{
		for (int i=0; i<numSamples; ++i)
		{
			const float x = dataL[i] * volumeLin;
			dataL[i] = x * gainLeft;
			dataR[i] = x * gainRight;
		}
	}
}

void DelayTabDsp::processStereo(const float* inL, const float* inR, int numSamples)
{
	const bool pingPong = delay.getPingPong();
	const float inGainL = pingPong && panning > 0 ? 1.f - panning/ 100.f : 1.f;
	const float inGainR = pingPong && panning < 0 ? 1.f - (-panning)/ 100.f : 1.f;

	const float outGainL = ! pingPong && panning > 0 ? 1.f - panning/ 100.f : 1.f;
	const float outGainR = ! pingPong && panning < 0 ? 1.f - (-panning)/ 100.f : 1.f;

	for (int i=0; i<numSamples; ++i)
	{
		dataL[i] = inL[i] * inGainL;
		dataR[i] = inR[i] * inGainR;
	}

	if (preDelayL.getDelayLengthSeconds() > 0)
	{
		preDelayL.processBlock(dataL, numSamples);
		preDelayR.processBlock(dataR, numSamples);

		for (int i=0; i<numSamples; ++i)
		{
			dataPreL[i] = dataL[i];
			dataPreR[i] = dataR[i];
		}
	}

	delay.processBlock(dataL, dataR, numSamples);

	if (preDelayL.getDelayLengthSeconds() > 0 && preVolume > -60)
	{
		const float preGain = Decibels::decibelsToGain((float) preVolume);
		const float gainL = volumeLin * outGainL;
		const float gainR = volumeLin * outGainR;
		const float preGainL = preGain * outGainL;
		const float preGainR = preGain * outGainR;

		for (int i=0; i<numSamples; ++i)
		{
			dataL[i] = dataL[i] * gainL + dataPreL[i] * preGainL;
			dataR[i] = dataR[i] * gainR + dataPreR[i] * preGainR;
		}
	}
	else
	{
		const float gainL = volumeLin * outGainL;
		const float gainR = volumeLin * outGainL;

		for (int i=0; i<numSamples; ++i)
		{
			dataL[i] *= gainL;
			dataR[i] *= gainR;
		}
	}
}

