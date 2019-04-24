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



Clock::Clock()
{
	setTimeDisplayFormat (showTime + show24Hr);
}

Clock::~Clock()
{
	stopTimer();
}

int Clock::getRequiredWidth()
{
	return getFont().getStringWidth (timeAsString) + 10;
}

void Clock::setTimeDisplayFormat(const int newFormat)
{	
	displayFormat = newFormat;
	
	if ((displayFormat & showDayShort) && (displayFormat & showDayLong))
		displayFormat -= showDayShort;
	if ((displayFormat & showTime))
		startTimer (5900);
	if ((displayFormat & showSeconds))
		startTimer (950);
//	if ((displayFormat & showTenthSeconds))
//		startTimer(99);
	
	timerCallback();
}

void Clock::timerCallback()
{
	Time currentTime = Time::getCurrentTime();	
	timeAsString = String();
	
	String formatString;
	formatString    << ((displayFormat & showDayShort)  ? "%a " : "")
                    << ((displayFormat & showDayLong)   ? "%A " : "")
                    << ((displayFormat & showDate)      ? "%x " : "")
                    << ((displayFormat & showTime)      ? ((displayFormat & show24Hr) ? "%H:%M" : "%I:%M") : "")
                    << ((displayFormat & showSeconds)   ? ":%S " : "");
	
	if (formatString.isNotEmpty())
		timeAsString << currentTime.formatted (formatString);
	
	setText (timeAsString, dontSendNotification);
}

