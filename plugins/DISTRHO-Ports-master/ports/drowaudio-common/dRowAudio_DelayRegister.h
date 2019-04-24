/*
 *  dRowAudio_DelayRegister.h
 *
 *  Created by David Rowland on 06/04/2009.
 *  Copyright 2009 dRowAudio. All rights reserved.
 *
 */

#ifndef _DROWAUDIODELAYREGISTER_H_
#define _DROWAUDIODELAYREGISTER_H_

#include "includes.h"

/**
	This is a simple delay register.
	It will delay any input by a set number of samples.
 */
class DelayRegister
{
public:
	
	/** Creates a default DelayRegister.
		You can optionally set the size of the buffer in samples.
	 */
	DelayRegister(int bufferSize =4096) throw();

	///	Destructor.
	~DelayRegister() throw();

	/**	Resizes the buffer to a specified sample size.
	 
		Please note that as the buffer size has to be a power of 2, the maximum delay time
		possible may be larger than that specified. Use getMaxDelayTime() to find the
		actual maximum possible.
	 */
	void setBufferSize (int newBufferSize) throw();
	
	/// Returns the current size of the buffer.
	int getBufferSize ()	{	return registerSize;	}
	
	/**	Resizes the buffer to a specified size based on a given sample rate and delay time in Milliseconds.
		
		Please note that as the buffer size has to be a power of 2, the maximum delay time
		possible may be larger than that specified. Use getMaxDelayTime() to find the
		actual maximum possible.
	 */
	void setMaxDelayTime(double sampleRate, float maxDelayTimeMs) throw();
	
	/** Returns the maximum delay time possible for a given sample rate.
	 */
	float getMaxDelayTime(double sampleRate) {	return (registerSize / sampleRate) * 1000.0f;	}
	
	/**	Sets the delay time for the filter to use.
	 
		As the delay buffer is interpolated from the delay time can be
		any positive number as long as its within the buffer size.
		This parameter can also be swept for modulating effects such as
		chorus or flange.
	 */
	void setDelayTime(double sampleRate, float newDelayTime) throw();
		
	/// Processes a single sample and returns a new, delayed value.
	float processSingleSample(float newSample) throw();
	
	/// Processes an array of samples which are modified.
	void processSamples (float* const samples,
						 const int numSamples) throw();

	
private:

	CriticalSection processLock;

	float* delayRegister;
	int registerSize, registerSizeMask;
	float delayTime, delaySamples;
	int bufferWritePos, bufferReadPos;
	
	JUCE_LEAK_DETECTOR (DelayRegister);
};

#endif //_DROWAUDIODELAYREGISTER_H_