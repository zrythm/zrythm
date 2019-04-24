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

#ifndef __DROWAUDIO_SOUNDTOUCHPROCESSOR_H__
#define __DROWAUDIO_SOUNDTOUCHPROCESSOR_H__

#if DROWAUDIO_USE_SOUNDTOUCH || DOXYGEN

} // namespace drow

#include "soundtouch/SoundTouch.h"

namespace drow {

//==============================================================================
/** Wraps a SoundTouch object to enable pitch and tempo adjustments to an audio buffer;
 
    To use this is very simple, just create one, initialise it with the desired number
    of channels and sample rate then feed it with some samples and read them back out.
    This is not thread safe so make sure that the read and write methods are not called
    simultaneously by different threads.
 */
class SoundTouchProcessor
{
public:
    //==============================================================================
    /** A struct use to hold all the different playback settings. */
    struct PlaybackSettings
    {
        PlaybackSettings()
          : rate (1.0f), tempo (1.0f), pitch (1.0f)
        {}

        PlaybackSettings (float rate_, float tempo_, float pitch_)
            : rate (rate_), tempo (tempo_), pitch (pitch_)
        {}

        float rate, tempo, pitch;
    };
    
    //==============================================================================
    /** Create a default SoundTouchProcessor.
        Make sure that you call initialise before any processing take place.
        This will apply no shifting/stretching by default, use setPlaybackSetting() to
        apply these effects.
     */
    SoundTouchProcessor();
    
    /** Destructor.
     */
    ~SoundTouchProcessor();
    
    /** Puts the processor into a ready state.
        This must be set before any processing occurs as the results are undefiend if not.
        It is the callers responsibility to make sure the numChannels parameter matches
        those supplied to the read/write methods.
     */
    void initialise (int numChannels, double sampleRate);
    
    /** Writes samples into the pipline ready to be processed.
        Remember to keep a 1:1 ratio of input and output samples more or less samples may
        be required as input compared to output (think of a time stretch). You can find
        this ratio using getNumSamplesRequiredRatio().
     */
    void writeSamples (float** sourceChannelData, int numChannels, int numSamples, int startSampleOffset = 0);
    
    /** Reads out processed samples.
        This will read out as many samples as the processor has ready. Any additional
        space in the buffer will be slienced. As the processor takes a certain ammount of
        samples to calculate an output there is a latency of around 100ms involved in the process.
     */
    void readSamples (float** destinationChannelData, int numChannels, int numSamples, int startSampleOffset = 0);
    
    /** Clears the pipeline of all samples, ready for new processing. */
    void clear()                                                {   soundTouch.clear();             }
    
    /** Flushes the last samples from the processing pipeline to the output.
        Clears also the internal processing buffers.
     
        Note: This function is meant for extracting the last samples of a sound
        stream. This function may introduce additional blank samples in the end
        of the sound stream, and thus it's not recommended to call this function
        in the middle of a sound stream.
     */
    void flush()                                                {   soundTouch.flush();             }
    
    /** Returns the number of samples ready. */
    int getNumReady()                                           {   return soundTouch.numSamples(); }
    
    /** Returns the number of samples in the pipeline but currently unprocessed. */
    int getNumUnprocessedSamples()                              {   return soundTouch.numUnprocessedSamples();  }
    
    /** Sets all of the settings at once. */
    void setPlaybackSettings (PlaybackSettings newSettings);
    
    /** Returns all of the settings. */
    PlaybackSettings getPlaybackSettings()                      {   return settings;                            }
    
    /** Sets a custom SoundTouch setting.
        See SoundTouch.h for details.
     */
    void setSoundTouchSetting (int settingId, int settingValue);
    
    /** Gets a custom SoundTouch setting.
        See SoundTouch.h for details.
     */
    int getSoundTouchSetting (int settingId);
    
    /** Returns the effective playback ratio i.e. the number of output samples produced per input sample. */
    double getEffectivePlaybackRatio()                          {   return (double) soundTouch.getEffectiveRate() * soundTouch.getEffectiveTempo(); }
            
private:
    //==============================================================================
    soundtouch::SoundTouch soundTouch;
    
    CriticalSection lock;
    HeapBlock<float> interleavedInputBuffer, interleavedOutputBuffer;
    int interleavedInputBufferSize, interleavedOutputBufferSize;
    PlaybackSettings settings;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundTouchProcessor);
};

#endif
#endif // __DROWAUDIO_SOUNDTOUCHPROCESSOR_H__
