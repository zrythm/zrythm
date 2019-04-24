/*
 *  dRowAudio_LBCF.cpp
 *
 *  Created by David Rowland on 07/04/2009.
 *  Copyright 2009 dRowAudio. All rights reserved.
 *
 */

#include "dRowAudio_LBCF.h"

LBCF::LBCF() throw()
	:	fbCoeff(0.5),
		bufferWritePos(0)
{
	registerSize = BUFFERSIZE;
	registerSizeMask = registerSize - 1;

	delayRegister = new float[BUFFERSIZE];
	// zero register
	for (int i = 0; i < BUFFERSIZE; i++)
		delayRegister[i] = 0;
}

LBCF::~LBCF() throw()
{
	delete[] delayRegister;
}

void LBCF::setFBCoeff(float newFBCoeff) throw()
{
	fbCoeff = newFBCoeff;
}

void LBCF::setDelayTime(double sampleRate, float newDelayTime) throw()
{
	delayTime = newDelayTime;

	delaySamples = (int)(delayTime * (sampleRate * 0.001));

	if (delaySamples >= BUFFERSIZE)
	{
//		jassert(delaySamples < BUFFERSIZE);
		delaySamples = BUFFERSIZE;
	}
}

void LBCF::setLowpassCutoff(double sampleRate, float cutoffFrequency) throw()
{
	lowpassFilter.makeLowPass(sampleRate, cutoffFrequency);
}

float LBCF::processSingleSample(float newSample) throw()
{
	bufferWritePos = (bufferWritePos+1) & registerSizeMask;

	bufferReadPos = bufferWritePos - delaySamples;
	if (bufferReadPos < 0)
		bufferReadPos += BUFFERSIZE;

	float fOut = newSample + delayRegister[bufferReadPos];

	// feedback and lowpass
	delayRegister[bufferWritePos] = lowpassFilter.processSingleSample(fbCoeff *fOut);

	return fOut;
}

void LBCF::processSamples (float* const samples,
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

		float fOut = in + delayRegister[bufferReadPos];

		// feedback and lowpass
		delayRegister[bufferWritePos] = lowpassFilter.processSingleSample(fbCoeff * fOut);

		samples[i] = fOut;
	}
}

void LBCF::processSamplesAdding (float* const sourceSamples, float* const destSamples,
								 const int numSamples) throw()
{
	const ScopedLock sl (processLock);

	for (int i = 0; i < numSamples; ++i)
	{
		const float in = sourceSamples[i];

		bufferWritePos = (bufferWritePos+1) & registerSizeMask;

		bufferReadPos = bufferWritePos - delaySamples;
		if (bufferReadPos < 0)
			bufferReadPos += BUFFERSIZE;

		float fOut = in + delayRegister[bufferReadPos];

		// feedback and lowpass
		delayRegister[bufferWritePos] = lowpassFilter.processSingleSample(fbCoeff * fOut);

		destSamples[i] += fOut;
	}
}
