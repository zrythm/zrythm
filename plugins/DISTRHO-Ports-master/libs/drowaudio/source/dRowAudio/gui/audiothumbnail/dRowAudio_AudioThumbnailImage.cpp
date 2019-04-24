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

AudioThumbnailImage::AudioThumbnailImage (AudioFilePlayer& sourceToBeUsed,
                                          TimeSliceThread& backgroundThread_,
                                          AudioThumbnailBase& thumbnailToUse,
                                          int sourceSamplesPerThumbnailSample_)
    : filePlayer                        (sourceToBeUsed),
      backgroundThread                  (backgroundThread_),
      audioThumbnail                    (thumbnailToUse),
      sourceSamplesPerThumbnailSample   (sourceSamplesPerThumbnailSample_),
      backgroundColour                  (Colours::black),
      waveformColour                    (Colours::green),
      sourceLoaded                      (false),
      renderComplete                    (true),
	  currentSampleRate                 (44100.0),
      oneOverSampleRate                 (1.0 / currentSampleRate),
      lastTimeDrawn                     (0.0),
      resolution                        (3.0)
{
    waveformImage = Image (Image::RGB, 1, 1, false);
    refreshFromFilePlayer();
    
	// register with the file player to recieve update messages
	filePlayer.addListener (this);
}

AudioThumbnailImage::~AudioThumbnailImage()
{
	filePlayer.removeListener (this);
    backgroundThread.removeTimeSliceClient (this);
	
    stopTimer();
}

void AudioThumbnailImage::setBackgroundColour (const Colour& newBackgroundColour)
{
    backgroundColour = newBackgroundColour;
    triggerWaveformRefresh();
}

void AudioThumbnailImage::setWaveformColour (const Colour& newWaveformColour)
{
    waveformColour = newWaveformColour;
    triggerWaveformRefresh();
}

void AudioThumbnailImage::setResolution (double newResolution)
{
    resolution = newResolution;
    triggerWaveformRefresh();
}

//====================================================================================
const Image AudioThumbnailImage::getImageAtTime (double startTime, double duration)
{
    if (sourceLoaded)
    {
        const int startPixel = roundToInt (startTime * oneOverFileLength * waveformImage.getWidth());
        const int numPixels = roundToInt (duration * oneOverFileLength * waveformImage.getWidth());
        
        return waveformImage.getClippedImage (Rectangle<int> (startPixel, 0, numPixels, waveformImage.getHeight()));
    }
    else
    {
        return Image();
    }
}

//====================================================================================
void AudioThumbnailImage::timerCallback()
{
    const ScopedReadLock sl (imageLock);

    listeners.call (&Listener::imageUpdated, this);

    if (renderComplete)
    {
        listeners.call (&Listener::imageFinished, this);

        stopTimer();
    }
}

int AudioThumbnailImage::useTimeSlice()
{
    refreshWaveform();
    
    return 25;
}

void AudioThumbnailImage::fileChanged (AudioFilePlayer* player)
{
    if (player == &filePlayer)
        refreshFromFilePlayer();
}

//==============================================================================
void AudioThumbnailImage::addListener (AudioThumbnailImage::Listener* const listener)
{
    listeners.add (listener);
}

void AudioThumbnailImage::removeListener (AudioThumbnailImage::Listener* const listener)
{
    listeners.remove (listener);
}

//==============================================================================
void AudioThumbnailImage::refreshFromFilePlayer()
{
    sourceLoaded = false;
    
    if (filePlayer.getAudioFormatReaderSource() != nullptr)
    {
        currentSampleRate = filePlayer.getAudioFormatReaderSource()->getAudioFormatReader()->sampleRate;
        
        if (currentSampleRate > 0.0)
        {
            oneOverSampleRate = 1.0 / currentSampleRate;
            fileLength = filePlayer.getLengthInSeconds();
            
            if (fileLength > 0)
            {
                oneOverFileLength = 1.0 / fileLength;
                
                const ScopedWriteLock sl (imageLock);
                
                const int imageWidth = roundToInt (filePlayer.getTotalLength() / sourceSamplesPerThumbnailSample);
                waveformImage = Image (Image::RGB, jmax (1, imageWidth), 100, true);
                // image will be cleared in triggerWaveformRefresh()
                
                const File newFile (filePlayer.getFile());
                
                if (newFile.existsAsFile())
                {
                    audioThumbnail.setSource (new FileInputSource (newFile));
                    sourceLoaded = true;
                }
                else if (filePlayer.getInputType() == AudioFilePlayer::memoryInputStream
                         || filePlayer.getInputType() == AudioFilePlayer::memoryBlock)
                {
                    MemoryInputStream* memoryInputStream = dynamic_cast<MemoryInputStream*> (filePlayer.getInputStream());

                    if (memoryInputStream != nullptr)
                    {
                        audioThumbnail.setSource (new MemoryInputSource (memoryInputStream));
                        sourceLoaded = true;
                    }
                }
            }
        }
    }
    
    if (sourceLoaded)
    {
        renderComplete = false;
    }
    else
    {
        audioThumbnail.setSource (nullptr);
        renderComplete = true;
    }
    
    triggerWaveformRefresh();
}

void AudioThumbnailImage::triggerWaveformRefresh()
{
    // we need to remove ourselves from the thread first so we don't
    // end up drawing into the middle of the image
    backgroundThread.removeTimeSliceClient (this);

    {
        const ScopedWriteLock sl (imageLock);

        lastTimeDrawn = 0.0;
        waveformImage.clear (waveformImage.getBounds(), backgroundColour);
        renderComplete = false;
    }
    
    listeners.call (&Listener::imageChanged, this);
    
    if (sourceLoaded)
        backgroundThread.addTimeSliceClient (this);

    timerCallback();
    startTimer (100);
}

void AudioThumbnailImage::refreshWaveform()
{
	if (sourceLoaded && audioThumbnail.getNumSamplesFinished() > 0)
	{
        const double timeRendered = audioThumbnail.getNumSamplesFinished() * oneOverSampleRate;
        
        const double timeToDraw = jmin (1.0, timeRendered - lastTimeDrawn);
        const double endTime = lastTimeDrawn + timeToDraw;
        const int64 endSamples = roundToInt (endTime * currentSampleRate);
        
        imageLock.enterRead();
        const int waveformImageWidth = waveformImage.getWidth();
        const int waveformImageHeight = waveformImage.getHeight();
        imageLock.exitRead();
        
        const int nextPixel = roundToInt (endTime * oneOverFileLength * waveformImageWidth);
        const int startPixelX = roundToInt (lastTimeDrawn * oneOverFileLength * waveformImageWidth);
        const int numPixels = nextPixel - startPixelX;
        const int numTempPixels = roundToInt (numPixels * resolution);
        
        if (numTempPixels > 0)
        {
            if (tempSectionImage.getWidth() < numTempPixels)
            {
                tempSectionImage = Image (Image::RGB,
                                          numTempPixels, waveformImageHeight,
                                          false);
            }
            
            Rectangle<int> rectangleToDraw (0, 0, numTempPixels, waveformImageHeight);
            
            Graphics gTemp (tempSectionImage);
            tempSectionImage.clear (tempSectionImage.getBounds(), backgroundColour);
            gTemp.setColour (waveformColour);
            audioThumbnail.drawChannel (gTemp, rectangleToDraw,
                                        lastTimeDrawn, endTime,
                                        0, 1.0f);
            
            lastTimeDrawn = endTime;
            
            const ScopedWriteLock sl (imageLock);
            
            Graphics g (waveformImage);
            g.drawImage (tempSectionImage,
                         startPixelX, 0, numPixels, waveformImageHeight,
                         0, 0, numTempPixels, tempSectionImage.getHeight());
        }
        
        if (endSamples == audioThumbnail.getNumSamplesFinished())
            renderComplete = true;
	}
    
    if (renderComplete)
        backgroundThread.removeTimeSliceClient (this);
}
