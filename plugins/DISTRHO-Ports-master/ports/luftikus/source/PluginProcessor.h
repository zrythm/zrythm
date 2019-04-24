/*
==============================================================================

  This file was auto-generated!

  It contains the basic startup code for a Juce application.

==============================================================================
*/

#ifndef __PLUGINPROCESSOR_H_25C19BD3__
#define __PLUGINPROCESSOR_H_25C19BD3__

#include "JuceHeader.h"
#include "dsp/eqdsp.h"

//==============================================================================
/**
*/

class MasterVolume
{
public:
	MasterVolume()
		: vol(1.f),
		  minVol(0.1f),
		  maxVol(10.f),
		  minVolDb(Decibels::gainToDecibels(minVol)),
		  maxVolDb(Decibels::gainToDecibels(maxVol))
	{
	}

	void setVolume(float newVol)
	{
		vol = jlimit(minVol, maxVol, newVol);
	}

	float getVolume() const
	{
		return vol;
	}

	void setVolumeNormalized(float newValue)
	{
		const float newVol = Decibels::decibelsToGain(newValue*(maxVolDb-minVolDb)+minVolDb);
		vol = jlimit(minVol, maxVol, newVol);
	}

	float getVolumeNormalized() const
	{
		const float value = (Decibels::gainToDecibels(vol) - minVolDb) / (maxVolDb - minVolDb);
		return value;
	}

	float getVolumeDb() const
	{
		return minVolDb + (maxVolDb - minVolDb) * getVolumeNormalized();
	}

	void processBlock(AudioSampleBuffer& buffer) const
	{
		buffer.applyGain(vol);
	}

	Range<float> getRangeDb() const
	{
		return Range<float> (minVolDb, maxVolDb);
	}

	float plainToNormalized(float plain) const
	{
		return jlimit(0.f, 1.f, (plain - minVolDb) / (maxVolDb - minVolDb));
	}

private:

	float vol;
	const float minVol;
	const float maxVol;
	const float minVolDb;
	const float maxVolDb;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterVolume)
};


class LuftikusAudioProcessor  : public AudioProcessor
{
public:

	enum Parameters
	{
		kHiType = EqDsp::kNumTypes,
		kKeepGain,
		kAnalog,
		kMastering,
		kMasterVol,

		kNumParameters
	};

	enum GUIType
	{
		kLuftikus,
		kLkjb,
		kGui,

		kNumTypes
	};


	//==============================================================================
	LuftikusAudioProcessor();
	~LuftikusAudioProcessor();

	//==============================================================================
	void prepareToPlay (double sampleRate, int samplesPerBlock);
	void releaseResources();

	void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

	//==============================================================================
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
	AudioProcessorEditor* createEditor();
	bool hasEditor() const;
#endif

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

  //==============================================================================
	int getNumPrograms();
	int getCurrentProgram();
	void setCurrentProgram (int index);
	const String getProgramName (int index);
	void changeProgramName (int index, const String& newName);

  //==============================================================================
	void getStateInformation (MemoryBlock& destData);
	void setStateInformation (const void* data, int sizeInBytes);
	bool silenceInProducesSilenceOut(void) const;

	double getTailLengthSeconds() const;

	const MasterVolume& getMasterVolume() const;

	void setGuiType(GUIType newType);
	GUIType getGuiType();

	bool showTooltips;

	GUIType getTypeFromFile();
	GUIType guiType;

private:


	File guiTypeFile;
	MultiEq eqDsp;

	float analog;
	float mastering;
	MasterVolume masterVolume;
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LuftikusAudioProcessor);
};

#endif  // __PLUGINPROCESSOR_H_25C19BD3__
