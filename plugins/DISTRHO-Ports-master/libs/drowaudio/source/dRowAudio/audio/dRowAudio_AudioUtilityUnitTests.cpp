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

#if DROWAUDIO_UNIT_TESTS


//==============================================================================
class AudioUtilityUnitTests  : public UnitTest
{
public:
    AudioUtilityUnitTests() : UnitTest ("AudioUtilityUnitTests") {}
    
    void runTest()
    {
        beginTest ("AudioUtilityUnitTests");
        
        
        expectEquals (semitonesToPitchRatio (12.0), 2.0);
        expectEquals (semitonesToPitchRatio (-12.0), 0.5);
        expectEquals (semitonesToPitchRatio (-24.0), 0.25);

        expectEquals (pitchRatioToSemitones (2.0), 12.0);
        expectEquals (pitchRatioToSemitones (0.5), -12.0);
        expectEquals (pitchRatioToSemitones (0.25), -24.0);
    }
};

static AudioUtilityUnitTests audioUtilityUnitTests;

//==============================================================================
class AudioSampleBufferUnitTests  : public UnitTest
{
public:
    AudioSampleBufferUnitTests() : UnitTest ("AudioSampleBufferUnitTests") {}
    
    void runTest()
    {
        beginTest ("AudioSampleBufferUnitTests");
        
        {
            AudioSampleBuffer buffer (1, 512);
            expect (isAudioSampleBuffer (buffer.getArrayOfReadPointers(), getNumBytesForAudioSampleBuffer (buffer)));
        }

        {
            AudioSampleBuffer buffer (6, 1024);
            expect (isAudioSampleBuffer (buffer.getArrayOfReadPointers(), getNumBytesForAudioSampleBuffer (buffer)));
        }
        
        {
            AudioSampleBuffer buffer (3, 256);

            MemoryInputStream stream (buffer.getArrayOfReadPointers(), getNumBytesForAudioSampleBuffer (buffer), false);
            unsigned int numChannels;
            int64 numSamples;
            expect (isAudioSampleBuffer (stream, numChannels, numSamples));
            expectEquals ((int) numChannels, 3);
            expectEquals ((int) numSamples, 256);
        }
    }
};

static AudioSampleBufferUnitTests audioSampleBufferUnitTests;

//==============================================================================


#endif // DROWAUDIO_UNIT_TESTS