#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "JuceHeader.h"
#include "RefineDsp.h"

class ReFinedAudioProcessor  : public AudioProcessor
{
public:
    ReFinedAudioProcessor();
    ~ReFinedAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    const RefineDsp& getDsp() const { return dsp; }

    ScopedPointer<AudioProcessorValueTreeState> parameters;

private:

    RefineDsp dsp;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReFinedAudioProcessor)
};


#endif  // PLUGINPROCESSOR_H_INCLUDED
