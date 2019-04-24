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

#ifndef __DROWAUDIO_LOOPINGAUDIOSOURCE_H__
#define __DROWAUDIO_LOOPINGAUDIOSOURCE_H__

#include "../utility/dRowAudio_Utility.h"

//==============================================================================
/** A type of PositionalAudioSource that will read from a PositionableAudioSource
    and can loop between to set times.

    @see PositionableAudioSource, AudioTransportSource, BufferingAudioSource
*/
class LoopingAudioSource  : public PositionableAudioSource
{
public:
    //==============================================================================
    /** Creates an LoopingAudioFormatReaderSource for a given reader.

        @param sourceReader                     the reader to use as the data source
        @param deleteReaderWhenThisIsDeleted    if true, the reader passed-in will be deleted
                                                when this object is deleted; if false it will be
                                                left up to the caller to manage its lifetime
    */
    LoopingAudioSource (PositionableAudioSource* const inputSource,
                        const bool deleteInputWhenDeleted);

    /** Destructor. */
    ~LoopingAudioSource();

    //==============================================================================
    /** Sets the start and end times of the loop.
        This doesn't actually activate the loop, use setLoopBetweenTimes() to toggle this.
     */
    void setLoopTimes (double startTime, double endTime);
    
    /** Sets the arguments to the currently set start and end times.
     */
    void getLoopTimes (double& startTime, double& endTime);
    
    /** Enables the loop point set.
     */
    void setLoopBetweenTimes (bool shouldLoop);

    /** Returns true if the loop is activated.
     */
    bool getLoopBetweenTimes();
    
    //==============================================================================
    /** Implementation of the AudioSource method. */
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);

    /** Implementation of the AudioSource method. */
    void releaseResources();

    /** Implementation of the AudioSource method. */
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill);

    //==============================================================================
    /** Implements the PositionableAudioSource method. */
    void setNextReadPosition (int64 newPosition);

    /** Sets the next read position ignoring the loop bounds. */
    void setNextReadPositionIgnoringLoop (int64 newPosition);

    /** Implements the PositionableAudioSource method. */
    int64 getNextReadPosition() const;
    
    /** Implements the PositionableAudioSource method. */
    int64 getTotalLength() const;
    
    /** Implements the PositionableAudioSource method. */
    bool isLooping() const;
    
private:
    //==============================================================================
    OptionalScopedPointer<PositionableAudioSource> input;
    CriticalSection loopPosLock;

    bool volatile isLoopingBetweenTimes;
    double loopStartTime, loopEndTime;
    int64 loopStartSample, loopEndSample;
    double currentSampleRate;

    AudioSourceChannelInfo tempInfo;
    AudioSampleBuffer tempBuffer;
        
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoopingAudioSource);
};

#endif   // __DROWAUDIO_LOOPINGAUDIOSOURCE_H__