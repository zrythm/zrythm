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

#ifndef __DROWAUDIO_AUDIOFILEPLAYEREXT_H__
#define __DROWAUDIO_AUDIOFILEPLAYEREXT_H__

#if DROWAUDIO_USE_SOUNDTOUCH

#include "dRowAudio_SoundTouchAudioSource.h"
#include "dRowAudio_ReversibleAudioSource.h"
#include "dRowAudio_LoopingAudioSource.h"
#include "dRowAudio_FilteringAudioSource.h"

//==============================================================================
/**
    This class can be used to load and play an audio file from disk.

    This combines the functionality of an AudioTransportSource,
    AudioFormatReader and AudioFormatReaderSource.
 
    @see AudioTransportSource
    @see AudioFormatReader
    @see AudioFormatReaderSource
 */
class AudioFilePlayerExt :  public AudioFilePlayer
{
public:
    //==============================================================================
    enum PlayerSettingCode
    {
        SoundTouchSetting           = 0x0001,
        LoopBeetweenTimesSetting    = 0x0002,
        PlayDirectionSetting        = 0x0003,
        FilterGainSetting           = 0x0004
    };
    
    //==============================================================================
	/** Creates an empty AudioFilePlayerExt.
     */
	AudioFilePlayerExt();
    
	/** Destructor.
     */
	~AudioFilePlayerExt();
	
    //==============================================================================
    /** Sets the current library entry.
     */
    void setLibraryEntry (ValueTree newEntry)               {   libraryEntry = newEntry;    }

    /** Returns the currents library entry.
     */
    ValueTree getLibraryEntry()                             {   return  libraryEntry;       }
    
    //==============================================================================
    /** Sets SoundTouchProcessor settings.
     */
    void setPlaybackSettings (SoundTouchProcessor::PlaybackSettings newSettings);
    
    /** Returns the current SoundTouchProcessor settings.
     */
    SoundTouchProcessor::PlaybackSettings getPlaybackSettings();
    
    /** Sets whether the source should play forwards or backwards.
     */
	void setPlayDirection (bool shouldPlayForwards);
    
    /** Returns true if the source is playing forwards.
     */
	bool getPlayDirection();

    /** Sets the gain of one of the FilteringAudioSource filters.
     */
    void setFilterGain (FilteringAudioSource::FilterType type, float newGain);
    
    //==============================================================================
    /** Sets the start and end times of the loop.
        This doesn't actually activate the loop, use setLoopBetweenTimes() to toggle this.
     */
	void setLoopTimes (double startTime, double endTime);
	
    /** Enables the loop point set.
     */
    void setLoopBetweenTimes (bool shouldLoop);
    
    /** Returns true if the loop is activated.
     */
    bool getLoopBetweenTimes();
    
    /** Sets the next play position in seconds disregarding the loop boundries.
     */
    void setPosition (double newPosition, bool ignoreAnyLoopBounds = false);
    
    //==============================================================================
    /** Returns the SoundTouchAudioSource being used.
     */
    inline SoundTouchAudioSource* getSoundTouchAudioSource()       {   return soundTouchAudioSource;   }

	/** Returns the FilteringAudioSource being used.
     */
    inline FilteringAudioSource* getFilteringAudioSource()         {   return filteringAudioSource;    }
    
private:	
    //==============================================================================
    ScopedPointer<BufferingAudioSource> bufferingAudioSource;
    ScopedPointer<LoopingAudioSource> loopingAudioSource;
    ScopedPointer<SoundTouchAudioSource> soundTouchAudioSource;
    ScopedPointer<ReversibleAudioSource> reversibleAudioSource;
    ScopedPointer<FilteringAudioSource> filteringAudioSource;

    SoundTouchProcessor::PlaybackSettings currentSoundtouchSettings;
    bool shouldBeLooping;
    double currentLoopStartTime, currentLoopEndTime;
    
    ValueTree libraryEntry;

    //==============================================================================
	bool setSourceWithReader (AudioFormatReader* reader);
    void updateLoopTimes();
    
    //==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFilePlayerExt);
};


#endif
#endif // __DROWAUDIO_AUDIOFILEPLAYEREXT_H__
