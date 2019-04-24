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

#ifndef __DROWAUDIO_CPUMETER_H__
#define __DROWAUDIO_CPUMETER_H__

//==============================================================================
/** Handy class that will display the cpu usage of a given AudioDeviceManager
    as a percentage.
 */
class CpuMeter : public Label,
				 public Timer
{
public:
    //==============================================================================
	/**	Creates a CpuMeter.
		You need to provide the device manager to monitor and optionally the refresh
        rate of the display.
	 */
	CpuMeter (AudioDeviceManager* deviceManagerToUse, int updateIntervalMs = 50);
	
	/**	Descructor.
     */
	~CpuMeter();
	
	/** Returns the current cpu usage as a percentage.
     */
	double getCurrentCpuUsage() const
    {
        return currentCpuUsage;
    }
	
	/** Changes the colour of the text.
     */
	void setTextColour (const Colour& newTextColour)
    {
        setColour (Label::textColourId, newTextColour);
    }

	//==============================================================================
    /** @internal */
	void resized();

	/** @internal */
	void timerCallback();
	
private:
    //==============================================================================
	AudioDeviceManager* deviceManager;
	int updateInterval;
	double currentCpuUsage;
	
    //==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CpuMeter);
};

#endif	//__DROWAUDIO_CPUMETER_H__