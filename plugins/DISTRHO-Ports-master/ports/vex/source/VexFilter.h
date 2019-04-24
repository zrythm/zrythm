/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2008 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2008 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU Lesser General Public License, as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

   @author  rockhardbuns
   @tweaker Lucio Asnaghi
   @tweaker falkTX

 ==============================================================================
*/

#ifndef __JUCETICE_VEXPLUGINFILTER_HEADER__
#define __JUCETICE_VEXPLUGINFILTER_HEADER__

#include "JucePluginCharacteristics.h"

#include "vex/VexArp.h"
#include "vex/VexChorus.h"
#include "vex/VexDelay.h"
#include "vex/VexReverb.h"
#include "vex/VexSyntModule.h"

#include "VexEditorComponent.h"

class VexFilter : public AudioProcessor,
                  public VexEditorComponent::Callback
{
public:
    VexFilter();
    ~VexFilter();

    bool silenceInProducesSilenceOut() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    const String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return JucePlugin_WantsMidiInput; }
    bool producesMidi() const override { return JucePlugin_ProducesMidiOutput; }

    int getNumPrograms() { return 0; }
    int getCurrentProgram() { return 0; }
    void setCurrentProgram(int) {}
    const String getProgramName(int) { return String(); }
    void changeProgramName(int, const juce::String&) {}

    const String getInputChannelName (const int channelIndex) const override;
    const String getOutputChannelName (const int channelIndex) const override;
    bool isInputChannelStereoPair (int index) const override;
    bool isOutputChannelStereoPair (int index) const override;

    int getNumParameters() override { return kParamCount; }
    float getParameter (int index) override;
    void setParameter (int index, float newValue) override;

    const String getParameterName (int index) override;
    const String getParameterText (int index) override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer& output, MidiBuffer& midiMessages) override;

    void setStateInformation (const void* data, int sizeInBytes) override;
    void getStateInformation (MemoryBlock& destData) override;

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;
#endif

    void getChangedParameters(bool params[92]) override;
    float getFilterParameterValue(const uint32_t index) const override;
    String getFilterWaveName(const int part) const override;
    void editorParameterChanged(const uint32_t index, const float value) override;
    void editorWaveChanged(const int part, const String& wave) override;

private:
    static const unsigned int kParamCount = 92;

    float fParameters[kParamCount];
    bool  fParamsChanged[92];

    AudioSampleBuffer obf;
    AudioSampleBuffer dbf1; // delay
    AudioSampleBuffer dbf2; // chorus
    AudioSampleBuffer dbf3; // reverb

    VexArpSettings fArpSet1, fArpSet2, fArpSet3;
    VexArp fArp1, fArp2, fArp3;

    VexChorus fChorus;
    VexDelay fDelay;
    VexReverb fReverb;
    VexSyntModule fSynth;
};

#endif
