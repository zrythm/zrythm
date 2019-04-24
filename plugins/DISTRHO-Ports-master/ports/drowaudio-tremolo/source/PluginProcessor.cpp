/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginHelpers.h"

//==============================================================================
TremoloAudioProcessor::TremoloAudioProcessor()
    : tremoloBufferL    (2048),
      tremoloBufferR    (2048),
      dummyBuffer       (1),
      parameterUpdater  (*this)
{
    // set up the parameters
    for (int i = 0; i < Parameters::numParameters; ++i)
    {
        PluginParameter* param = new PluginParameter();
        parameters.add (param);
        param->getValueObject().addListener (this);
        param->init (Parameters::names[i],
                     Parameters::units[i],
                     Parameters::descriptions[i],
                     Parameters::defaults[i],
                     Parameters::mins[i],
                     Parameters::maxs[i],
                     Parameters::defaults[i]);
        parameterUpdater.addParameter (i);
    }

    parameterUpdater.dispatchParameters();

    // make sure to initialize everything
    int blockSize = getBlockSize();
    if (blockSize <= 0)
        blockSize = 512;

    double sampleRate = getSampleRate();
    if (sampleRate <= 0.0)
        sampleRate = 44100.0;

    prepareToPlay(blockSize, sampleRate);
    releaseResources();
}

TremoloAudioProcessor::~TremoloAudioProcessor()
{
    for (int i = 0; i < Parameters::numParameters; ++i)
        parameters[i]->getValueObject().removeListener (this);
}

//==============================================================================
const String TremoloAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int TremoloAudioProcessor::getNumParameters()
{
    return parameters.size();
}

float TremoloAudioProcessor::getParameter (int index)
{
    if (index >= 0 && index < parameters.size())
        return (float) parameters[index]->getNormalisedValue();

    return 0.0f;
}

void TremoloAudioProcessor::setParameter (int index, float newValue)
{
    if (index >= 0 && index < parameters.size())
        parameters[index]->setNormalisedValue (newValue);
}

const String TremoloAudioProcessor::getParameterName (int index)
{
    if (index >= 0 && index < parameters.size())
        return parameters[index]->getName();

    return String();
}

const String TremoloAudioProcessor::getParameterText (int index)
{
    if (index >= 0 && index < parameters.size())
        return String (parameters[index]->getValue(), 2);

    return String();
}

const String TremoloAudioProcessor::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String TremoloAudioProcessor::getOutputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

bool TremoloAudioProcessor::isInputChannelStereoPair (int /*index*/) const
{
    return true;
}

bool TremoloAudioProcessor::isOutputChannelStereoPair (int /*index*/) const
{
    return true;
}

bool TremoloAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool TremoloAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

int TremoloAudioProcessor::getNumPrograms()
{
    return 0;
}

int TremoloAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TremoloAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const String TremoloAudioProcessor::getProgramName (int /*index*/)
{
    return String();
}

void TremoloAudioProcessor::changeProgramName (int /*index*/, const String& /*newName*/)
{
}

//==============================================================================
void TremoloAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    currentSampleRate = sampleRate;
    tremoloBufferPosition = 0;
    parameterUpdated (Parameters::rate);
}

void TremoloAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void TremoloAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& /*midiMessages*/)
{
    // update any pending parameters
    parameterUpdater.dispatchParameters();

    const int tremoloBufferSize = tremoloBufferL.getSize();
    const float* tremoloData[2] = {tremoloBufferL.getData(), tremoloBufferR.getData()};

    // find the number of samples in the buffer to process
    int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

	// initialise the pointer to samples
	float* channelData[numChannels];
    for (int c = 0; c < numChannels; ++c)
		channelData[c] = buffer.getWritePointer (c);

	//===================================================================
	// Main Sample Loop
	//===================================================================
    while (--numSamples >= 0)
    {
        for (int c = 0; c < 2 && c < numChannels; ++c)
            *channelData[c]++ *= linearInterpolate (tremoloData[c], tremoloBufferSize, tremoloBufferPosition);

        // incriment buffer position
		tremoloBufferPosition += tremoloBufferIncriment;
		if (tremoloBufferPosition >= tremoloBufferSize)
			tremoloBufferPosition -= tremoloBufferSize;
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
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
bool TremoloAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* TremoloAudioProcessor::createEditor()
{
    return new TremoloAudioProcessorEditor (this);
}
#endif

//==============================================================================
void TremoloAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    PresetHelpers::savePreset (*this, destData);
}

void TremoloAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    PresetHelpers::loadPreset (*this, data, sizeInBytes);
}

//==============================================================================
PluginParameter* TremoloAudioProcessor::getParameterObject (int index)
{
    return parameters[index];
}

Value& TremoloAudioProcessor::getParameterValueObject (int index)
{
    if (isPositiveAndBelow (index, parameters.size()))
        return parameters[index]->getValueObject();

    return dummyValue;
}

void TremoloAudioProcessor::valueChanged (Value& changedValue)
{
    // lock so we dont change things during the rendering callback
    for (int i = 0; i < Parameters::numParameters; ++i)
    {
        if (changedValue.refersToSameSourceAs (getParameterValueObject (i)))
        {
            // we don't need to add if its already pending
            parameterUpdater.addParameter (i);
            break;
        }
    }
}
//==============================================================================
Buffer& TremoloAudioProcessor::getTremoloBuffer (int index)
{
    if (index == 0)
        return tremoloBufferL;
    if (index == 1)
        return tremoloBufferR;

    return dummyBuffer;
}

//==============================================================================
void TremoloAudioProcessor::parameterUpdated (int parameter)
{
    if (parameter == Parameters::rate)
    {
        const float samplesPerTremoloCycle = currentSampleRate / parameters.getUnchecked (Parameters::rate)->getValue();
        tremoloBufferIncriment = tremoloBufferL.getSize() / samplesPerTremoloCycle;
    }
    else if (parameter == Parameters::depth
             || parameter == Parameters::shape
             || parameter == Parameters::phase)
    {
        fillBuffer (tremoloBufferL.getData(), 0);
        fillBuffer (tremoloBufferR.getData(), degreesToRadians (parameters.getUnchecked (Parameters::phase)->getValue()));
        sendChangeMessage();
    }
}

void TremoloAudioProcessor::fillBuffer (float* bufferToFill, float phaseAngleRadians)
{
	// Scale parameters
	const float depth = (float) parameters.getUnchecked (Parameters::depth)->getNormalisedValue() * 0.5f;
	const float shape = (float) parameters.getUnchecked (Parameters::shape)->getValue();

    const int tremoloBufferSize = tremoloBufferL.getSize();
    const float oneOverTremoloBufferSize = 1.0f / tremoloBufferSize;

	// create buffer with sine data
	for (int i = 0; i < tremoloBufferSize; ++i)
	{
		// fill buffer with sine data
		const float radians = i * 2.0f * (float_Pi * oneOverTremoloBufferSize);
		float rawBufferData = sin (radians + phaseAngleRadians);

		if (rawBufferData >= 0)
        {
			bufferToFill[i] = ((pow (rawBufferData, shape) * depth) + (1.0f - depth));
        }
		else
		{
			rawBufferData *= -1.0f;
			rawBufferData = pow (rawBufferData, shape);
			rawBufferData *= -1.0f;
			bufferToFill[i] = rawBufferData * depth + (1.0f - depth);
		}
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TremoloAudioProcessor();
}
