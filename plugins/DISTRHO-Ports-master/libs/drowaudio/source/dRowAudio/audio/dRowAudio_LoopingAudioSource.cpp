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



//==============================================================================
LoopingAudioSource::LoopingAudioSource (PositionableAudioSource* const inputSource,
                                        const bool deleteInputWhenDeleted)
    : input (inputSource, deleteInputWhenDeleted),
      isLoopingBetweenTimes (false),
      loopStartSample (0),
      loopEndSample (0),
      tempBuffer (2, 512)
{
    jassert (inputSource != nullptr);
    
    tempInfo.numSamples = tempBuffer.getNumSamples();
    tempInfo.startSample = 0;
    tempInfo.buffer = &tempBuffer;
}

LoopingAudioSource::~LoopingAudioSource()
{
}

//==============================================================================
void LoopingAudioSource::setLoopTimes (double startTime, double endTime)
{
    jassert (endTime > startTime); // end time has to be after start!
    
    {
        const ScopedLock sl (loopPosLock);
        
        loopStartTime = startTime;
        loopEndTime = endTime;
        
        loopStartSample = (int64) (startTime * currentSampleRate);
        loopEndSample = (int64) (endTime * currentSampleRate);
    }

    // need to update read position based on new limits
    setNextReadPosition (getNextReadPosition());
}

void LoopingAudioSource::getLoopTimes (double& startTime, double& endTime)
{
    startTime = loopStartTime;
    endTime =loopEndTime;
}

void LoopingAudioSource::setLoopBetweenTimes (bool shouldLoop)
{
    isLoopingBetweenTimes = shouldLoop;
}

bool LoopingAudioSource::getLoopBetweenTimes()
{
    return isLoopingBetweenTimes;
}

//==============================================================================
void LoopingAudioSource::prepareToPlay (int samplesPerBlockExpected,
                                        double sampleRate)
{
    currentSampleRate = sampleRate;
    input->prepareToPlay (samplesPerBlockExpected, sampleRate);
    
    if (tempBuffer.getNumSamples() < samplesPerBlockExpected)
    {
        tempBuffer.setSize (2, samplesPerBlockExpected);
        tempInfo.numSamples = tempBuffer.getNumSamples();
    }
}

void LoopingAudioSource::releaseResources()
{
    input->releaseResources();
}

void LoopingAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& info)
{
    if (info.numSamples > 0)
    {
        if (isLoopingBetweenTimes)
        {
            const ScopedLock sl (loopPosLock);

            const int64 newStart = getNextReadPosition();
            int64 newEnd = loopStartSample + ((newStart + info.numSamples) % loopEndSample);
            
            if (newStart > loopEndSample)
                newEnd = newStart + info.numSamples;
            
            if (newEnd > newStart)
            {
                input->getNextAudioBlock (info);
            }
            else
            {
                const int64 numEndSamps = loopEndSample - newStart;
                const int64 numStartSamps = newEnd - loopStartSample;
                
                tempInfo.startSample = 0;
                tempInfo.numSamples = (int) numEndSamps;
                input->getNextAudioBlock (tempInfo);
                
                tempInfo.startSample = (int) numEndSamps;
                tempInfo.numSamples = (int) numStartSamps;
                input->setNextReadPosition (loopStartSample);
                input->getNextAudioBlock (tempInfo);

                for (int i = 0; i < info.buffer->getNumChannels(); ++i)
                    info.buffer->copyFrom (i, info.startSample, 
                                           tempBuffer,
                                           i, 0, info.numSamples);
            }
        }
        else
        {
            input->getNextAudioBlock (info);
        }
    }    
}

//==============================================================================
void LoopingAudioSource::setNextReadPosition (int64 newPosition)
{
    const ScopedLock sl (loopPosLock);
    
    if (isLoopingBetweenTimes 
        && getNextReadPosition() > loopStartSample
        && getNextReadPosition() < loopEndSample)
    {
        const int64 numLoopSamples = loopEndSample - loopStartSample;
        
        if (newPosition > loopEndSample)
            newPosition = loopStartSample + ((newPosition - loopEndSample) % numLoopSamples);
        else if (newPosition < loopStartSample)
            newPosition = loopEndSample - ((loopStartSample - newPosition) % numLoopSamples);
    }
    
    input->setNextReadPosition (newPosition);
}

void LoopingAudioSource::setNextReadPositionIgnoringLoop (int64 newPosition)
{
    input->setNextReadPosition (newPosition);
}

int64 LoopingAudioSource::getNextReadPosition() const
{
    return input->getNextReadPosition();
}

int64 LoopingAudioSource::getTotalLength() const
{
    return input->getTotalLength();
}

bool LoopingAudioSource::isLooping() const
{
    return input->isLooping();
}

