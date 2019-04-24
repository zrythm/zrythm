/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#ifndef __PLUGINPROCESSOR_H_CC1BF768__
#define __PLUGINPROCESSOR_H_CC1BF768__

#include "includes.h"
#include "Parameters.h"
#include "ParameterUpdater.h"


//==============================================================================
/**
*/
class TremoloAudioProcessor :   public AudioProcessor,
                                public Value::Listener,
                                public ParameterUpdater::Listener,
                                public ChangeBroadcaster
{
public:
    //==============================================================================
    TremoloAudioProcessor();
    ~TremoloAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    //==============================================================================
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
#endif

    //==============================================================================
    const String getName() const override;

    int getNumParameters() override;

    float getParameter (int index) override;
    void setParameter (int index, float newValue) override;

    const String getParameterName (int index) override;
    const String getParameterText (int index) override;

    const String getInputChannelName (int channelIndex) const override;
    const String getOutputChannelName (int channelIndex) const override;
    bool isInputChannelStereoPair (int index) const override;
    bool isOutputChannelStereoPair (int index) const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool silenceInProducesSilenceOut() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    /** Returns a PluginParameter so any UIs that have been created can reference
        the underlying Values.
     */
    PluginParameter* getParameterObject (int index);

    /** Quicker way of getting the Value object of a PluginParameter.
     */
    Value& getParameterValueObject (int index);

    /** This will be called when one of our PluginParameter values changes.
        We go through them one by one to see which one has changed and respond to
        it accordingly.
     */
    void valueChanged (Value& changedValue) override;

    //==============================================================================
    Buffer& getTremoloBuffer (int index);

private:
    //==============================================================================
    OwnedArray<PluginParameter> parameters;
    Value dummyValue;

    Buffer tremoloBufferL, tremoloBufferR, dummyBuffer;

    double currentSampleRate;
    float tremoloBufferPosition;
    float tremoloBufferIncriment;

    ParameterUpdater parameterUpdater;

    //==============================================================================
    void parameterUpdated (int index) override;

    void fillBuffer (float* bufferToFill, float phaseAngleRadians);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TremoloAudioProcessor);
};

#endif  // __PLUGINPROCESSOR_H_CC1BF768__
