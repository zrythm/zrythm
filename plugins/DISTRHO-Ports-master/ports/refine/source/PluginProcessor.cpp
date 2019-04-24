#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "xmmintrin.h"

ReFinedAudioProcessor::ReFinedAudioProcessor()    
{
    parameters = new AudioProcessorValueTreeState(*this, nullptr);
    parameters->createAndAddParameter("red", "red", "", NormalisableRange<float>(0.f, 1.f), 0.f, [](float val) { return String(val, 2); }, [](const String& s) { return (float) s.getDoubleValue(); });
    parameters->createAndAddParameter("green", "green", "", NormalisableRange<float>(0.f, 1.f), 0.f, [](float val) { return String(val, 2); }, [](const String& s) { return (float) s.getDoubleValue(); });
    parameters->createAndAddParameter("blue", "blue", "", NormalisableRange<float>(0.f, 1.f), 0.f, [](float val) { return String(val, 2); }, [](const String& s) { return (float) s.getDoubleValue(); });
    parameters->createAndAddParameter("x2", "x2", "", NormalisableRange<float>(0.f, 1.f), 0.f, [](float val) { return val < 0.5f ? "Off" : "On"; }, [](const String& s) { return s.trim() == "1" || s.trim().toLowerCase() == "on" ? 1.f : 0.f; });
}

ReFinedAudioProcessor::~ReFinedAudioProcessor()
{
}

const String ReFinedAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ReFinedAudioProcessor::acceptsMidi() const
{
    return false;
}

bool ReFinedAudioProcessor::producesMidi() const
{
    return false;
}

double ReFinedAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ReFinedAudioProcessor::getNumPrograms()
{
    return 1;
}

int ReFinedAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ReFinedAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const String ReFinedAudioProcessor::getProgramName (int /*index*/)
{
    return String();
}

void ReFinedAudioProcessor::changeProgramName (int /*index*/, const String& /*newName*/)
{
}

void ReFinedAudioProcessor::prepareToPlay (double newSampleRate, int /*samplesPerBlock*/)
{
    dsp.setSampleRate(newSampleRate);    
}

void ReFinedAudioProcessor::releaseResources()
{
}

void ReFinedAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& /*midiMessages*/)
{
    {
        const float low = parameters->getParameter("red")->getValue();
        const float mid = parameters->getParameter("green")->getValue();
        const float high = parameters->getParameter("blue")->getValue();
        const float x2mode = parameters->getParameter("x2")->getValue();

        dsp.setLow(0.9f * low + 0.05f * mid + 0.05f * high);
        dsp.setMid(0.9f * mid + 0.05f * high + 0.05f * low);
        dsp.setHigh(0.9f * high + 0.05f * low + 0.05f * mid);
        dsp.setX2Mode(x2mode > 0.5f);
    }

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    float* chL = buffer.getWritePointer(0);
    float* chR = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    dsp.processBlock(chL, chR, numSamples);
}

bool ReFinedAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* ReFinedAudioProcessor::createEditor()
{
    return new ReFinedAudioProcessorEditor (*this);
}

void ReFinedAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    XmlElement xml("REFINED");

    for (int i = 0; i < getNumParameters(); ++i)
        xml.setAttribute(getParameterName(i), getParameter(i));

    copyXmlToBinary(xml, destData);
}

void ReFinedAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    ScopedPointer<XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr)
    {
        for (int i = 0; i < getNumParameters(); ++i)
            setParameterNotifyingHost(i, (float) xml->getDoubleAttribute(getParameterName(i), getParameter(i)));
    }
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReFinedAudioProcessor();
}
