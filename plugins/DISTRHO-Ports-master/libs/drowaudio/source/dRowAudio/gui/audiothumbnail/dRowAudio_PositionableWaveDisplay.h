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

#ifndef __DROWAUDIO_POSITIONALWAVEDISPLAY_H__
#define __DROWAUDIO_POSITIONALWAVEDISPLAY_H__

//====================================================================================
/**
    A class to display the entire waveform of an audio file.
	
	This will load an audio file and display its waveform. Clicking on the waveform will
	reposition the transport source.
 */
class PositionableWaveDisplay : public Component,
                                public AudioThumbnailImage::Listener,
                                public TimeSliceClient,
                                public AsyncUpdater
{
public:
	//====================================================================================
	/** Creates the display.
		The AudioThumbnailImage associated with the display must be passed in.
	 */
	explicit PositionableWaveDisplay (AudioThumbnailImage& sourceToBeUsed,
                                      TimeSliceThread& threadToUse);
	
	/** Destructor.
     */
	~PositionableWaveDisplay();

    //====================================================================================
	/** Sets whether or not the transport cursor should be displayed;
     */
    void setCursorDisplayed (bool shoudldDisplayCursor);
    
    /** Sets the colour to use for the background.
     */
    void setBackgroundColour (const Colour& newBackgroundColour);
    
    /** Sets the colour to use for the waveform.
     */
    void setWaveformColour (const Colour& newWaveformColour);
        
	/** Sets the current horizontal zoom.
        1.0 displays the whole waveform, 0.5 will show half etc. 
     */
	void setZoomRatio (double newZoomRatio);
    
    /** Sets an offset used to start the waveform at a faction of the display.
        A value of 0.5 will show the waveform starting at the halfway point etc.
     */
	void setStartOffsetRatio (double newStartOffsetRatio);
    
    /** Sets a new vertical zoom ratio.
        Values greater than 1.0 will expand the waveform vertically, less will contract it.
     */
    void setVerticalZoomRatio (double newVerticalZoomRatio);
        
	//====================================================================================
	/** @internal */
    void imageChanged (AudioThumbnailImage* audioThumbnailImage);

	//====================================================================================
	/** @internal */
    void resized ();
	
	/** @internal */
	void paint (Graphics &g);
    
	/** @internal */
    int useTimeSlice();

    /** @internal */
    void handleAsyncUpdate();
    
private:
	//==============================================================================
    AudioThumbnailImage& audioThumbnailImage;
    TimeSliceThread& threadToUse;
    CriticalSection imageLock;

    AudioFilePlayer& audioFilePlayer;
	double fileLength, oneOverFileLength, currentSampleRate;
    double zoomRatio, startOffsetRatio, verticalZoomRatio;
	
    Colour backgroundColour, waveformColour;
	Image cachedImage, cursorImage;
	
    StateVariable<double> drawTimes;
    
    AudioTransportCursor audioTransportCursor;

    //==============================================================================
    void refreshCachedImage();
    
	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PositionableWaveDisplay);
};


#endif //__DROWAUDIO_POSITIONALWAVEDISPLAY_H__
