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

#ifndef __DROWAUDIO_DRAGGABLEWAVEDISPLAY_H__
#define __DROWAUDIO_DRAGGABLEWAVEDISPLAY_H__

//==============================================================================
/** A class to display the waveform of an audio file which can be dragged to
    reposition the source.
    This class takes an AudioThumbnailImage as its source an then will get
    updated as the thumbnail is generated. You can set the zoom levels of the
    waveform and drag the image to reposition the source.
*/
class DraggableWaveDisplay :    public Component,
                                public MultiTimer,
                                public AudioThumbnailImage::Listener
{
public:
    //==============================================================================
	/** Creates the display.
        The file player associated with the display must be passed in along with
        the current sample rate. This can later be changed with setSampleRate.
	 */
	explicit DraggableWaveDisplay (AudioThumbnailImage& sourceToBeUsed);
	
	/** Destructor.
		Your subclass will need to call signalThreadShouldExit() in its destructor as
		it will get destructed before this superclass.
	 */
	~DraggableWaveDisplay();
	
    //====================================================================================
	/** Sets the current horizontal zoom.
        This must be greater than 0 and the larger the number the more zoomed in the wave will be.
        A value of 1 is the waveform at its rendered resolution so any larger and blocking may occur.
	 */
	void setHorizontalZoom (float newZoomFactor);
	
	/** Sets the offset of the white line that marks the current position.
        This is as a fraction of the width of the display.
	 */
	void setPlayheadPosition (float newPlayheadPosition);
	
	/** Turns dragging to reposition the transport on or off.
     */
	void setDraggable (bool isWaveformDraggable);
	
	/** Returns true if dragging the waveform will reposition the audio source 
     */
	bool getDraggable()              {   return isDraggable;   }
	    
    //====================================================================================
	/** @internal */
    void imageChanged (AudioThumbnailImage* audioThumbnailImage);

	//====================================================================================
	/** @internal */
	void resized();
	
	/** @internal */
	void paint (Graphics &g);
	
	/** @internal */
    void mouseDown (const MouseEvent &e);
	
	/** @internal */
	void mouseUp (const MouseEvent &e);
    
	//====================================================================================
    /** @internal. */
	void timerCallback (int timerId);
				
private:
	//==============================================================================	
    /** Converts a number of pixels to a time at the current resolution. 
     */
    inline double pixelsToTime (double numPixels);

    /** Converts a time to a number of pixels at the current resolution. 
     */
	inline double timeToPixels (double timeInSecs);

    /** Used to start and stop the various internal timers.
     */
	enum
	{
		waveformUpdated,
		waveformMoved,
        waveformLoading
	};
    
    //==============================================================================	
    AudioThumbnailImage& audioThumbnailImage;
    AudioFilePlayer& filePlayer;
	double fileLengthSecs, oneOverFileLength;
    double currentSampleRate, oneOverSampleRate;
    double timePerPixel;
	float zoomRatio, oneOverZoomRatio;
	float playheadPos;
	
	bool waveformIsFullyLoaded;
	
    CriticalSection lock;
    Image playheadImage;

	bool isMouseDown, isDraggable, shouldBePlaying, mouseShouldTogglePlay;
	StateVariable<int> mouseX, movedX;
    
	friend class SwitchableDraggableWaveDisplay;
    
    //==============================================================================	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggableWaveDisplay);
};


#endif  // __DROWAUDIO_DRAGGABLEWAVEDISPLAY_H__