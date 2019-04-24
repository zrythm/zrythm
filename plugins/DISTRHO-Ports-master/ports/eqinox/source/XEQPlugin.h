/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2004 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2004 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ------------------------------------------------------------------------------

 If you'd like to release a closed-source product which uses JUCE, commercial
 licenses are also available: visit www.rawmaterialsoftware.com/juce for
 more information.

 ==============================================================================
*/

#ifndef __JUCETICE_XEQ_H
#define __JUCETICE_XEQ_H

#include "StandardHeader.h"
#include "JucePluginCharacteristics.h"

#include "Filters/jucetice_EQ.h"
#include "Filters/jucetice_Limiter.h"


//==============================================================================
/**
    Parameters enum definition
*/
enum
{
    PAR_GAIN = 0,
    PAR_DRYWET,

    PAR_THRESHOLD,
    PAR_KNEE,
    PAR_ATTACK,
    PAR_RELEASE,
    PAR_TRIM,

    PAR_BAND1GAIN,
    PAR_BAND1FREQ,
    PAR_BAND1BW,
    PAR_BAND2GAIN,
    PAR_BAND2FREQ,
    PAR_BAND2BW,
    PAR_BAND3GAIN,
    PAR_BAND3FREQ,
    PAR_BAND3BW,
    PAR_BAND4GAIN,
    PAR_BAND4FREQ,
    PAR_BAND4BW,
    PAR_BAND5GAIN,
    PAR_BAND5FREQ,
    PAR_BAND5BW,
    PAR_BAND6GAIN,
    PAR_BAND6FREQ,
    PAR_BAND6BW,

    NUM_PARAMETERS
};


class XEQComponent;

//==============================================================================
/**
    A simple plugin filter that just applies a gain change to its input.

*/
class XEQPlugin  : public AudioPlugin
{
public:

    //==============================================================================
    XEQPlugin();
    ~XEQPlugin() override;

    bool hasEditor() const override { return true; }

    bool silenceInProducesSilenceOut() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    const String getName() const override          { return JucePlugin_Name; }
    bool acceptsMidi() const override              { return JucePlugin_WantsMidiInput; }
    bool producesMidi() const override             { return JucePlugin_ProducesMidiOutput; }

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    XEQComponent* getEditor();

    Equalizer* getEqualizer ();
    Limiter* getLimiter ();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void initialiseToDefault ();

    //==============================================================================
    static int numInstances;

protected:

    //==============================================================================
    friend class XEQComponent;

    Equalizer equalizer;
    Limiter limiter;

    AudioSampleBuffer denormalBuffer;
    AudioSampleBuffer inputBuffers;
    AudioSampleBuffer tempBuffers;

    //==============================================================================
    float getGain (int n);
    void setGain (int n, float newGain);
    float getDryWet (int n);
    void setDryWet (int n, float newGain);

    float getBandGain (int band);
    void setBandGain (int band, float newGain);
    float getBandFreq (int band);
    void setBandFreq (int band, float newGain);
    float getBandBw (int band);
    void setBandBw (int band, float newGain);

    AudioParameter params [NUM_PARAMETERS];
};


#endif
