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


AudioFilePlayer::AudioFilePlayer()
    : bufferingTimeSliceThread (new TimeSliceThread ("Shared Buffering Thread"), true),
      formatManager (new AudioFormatManager(), true)
{
    bufferingTimeSliceThread->startThread (3);
	formatManager->registerBasicFormats();
    
    commonInitialise();
}

AudioFilePlayer::AudioFilePlayer (TimeSliceThread* threadToUse,
                                  AudioFormatManager* formatManagerToUse)
    : bufferingTimeSliceThread ((threadToUse == nullptr ? new TimeSliceThread ("Shared Buffering Thread")
                                                        : threadToUse),
                                threadToUse == nullptr),
      formatManager ((formatManagerToUse == nullptr ? new AudioFormatManager()
                                                    : formatManagerToUse),
                     formatManagerToUse == nullptr)
{
    commonInitialise();
}

AudioFilePlayer::~AudioFilePlayer()
{
	audioTransportSource.setSource (nullptr);
    audioTransportSource.removeChangeListener (this);
}

//==============================================================================
void AudioFilePlayer::start()
{
    audioTransportSource.start();
    
    listeners.call (&Listener::playerStoppedOrStarted, this);
}

void AudioFilePlayer::stop()
{
    audioTransportSource.stop();
    
    listeners.call (&Listener::playerStoppedOrStarted, this);
}

void AudioFilePlayer::startFromZero()
{
	if (audioFormatReaderSource == nullptr)
        return;
	
	audioTransportSource.setPosition (0.0);
	audioTransportSource.start();
    
    listeners.call (&Listener::playerStoppedOrStarted, this);
}

void AudioFilePlayer::pause()
{
	if (audioTransportSource.isPlaying())
		audioTransportSource.stop();
	else
		audioTransportSource.start();
    
    listeners.call (&Listener::playerStoppedOrStarted, this);
}

//==============================================================================
void AudioFilePlayer::setPosition (double newPosition, bool /*ignoreAnyLoopPoints*/)
{
    audioTransportSource.setPosition (newPosition);
}

//==============================================================================
void AudioFilePlayer::setAudioFormatManager (AudioFormatManager* newManager, bool deleteWhenNotNeeded)
{
    formatManager.set (newManager, deleteWhenNotNeeded);
}

void AudioFilePlayer::setTimeSliceThread (TimeSliceThread* newThreadToUse, bool deleteWhenNotNeeded)
{
    bufferingTimeSliceThread.set (newThreadToUse, deleteWhenNotNeeded);
}

//==============================================================================
void AudioFilePlayer::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    if (masterSource != nullptr)
        masterSource->prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void AudioFilePlayer::releaseResources()
{
    if (masterSource != nullptr)
        masterSource->releaseResources();
}

void AudioFilePlayer::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    if (masterSource != nullptr)
        masterSource->getNextAudioBlock (bufferToFill);
}

void AudioFilePlayer::setLooping (bool shouldLoop)
{   
    if (audioFormatReaderSource != nullptr)
        audioFormatReaderSource->setLooping (shouldLoop); 
}

//==============================================================================
bool AudioFilePlayer::fileChanged (const File& file)
{
    if (setSourceWithReader (formatManager->createReaderFor (file)))
        return true;
    
    clear();
    return false;
}

bool AudioFilePlayer::streamChanged (InputStream* inputStream)
{
    if (setSourceWithReader (formatManager->createReaderFor (inputStream)))
        return true;
    
    clear();
    return false;
}

void AudioFilePlayer::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source == &audioTransportSource)
        listeners.call (&Listener::playerStoppedOrStarted, this);
}

//==============================================================================
void AudioFilePlayer::addListener (AudioFilePlayer::Listener* const listener)
{
    listeners.add (listener);
}

void AudioFilePlayer::removeListener (AudioFilePlayer::Listener* const listener)
{
    listeners.remove (listener);
}

//==============================================================================
bool AudioFilePlayer::setSourceWithReader (AudioFormatReader* reader)
{
    bool shouldBeLooping = isLooping();
	audioTransportSource.setSource (nullptr);

	if (reader != nullptr)
	{										
		// we SHOULD let the AudioFormatReaderSource delete the reader for us..
		audioFormatReaderSource = new AudioFormatReaderSource (reader, true);
        audioTransportSource.setSource (audioFormatReaderSource,
                                        32768,
                                        bufferingTimeSliceThread);
        
        if (shouldBeLooping)
            audioFormatReaderSource->setLooping (true);
        
		// let our listeners know that we have loaded a new file
		audioTransportSource.sendChangeMessage();
        listeners.call (&Listener::fileChanged, this);

		return true;
	}
	
    audioTransportSource.sendChangeMessage();
    listeners.call (&Listener::fileChanged, this);

    return false;    
}

//==============================================================================
void AudioFilePlayer::commonInitialise()
{
    audioTransportSource.addChangeListener (this);
    masterSource = &audioTransportSource;
}
