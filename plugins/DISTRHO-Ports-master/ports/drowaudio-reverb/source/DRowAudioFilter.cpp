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

#include "includes.h"
#include "DRowAudioFilter.h"
#include "DRowAudioEditorComponent.h"


//==============================================================================
/**
    This function must be implemented to create a new instance of your
    plugin object.
*/
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DRowAudioFilter();
}

//==============================================================================
DRowAudioFilter::DRowAudioFilter()
{
	// set up the parameters with the required limits and units

	params[PREDELAY].init(parameterNames[PREDELAY], UnitMilliseconds, String(),
						  50, 0.025, 200, 50);
	params[PREDELAY].setStep(0.1);

	params[ROOMSHAPE].init(parameterNames[ROOMSHAPE], UnitGeneric, String(),
						   3, 3, 7, 3);
	params[ROOMSHAPE].setStep(1);
	prevRoomShape = 0;

	params[EARLYDECAY].init(parameterNames[EARLYDECAY], UnitSeconds, String(),
							5, 0, 20, 5);

	params[EARLYLATEMIX].init(parameterNames[EARLYLATEMIX], UnitPercent, String(),
							  50, 0, 100, 50);
	params[EARLYLATEMIX].setValue(50);

	params[FBCOEFF].init(parameterNames[FBCOEFF], UnitSeconds, String(),
						 5, 0, 20, 5);
	params[FBCOEFF].setSkewFactor(0.4);

	params[DELTIME].init(parameterNames[DELTIME], UnitGeneric, String(),
						 80, 0.15, 100, 80);

	params[FILTERCF].init(parameterNames[FILTERCF], UnitHertz, String(),
						  3500, 20, 20000, 3500);
	params[FILTERCF].setSkewFactor(0.5);
	params[FILTERCF].setStep(1);

	params[DIFFUSION].init(parameterNames[DIFFUSION], UnitPercent, String(),
						  50, 0, 100, 50);
	params[DIFFUSION].setValue(60);

	params[SPREAD].init(parameterNames[SPREAD], UnitPercent, String(),
						0, 00, 100, 0);

	params[LOWEQ].init(parameterNames[LOWEQ], UnitHertz, String(),
						0, -60, 24, 0);
	params[LOWEQ].setSkewFactorFromMidPoint(0.0);
	params[LOWEQ].setStep(0.1);

	params[HIGHEQ].init(parameterNames[HIGHEQ], UnitHertz, String(),
						0, -60, 24, 0);
	params[HIGHEQ].setSkewFactorFromMidPoint(0.0);
	params[HIGHEQ].setStep(0.1);

	params[WETDRYMIX].init(parameterNames[WETDRYMIX], UnitPercent, String(),
						   50, 0, 100, 50);

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

DRowAudioFilter::~DRowAudioFilter()
{
}

//==============================================================================
const String DRowAudioFilter::getName() const
{
    return "dRowAudio: Reverb";
}

int DRowAudioFilter::getNumParameters()
{
    return noParams;
}

float DRowAudioFilter::getParameter (int index)
{
	if (index >= 0 && index < noParams)
		return params[index].getNormalisedValue();
	else return 0.0f;
}

double DRowAudioFilter::getScaledParameter (int index)
{
	if (index >= 0 && index < noParams)
		return params[index].getValue();
	else return 0.0f;
}

void DRowAudioFilter::setParameter (int index, float newValue)
{
	for (int i = 0; i < noParams; i++)
	{
		if (index == i) {
			if (params[i].getNormalisedValue() != newValue) {
				params[i].setNormalisedValue(newValue);
				sendChangeMessage ();
			}
			break;
		}
	}
}

void DRowAudioFilter::setScaledParameter (int index, float newValue)
{
	for (int i = 0; i < noParams; i++)
	{
		if (index == i) {
			if (params[i].getValue() != newValue) {
				params[i].setValue(newValue);
				sendChangeMessage ();
			}
                        break;
		}
	}
}

void DRowAudioFilter::setScaledParameterNotifyingHost(int index, float newValue)
{
	for (int i = 0; i < noParams; i++)
        {
		if (index == i)
                {
			if (params[i].getValue() != newValue)
				setParameterNotifyingHost(index, params[i].normaliseValue(newValue));
                        break;
                }
        }
}

const String DRowAudioFilter::getParameterName (int index)
{
	for (int i = 0; i < noParams; i++)
		if (index == i)
			return String(parameterNames[i]);

    return String();
}

const String DRowAudioFilter::getParameterText (int index)
{
	for (int i = 0; i < noParams; i++)
		if (index == i)
			return String(params[i].getValue(), 2);

    return String();
}

PluginParameter* DRowAudioFilter::getParameterPointer(int index)
{
	for (int i = 0; i < noParams; i++)
		if (index == i)
			return &params[i];

    return 0;
}
//=====================================================================
// methods for AU Compatibility
double DRowAudioFilter::getParameterMin(int index)
{
	for (int i = 0; i < noParams; i++)
		if (index == i)
			return params[i].getMin();
    return 0.0f;
}

double DRowAudioFilter::getParameterMax(int index)
{
	for (int i = 0; i < noParams; i++)
		if (index == i)
			return params[i].getMax();
    return 0.0f;
}

double DRowAudioFilter::getParameterDefault(int index)
{
	for (int i = 0; i < noParams; i++)
		if (index == i)
			return params[i].getDefault();
    return 0.0f;
}

ParameterUnit DRowAudioFilter::getParameterUnit(int index)
{
	for (int i = 0; i < noParams; i++)
		if (index == i)
			return params[i].getUnit();
    return (ParameterUnit)0;
}

double  DRowAudioFilter::getParameterStep(int index)
{
	for (int i = 0; i < noParams; i++)
		if (index == i)
			return params[i].getStep();
    return 0.0f;
}
double  DRowAudioFilter::getParameterSkewFactor(int index)
{
	for (int i = 0; i < noParams; i++)
		if (index == i)
			return params[i].getSkewFactor();
    return 0.0f;
}

void DRowAudioFilter::smoothParameters()
{
	for (int i = 0; i < noParams; i++)
		params[i].smooth();
}
//=====================================================================

const String DRowAudioFilter::getInputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

const String DRowAudioFilter::getOutputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

bool DRowAudioFilter::isInputChannelStereoPair (int index) const
{
    return false;
}

bool DRowAudioFilter::isOutputChannelStereoPair (int index) const
{
    return false;
}

bool DRowAudioFilter::acceptsMidi() const
{
    return true;
}

bool DRowAudioFilter::producesMidi() const
{
    return true;
}

//==============================================================================
void DRowAudioFilter::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	currentSampleRate = sampleRate;

	// make sure the delay register has enough memory for teh whole range of the parameter
	preDelayFilterL.setMaxDelayTime(currentSampleRate, params[PREDELAY].getMax());
	preDelayFilterR.setMaxDelayTime(currentSampleRate, params[PREDELAY].getMax());

        wetBuffer.setSize(2, samplesPerBlock);
        wetBuffer.clear();
}

void DRowAudioFilter::releaseResources()
{
}

void DRowAudioFilter::processBlock (AudioSampleBuffer& buffer,
									MidiBuffer& midiMessages)
{
	smoothParameters();
	const int numInputChannels = getTotalNumInputChannels();
	int numSamples = buffer.getNumSamples();

	// set up the parameters to be used
	float preDelay = (float)params[PREDELAY].getSmoothedValue();
	float earlyDecay = (float)params[EARLYDECAY].getSmoothedNormalisedValue();
	earlyDecay = sqrt(sqrt(earlyDecay));
	float late = (float)params[EARLYLATEMIX].getSmoothedNormalisedValue();
	float early = 1.0f - late;
	float fbCoeff = (float)params[FBCOEFF].getSmoothedNormalisedValue();
	fbCoeff = -sqrt(sqrt(fbCoeff));
	float delayTime = (float)params[DELTIME].getSmoothedValue();
	float filterCf = (float)params[FILTERCF].getSmoothedValue();
	float allpassCoeff = (float)params[DIFFUSION].getSmoothedNormalisedValue();
	float spread1 = (float)params[SPREAD].getSmoothedNormalisedValue();
	float spread2 = 1.0f - spread1;
	float lowEQGain = (float)decibelsToAbsolute(params[LOWEQ].getSmoothedValue());
	float highEQGain = (float)decibelsToAbsolute(params[HIGHEQ].getSmoothedValue());
	float wet = (float)params[WETDRYMIX].getSmoothedNormalisedValue();
	float dry = 1.0f - wet;

	float width = (spread1-0.5f) * 0.1f * delayTime;


	// we can only deal with 2-in, 2-out at the moment
	if (numInputChannels == 2)
	{
		// pre-delay section
		preDelayFilterL.setDelayTime(currentSampleRate, preDelay);
		preDelayFilterR.setDelayTime(currentSampleRate, preDelay);


		// early reflections section
		int roomShape = roundFloatToInt(params[ROOMSHAPE].getValue());
		float delayCoeff = 0.08f * delayTime;
		if (roomShape != prevRoomShape)
		{
			delayLineL.removeAllTaps();
			delayLineR.removeAllTaps();

			for(int i = 0; i < 5; i++) {
				delayLineL.addTapAtTime(earlyReflectionCoeffs[roomShape-3][i], currentSampleRate);
				delayLineR.addTapAtTime(earlyReflectionCoeffs[roomShape-3][i], currentSampleRate);
			}

			// we have to set this here incase the delay time has not changed
			delayLineL.setTapSpacingExplicitly(delayCoeff);
			delayLineR.setTapSpacingExplicitly(delayCoeff + spread1);

			prevRoomShape = roomShape;
		}

		delayLineL.setTapSpacing(delayCoeff);
		delayLineL.scaleFeedbacks(earlyDecay);
		delayLineR.setTapSpacing(delayCoeff + spread1);
		delayLineR.scaleFeedbacks(earlyDecay);


		// comb filter section
		for (int i = 0; i < 8; ++i)
		{
			delayTime *= filterMultCoeffs[i];
			delayTime += Random::getSystemRandom().nextInt(100)*0.0001;

			setupFilter(combFilterL[i], fbCoeff, delayTime, filterCf);
			setupFilter(combFilterR[i], fbCoeff, delayTime + width, filterCf);
		}

		// allpass section
		for (int i = 0; i < 4; ++i)
		{
			delayTime *= allpassMultCoeffs[i];
			delayTime -= Random::getSystemRandom().nextInt(100)*0.0001;

			allpassFilterL[i].setGain(allpassCoeff);
			allpassFilterL[i].setDelayTime(currentSampleRate, delayTime);

			allpassFilterR[i].setGain(allpassCoeff);
			allpassFilterR[i].setDelayTime(currentSampleRate, delayTime + width);
		}

		// final EQ section
		lowEQL.makeLowShelf(currentSampleRate, 500, 1, lowEQGain);
		lowEQR.makeLowShelf(currentSampleRate, 500, 1, lowEQGain);
		highEQL.makeHighShelf(currentSampleRate, 3000, 1, highEQGain);
		highEQR.makeHighShelf(currentSampleRate, 3000, 1, highEQGain);


		//========================================================================
		//	Processing
		//========================================================================
		int noSamples = buffer.getNumSamples();
		int noChannels = buffer.getNumChannels();

		// create a copy of the input buffer so we can apply a wet/dry mix later
		wetBuffer.copyFrom(0, 0, buffer, 0, 0, noSamples);
		wetBuffer.copyFrom(1, 0, buffer, 1, 0, noSamples);

		// mono mix wet buffer (used for stereo spread later)
		float *pfWetL = wetBuffer.getWritePointer(0);
		float *pfWetR = wetBuffer.getWritePointer(1);
		while (--numSamples >= 0)
		{
			*pfWetL = *pfWetR = (0.5f * (*pfWetL + *pfWetR));
			pfWetL++;
			pfWetR++;
		}
		numSamples = buffer.getNumSamples();

		// apply the pre-delay to the wet buffer
		preDelayFilterL.processSamples(wetBuffer.getWritePointer(0), noSamples);
		preDelayFilterR.processSamples(wetBuffer.getWritePointer(1), noSamples);


		// create a buffer to hold the early reflections
		AudioSampleBuffer earlyReflections(noChannels, noSamples);
		earlyReflections.copyFrom(0, 0, wetBuffer, 0, 0, noSamples);
		earlyReflections.copyFrom(1, 0, wetBuffer, 1, 0, noSamples);

		// and process the early reflections
		delayLineL.processSamples(earlyReflections.getWritePointer(0), noSamples);
		delayLineR.processSamples(earlyReflections.getWritePointer(1), noSamples);


		// create a buffer to hold the late reverb
		AudioSampleBuffer lateReverb(noChannels, noSamples);
		lateReverb.clear();

		float *pfLateL = lateReverb.getWritePointer(0);
		float *pfLateR = lateReverb.getWritePointer(1);
		pfWetL = wetBuffer.getWritePointer(0);
		pfWetR = wetBuffer.getWritePointer(1);

		// comb filter section
		for (int i = 0; i < 8; ++i)
		{
			combFilterL[i].processSamplesAdding(pfWetL, pfLateL, noSamples);
			combFilterR[i].processSamplesAdding(pfWetR, pfLateR, noSamples);
		}

		// allpass filter section
		for (int i = 0; i < 4; ++i)
		{
			allpassFilterL[i].processSamples(lateReverb.getWritePointer(0), noSamples);
			allpassFilterR[i].processSamples(lateReverb.getWritePointer(1), noSamples);
		}


		// clear wet buffer
		wetBuffer.clear();
		// add early reflections to wet buffer
		wetBuffer.addFrom(0, 0, earlyReflections, 0, 0, noSamples, early);
		wetBuffer.addFrom(1, 0, earlyReflections, 1, 0, noSamples, early);
		// add late reverb to wet buffer
		lateReverb.applyGain(0, noSamples, 0.1f);
		wetBuffer.addFrom(0, 0, lateReverb, 0, 0, noSamples, late);
		wetBuffer.addFrom(1, 0, lateReverb, 1, 0, noSamples, late);

		// final EQ
		lowEQL.processSamples(pfWetL, noSamples);
		lowEQR.processSamples(pfWetR, noSamples);

		highEQL.processSamples(pfWetL, noSamples);
		highEQR.processSamples(pfWetR, noSamples);

		// create stereo spread
		while (--numSamples >= 0)
		{
			float fLeft = *pfWetL;
			float fRight = *pfWetR;
			*pfWetL = (fLeft * spread1) + (fRight * spread2);
			*pfWetR = (fRight * spread1) + (fLeft * spread2);
			pfWetL++;
			pfWetR++;
		}
		numSamples = buffer.getNumSamples();

		// apply wet/dry mix gains
		wetBuffer.applyGain(0, noSamples, wet);
		buffer.applyGain(0, noSamples, dry);

		// add wet buffer to output buffer
		buffer.addFrom(0, 0, wetBuffer, 0, 0, noSamples);
		buffer.addFrom(1, 0, wetBuffer, 1, 0, noSamples);
	}
	//========================================================================


    // in case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = numInputChannels; i < getTotalNumOutputChannels(); ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
}

void DRowAudioFilter::setupFilter(LBCF &filter, float fbCoeff, float delayTime, float filterCf)
{
	filter.setFBCoeff(fbCoeff);
	filter.setDelayTime(currentSampleRate, delayTime);
	filter.setLowpassCutoff(currentSampleRate, filterCf);
}

//==============================================================================
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
AudioProcessorEditor* DRowAudioFilter::createEditor()
{
    return new DRowAudioEditorComponent (this);
}
#endif

//==============================================================================
void DRowAudioFilter::getStateInformation (MemoryBlock& destData)
{
    // you can store your parameters as binary data if you want to or if you've got
    // a load of binary to put in there, but if you're not doing anything too heavy,
    // XML is a much cleaner way of doing it - here's an example of how to store your
    // params as XML..

    // create an outer XML element..
    XmlElement xmlState ("MYPLUGINSETTINGS");

    // add some attributes to it..
    xmlState.setAttribute ("pluginVersion", 1);
	for(int i = 0; i < noParams; i++) {
		params[i].writeXml(xmlState);
	}

    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (xmlState, destData);
}

void DRowAudioFilter::setStateInformation (const void* data, int sizeInBytes)
{
    // use this helper function to get the XML from this binary blob..
    XmlElement* const xmlState = getXmlFromBinary (data, sizeInBytes);

    if (xmlState != 0)
    {
        // check that it's the right type of xml..
        if (xmlState->hasTagName ("MYPLUGINSETTINGS"))
        {
            // ok, now pull out our parameters..
			for(int i = 0; i < noParams; i++) {
				params[i].readXml(xmlState);
			}

            sendChangeMessage ();
        }

        delete xmlState;
    }
}
