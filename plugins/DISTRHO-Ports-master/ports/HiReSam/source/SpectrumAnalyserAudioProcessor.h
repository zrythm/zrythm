/*
  ==============================================================================

    SpectrumAnalyserAudioProcessor.h
    Created: 10 Jun 2014 8:19:00pm
    Author:  Samuel Gaehwiler

  ==============================================================================
*/

#ifndef SPECTRUM_ANALYSER_AUDIO_PROCESSOR_H_INCLUDED
#define SPECTRUM_ANALYSER_AUDIO_PROCESSOR_H_INCLUDED

#include "SpectrumAnalyserHeader.h"
#include "SpectrumProcessor.h"

//==============================================================================
/**
*/
class SpectrumAnalyserAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    SpectrumAnalyserAudioProcessor();
    ~SpectrumAnalyserAudioProcessor();

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
    
    Value sampleRate;

private:
    TimeSliceThread renderThread;
    SpectrumProcessor spectrumProcessor;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyserAudioProcessor)
};

#endif  // SPECTRUM_ANALYSER_AUDIO_PROCESSOR_H_INCLUDED
