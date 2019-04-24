/*
==============================================================================

  This file was auto-generated!

  It contains the basic startup code for a Juce application.

==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
PitchedDelayAudioProcessor::PitchedDelayAudioProcessor()
: currentTab(-1),
	showTooltips(true)
{
	for (int i=0; i<NUMDELAYTABS; ++i)
	{
		DelayTabDsp* delay = new DelayTabDsp("Tab" + String(i+1));
		delays.add(delay);
	}

	params[kMasterVolume] = 0.25119f;
	params[kDryVolume] = 0.25119f;

	setLatencySamples(delays[0]->getLatencySamples());

}

PitchedDelayAudioProcessor::~PitchedDelayAudioProcessor()
{
}

//==============================================================================
const String PitchedDelayAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

int PitchedDelayAudioProcessor::getNumParameters()
{
	const int numParams = getNumDelayParameters() + kNumParameters;

  return numParams;
}

float PitchedDelayAudioProcessor::getParameter (int index)
{
	const int numDelayParams = getNumDelayParameters();

	if (index < numDelayParams)
	{
		const int numDelay = index / delays[0]->getNumParameters();
		const int paramIdx = index % delays[0]->getNumParameters();

		DelayTabDsp* dsp = delays[numDelay];
		jassert(dsp != 0);

		return dsp->getParamNormalized(paramIdx);
	}
	else
	{
		return params[index - numDelayParams];
	}
	
}

void PitchedDelayAudioProcessor::setParameter (int index, float newValue)
{
	const int numDelayParams = getNumDelayParameters();

	if (index < numDelayParams)
	{
		const int numDelay = index / delays[0]->getNumParameters();
		const int paramIdx = index % delays[0]->getNumParameters();

		DelayTabDsp* dsp = delays[numDelay];
		jassert(dsp != 0);

		dsp->setParamNormalized(paramIdx, newValue);	
	}
	else
	{
		params[index - numDelayParams] = newValue;
	}

}

const String PitchedDelayAudioProcessor::getParameterName (int index)
{
	const int numDelayParams = getNumDelayParameters();

	if (index < numDelayParams)
	{
		const int numDelay = index / delays[0]->getNumParameters();
		const int paramIdx = index % delays[0]->getNumParameters();

		DelayTabDsp* dsp = delays[numDelay];
		jassert(dsp != 0);

		return dsp->getId() + "-" + dsp->getParamName(paramIdx);
	}
	else
	{
		switch(index - numDelayParams)
		{
		case kDryVolume:
			return "DryVolume";
		case kMasterVolume:
			return "MasterVolume";
		default:
			jassertfalse;
			return "";
		}
	}

}

const String PitchedDelayAudioProcessor::getParameterText (int index)
{
	const int numDelayParams = getNumDelayParameters();

	if (index < numDelayParams)
	{
		const int numDelay = index / delays[0]->getNumParameters();
		const int paramIdx = index % delays[0]->getNumParameters();

		DelayTabDsp* dsp = delays[numDelay];
		jassert(dsp != 0);

		return dsp->getParamText(paramIdx) + " " + dsp->getParamUnit(paramIdx);
	}
	else
	{
		switch(index - numDelayParams)
		{
		case kMasterVolume:
			return String(Decibels::gainToDecibels(params[kMasterVolume]), 2) + " dB";
		case kDryVolume:
			return String(Decibels::gainToDecibels(params[kDryVolume]), 2) + " dB";
		default:
			jassertfalse;
			return "";
		}
	}
}

const String PitchedDelayAudioProcessor::getInputChannelName (int channelIndex) const
{
  return String (channelIndex + 1);
}

const String PitchedDelayAudioProcessor::getOutputChannelName (int channelIndex) const
{
  return String (channelIndex + 1);
}

bool PitchedDelayAudioProcessor::isInputChannelStereoPair (int /*index*/) const
{
  return true;
}

bool PitchedDelayAudioProcessor::isOutputChannelStereoPair (int /*index*/) const
{
  return true;
}

bool PitchedDelayAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool PitchedDelayAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool PitchedDelayAudioProcessor::silenceInProducesSilenceOut() const
{
	return false;
}

int PitchedDelayAudioProcessor::getNumPrograms()
{
  return kNumPresets+1;
}

int PitchedDelayAudioProcessor::getCurrentProgram()
{
  return 0;
}

void PitchedDelayAudioProcessor::setCurrentProgram (int index)
{
	const int sync = index - 1;

	if (sync >= 0)
	{
		for (int i=0; i<delays.size(); ++i)
			delays.getUnchecked(i)->setParam(DelayTabDsp::kSync, (double) sync);
	}
}

const String PitchedDelayAudioProcessor::getProgramName (int index)
{
	switch(index - 1)
	{
	case kPresetAllSeconds:
		return "All tabs in seconds";
	case kPresetAll1_2:
		return "All tabs in 1/2";
	case kPresetAll1_2T:
		return "All tabs in 1/2T";
	case kPresetAll1_4:
		return "All tabs in 1/4";
	case kPresetAll1_4T:
		return "All tabs in 1/4T";
	case kPresetAll1_8:
		return "All tabs in 1/8";
	case kPresetAll1_8T:
		return "All tabs in 1/8T";
	case kPresetAll1_16:
		return "All tabs in 1/16";
	default:
		return "";
	}
}

void PitchedDelayAudioProcessor::changeProgramName (int /*index*/, const String& /*newName*/)
{
}

//==============================================================================
void PitchedDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

	int dspSR = 44100;
	int underSampling = 1;

	switch ((int) sampleRate)
	{
	case 44100:
		dspSR = 44100;
		break;
	case 88200:
		dspSR = 44100;
		underSampling = 2;
		break;
	case 176400:
		dspSR = 44100;
		underSampling = 4;
		break;
	case 48000:
		dspSR = 48000;
		break;
	case 96000:
		dspSR = 48000;
		underSampling = 2;
		break;
	case 192000:
		dspSR = 48000;
		underSampling = 4;
		break;
	default:
		jassertfalse;
	}

	{
		int blockSize = 0;
		downSamplers.clear();
		upSamplers.clear();
		Array<int> lengths;

		for (int i=1; i <= underSampling; i = i << 1)
		{
			const int curLength = samplesPerBlock / i;
			blockSize += 2*curLength;
			lengths.add(curLength);

			if (i > 1)
			{
				upSamplers.add(new UpSampler2x(4, false));
				downSamplers.add(new DownSampler2x(4, false));
			}
		}

		overSampleBuffers.allocate(blockSize, true);
		osBufferL.allocate(lengths.size(), false);
		osBufferR.allocate(lengths.size(), false);

		int n=0;
		for (int i=0; i<lengths.size(); ++i)
		{
			osBufferL[i] = &(overSampleBuffers[n]);
			n += lengths[i];
			osBufferR[i] = &(overSampleBuffers[n]);
			n += lengths[i];
		}
	}

	for (int i=0; i<NUMDELAYTABS; ++i)
	{
		DelayTabDsp* dsp = delays[i];

		dsp->prepareToPlay((double) dspSR, samplesPerBlock);
	}


	const int latency = delays[0]->getLatencySamples() / underSampling;
	setLatencySamples(latency);
	latencyCompensation.setLength(latency);
}

void PitchedDelayAudioProcessor::releaseResources()
{
	upSamplers.clear();
	downSamplers.clear();
}

void PitchedDelayAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& /*midiMessages*/)
{
	const int numSamples = buffer.getNumSamples();
	float* chL = buffer.getWritePointer(0);
	float* chR = buffer.getWritePointer(buffer.getNumChannels()-1);


	float* dspProcL = osBufferL[0];
	float* dspProcR = osBufferR[0];

	{
		for (int i=0; i< numSamples; ++i)
		{
			dspProcL[i] = chL[i];
			dspProcR[i] = chR[i];
		}

		int blockSize = numSamples;

		for (int i=0; i<downSamplers.size(); ++i)
		{
			blockSize /= 2;
			DownSampler2x* down = downSamplers[i];
			jassert(down != nullptr);
			down->processBlock(dspProcL, dspProcR, osBufferL[i+1], osBufferR[i+1], blockSize);
			dspProcL = osBufferL[i+1];
			dspProcR = osBufferR[i+1];
		}

		for (int i=0; i<NUMDELAYTABS; ++i)
		{
			DelayTabDsp* dsp = delays[i];
			dsp->processBlock(dspProcL, dspProcR, blockSize);
		}

		for (int n=0; n<blockSize; ++n)
		{
			dspProcL[n] = 0;
			dspProcR[n] = 0;
		}

		for (int i=0; i<NUMDELAYTABS; ++i)
		{
			DelayTabDsp* dsp = delays[i];

			if (! dsp->isEnabled())
				continue;

			const float* procL = dsp->getLeftData();
			const float* procR = dsp->getRightData();

			for (int n=0; n<blockSize; ++n)
			{
				dspProcL[n] += procL[n];
				dspProcR[n] += procR[n];
			}
		}

		for (int i=upSamplers.size()-1; i >= 0; --i)
		{
			UpSampler2x* up = upSamplers[i];
			jassert(up != nullptr);
			up->processBlock(dspProcL, dspProcR, osBufferL[i], osBufferR[i], blockSize);
			blockSize *= 2;
			dspProcL = osBufferL[i];
			dspProcR = osBufferR[i];
		}
	}

	for (int i=0; i<numSamples; ++i)
	{
		chL[i] *= params[kDryVolume] * 3.981f; // params[kDryVolume] == 1 -> +12 dB
		chR[i] *= params[kDryVolume] * 3.981f; // params[kDryVolume] == 1 -> +12 dB
	}

	latencyCompensation.processBlock(chL, chR, numSamples);

	for (int i=0; i<numSamples; ++i)
	{
		chL[i] = (chL[i] + dspProcL[i]) * params[kMasterVolume] * 3.981f; // params[kMasterVolume] == 1 -> +12 dB
		chR[i] = (chR[i] + dspProcR[i]) * params[kMasterVolume] * 3.981f; // params[kMasterVolume] == 1 -> +12 dB
	}


  for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
  {
      buffer.clear (i, 0, buffer.getNumSamples());
  }
}

//==============================================================================
bool PitchedDelayAudioProcessor::hasEditor() const
{
  return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* PitchedDelayAudioProcessor::createEditor()
{
  return new PitchedDelayAudioProcessorEditor (this);
}

//==============================================================================
void PitchedDelayAudioProcessor::getStateInformation (MemoryBlock& destData)
{
	XmlElement xml("PitchedDelay");

	for (int i=0; i<getNumParameters(); ++i)
		xml.setAttribute(getParameterName(i), getParameter(i));

	xml.setAttribute("currenttab", jmax(0, currentTab));
	xml.setAttribute("showtooltips", showTooltips ? 1 : 0);

	xml.setAttribute("extended", "1");

	copyXmlToBinary(xml, destData);
}

void PitchedDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	ScopedPointer<XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

	if (xml != 0 && xml->getTagName() == "PitchedDelay")
	{
		const bool extendedsync = xml->hasAttribute("extended");

		for (int i=0; i<getNumParameters(); ++i)
		{
			const String paramName(getParameterName(i));

			float val = (float) xml->getDoubleAttribute(paramName, -1000);

			if (! extendedsync && paramName.contains("Sync"))
				val = val*7/9;

			if (! extendedsync && paramName.contains("PitchType"))
				val = val*5/8;			

			if (val > -1000)
				setParameterNotifyingHost(i, val);
		}

		const int curTab = xml->getIntAttribute("currentTab", -1000);
		showTooltips = xml->getIntAttribute("showtooltips") == 1;
		currentTab = curTab > -1000 ? curTab : 0;
	}


}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return new PitchedDelayAudioProcessor();
}
