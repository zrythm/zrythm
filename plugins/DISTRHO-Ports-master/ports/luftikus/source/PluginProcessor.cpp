/*
==============================================================================

  This file was auto-generated!

  It contains the basic startup code for a Juce application.

==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//#pragma warning (disable:4100)

//==============================================================================
LuftikusAudioProcessor::LuftikusAudioProcessor()
: showTooltips(true),
  guiType(kGui),
  guiTypeFile(File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("lkjb").getChildFile("Luftikus").getChildFile("Luftikus_GUI.xml")),
  eqDsp(jmax(JucePlugin_MaxNumInputChannels, JucePlugin_MaxNumOutputChannels)),
  analog(0.f),
  mastering(0.f)
{
	//guiType = getTypeFromFile();
}

LuftikusAudioProcessor::~LuftikusAudioProcessor()
{
}

//==============================================================================
const String LuftikusAudioProcessor::getName() const
{
  return JucePlugin_Name;
}

int LuftikusAudioProcessor::getNumParameters()
{
	return kNumParameters;
}

float LuftikusAudioProcessor::getParameter (int index)
{
	if (index < EqDsp::kShelfHi)
		return (eqDsp.getGain((EqDsp::Type) index) + 10.f)/20.f;
	else if (index == EqDsp::kShelfHi)
		return (eqDsp.getGain((EqDsp::Type) index))/10.f;
	else if (index == kHiType)
		return ((int) eqDsp.getHighShelf()) / ((float) EqDsp::kNumHighSelves - 1.f);
	else if (index == kKeepGain)
		return eqDsp.getKeepGain() ? 1.f : 0.f;
	else if (index == kAnalog)
		return analog;
	else if (index == kMastering)
		return mastering;
	else if (index == kMasterVol)
		return masterVolume.getVolumeNormalized();

	jassertfalse;
	return 0;
}

void LuftikusAudioProcessor::setParameter (int index, float newValue)
{
	if (index < EqDsp::kShelfHi)
	{
		eqDsp.setGain((EqDsp::Type) index, 20.f * (newValue - 0.5f));
	}
	else if (index == EqDsp::kShelfHi)
	{
		eqDsp.setGain((EqDsp::Type) index, 10.f * newValue);
	}
	else if (index == kHiType)
	{
		eqDsp.setHighShelf((EqDsp::HighShelf) jlimit(0, EqDsp::kNumHighSelves-1, int(newValue*(EqDsp::kNumHighSelves - 1.f) + 0.5f)));
	}
	else if (index == kKeepGain)
	{
		eqDsp.setKeepGain(newValue > 0.5f);
	}
	else if (index == kAnalog)
	{
		analog = newValue > 0.5f ? 1.f : 0.f;
		eqDsp.setAnalog(analog > 0.5f);
	}
	else if (index == kMastering)
	{
		mastering = newValue > 0.5f ? 1.f : 0.f;
		eqDsp.setMastering(mastering > 0.5f);
	}
	else if (index == kMasterVol)
	{
		masterVolume.setVolumeNormalized(newValue);
	}
}

const String LuftikusAudioProcessor::getParameterName (int index)
{
	switch (index)
	{
	case EqDsp::kBand10:
		return "Gain 10 Hz";
	case EqDsp::kBand40:
		return "Gain 40 Hz";
	case EqDsp::kBand160:
		return "Gain 160 Hz";
	case EqDsp::kBand640:
		return "Gain 640 Hz";
	case EqDsp::kShelf2k5:
		return "Gain 2.5 kHz";
	case EqDsp::kShelfHi:
		return "Gain High";
	case kHiType:
		return "Type High";
	case kKeepGain:
		return "Keep Gain";
	case kAnalog:
		return "Analog";
	case kMastering:
		return "Mastering";
	case kMasterVol:
		return "MasterVol";
	default:
		jassertfalse;
		return String();
	}
}

const String LuftikusAudioProcessor::getParameterText (int index)
{
	if (index < kHiType)
	{
		return String(eqDsp.getGain((EqDsp::Type) index), 1);
	}
	else if (index == kHiType)
	{
		switch (eqDsp.getHighShelf())
		{
		case EqDsp::kHighOff:
			return "Off";
		case EqDsp::kHigh2k5:
			return "2k5";
		case EqDsp::kHigh5k:
			return "5k";
		case EqDsp::kHigh10k:
			return "10k";
		case EqDsp::kHigh20k:
			return "20k";
		case EqDsp::kHigh40k:
			return "40k";
		default:
			jassertfalse;
			return "";
		}
	}
	else if (index == kKeepGain)
	{
		return eqDsp.getKeepGain() ? "On" : "Off";
	}
	else if (index == kAnalog)
	{
		return analog > 0.5f ? "On" : "Off";
	}
	else if (index == kMastering)
	{
		return mastering > 0.5f ? "On" : "Off";
	}
	else if (index == kMasterVol)
	{
		return String(masterVolume.getVolumeDb(), 1) + " dB";
	}

	jassertfalse;
	return "";
}

const String LuftikusAudioProcessor::getInputChannelName (int channelIndex) const
{
	return String (channelIndex + 1);
}

const String LuftikusAudioProcessor::getOutputChannelName (int channelIndex) const
{
	return String (channelIndex + 1);
}

bool LuftikusAudioProcessor::isInputChannelStereoPair (int /*index*/) const
{
	return true;
}

bool LuftikusAudioProcessor::isOutputChannelStereoPair (int /*index*/) const
{
	return true;
}

bool LuftikusAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool LuftikusAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

int LuftikusAudioProcessor::getNumPrograms()
{
	return 0;
}

int LuftikusAudioProcessor::getCurrentProgram()
{
	return 0;
}

void LuftikusAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const String LuftikusAudioProcessor::getProgramName (int /*index*/)
{
	return String();
}

void LuftikusAudioProcessor::changeProgramName (int /*index*/, const String& /*newName*/)
{
}

//==============================================================================
void LuftikusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	eqDsp.setSampleRate(sampleRate);
	eqDsp.setBlockSize(samplesPerBlock);
}

void LuftikusAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

void LuftikusAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& /*midiMessages*/)
{
	const int numSamples = buffer.getNumSamples();
	const int numChannels = buffer.getNumChannels();

	switch (numChannels)
	{
	case 1:
		{
			float* in = buffer.getWritePointer(0);
			eqDsp.processBlock(&in, 1, numSamples);
		}
		break;
	case 2:
		{
			float* inL = buffer.getWritePointer(0);
			float* inR = buffer.getWritePointer(1);
			float* data[2] = {inL, inR};

			eqDsp.processBlock(data, 2, numSamples);
		}
		break;
	case 6:
		{
			float* ch0 = buffer.getWritePointer(0);
			float* ch1 = buffer.getWritePointer(1);
			float* ch2 = buffer.getWritePointer(2);
			float* ch3 = buffer.getWritePointer(3);
			float* ch4 = buffer.getWritePointer(4);
			float* ch5 = buffer.getWritePointer(5);

			float* data[6] = {ch0, ch1, ch2, ch3, ch4, ch5};
			eqDsp.processBlock(data, 6, numSamples);
		}
		break;
	default:
		jassertfalse;
		break;
	}

	masterVolume.processBlock(buffer);
}

//==============================================================================
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
bool LuftikusAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* LuftikusAudioProcessor::createEditor()
{
	return new LuftikusAudioProcessorEditor (this, guiType);
}
#endif

//==============================================================================
void LuftikusAudioProcessor::getStateInformation (MemoryBlock& destData)
{
	XmlElement xml("LUFTIKUSDATA");

	for (int i=0; i<getNumParameters(); ++i)
		xml.setAttribute(getParameterName(i).replace(" ", "_", false).replace(".", "-", false), getParameter(i));

	xml.setAttribute("tooltips", showTooltips ? 1 : 0);
	copyXmlToBinary(xml, destData);
}

void LuftikusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	ScopedPointer<XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

	if (xml != nullptr)
	{
		for (int i=0; i<getNumParameters(); ++i)
			setParameterNotifyingHost(i, (float) xml->getDoubleAttribute(getParameterName(i).replace(" ", "_", false).replace(".", "-", false)));

		showTooltips = xml->getBoolAttribute("tooltips", true);
	}
}

bool LuftikusAudioProcessor::silenceInProducesSilenceOut(void) const
{
	return false;
}

double LuftikusAudioProcessor::getTailLengthSeconds() const
{
	return 0;
}

const MasterVolume& LuftikusAudioProcessor::getMasterVolume() const
{
	return masterVolume;
}
void LuftikusAudioProcessor::setGuiType(LuftikusAudioProcessor::GUIType newType)
{
	guiType = newType;

	if (guiType != getTypeFromFile())
	{
		if (guiType == kGui)
		{
			guiTypeFile.deleteFile();

			if (guiTypeFile.getParentDirectory().getNumberOfChildFiles(File::findFilesAndDirectories) == 0)
				guiTypeFile.getParentDirectory().deleteFile();
		}
		else
		{
			ScopedPointer<XmlElement> xml(XmlDocument::parse(guiTypeFile));

			if (xml == nullptr)
				xml = new XmlElement("LUFTIKUS");

			const String type(guiType == kLuftikus ? "Luftikus" : guiType == kLkjb ? "lkjb" : "gui");
			xml->setAttribute("guitype", type);
			DBG("Setting " + type);

			guiTypeFile.deleteFile();
			guiTypeFile.create();
			xml->writeToFile(guiTypeFile, "");
		}		
	}
}
LuftikusAudioProcessor::GUIType LuftikusAudioProcessor::getGuiType()
{
	//return kLuftikus;
	return guiType;
}

LuftikusAudioProcessor::GUIType LuftikusAudioProcessor::getTypeFromFile()
{
	ScopedPointer<XmlElement> xml(XmlDocument::parse(guiTypeFile));

	if (xml != nullptr && xml->hasTagName("LUFTIKUS") && xml->hasAttribute("guitype"))
	{
		const String type(xml->getStringAttribute("guitype"));
		DBG("Getting " + type);

		if (type == "Luftikus")
			return kLuftikus;
		if (type == "lkjb")
			return kLkjb;
	}

	return kGui;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new LuftikusAudioProcessor();
}
