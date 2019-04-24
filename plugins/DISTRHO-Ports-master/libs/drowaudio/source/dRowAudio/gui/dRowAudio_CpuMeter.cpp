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



CpuMeter::CpuMeter (AudioDeviceManager* deviceManagerToUse, int updateIntervalMs)
	: Label ("CpuMeter", "00.00%"),
	  deviceManager (deviceManagerToUse),
	  updateInterval (updateIntervalMs),
	  currentCpuUsage (0.0)
{
	if (deviceManagerToUse != nullptr)
		startTimer (updateInterval);
}

CpuMeter::~CpuMeter()
{
}

void CpuMeter::resized()
{
    const int w = getWidth();
    const int h = getHeight();

	setFont ((h < (w * 0.24f) ? h : w * 0.24f));
}

void CpuMeter::timerCallback()
{
	currentCpuUsage = (deviceManager->getCpuUsage() * 100.0);
	String displayString (currentCpuUsage, 2);
	displayString << "%";
    
	setText (displayString, dontSendNotification);
}

