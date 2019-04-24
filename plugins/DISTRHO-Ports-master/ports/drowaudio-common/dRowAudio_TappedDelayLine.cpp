/*
 *  dRowAudio_TappedDelayLine.cpp
 *
 *  Created by David Rowland on 13/04/2009.
 *  Copyright 2009 dRowAudio. All rights reserved.
 *
 */

#include "dRowAudio_TappedDelayLine.h"

TappedDelayLine::TappedDelayLine(int initialBufferSize)
	:	pfDelayBuffer(0)
{
	initialiseBuffer(initialBufferSize);
}

TappedDelayLine::TappedDelayLine(float bufferLengthMs, double sampleRate)
	:	pfDelayBuffer(0)
{
	int initialBufferSize = (int)((bufferLengthMs * 0.001) * sampleRate);
	
	initialiseBuffer(initialBufferSize);
}

void TappedDelayLine::initialiseBuffer(int bufferSize_)
{
	bufferSize = bufferSize_;
	bufferWritePos = 0;
	inputGain = 1.0f;
	feedbackGain = 0.99f;
	spacingCoefficient = 1.0f;
	feedbackCoefficient = 1.0f;
	
	delete[] pfDelayBuffer;
	
	pfDelayBuffer = new float[bufferSize];
	for (int i = 0; i < bufferSize; i++)
		pfDelayBuffer[i] = 0;		
}

TappedDelayLine::~TappedDelayLine()
{
	readTaps.clear();
	delete[] pfDelayBuffer;
	pfDelayBuffer = 0;
}

void TappedDelayLine::addTap(int noDelaySamples, int sampleRate)
{
	jassert(noDelaySamples < bufferSize);
	
	Tap newTap;
	newTap.originalDelaySamples = noDelaySamples;
	newTap.delaySamples = newTap.originalDelaySamples;
	
	newTap.originalTapFeedback = feedbackCoefficient;
	newTap.tapFeedback = newTap.originalTapFeedback;
	
	newTap.sampleRateWhenCreated = sampleRate;
	newTap.tapGain = 0.15f;
	
	readTaps.add(newTap);
	
	noTaps = readTaps.size();	
}

void TappedDelayLine::addTapAtTime(int newTapPosMs, double sampleRate)
{
	int newTapPosSamples = (int)((newTapPosMs * 0.001) * sampleRate);
	
	addTap(newTapPosSamples, sampleRate);
}

bool TappedDelayLine::setTapDelayTime(int tapIndex, int newTapPosMs, double sampleRate)
{
	return setTapDelaySamples(tapIndex, msToSamples(newTapPosMs, sampleRate));
}

bool TappedDelayLine::setTapDelaySamples(int tapIndex, int newDelaySamples)
{
	if ( tapIndex < readTaps.size() )
	{
		readTaps.getReference(tapIndex).delaySamples = newDelaySamples;
		
		return true;
	}
	return false;
}

void TappedDelayLine::setTapSpacing(float newSpacingCoefficient)
{
	if ( !almostEqual<float>(spacingCoefficient, newSpacingCoefficient) )
	{
		spacingCoefficient = fabsf(newSpacingCoefficient);

		for (int i = 0; i < readTaps.size(); i++)
		{
			int newDelaySamples = (readTaps[i].originalDelaySamples * spacingCoefficient);
			jlimit(0, bufferSize, newDelaySamples);
			readTaps.getReference(i).delaySamples = newDelaySamples;
		}
	}
}

void TappedDelayLine::setTapSpacingExplicitly(float newSpacingCoefficient)
{
	spacingCoefficient = fabsf(newSpacingCoefficient);
	
	for (int i = 0; i < readTaps.size(); i++)
	{
		int newDelaySamples = (readTaps[i].originalDelaySamples * spacingCoefficient);
		jlimit(0, bufferSize, newDelaySamples);
		readTaps.getReference(i).delaySamples = newDelaySamples;
	}
}

void TappedDelayLine::scaleFeedbacks(float newFeedbackCoefficient)
{
	if ( !almostEqual<float>(feedbackCoefficient, newFeedbackCoefficient) )
	{
		feedbackCoefficient = newFeedbackCoefficient;
		
		for (int i = 0; i < readTaps.size(); i++)
			readTaps.getReference(i).tapFeedback = readTaps.getReference(i).originalTapFeedback * feedbackCoefficient;
	}
}

Array<int> TappedDelayLine::getTapSamplePositions()
{
	Array<int> tapSamplePositions;
	
	for (int i = 0; i < readTaps.size(); i++)
		tapSamplePositions.add(readTaps[i].delaySamples);
	
	return tapSamplePositions;
}

bool TappedDelayLine::removeTapAtIndex(int tapIndex)
{
	readTaps.remove(tapIndex);
	if (readTaps.size() < noTaps) {
		noTaps = readTaps.size();
		return true;
	}
	
	return false;
}

bool TappedDelayLine::removeTapAtSample(int sampleForRemovedTap)
{
	for (int i = 0; i < readTaps.size(); i++)
		if (readTaps[i].delaySamples == sampleForRemovedTap) {
			readTaps.remove(i);
			noTaps = readTaps.size();
			return true;
		}
	
	return false;
}

bool TappedDelayLine::removeTapAtMs(int timeMsForRemovedTap, int sampleRate)
{
	int tapsample = timeMsForRemovedTap * 0.001 * sampleRate;
	
	return removeTapAtSample(tapsample);
}

void TappedDelayLine::removeAllTaps()
{
	readTaps.clear();
	noTaps = readTaps.size();
}

void TappedDelayLine::updateDelayTimes(double newSampleRate)
{
	for (int i = 0; i < readTaps.size(); i++)
	{
		if ( (int)readTaps[i].sampleRateWhenCreated != 0
			 && !almostEqual<double>(newSampleRate, readTaps.getReference(i).sampleRateWhenCreated) )
		{
			double scale = (newSampleRate / readTaps.getReference(i).sampleRateWhenCreated);
			readTaps.getReference(i).delaySamples *= scale;
			readTaps.getReference(i).sampleRateWhenCreated = newSampleRate;
		}
	}
}

float TappedDelayLine::processSingleSample(float newSample) throw()
{
	// incriment buffer position and store new sample
	if(++bufferWritePos > bufferSize)
		bufferWritePos = 0;
	float *bufferInput = &pfDelayBuffer[bufferWritePos];
	*bufferInput = 0;
	
	float fOut = (inputGain * newSample);
	for (int i = 0; i < noTaps; ++i)
	{
		const Tap currentTap = readTaps.getReference(i);

		int tapReadPos = bufferWritePos - currentTap.delaySamples;
		if (tapReadPos < 0)
			tapReadPos += bufferSize;
		
		float tapOutput = currentTap.tapGain * pfDelayBuffer[tapReadPos];
		fOut += tapOutput;
		*bufferInput += currentTap.tapFeedback * tapOutput;
	}
	
	*bufferInput += newSample;

	return fOut;
}

void TappedDelayLine::processSamples (float* const samples,
									  const int numSamples) throw()
{
    const ScopedLock sl (processLock);
	
	for (int i = 0; i < numSamples; ++i)
	{
		const float in = samples[i];
		
		// incriment buffer position and store new sample
		if(++bufferWritePos > bufferSize)
			bufferWritePos = 0;
		float *bufferInput = &pfDelayBuffer[bufferWritePos];
		*bufferInput = 0;
		
		float fOut = (inputGain * in);
		for (int t = 0; t < noTaps; ++t)
		{
			const Tap currentTap = readTaps.getReference(t);
			
			int tapReadPos = bufferWritePos - currentTap.delaySamples;
			if (tapReadPos < 0)
				tapReadPos += bufferSize;
			
			float tapOutput = currentTap.tapGain * pfDelayBuffer[tapReadPos];
			fOut += tapOutput;
			*bufferInput += currentTap.tapFeedback * tapOutput;
		}
		
		*bufferInput += in;
		
		samples[i] = fOut;
	}
}
