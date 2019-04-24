/*
 *  dRowAudio_TappedDelayLine.h
 *
 *  Created by David Rowland on 13/04/2009.
 *  Copyright 2009 dRowAudio. All rights reserved.
 *
 */

#ifndef _DROWAUDIO_TAPPEDDELAYLINE_H_
#define _DROWAUDIO_TAPPEDDELAYLINE_H_

#include "includes.h"

// #include "../../utility/dRowAudio_Utility.h"
// #include "../dRowAudio_AudioUtility.h"

struct Tap
{
	int delaySamples;
	int originalDelaySamples;
	int sampleRateWhenCreated;
	float tapGain;
	float tapFeedback;
	float originalTapFeedback;
/*	float tapPan;
	float tapHPCutoff;
	float tapLPCutoff;
 */
};

class TappedDelayLine
{
public:
	/** Creates a TappedDelayline with a given size in samples.
		If no size is specified a default of 9600 is used.
	 */
	TappedDelayLine(int initialBufferSize =96000);
	TappedDelayLine(float bufferLengthMs, double sampleRate);
	
	/// Destructor
	~TappedDelayLine();
	
	/** Adds a tap to the delay line at a given number of samples.
		This will not make a note of the current sample rate being used
		unless you explecity specify it. Use
		addTap(int newTapPosMs, double sampleRate) if you need the delay
		to be dependant on time.
	 */
	void addTap(int noDelaySamples, int sampleRate =0);
	
	/** Adds a tap to the delay line at a given time.
		If the sample rate changes make sure you call updateDelayTimes()
		to recalculate the number of samples for the taps to delay by.
	 */
	void addTapAtTime(int newTapPosMs, double sampleRate);
	
	/** Moves a specific tap to a new delay position.
		This will return true if a tap was actually changed.
	 */
	bool setTapDelayTime(int tapIndex, int newTapPosMs, double sampleRate);

	/** Changes the number of samples a specific tap will delay.
		This will return true if a tap was actually changed.
	 */
	bool setTapDelaySamples(int tapIndex, int newDelaySamples);
	
	/**	Scales the spacing between the taps.
		This value must be greater than 0. Values < 1 will squash the taps, 
		creating a denser delay, values greater than 1 will expand the taps
		spacing creating a more sparse delay.
		The value is used as a proportion of the explicitly set delay time.
		This is simpler than manually setting all of the tap positions.
	 */
	void setTapSpacing(float newSpacingCoefficient);

	/**	This has the same effect as setTapSpacing() but does not check to
		see if the coeficient has changed so will always update the spacing.
	 */
	void setTapSpacingExplicitly(float newSpacingCoefficient);

	/**	Scales all of the taps feedback coeficients in one go.
		This should be between 0 and 1 to avoid blowing up the line.
		The value is a proportion of the explicitly set feedback coefficient
		for each tap so setting this to 1 will return them all to their default.
	 */
	void scaleFeedbacks(float newFeedbackCoefficient);

	/**	Returns an array of sample positions where there are taps.
		This can then be used to remove a specific tap.
	 */
	Array<int> getTapSamplePositions();
	
	/** Removes a tap at a specific index.
		Returns true if a tap was removed.
	 */
	bool removeTapAtIndex(int tapIndex);
	
	/** Removes a tap with a specific delay samples.
		Returns true if a tap was revoved, false otherwise.
	 */
	bool removeTapAtSample(int sampleForRemovedTap);
	
	/** Attempts to remove a tap at a specific time.
		Returns true if a tap was revoved, false otherwise.
	 */ 
	bool removeTapAtMs(int timeMsForRemovedTap, int sampleRate);
	
	/**	Removes all the taps.
	 */
	void removeAllTaps();
	
	/**	Updates the delay samples of all the taps based on their time.
		Call this if you change sample rates to make sure the taps are
		still positioned at the right time.
	 */
	void updateDelayTimes(double newSampleRate);
	
	/** Resizes the buffer to a given number of samples.
		This will return the number of taps that have been removed if
		they overrun the new buffer size.
	 */
	int setBufferSize(int noSamples);
	
	/** Resizes the buffer to a size given the time and sample rate.
		This will return the number of taps that have been removed if
		they overrun the new buffer size.
	 */
	int setBufferSize(int timeMs, double sampleRate);
	
	/// Returns the current size in samples of the buffer.
	int getBufferSize()	{	return bufferSize;	}
	
	/// Returns the current length in milliseconds of the buffer for a given sample rate.
	int getBufferLengthMs(double sampleRate)	{	return bufferSize * sampleRate;	}
	
	/// Returns the number of taps currently being used.
	int getNumberOfTaps()	{	return readTaps.size();	}
	
	/// Processes a single sample returning a new sample with summed delays.
	float processSingleSample(float newSample) throw();
	
	/// Processes a number of samples in one go.
	void processSamples(float* const samples,
						const int numSamples) throw();
	
private:
	
	CriticalSection processLock;
	
	float *pfDelayBuffer;
	int bufferSize, bufferWritePos;
	
	float inputGain, feedbackGain;
	int noTaps;
	Array<Tap> readTaps;
	
	float spacingCoefficient, feedbackCoefficient;
	
	void initialiseBuffer(int bufferSize);
	
	JUCE_LEAK_DETECTOR (TappedDelayLine);
};

#endif //_DROWAUDIO_TAPPEDDELAYLINE_H_