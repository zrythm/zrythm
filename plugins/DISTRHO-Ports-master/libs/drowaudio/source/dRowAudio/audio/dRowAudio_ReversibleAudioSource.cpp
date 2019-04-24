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
ReversibleAudioSource::ReversibleAudioSource (PositionableAudioSource* const inputSource,
											  const bool deleteInputWhenDeleted)
    : input (inputSource, deleteInputWhenDeleted),
      previousReadPosition (0),
	  isForwards(true)
{
    jassert (inputSource != 0);
}

ReversibleAudioSource::~ReversibleAudioSource()
{
}

void ReversibleAudioSource::prepareToPlay (int samplesPerBlockExpected,
										   double sampleRate)
{
	input->prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void ReversibleAudioSource::releaseResources()
{
	input->releaseResources();
}

void ReversibleAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& info)
{
	if (isForwards) 
    {
        input->getNextAudioBlock (info);
        previousReadPosition = input->getNextReadPosition();
    }
    else
	{
        int64 nextReadPosition = previousReadPosition - info.numSamples;
        
		if (nextReadPosition < 0 && input->isLooping())
			nextReadPosition += input->getTotalLength();
		
		input->setNextReadPosition (nextReadPosition);
        input->getNextAudioBlock (info);

		if (info.buffer->getNumChannels() == 1)
		{
			reverseArray (info.buffer->getWritePointer (0) + info.startSample, info.numSamples);
		}
		else if (info.buffer->getNumChannels() == 2) 
		{
			reverseTwoArrays (info.buffer->getWritePointer (0) + info.startSample,
                              info.buffer->getWritePointer (1) + info.startSample,
                              info.numSamples);
		}
		else
		{
			for (int c = 0; c < info.buffer->getNumChannels(); c++)
				reverseArray (info.buffer->getWritePointer (c) + info.startSample, info.numSamples);
		}
        
        previousReadPosition = nextReadPosition;
	}
}

