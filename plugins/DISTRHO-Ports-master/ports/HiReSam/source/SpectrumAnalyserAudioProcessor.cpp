/*
  ==============================================================================

    SpectrumAnalyserAudioProcessor.cpp
    Created: 10 Jun 2014 8:19:00pm
    Author:  Samuel Gaehwiler

  ==============================================================================
*/

#include "SpectrumAnalyserAudioProcessor.h"
#include "SpectrumAnalyserAudioProcessorEditor.h"


//==============================================================================
SpectrumAnalyserAudioProcessor::SpectrumAnalyserAudioProcessor()
  : sampleRate {44100.0},
    renderThread("FFT Render Thread"),
    spectrumProcessor(11) // FFT Size of 2^11 = 2048
{
    renderThread.addTimeSliceClient (&spectrumProcessor);
    renderThread.startThread (3);
}

SpectrumAnalyserAudioProcessor::~SpectrumAnalyserAudioProcessor()
{
    renderThread.removeTimeSliceClient (&spectrumProcessor);
    renderThread.stopThread (500);
}

//==============================================================================
const String SpectrumAnalyserAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int SpectrumAnalyserAudioProcessor::getNumParameters()
{
    return 0;
}

float SpectrumAnalyserAudioProcessor::getParameter (int index)
{
    return 0.0f;
}

void SpectrumAnalyserAudioProcessor::setParameter (int index, float newValue)
{
}

const String SpectrumAnalyserAudioProcessor::getParameterName (int index)
{
    return String();
}

const String SpectrumAnalyserAudioProcessor::getParameterText (int index)
{
    return String();
}

const String SpectrumAnalyserAudioProcessor::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String SpectrumAnalyserAudioProcessor::getOutputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

bool SpectrumAnalyserAudioProcessor::isInputChannelStereoPair (int index) const
{
    return true;
}

bool SpectrumAnalyserAudioProcessor::isOutputChannelStereoPair (int index) const
{
    return true;
}

bool SpectrumAnalyserAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SpectrumAnalyserAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SpectrumAnalyserAudioProcessor::silenceInProducesSilenceOut() const
{
    return true;
}

double SpectrumAnalyserAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SpectrumAnalyserAudioProcessor::getNumPrograms()
{
    return 0;
}

int SpectrumAnalyserAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SpectrumAnalyserAudioProcessor::setCurrentProgram (int index)
{
}

const String SpectrumAnalyserAudioProcessor::getProgramName (int index)
{
    return String();
}

void SpectrumAnalyserAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void SpectrumAnalyserAudioProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    spectrumProcessor.setSampleRate (newSampleRate);
}

void SpectrumAnalyserAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void SpectrumAnalyserAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    for (int channel = 0; channel < getTotalNumInputChannels(); ++channel)
    {
        float* channelData = buffer.getWritePointer (channel);

        // Only proceed if the editor is visible.
        if (getActiveEditor() != nullptr
            && channel == 0)
        {
            spectrumProcessor.copySamples(channelData, buffer.getNumSamples());
        }
    }

    // In case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
}

//==============================================================================
bool SpectrumAnalyserAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* SpectrumAnalyserAudioProcessor::createEditor()
{
    return new SpectrumAnalyserAudioProcessorEditor (this,
                                                     spectrumProcessor.getRepaintViewerValue(),
                                                     spectrumProcessor.getMagnitudesBuffer(),
                                                     spectrumProcessor.getDetectedFrequency());
}

//==============================================================================
void SpectrumAnalyserAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SpectrumAnalyserAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectrumAnalyserAudioProcessor();
}
