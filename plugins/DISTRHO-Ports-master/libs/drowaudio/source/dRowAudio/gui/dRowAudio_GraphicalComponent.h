/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/

#ifndef __DROWAUDIO_GRAPHICALCOMPONENT_H__
#define __DROWAUDIO_GRAPHICALCOMPONENT_H__

//==============================================================================
/**	This class is an abstract base blass for some kind of graphical component
	that requires some intenisve processing.
	Inherit your class from this then register it with a TimeSliceThread
	to continually call the process() method where you can do your required
	processing on a background thread to avoid blocking the Message thread for too long.
 
	@see SegmentedMeter
 */
class GraphicalComponent :	public Component,
							public TimeSliceClient,
							public Timer
{
protected:
    //==============================================================================
	/**	Creates a GraphicalComponent.
		Don't instantiate directly, use as a base class.
	 */
	GraphicalComponent();
	
public:
    //==============================================================================
	/**	Destructor.
	 */
	~GraphicalComponent();

	/**	Overload to do your processing.
		Once registered with a GraphicalComponentManager this will repeatedly get called.
		To save CPU cycles this will only get called if paused is false and some new data
		has been set with copySamples(). The idea is that you push some new data to your
		class with copySamples() then do whatever processing you require here.
	 */
	virtual void process() = 0;
	
	/**	Pauses the processing of the GraphicalComponent.
	 */
	void pause (bool shouldPause)	{	paused = shouldPause;	}
	
	/**	Returns true if the processing is currently suspended.
	 */
	bool isPaused()					{	return paused;          }
	
    //==============================================================================
	/** Copies data to the component to use.
		This should be as quick as possible as is accessed from what
		ever thread calls it so could cause blocking.
		By default this just copys the values passed to it into the samples heap block,
		extending the memory if needed. You can overide this for more specialised behaviour.
	 */
	virtual void copySamples (const float* values, int numSamples);

	/** Copies data from a number of channels to the component to use.
		This is a lot slower than copySamples(float *values, int numSamples) but if the
		number of channels is 2 it will use the maximum sample from the pair of channels.
	 */
	virtual void copySamples (float** values, int numSamples, int numChannels);
	
	/** @internal */
	int useTimeSlice();
	
	/** @internal */
	void timerCallback() {}

protected:
    //==============================================================================
	CriticalSection lock;
	bool paused;
	bool needToProcess;
	
	int sleepTime, numSamples;
	HeapBlock<float> samples;
	
    //==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphicalComponent);
};

#endif  // __DROWAUDIO_GRAPHICALCOMPONENT_H__