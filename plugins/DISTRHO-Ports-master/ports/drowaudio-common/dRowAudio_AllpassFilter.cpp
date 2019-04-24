/*
 *  dRowAudio_AllpassFilter.cpp
 *
 *  Created by David Rowland on 09/04/2009.
 *  Copyright 2009 dRowAudio. All rights reserved.
 *
 */

#include "dRowAudio_AllpassFilter.h"

AllpassFilter::AllpassFilter() throw()
	:	gain(0.1f),
		bufferWritePos(0)
{
	registerSize = BUFFERSIZE;
	registerSizeMask = registerSize - 1;

	delayRegister = new float[BUFFERSIZE];
	// zero register
	for (int i = 0; i < BUFFERSIZE; i++)
		delayRegister[i] = 0;
}

AllpassFilter::~AllpassFilter() throw()
{
	delete[] delayRegister;
}

void AllpassFilter::setGain(float newGain) throw()
{
	gain = newGain;
}

void AllpassFilter::setDelayTime(double sampleRate, float newDelayTime) throw()
{
	delayTime = newDelayTime;

	delaySamples = (int)(delayTime * (sampleRate * 0.001));

	if (delaySamples >= BUFFERSIZE)
	{
		jassert(delaySamples < BUFFERSIZE);
		delaySamples = BUFFERSIZE;
	}
}

float AllpassFilter::processSingleSample(float newSample) throw()
{
	bufferWritePos = (bufferWritePos+1) & registerSizeMask;

	bufferReadPos = bufferWritePos - delaySamples;
	if (bufferReadPos < 0)
		bufferReadPos += BUFFERSIZE;

	float fDel = delayRegister[bufferReadPos];
	delayRegister[bufferWritePos] = (gain * fDel) + newSample;
	float fOut = fDel - (gain * delayRegister[bufferWritePos]);

	return fOut;
}

void AllpassFilter::processSamples (float* const samples,
									const int numSamples) throw()
{
	const ScopedLock sl (processLock);

	for (int i = 0; i < numSamples; ++i)
	{
		const float in = samples[i];

		bufferWritePos = (bufferWritePos+1) & registerSizeMask;

		bufferReadPos = bufferWritePos - delaySamples;
		if (bufferReadPos < 0)
			bufferReadPos += BUFFERSIZE;

		float fDel = delayRegister[bufferReadPos];
		delayRegister[bufferWritePos] = (gain * fDel) + in;
		float fOut = fDel - (gain * delayRegister[bufferWritePos]);

		samples[i] = fOut;
	}
}
