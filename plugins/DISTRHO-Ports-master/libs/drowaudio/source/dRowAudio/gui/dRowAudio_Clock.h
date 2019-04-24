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

#ifndef __DROWAUDIO_CLOCK_H__
#define __DROWAUDIO_CLOCK_H__

//==============================================================================
/**	A handy digital graphical clock.
	
	Just add one of these to your component and it will display the time,
	continually updating itself. Set the look and feel of it as you would
	a normal label.
 */
class Clock : public Label,
			  public Timer
{
public:
    //==============================================================================
	/**	A number of flags to set what sort of clock is displayed
	 */
	enum TimeDisplayFormat
	{
		showDate = 1,
		showTime = 2,
		showSeconds = 4,
		show24Hr = 8,
		showDayShort = 16,
		showDayLong = 32,
	};
	
    //==============================================================================
	/**	Constructor.
		Just add and make visible one of these to your component and it
		will display the current time and continually update itself.
	 */
	Clock();
	
	/**	Destructor.
	 */
	~Clock();

	/**	Sets the display format of the clock.
     
		To specify what sort of clock to display pass in a number of the
		TimeDisplayFormat flags. This is semi-inteligent so may choose to
		ignore certain flags such as the short day name if you have also
		specified the long day name.
	 */
	void setTimeDisplayFormat (const int newFormat);
	
	/**	Returns the width required to display all of the clock's information.
	 */
	int getRequiredWidth();
	
    //==============================================================================
	/** @internal */
	void timerCallback();
	
private:
    //==============================================================================
	int displayFormat;
	String timeAsString;
	
    //==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Clock);
};

#endif  // __DROWAUDIO_CLOCK_H__