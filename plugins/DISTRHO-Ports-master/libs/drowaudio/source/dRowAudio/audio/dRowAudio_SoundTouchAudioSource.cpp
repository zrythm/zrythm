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

#if DROWAUDIO_USE_SOUNDTOUCH



SoundTouchAudioSource::SoundTouchAudioSource (PositionableAudioSource* source_,
                                              bool deleteSourceWhenDeleted,
                                              int numberOfSamplesToBuffer_,
                                              int numberOfChannels_)
    : source (source_, deleteSourceWhenDeleted),
      numberOfSamplesToBuffer (jmax (1024, numberOfSamplesToBuffer_)),
      numberOfChannels (numberOfChannels_),
      buffer (numberOfChannels_, 0),
      nextReadPos (0),
      isPrepared (false)
{
    jassert (source_ != nullptr);

    soundTouchProcessor.clear();
}

SoundTouchAudioSource::~SoundTouchAudioSource()
{
    releaseResources();
}

void SoundTouchAudioSource::setPlaybackSettings (SoundTouchProcessor::PlaybackSettings newSettings)
{
    soundTouchProcessor.setPlaybackSettings (newSettings);
}

//==============================================================================
void SoundTouchAudioSource::prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate_)
{
    soundTouchProcessor.initialise (numberOfChannels, sampleRate);
    
    if (sampleRate_ != sampleRate
        || numberOfSamplesToBuffer != buffer.getNumSamples()
        || ! isPrepared)
    {
        isPrepared = true;
        sampleRate = sampleRate_;
        buffer.setSize (numberOfChannels, numberOfSamplesToBuffer);
        
        source->prepareToPlay (numberOfSamplesToBuffer, sampleRate_);
    }
}

void SoundTouchAudioSource::releaseResources()
{
    soundTouchProcessor.clear();
    
    isPrepared = false;
    
    source->releaseResources();
}

void SoundTouchAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& info)
{
    while (soundTouchProcessor.getNumReady() < info.numSamples)
        readNextBufferChunk();

    soundTouchProcessor.readSamples (info.buffer->getArrayOfWritePointers(), buffer.getNumChannels(),
                                     info.numSamples, info.startSample);

    effectiveNextPlayPos += (int64) (info.numSamples * soundTouchProcessor.getEffectivePlaybackRatio());
}

//==============================================================================
void SoundTouchAudioSource::setNextReadPosition (int64 newPosition)
{
    const ScopedLock sl (bufferStartPosLock);

    nextReadPos = effectiveNextPlayPos = newPosition;

    soundTouchProcessor.clear();
}

int64 SoundTouchAudioSource::getNextReadPosition() const
{
    return source->isLooping() ? effectiveNextPlayPos % source->getTotalLength()
                               : effectiveNextPlayPos;
}

//==============================================================================
void SoundTouchAudioSource::readNextBufferChunk()
{
    const ScopedLock sl (bufferStartPosLock);

    if (source->getNextReadPosition() != nextReadPos)
        source->setNextReadPosition (nextReadPos);
    
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = buffer.getNumSamples();
    
    source->getNextAudioBlock (info);
    nextReadPos += info.numSamples;

    soundTouchProcessor.writeSamples (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), info.numSamples);
}


#endif
