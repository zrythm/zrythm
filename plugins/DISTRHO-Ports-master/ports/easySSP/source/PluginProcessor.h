/*
  ==============================================================================

	This file was auto-generated!

	It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "JuceHeader.h"
#include "dsp-utility.h"
#include "GonioPoints.h"
#include "PluginState.h"
#include <vector>
#include <stack>
//==============================================================================
/**
*/

#define TOMATL_PLUGIN_SET_PROPERTY(name, value) mState. name = value; makeCurrentStateEffective()

class AdmvAudioProcessor  : public AudioProcessor
{
public:
	tomatl::dsp::SpectrumBlock* mSpectroSegments;
	GonioPoints<double>* mGonioSegments;
	//==============================================================================
	AdmvAudioProcessor();
	~AdmvAudioProcessor();

	//==============================================================================
	void prepareToPlay (double sampleRate, int samplesPerBlock);
	void releaseResources();

	void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

	//==============================================================================
	AudioProcessorEditor* createEditor();
	bool hasEditor() const;

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
	bool silenceInProducesSilenceOut() const;
	double getTailLengthSeconds() const;

	//==============================================================================
	int getNumPrograms();
	int getCurrentProgram();
	void setCurrentProgram (int index);
	const String getProgramName (int index);
	void changeProgramName (int index, const String& newName);

	const AdmvPluginState& getState() { return mState; }

	void setManualGonioScaleEnabled(bool value) { TOMATL_PLUGIN_SET_PROPERTY(mManualGoniometerScale, value); }
	void setManualGonioScaleValue(double value) { TOMATL_PLUGIN_SET_PROPERTY(mManualGoniometerScaleValue, value); }
	void setManualGonioScaleRelease(double value) { TOMATL_PLUGIN_SET_PROPERTY(mGoniometerScaleAttackRelease.second, value); }
	void setSpectroMagnitudeScale(std::pair<double, double> value) { TOMATL_PLUGIN_SET_PROPERTY(mSpectrometerMagnitudeScale, value); }
	void setSpectroFrequencyScale(std::pair<double, double> value) { TOMATL_PLUGIN_SET_PROPERTY(mSpectrometerFrequencyScale, value); }
	void setSpectroReleaseSpeed(double value) { TOMATL_PLUGIN_SET_PROPERTY(mSpectrometerReleaseSpeed, value); }
	void setSpectroFillMode(AdmvPluginState::SpectrumFillMode value) { TOMATL_PLUGIN_SET_PROPERTY(mSpectrumFillMode, value); }
	void setOutputMode(AdmvPluginState::OutputMode value) { TOMATL_PLUGIN_SET_PROPERTY(mOutputMode, value); }

	//==============================================================================
	void getStateInformation (MemoryBlock& destData);
	void setStateInformation (const void* data, int sizeInBytes);
	virtual void numChannelsChanged();

	size_t getCurrentInputCount() { return mCurrentInputCount; }

	// TODO: host can stop to supply blocks, and not call releaseResources(), so this isn't working properly
	bool isCurrentlyProcessing() { return mGonioSegments != NULL; }

	Colour getStereoPairColor(int index) 
	{ 
		/*0x4ae329,
			0x3192e7,
			0xc628e7,
			0x5218f7,
			0xc6ff18,
			0xf72021*/
		// TODO: store collection and return const references
		if (index == 0)
		{
			return Colour::fromString("ff4ae329");
		}
		else if (index == 1)
		{
			return Colour::fromString("ff3192e7");
		}
		else if (index == 2)
		{
			return Colour::fromString("ffc628e7");
		}
		else if (index == 3)
		{
			return Colour::fromString("ff5218f7");
		}
		else
		{
			return Colour::fromString("ffc6ff18");
		}
	}
	size_t getMaxStereoPairCount() { return mMaxStereoPairCount; }

	double mLastGonioScale;

private:
	std::vector<tomatl::dsp::GonioCalculator<double>*> mGonioCalcs;
	std::vector<tomatl::dsp::SpectroCalculator<double>*> mSpectroCalcs;
	size_t mMaxStereoPairCount;
	size_t mCurrentInputCount;
	AdmvPluginState mState;
	
	void makeCurrentStateEffective();

	uint8 getStateVersion()
	{
		return 4;
	}

	bool isBlockInformative(AudioSampleBuffer& buffer, size_t pairIndex)
	{
		float magnitude = 0.;
		//size_t checksum = 0;

		for (int i = 0; i < 2; ++i)
		{
			float* channelData = buffer.getWritePointer(pairIndex * 2 + i);

			for (int s = 0; s < buffer.getNumSamples(); ++s)
			{
				magnitude = std::max(magnitude, std::abs(channelData[s]));

				/*for (int b = 0; b < sizeof(float); ++b)
				{
					checksum = (checksum >> 1) + ((checksum & 1) << 15);
					checksum += *((char*)(channelData + s) + b);
					checksum &= 0xffff;
				}*/
			}
		}

		bool result = magnitude > 0.;

		return result;
	}

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdmvAudioProcessor)
};

#endif  // PLUGINPROCESSOR_H_INCLUDED
