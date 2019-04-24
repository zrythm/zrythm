/*
  ==============================================================================

	This file was auto-generated!

	It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <functional>

//==============================================================================
AdmvAudioProcessor::AdmvAudioProcessor() : mSpectroSegments(NULL), mGonioSegments(NULL), mLastGonioScale(1.), mMaxStereoPairCount(0), mCurrentInputCount(0)
{
	releaseResources();
}

AdmvAudioProcessor::~AdmvAudioProcessor()
{
	
}

//==============================================================================
const String AdmvAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

int AdmvAudioProcessor::getNumParameters()
{
	return 0;
}

float AdmvAudioProcessor::getParameter (int index)
{
	return 0.0f;
}

void AdmvAudioProcessor::setParameter (int index, float newValue)
{
}

const String AdmvAudioProcessor::getParameterName (int index)
{
	return String();
}

const String AdmvAudioProcessor::getParameterText (int index)
{
	return String();
}

const String AdmvAudioProcessor::getInputChannelName (int channelIndex) const
{
	return String (channelIndex + 1);
}

const String AdmvAudioProcessor::getOutputChannelName (int channelIndex) const
{
	return String (channelIndex + 1);
}

bool AdmvAudioProcessor::isInputChannelStereoPair (int index) const
{
	return true;
}

bool AdmvAudioProcessor::isOutputChannelStereoPair (int index) const
{
	return true;
}

bool AdmvAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
	return true;
   #else
	return false;
   #endif
}

bool AdmvAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
	return true;
   #else
	return false;
   #endif
}

bool AdmvAudioProcessor::silenceInProducesSilenceOut() const
{
	return false;
}

double AdmvAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int AdmvAudioProcessor::getNumPrograms()
{
	return 0;
}

int AdmvAudioProcessor::getCurrentProgram()
{
	return 0;
}

void AdmvAudioProcessor::setCurrentProgram (int index)
{
}

const String AdmvAudioProcessor::getProgramName (int index)
{
	return String();
}

void AdmvAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void AdmvAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	size_t fftSize = 2048;

	mMaxStereoPairCount = JucePlugin_MaxNumInputChannels / 2;

	for (size_t i = 0; i < mMaxStereoPairCount; ++i)
	{
		mGonioCalcs.push_back(new tomatl::dsp::GonioCalculator<double>(1600, sampleRate));
	}

	for (size_t i = 0; i < mMaxStereoPairCount; ++i)
	{
		mSpectroCalcs.push_back(new tomatl::dsp::SpectroCalculator<double>(sampleRate, std::pair<double, double>(10, getState().mSpectrometerReleaseSpeed), i, fftSize));
	}

	mSpectroSegments = new tomatl::dsp::SpectrumBlock[mMaxStereoPairCount];
	mGonioSegments = new GonioPoints<double>[mMaxStereoPairCount];

	makeCurrentStateEffective();
}

void AdmvAudioProcessor::releaseResources()
{
	for (size_t i = 0; i < mMaxStereoPairCount; ++i)
	{
		TOMATL_DELETE(mGonioCalcs[i]);
	}

	for (size_t i = 0; i < mMaxStereoPairCount; ++i)
	{
		TOMATL_DELETE(mSpectroCalcs[i]);
	}

	mGonioCalcs.clear();
	mSpectroCalcs.clear();

	TOMATL_BRACE_DELETE(mSpectroSegments);
	TOMATL_BRACE_DELETE(mGonioSegments);
}

void AdmvAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	double cp[2];
	
	int channelCount = 0;
	size_t sampleRate = getSampleRate();

	for (int channel = 0; channel < (getTotalNumInputChannels() - 1); channel += 2)
	{
		// No need to process signal if editor is closed
		if (getActiveEditor() == NULL)
		{
			break;
		}

		// TODO: investigate how to get number of input channels really connected to the plugin ATM.
		// It seems that getTotalNumInputChannels() will always return max possible defined by JucePlugin_MaxNumInputChannels
		// This solution is bad, because it iterates through all input buffers.
		if (!isBlockInformative(buffer, channel / 2))
		{
			mGonioSegments[channel / 2] = GonioPoints<double>();
			mSpectroSegments[channel / 2] = tomatl::dsp::SpectrumBlock();

			continue;
		}
		
		channelCount += 2;

		float* l = buffer.getWritePointer(channel + 0);
		float* r = buffer.getWritePointer(channel + 1);

		for (int i = 0; i < buffer.getNumSamples(); ++i)
		{
			std::pair<double, double>* res = mGonioCalcs[channel / 2]->handlePoint(l[i], r[i], sampleRate);

			cp[0] = l[i];
			cp[1] = r[i];

			mSpectroCalcs[channel / 2]->checkSampleRate(getSampleRate());
			tomatl::dsp::SpectrumBlock spectroResult = mSpectroCalcs[channel / 2]->process((double*)&cp);

			if (res != NULL)
			{
				mGonioSegments[channel / 2] = GonioPoints<double>(res, mGonioCalcs[channel / 2]->getSegmentLength(), channel / 2, sampleRate);
				mLastGonioScale = mGonioCalcs[channel / 2]->getCurrentScaleValue();
			}

			if (spectroResult.mLength > 0)
			{
				mSpectroSegments[channel / 2] = spectroResult;
			}
		}
	}
	
	mCurrentInputCount = channelCount;

	if (getState().mOutputMode == AdmvPluginState::outputMute)
	{
		buffer.clear();
	}
	else
	{
		// In case we have more outputs than inputs, we'll clear any output
		// channels that didn't contain input data, (because these aren't
		// guaranteed to be empty - they may contain garbage).
		for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		{
			buffer.clear(i, 0, buffer.getNumSamples());
		}
	}
}

//==============================================================================
bool AdmvAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

void AdmvAudioProcessor::makeCurrentStateEffective()
{
	for (size_t i = 0; i < mGonioCalcs.size(); ++i)
	{
		mGonioCalcs[i]->setCustomScaleEnabled(mState.mManualGoniometerScale);
		mGonioCalcs[i]->setCustomScale(mState.mManualGoniometerScaleValue);
		mGonioCalcs[i]->setReleaseSpeed(mState.mGoniometerScaleAttackRelease.second);
	}

	for (size_t i = 0; i < mSpectroCalcs.size(); ++i)
	{
		mSpectroCalcs[i]->setReleaseSpeed(mState.mSpectrometerReleaseSpeed);
	}

	if (getActiveEditor() != NULL)
	{
		((AdmvAudioProcessorEditor*)getActiveEditor())->updateFromState(mState);
	}
}

void AdmvAudioProcessor::numChannelsChanged()
{

}

AudioProcessorEditor* AdmvAudioProcessor::createEditor()
{
	auto editor = new AdmvAudioProcessorEditor (this);

	editor->updateFromState(mState);

	return editor;
}

//==============================================================================
void AdmvAudioProcessor::getStateInformation (MemoryBlock& destData)
{
	uint8 version = getStateVersion();

	destData.ensureSize(sizeof(mState) + 1, false);
	destData.copyFrom(&version, 0, 1);
	destData.copyFrom(&mState, 1, sizeof(mState));
}

void AdmvAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	uint8 version;

	memcpy(&version, data, 1);

	if (version == getStateVersion())
	{
		memcpy(&mState, (uint8*)data + 1, sizeInBytes - 1);
	}
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new AdmvAudioProcessor();
}
