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

DraggableWaveDisplay::DraggableWaveDisplay (AudioThumbnailImage& sourceToBeUsed)
    : audioThumbnailImage           (sourceToBeUsed),
	  filePlayer                    (sourceToBeUsed.getAudioFilePlayer()),
	  currentSampleRate             (44100.0),
      oneOverSampleRate             (1.0 / currentSampleRate),
	  timePerPixel                  (1.0),
	  zoomRatio                     (1.0f),
      oneOverZoomRatio              (1.0f / zoomRatio),
      playheadPos                   (0.5f),
	  isDraggable                   (true),
	  mouseShouldTogglePlay         (true)
{
    setOpaque (true);

    audioThumbnailImage.addListener (this);
    
	timePerPixel = audioThumbnailImage.getNumSourceSamplesPerThumbnailSamples() * oneOverSampleRate;
}

DraggableWaveDisplay::~DraggableWaveDisplay()
{
    audioThumbnailImage.removeListener (this);
	stopTimer (waveformUpdated);
}

//====================================================================================
void DraggableWaveDisplay::setHorizontalZoom (float newZoomFactor)
{
	jassert (newZoomFactor > 0.0f);
	
	zoomRatio = newZoomFactor;
    oneOverZoomRatio = 1.0f / zoomRatio;
    
    repaint();
}

void DraggableWaveDisplay::setPlayheadPosition (float newPlayheadPosition)
{
	playheadPos = jlimit (0.0f, 1.0f, newPlayheadPosition);
    
    repaint();
}

void DraggableWaveDisplay::setDraggable (bool isWaveformDraggable)
{
	isDraggable = isWaveformDraggable;
}

//====================================================================================
void DraggableWaveDisplay::resized()
{
    // redraw playhead image
    playheadImage = Image (Image::RGB, 3, jmax (1, getHeight()), false);

    Graphics g (playheadImage);
    g.fillAll (Colours::black);
    g.setColour (Colours::white);
    g.drawVerticalLine (1, 0.0f, (float) playheadImage.getHeight());
}

void DraggableWaveDisplay::paint (Graphics &g)
{
	const int w = getWidth();
	const int h = getHeight();
	
	g.fillAll (Colours::darkgrey);
	    
    const int playHeadXPos = roundToInt (playheadPos * w);
    const double timeToPlayHead = pixelsToTime (playHeadXPos);
    const double startTime = filePlayer.getAudioTransportSource()->getCurrentPosition() - timeToPlayHead;
    const double duration = filePlayer.getAudioTransportSource()->getLengthInSeconds();
    const double timeToDisplay = pixelsToTime (w);

    const Image clippedImage (audioThumbnailImage.getImageAtTime (startTime, timeToDisplay));

    int padLeft = 0, padRight = 0;
    if (startTime < 0.0)
        padLeft = roundToInt (timeToPixels (fabs (startTime)));
    if ((startTime + timeToDisplay) > duration)
        padRight = roundToInt (timeToPixels (fabs (duration - (startTime + timeToDisplay))));

    g.drawImage (clippedImage,
                 padLeft, 0, w - padLeft - padRight, h,
                 0, 0, clippedImage.getWidth(), clippedImage.getHeight(),
                 false);
    
    g.drawImageAt (playheadImage, playHeadXPos - 1, 0);
}

void DraggableWaveDisplay::mouseDown (const MouseEvent &e)
{
	mouseX.setBoth (e.x);
	isMouseDown = true;
	
	if (isDraggable)
	{
		if (mouseShouldTogglePlay)
		{
			if (filePlayer.getAudioTransportSource()->isPlaying())
			{
				shouldBePlaying = true;
				filePlayer.getAudioTransportSource()->stop();
			}
			else
            {
				shouldBePlaying = false;
            }
		}
        
		setMouseCursor (MouseCursor::DraggingHandCursor);
		
		startTimer (waveformMoved, 40);
	}
}

void DraggableWaveDisplay::mouseUp (const MouseEvent& /*e*/)
{
	isMouseDown = false;
	
	if (isDraggable)
	{
		if (mouseShouldTogglePlay)
		{
			if (shouldBePlaying && ! filePlayer.getAudioTransportSource()->isPlaying())
				filePlayer.getAudioTransportSource()->start();
			else if (! shouldBePlaying && filePlayer.getAudioTransportSource()->isPlaying())
				filePlayer.getAudioTransportSource()->stop();
		}
		
		setMouseCursor (MouseCursor::NormalCursor);
		
		stopTimer (waveformMoved);
	}
}

//====================================================================================
void DraggableWaveDisplay::timerCallback (const int timerId)
{
	if (timerId == waveformUpdated) //moved due to file position changing
	{
		movedX = roundToInt (timeToPixels (filePlayer.getAudioTransportSource()->getCurrentPosition()));

		if (! movedX.areEqual())
			repaint();
	}
	else if (timerId == waveformMoved) // being dragged
	{
		if (isMouseDown)
		{
			mouseX = getMouseXYRelative().getX();
			const int currentXDrag = mouseX.getDifference();
			
			if (currentXDrag != 0)
			{
				const double position = filePlayer.getAudioTransportSource()->getCurrentPosition() - pixelsToTime (currentXDrag);
				filePlayer.getAudioTransportSource()->setPosition (position);

				repaint();
			}
		}
	}
    else if (timerId == waveformLoading)
    {
        const int w = getWidth();
        const int playHeadXPos = roundToInt (playheadPos * w);
        const double timeToPlayHead = pixelsToTime (playHeadXPos);
        const double startTime = filePlayer.getAudioTransportSource()->getCurrentPosition() - timeToPlayHead;
        const double timeToDisplay = pixelsToTime (w);
        
        const double timeAtEnd = startTime + timeToDisplay;
        
        if (audioThumbnailImage.getTimeRendered() <= timeAtEnd)
            repaint();
        
        if (audioThumbnailImage.hasFinishedLoading())
        {
            repaint();
            stopTimer (waveformLoading);
        }
    }
}

void DraggableWaveDisplay::imageChanged (AudioThumbnailImage* changedAudioThumbnailImage)
{
	if (changedAudioThumbnailImage == &audioThumbnailImage)
	{
        bool sourceLoaded = false;
        
        if (filePlayer.getAudioFormatReaderSource() != nullptr
            && filePlayer.getAudioFormatReaderSource()->getAudioFormatReader() != nullptr)
        {
            currentSampleRate = filePlayer.getAudioFormatReaderSource()->getAudioFormatReader()->sampleRate;
        
            if (currentSampleRate > 0.0)
            {
                const double fileLength = filePlayer.getAudioTransportSource()->getLengthInSeconds();
                
                if (fileLength > 0.0)
                {
                    sourceLoaded = true;

                    oneOverFileLength = 1.0 / fileLength;
                    oneOverSampleRate = 1.0 / currentSampleRate;
                    timePerPixel = audioThumbnailImage.getNumSourceSamplesPerThumbnailSamples() * oneOverSampleRate;

                    startTimer (waveformUpdated, 15);
                    startTimer (waveformLoading, 40);
                }
            }
        }
        
        if (! sourceLoaded)
        {
            stopTimer (waveformUpdated);
            stopTimer (waveformLoading);
        }
	}
}

//==============================================================================	
double DraggableWaveDisplay::pixelsToTime (double numPixels)
{
	return numPixels * timePerPixel * oneOverZoomRatio;
}

double DraggableWaveDisplay::timeToPixels (double timeInSecs)
{
	return timeInSecs / (timePerPixel * oneOverZoomRatio);
}
