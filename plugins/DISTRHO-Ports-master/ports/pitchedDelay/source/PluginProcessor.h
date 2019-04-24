/*
==============================================================================

  This file was auto-generated!

  It contains the basic startup code for a Juce application.

==============================================================================
*/

#ifndef __PLUGINPROCESSOR_H_DCFCCE65__
#define __PLUGINPROCESSOR_H_DCFCCE65__

#include "JuceHeader.h"
#include "dsp/delaytabdsp.h"
#include "dsp/BandLimit.h"

#ifdef DEBUG
#define NUMDELAYTABS 2
#else
#define NUMDELAYTABS 5
#endif
#define PITCHRANGESEMITONES 12


//==============================================================================
/**
*/
class PitchedDelayAudioProcessor  : public AudioProcessor
{
public:
	enum
	{
		kDryVolume,
		kMasterVolume,

		kNumParameters
	};

	enum Presets
	{		
		kPresetAllSeconds,		
		kPresetAll1_2,
		kPresetAll1_2T,
		kPresetAll1_4,
		kPresetAll1_4T,
		kPresetAll1_8,
		kPresetAll1_8T,
		kPresetAll1_16,

		kNumPresets
	};


	//==============================================================================
	PitchedDelayAudioProcessor();
	~PitchedDelayAudioProcessor() override;

	//==============================================================================
	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

	void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

	//==============================================================================
	AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	double getTailLengthSeconds() const override { return 0.0; }

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
	bool silenceInProducesSilenceOut() const override;
	//==============================================================================
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram (int index) override;
	const String getProgramName (int index) override;
	void changeProgramName (int index, const String& newName) override;

	//==============================================================================
	void getStateInformation (MemoryBlock& destData) override;
	void setStateInformation (const void* data, int sizeInBytes) override;

	DelayTabDsp* getDelay(int index)
	{
		return delays[index];
	}



	int getNumDelays()
	{
		return delays.size();
	}

	int getNumDelayParameters() { return delays.size() * delays[0]->getNumParameters(); }

	int currentTab;

	bool showTooltips;

private:	

	float params[kNumParameters];
	SampleDelay latencyCompensation;

	OwnedArray<DelayTabDsp> delays;

	OwnedArray<DownSampler2x> downSamplers;
	OwnedArray<UpSampler2x> upSamplers;
	HeapBlock<float> overSampleBuffers;
	HeapBlock<float*> osBufferL;
	HeapBlock<float*> osBufferR;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchedDelayAudioProcessor);
};



#endif  // __PLUGINPROCESSOR_H_DCFCCE65__
