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

PositionableWaveDisplay::PositionableWaveDisplay (AudioThumbnailImage& sourceToBeUsed,
                                                  TimeSliceThread& threadToUse_)
    : audioThumbnailImage   (sourceToBeUsed),
      threadToUse           (threadToUse_),
      audioFilePlayer       (audioThumbnailImage.getAudioFilePlayer()),
      currentSampleRate     (44100.0),
      zoomRatio             (1.0),
      startOffsetRatio      (0.0),
      verticalZoomRatio     (1.0),
      backgroundColour      (Colours::black),
      waveformColour        (Colours::green),
      audioTransportCursor  (audioFilePlayer)
{
    setOpaque (true);
    
    audioThumbnailImage.addListener (this);
    
    cachedImage = Image (Image::RGB, 1, 1, false);
    cachedImage.clear (cachedImage.getBounds(), backgroundColour);
    
    addAndMakeVisible (&audioTransportCursor);
}

PositionableWaveDisplay::~PositionableWaveDisplay()
{
    threadToUse.removeTimeSliceClient (this);
    
    audioThumbnailImage.removeListener (this);
}

void PositionableWaveDisplay::setZoomRatio (double newZoomRatio)
{
    jassert (newZoomRatio >= 0.000001 && newZoomRatio < 10000.0);

    zoomRatio = jlimit (0.000001, 10000.0, newZoomRatio);
    audioTransportCursor.setZoomRatio (newZoomRatio);
    
    resized();
}

void PositionableWaveDisplay::setStartOffsetRatio (double newStartOffsetRatio)
{
    startOffsetRatio = newStartOffsetRatio;
    audioTransportCursor.setStartOffsetRatio (startOffsetRatio);
    repaint();
}

void PositionableWaveDisplay::setVerticalZoomRatio (double newVerticalZoomRatio)
{
    verticalZoomRatio = newVerticalZoomRatio;
    repaint();
}

void PositionableWaveDisplay::setCursorDisplayed (bool shoudldDisplayCursor)
{
    audioTransportCursor.setCursorDisplayed (shoudldDisplayCursor);
}

void PositionableWaveDisplay::setBackgroundColour (const Colour& newBackgroundColour)
{
    backgroundColour = newBackgroundColour;
    audioThumbnailImage.setBackgroundColour (backgroundColour);
    repaint();
}

void PositionableWaveDisplay::setWaveformColour (const Colour& newWaveformColour)
{
    waveformColour = newWaveformColour;
    audioThumbnailImage.setWaveformColour (waveformColour);
    repaint();
}

//====================================================================================
void PositionableWaveDisplay::resized()
{
    const ScopedLock sl (imageLock);
    
    cachedImage = Image (Image::RGB, jmax (1, int (getWidth() / zoomRatio)), jmax (1, getHeight()), false);
    cachedImage.clear (cachedImage.getBounds(), backgroundColour);

    refreshCachedImage();
    
    audioTransportCursor.setBounds (getLocalBounds());
}

void PositionableWaveDisplay::paint(Graphics &g)
{
	const int w = getWidth();
	const int h = getHeight();

    g.setColour (backgroundColour);	
    g.fillAll();
        
    const int newWidth = roundToInt (w / zoomRatio);
    const int startPixelX = roundToInt (w * startOffsetRatio);
    const int newHeight = roundToInt (verticalZoomRatio * h);
    const int startPixelY = roundToInt ((h * 0.5f) - (newHeight * 0.5f));

    const ScopedLock sl (imageLock);
    g.drawImage (cachedImage,
                 startPixelX, startPixelY, newWidth, newHeight,
                 0, 0, cachedImage.getWidth(), cachedImage.getHeight(), 
                 false);
}

//====================================================================================
void PositionableWaveDisplay::imageChanged (AudioThumbnailImage* changedAudioThumbnailImage)
{
	if (changedAudioThumbnailImage == &audioThumbnailImage)
	{
        {
            const ScopedLock sl (imageLock);
            cachedImage.clear (cachedImage.getBounds(), backgroundColour);
            triggerAsyncUpdate();
        }
        
        AudioFormatReaderSource* readerSource = audioFilePlayer.getAudioFormatReaderSource();
        
        AudioFormatReader* reader = nullptr;
        if (readerSource != nullptr)
            reader = readerSource->getAudioFormatReader();
        
        if (reader != nullptr && reader->sampleRate > 0.0
            && audioFilePlayer.getLengthInSeconds() > 0.0)
        {
            currentSampleRate = reader->sampleRate;
            fileLength = audioFilePlayer.getLengthInSeconds();
            
            if (fileLength > 0.0)
                oneOverFileLength = 1.0 / fileLength;

            refreshCachedImage();
        }
        else 
        {
            currentSampleRate = 44100;
            fileLength = 0.0;
            oneOverFileLength = 1.0;
        }
	}
}

int PositionableWaveDisplay::useTimeSlice()
{
    const ScopedLock sl (imageLock);

    drawTimes = audioThumbnailImage.getTimeRendered();

    if (! drawTimes.areEqual())
    {
        cachedImage = audioThumbnailImage.getImage()
                       .rescaled (cachedImage.getWidth(), cachedImage.getHeight());
        
        triggerAsyncUpdate();
    }
    
    if (audioThumbnailImage.hasFinishedLoading())
        threadToUse.removeTimeSliceClient (this);
    
    return 100;
}

void PositionableWaveDisplay::handleAsyncUpdate()
{
    repaint();
}

//====================================================================================
void PositionableWaveDisplay::refreshCachedImage()
{
    drawTimes.setBoth (0.0);
    threadToUse.addTimeSliceClient (this);
}
