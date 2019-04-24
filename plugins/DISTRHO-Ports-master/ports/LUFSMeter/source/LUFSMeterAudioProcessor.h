/*
 ===============================================================================
 
 LUFSMeterAudioProcessor.h
 
 
 This file is part of the LUFS Meter audio measurement plugin.
 Copyright 2011-12 by Klangfreund, Samuel Gaehwiler.
 
 -------------------------------------------------------------------------------
 
 The LUFS Meter can be redistributed and/or modified under the terms of the GNU 
 General Public License Version 2, as published by the Free Software Foundation.
 A copy of the license is included with these source files. It can also be found
 at www.gnu.org/licenses.
 
 The LUFS Meter is distributed WITHOUT ANY WARRANTY.
 See the GNU General Public License for more details.
 
 -------------------------------------------------------------------------------
 
 To release a closed-source product which uses the LUFS Meter or parts of it,
 a commercial license is available. Visit www.klangfreund.com/lufsmeter for more
 information.
 
 ===============================================================================
 */


#ifndef __LUFS_METER_AUDIO_PROCESSOR__
#define __LUFS_METER_AUDIO_PROCESSOR__

#include "MacrosAndJuceHeaders.h"
#include "Ebu128LoudnessMeter.h"


//==============================================================================
/**
*/
class LUFSMeterAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    LUFSMeterAudioProcessor();
    ~LUFSMeterAudioProcessor();

    //==============================================================================
    
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    
    void releaseResources();
    
    void reset();

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

    //==============================================================================
    AudioProcessorEditor* createEditor();
    bool hasEditor() const;

    //==============================================================================
    const String getName() const;

    int getNumParameters();

    float getParameter (int index);
    void setParameter (int index, float newValue);

    const String getParameterName (int index);
    const String getParameterText (int index);

    const String getInputChannelName (int channelIndex) const;
    const String getOutputChannelName (int channelIndex) const;
    bool isInputChannelStereoPair (int index) const;
    bool isOutputChannelStereoPair (int index) const;
    bool silenceInProducesSilenceOut() const;
    double getTailLengthSeconds() const;
    bool acceptsMidi() const;
    bool producesMidi() const;

    //==============================================================================
    // Returns the number of preset programs the filter supports.
    int getNumPrograms();
    
    // Returns the number of the currently active program.
    int getCurrentProgram();
    
    // Called by the host to change the current program.
    void setCurrentProgram (int index);
    
    // Must return the name of a given program.
    const String getProgramName (int index);
    
    // Called by the host to rename a program.
    void changeProgramName (int index, const String& newName);
    
    /** This method is called when the number of input or output channels is 
        changed. 
     */
    virtual void numChannelsChanged();

    //==============================================================================
    // The host will call this method when it wants to save the filter's 
    // internal state.
    void getStateInformation (MemoryBlock& destData);
    
    // The host will call this method if it wants to save the state of just 
    // the filter's current program. 
    void setStateInformation (const void* data, int sizeInBytes);
    
    //==============================================================================
    float getShortTermLoudness();
    
    vector<float>& getMomentaryLoudnessForIndividualChannels();
    
    float getMomentaryLoudness();
    
    float getIntegratedLoudness();

    float getLoudnessRangeStart();
    float getLoudnessRangeEnd();
    float getLoudnessRange();

    //==============================================================================
    // this keeps a copy of the last set of time info that was acquired during an audio
    // callback - the UI component will read this and display it.
    AudioPlayHead::CurrentPositionInfo lastPosInfo;
    
    // these are used to persist the UI's size - the values are stored along with the
    // filter's other parameters, and the UI component will update them when it gets
    // resized.
    int lastUIWidth;
    int lastUIHeight;
    
    // Plugin settings
    // ===============
    /**
        The width of a loudness bar, in pixels.
     
        The preferencesPane.loudnessBarSize sets this value and the
        LUFSMeterAudioProcessorEditor listens to it.
        LUFSMeterAudioProcessorEditor::resizeGuiComponents() is called
        on change.
     */
    Value loudnessBarWidth;

    /**
     The minimal loudness a loudness bar can display.
     
     The preferencesPane.loudnessBarRangeSlider sets this value and the
     individual loudness bars listen to it.
     */
    Value loudnessBarMinValue;
    
    /**
     The maximal loudness a loudness bar can display.
     
     The preferencesPane.loudnessBarRangeSlider sets this value and the
     individual loudness bars listen to it.
     */
    Value loudnessBarMaxValue;
    
    Value showIntegratedLoudnessHistory;
    Value showLoudnessRangeHistory;
    Value showShortTermLoudnessHistory;
    Value showMomentaryLoudnessHistory;
    
    Value numberOfInputChannels;
    
    // The actual loudness metering algorithm
    Ebu128LoudnessMeter ebu128LoudnessMeter;

private:
    //==============================================================================
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LUFSMeterAudioProcessor);
};

#endif  // __LUFS_METER_AUDIO_PROCESSOR__
