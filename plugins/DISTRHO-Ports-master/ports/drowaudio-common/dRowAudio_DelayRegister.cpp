/*
 *  dRowAudio_DelayRegister.cpp
 *
 *  Created by David Rowland on 06/04/2009.
 *  Copyright 2009 dRowAudio. All rights reserved.
 *
 */

#include "dRowAudio_DelayRegister.h"

DelayRegister::DelayRegister(int bufferSize) throw()
	:	delayRegister(0),
		delaySamples(5),
		bufferWritePos(0),
		bufferReadPos(0)
{
	setBufferSize(bufferSize);
}

DelayRegister::~DelayRegister() throw()
{
	delete[] delayRegister;
	delayRegister = 0;
}

void DelayRegister::setBufferSize (int newBufferSize) throw()
{
	// find the next power of 2 for the buffer size
	registerSize = pow(2, ((int)log2(newBufferSize)) + 1);
	registerSizeMask = registerSize - 1;

	delete[] delayRegister;
	delayRegister = new float[registerSize];
	// zero register
	for (int i = 0; i < registerSize; i++)
		delayRegister[i] = 0;
}

void DelayRegister::setMaxDelayTime(double sampleRate, float maxDelayTimeMs) throw()
{
	int newBufferSize = (maxDelayTimeMs * 0.001 ) * sampleRate;
	setBufferSize(newBufferSize );
}

void DelayRegister::setDelayTime(double sampleRate, float newDelayTime) throw()
{
	delayTime = newDelayTime;

	delaySamples = (delayTime * (sampleRate * 0.001));

	if ((int)delaySamples >= registerSize)
	{
		jassert(delaySamples < registerSize);
		delaySamples = (float)registerSize;
	}
}

float DelayRegister::processSingleSample(float newSample) throw()
{
	bufferWritePos = (bufferWritePos+1) & registerSizeMask;

	bufferReadPos = bufferWritePos - delaySamples;
	if (bufferReadPos < 0)
		bufferReadPos += registerSize;

	delayRegister[bufferWritePos] = newSample;

	return delayRegister[bufferReadPos];
}

void DelayRegister::processSamples (float* const samples,
								 const int numSamples) throw()
{
    const ScopedLock sl (processLock);

	for (int i = 0; i < numSamples; ++i)
	{
		const float in = samples[i];

		bufferWritePos = (bufferWritePos+1) & registerSizeMask;

		bufferReadPos = bufferWritePos - delaySamples;
		if (bufferReadPos < 0)
			bufferReadPos += registerSize;

		delayRegister[bufferWritePos] = in;

		samples[i] = delayRegister[bufferReadPos];
	}
}
