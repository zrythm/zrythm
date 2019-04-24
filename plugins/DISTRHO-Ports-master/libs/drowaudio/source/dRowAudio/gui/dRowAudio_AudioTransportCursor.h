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

#ifndef __DROWAUDIO_AUDIOTRANSPORTCURSOR_H__
#define __DROWAUDIO_AUDIOTRANSPORTCURSOR_H__

#include "../utility/dRowAudio_StateVariable.h"
#include "../audio/dRowAudio_AudioUtility.h"

//====================================================================================
/** A class to display a scolling cursor to prepresent the position of an audio file.
	
	Clicking and dragging on the display will reposition the transport source.
 */
class AudioTransportCursor :    public Component,
                                public Timer,
                                public AudioFilePlayer::Listener
{
public:
	//====================================================================================
	/** Creates the display.
		The AudioFilePlayer associated with the display must be passed in.
	 */
	explicit AudioTransportCursor (AudioFilePlayer& sourceToBeUsed);
	
	/** Destructor.
     */
	~AudioTransportCursor();

    //====================================================================================
	/** Sets whether or not the transport cursor should be displayed;
     */
    void setCursorDisplayed (bool shouldDisplayCursor);
    
	/** Sets the current horizontal zoom.
        1.0 means the end of the audio file reaches the end of the component, 0.5 means
        the end of the component is halfway through the file etc.
     */
	void setZoomRatio (double newZoomRatio);
    
    /** Sets an offset used to start the waveform at a faction of the component's width.
        A value of 0.5 means the start of the component is halfway through the file etc.
     */
	void setStartOffsetRatio (double newStartOffsetRatio);
            
	//====================================================================================
	/** @internal */
    void fileChanged (AudioFilePlayer* player);
    
	/** @internal */
    void playerStoppedOrStarted (AudioFilePlayer* player);
    
	//====================================================================================
	/** @internal */
    void resized ();
	
	/** @internal */
	void paint (Graphics &g);

	/** @internal */
	void mouseDown (const MouseEvent &e);
	
	/** @internal */
	void mouseUp (const MouseEvent &e);
	
	/** @internal */
	void mouseDrag (const MouseEvent &e);
    
	//====================================================================================
	/** @internal */
	void timerCallback();
		
private:
	//==============================================================================
    AudioFilePlayer& audioFilePlayer;

	double fileLength, oneOverFileLength, currentSampleRate;
    double zoomRatio, startOffsetRatio;
    bool shouldStopTimer;
	
	Image cursorImage;
	
	StateVariable<int> transportLineXCoord;
	float currentXScale;
	bool showTransportCursor;
    
	bool interestedInDrag;
	double currentMouseX;

    //==============================================================================
    void refreshFromFilePlayer();
    void startTimerIfNeeded();
    void setPlayerPosition (int mousePosX, bool ignoreAnyLoopPoints);
    
	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioTransportCursor);
};

#endif // __DROWAUDIO_AUDIOTRANSPORTCURSOR_H__