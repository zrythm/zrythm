/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "JuceHeader.h"
#include "ADRess.h"

//==============================================================================
/**
*/
class StereoSourceSeparationAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    StereoSourceSeparationAudioProcessor();
    ~StereoSourceSeparationAudioProcessor();

    enum Parameters
    {
        kStatus,
        kDirection,
        kWidth,
        kFilterType,
        kCutOffFrequency,
        kNumParameters
    };
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();

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

    bool acceptsMidi() const;
    bool producesMidi() const;
    bool silenceInProducesSilenceOut() const;
    double getTailLengthSeconds() const;

    //==============================================================================
    int getNumPrograms();
    int getCurrentProgram();
    void setCurrentProgram (int index);
    const String getProgramName (int index);
    void changeProgramName (int index, const String& newName);

    //==============================================================================
    void getStateInformation (MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);

private:
    const int NUM_CHANNELS;
    const int BLOCK_SIZE;
    const int HOP_SIZE;
    const int BETA;
    
    AudioSampleBuffer inputBuffer_;
    int inputBufferLength_;
    int inputBufferWritePosition_;
    
    AudioSampleBuffer outputBuffer_;
    int outputBufferLength_;
    int outputBufferReadPosition_, outputBufferWritePosition_;
    
    AudioSampleBuffer processBuffer_;
    
    ADRess* separator_;
    
    int samplesSinceLastFFT_;
    
    ADRess::Status_t status_;
    int direction_;
    int width_;
    ADRess::FilterType_t filterType_;
    float cutOffFrequency_;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoSourceSeparationAudioProcessor)
};

#endif  // PLUGINPROCESSOR_H_INCLUDED
