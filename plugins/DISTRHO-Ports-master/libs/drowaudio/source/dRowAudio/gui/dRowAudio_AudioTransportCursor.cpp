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



AudioTransportCursor::AudioTransportCursor (AudioFilePlayer& sourceToBeUsed)
    : audioFilePlayer       (sourceToBeUsed),
      currentSampleRate     (44100.0),
      zoomRatio             (1.0),
      startOffsetRatio      (0.0),
      shouldStopTimer       (false),
      showTransportCursor   (true)
{
    refreshFromFilePlayer();
    audioFilePlayer.addListener (this);
}

AudioTransportCursor::~AudioTransportCursor()
{
    audioFilePlayer.removeListener (this);
	stopTimer();
}

void AudioTransportCursor::setZoomRatio (double newZoomRatio)
{
    if (newZoomRatio < 0.0 || newZoomRatio > 10000.0)
    {
        jassertfalse; // zoom ratio has to be > 0 && < 10000
        newZoomRatio = 1.0;
    }

    zoomRatio = newZoomRatio;
    
    repaint();
}

void AudioTransportCursor::setStartOffsetRatio (double newStartOffsetRatio)
{
    startOffsetRatio = newStartOffsetRatio;
    
    repaint();
}

void AudioTransportCursor::setCursorDisplayed (bool shouldDisplayCursor)
{
    showTransportCursor = shouldDisplayCursor;
    
    startTimerIfNeeded();
}

//====================================================================================
void AudioTransportCursor::resized()
{
    cursorImage = Image (Image::RGB, 3, jmax (1, getHeight()), true);
    Graphics g (cursorImage);
    g.fillAll (Colours::black);
    g.setColour (Colours::white);
	g.drawVerticalLine (1, 0.0f, (float) cursorImage.getHeight());
}

void AudioTransportCursor::paint (Graphics &g)
{
    if (showTransportCursor)
        g.drawImageAt (cursorImage, transportLineXCoord.getCurrent() - 1, 0);
}

//====================================================================================
void AudioTransportCursor::timerCallback()
{
    const int w = getWidth();
    const int h = getHeight();

    const int startPixel = roundToInt (w * startOffsetRatio);
    transportLineXCoord = startPixel + roundToInt ((w * oneOverFileLength * audioFilePlayer.getCurrentPosition()) / zoomRatio);

    // if the line has moved repaint the old and new positions of it
    if (! transportLineXCoord.areEqual())
    {
        repaint (transportLineXCoord.getPrevious() - 2, 0, 5, h);
        repaint (transportLineXCoord.getCurrent() - 2, 0, 5, h);
	}
    
    if (shouldStopTimer)
    {
        shouldStopTimer = false;
        stopTimer();
    }
}

//====================================================================================
void AudioTransportCursor::fileChanged (AudioFilePlayer* player)
{
	if (player == &audioFilePlayer)
        refreshFromFilePlayer();
}

void AudioTransportCursor::playerStoppedOrStarted (AudioFilePlayer* player)
{
	if (player == &audioFilePlayer)
        startTimerIfNeeded();
}

//==============================================================================
void AudioTransportCursor::mouseDown (const MouseEvent &e)
{
    if (showTransportCursor)
    {
        setMouseCursor (MouseCursor::IBeamCursor);

        currentXScale = (float) ((fileLength / getWidth()) * zoomRatio);
        setPlayerPosition (e.x, true);

        shouldStopTimer = false;
        startTimer (40);
        
        repaint();
    }
}

void AudioTransportCursor::mouseUp (const MouseEvent& /*e*/)
{
    if (showTransportCursor)
    {
        setMouseCursor (MouseCursor::NormalCursor);
        
        startTimerIfNeeded();
    }
}

void AudioTransportCursor::mouseDrag (const MouseEvent& e)
{
    if (showTransportCursor)
        setPlayerPosition (e.x, false);
}

//==============================================================================
void AudioTransportCursor::refreshFromFilePlayer()
{
    AudioFormatReaderSource* readerSource = audioFilePlayer.getAudioFormatReaderSource();
    
    AudioFormatReader* reader = nullptr;
    if (readerSource != nullptr)
        reader = readerSource->getAudioFormatReader();
    
    if (reader != nullptr && reader->sampleRate > 0.0
        && audioFilePlayer.getLengthInSeconds() > 0.0)
    {
        currentSampleRate = reader->sampleRate;
        fileLength = audioFilePlayer.getLengthInSeconds();
        oneOverFileLength = 1.0 / fileLength;
    }
    else
    {
        currentSampleRate = 44100;
        fileLength = 0.0;
        oneOverFileLength = 1.0;
    }
    
    startTimerIfNeeded();
}

void AudioTransportCursor::startTimerIfNeeded()
{
    if (showTransportCursor)
    {
        shouldStopTimer = false;
        startTimer (40);
    }
    else
    {
        shouldStopTimer = true;
    }
}

void AudioTransportCursor::setPlayerPosition (int mousePosX, bool ignoreAnyLoopPoints)
{
    const int startPixel = roundToInt (getWidth() * startOffsetRatio);
    double position = currentXScale * (mousePosX - startPixel);
    audioFilePlayer.setPosition (position, ignoreAnyLoopPoints);
}
