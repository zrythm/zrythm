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

#ifndef __DROWAUDIO_SEGMENTEDMETER_H__
#define __DROWAUDIO_SEGMENTEDMETER_H__

#include "dRowAudio_GraphicalComponent.h"
#include "../utility/dRowAudio_StateVariable.h"

//==============================================================================
/**	A segmented graphical VU meter.

    This class is a very efficient way of creating meters as it will only repaint itself when
	necessarry and does all its processing on a shared background thread.
 
	It is very customisable letting you set the number of each segments, how many decibels
	each segment represents and the colours of the segments.
 
	To use one, register it with a TimeSliceThread and then in your audio callback
	push some values to it with copyValues(...).
 
	Eg. in your Component
	@code
		graphicalManager = new TimeSliceThread();
        graphicalManager->startThread (2);
		addAndMakeVisible (meter = new SegmentedMeter());
		graphicalManager->addTimeSliceClient (meter);
	@endcode
 
	and in your audioCallback
	@code
		if (currentMeter != nullptr) {
			currentMeter->copyValues (outputChannelData[0], numSamples);
		}
	@endcode
 
 */
class SegmentedMeter :	public GraphicalComponent
{
public:
    //==============================================================================
	/**	Creates a SegmentedMeter.
		Initially this will do nothing as you need to register it with a
        TimeSliceThread then push some values to it with copyValues().
	 */
	SegmentedMeter();
	
	/**	Destructor.
	 */
	~SegmentedMeter();
	
	/**	Calculates the number of segments currently required and triggers a repaint if necessary.
	 */
	void calculateSegments();
	
	/**	Returns the total number of segments the meter has.
	 */
	int getTotalNumSegments()           {	return totalNumSegs;	}
	
	/**	Returns the number of segments that represent the over level.
	 */
	int getNumOverSegments()            {   return numRedSeg;       }
    
	/**	Returns the total number of segments that represent the warning level.
	 */
	int	getNumWarningSegments()         {	return numYellowSeg;	}
    
	/**	Returns the total number of segments that represent the safe level.
	 */
	int	getNumSafeSegments()            {	return numGreenSeg;     }
	
	/**	Returns the number of decibels each segment represents.
	 */
	float getNumDbPerSegment()          {	return decibelsPerSeg;  }
    
	/**	Sets the number of decibels each segment represents.
	 */
	void setNumDecibelsPerSeg (float numDecibelsPerSegment)
	{
		decibelsPerSeg = numDecibelsPerSegment;
	}
	
	/**	Forces the meter to repaint itself.
        You may need to use this if a container component moves without moving
        or resizing its parent directly, eg. if you are housing your component
        in a tabbed component.
	 */
	void flagForRepaint()
	{	
		needsRepaint = true;
		repaint();
	}
    
	//==============================================================================
    /**	Draws the meter. */
	void paint (Graphics &g);
	
	/**	@internal
	 */
	void timerCallback();
	
	/**	@internal
	 */
	void resized();

	/**	@internal
	 */
	void moved()
	{
		needsRepaint = true;
	}
		
	/**	Processes the channel data for the value to display.
	 */
	virtual void process();
		
private:
	//==============================================================================
    int numRedSeg, numYellowSeg, numGreenSeg, totalNumSegs;
	float decibelsPerSeg;
	StateVariable<int> numSegs;
	int sampleCount, samplesToCount;
    float sampleMax;
	StateVariable<float> level;
	bool needsRepaint;
	
    Image onImage, offImage;
    
	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SegmentedMeter);
};

#endif //__DROWAUDIO_SEGMENTEDMETER_H__