/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-7 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#ifndef _DROWAUDIOFILTER_H_
#define _DROWAUDIOFILTER_H_

#include "Parameters.h"

//==============================================================================
/**
    A reverb filter that impliments a Schoeder/Moorer model with adjustable
	pre-delay, early reflections and reverb tail.
*/
class DRowAudioFilter  : public AudioProcessor,
                         public ChangeBroadcaster
{
public:
    //==============================================================================
    DRowAudioFilter();
    ~DRowAudioFilter() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    //==============================================================================
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;
#endif

    //==============================================================================
    const String getName() const override;

    int getNumParameters() override;

    float getParameter (int index) override;
    void setParameter (int index, float newValue) override;

    const String getParameterName (int index) override;
    const String getParameterText (int index) override;

    const String getInputChannelName (const int channelIndex) const override;
    const String getOutputChannelName (const int channelIndex) const override;
    bool isInputChannelStereoPair (int index) const override;
    bool isOutputChannelStereoPair (int index) const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;

    //==============================================================================
    int getNumPrograms() override                                      { return 0; }
    int getCurrentProgram() override                                   { return 0; }
    void setCurrentProgram (int index) override                        { }
    const String getProgramName (int index) override                   { return String(); }
    void changeProgramName (int index, const String& newName) override { }

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool silenceInProducesSilenceOut() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
	// Custom Methods
	void setupParams();
	void updateParameters();

	// AU Compatibility Methods
	double getScaledParameter(int index);
	void setScaledParameter(int index, float newValue);
	void setScaledParameterNotifyingHost(int index, float newValue);
	double getParameterMin(int index);
	double getParameterMax(int index);
	double getParameterDefault(int index);
	ParameterUnit getParameterUnit(int index);
	double getParameterStep(int index);
	double getParameterSkewFactor(int index);
	void smoothParameters();
	PluginParameter* getParameterPointer(int index);

	float* getDistortionBuffer()	{	return distortionBuffer;		}
	int getDistortionBufferSize()	{	return distortionBufferSize;	}

private:

	PluginParameter params[noParams];

	double currentSampleRate;

	static const int distortionBufferSize = 1024;
	static const int distortionBufferMax = distortionBufferSize - 1;
	HeapBlock <float> distortionBuffer;

	ScopedPointer <OnePoleFilter> inFilterL;
	ScopedPointer <OnePoleFilter> inFilterR;
	ScopedPointer <OnePoleFilter> outFilterL;
	ScopedPointer <OnePoleFilter> outFilterR;

	void parameterChanged (int index, float newValue);
	void refillBuffer();
};

#endif //_DROWAUDIOFILTER_H_
