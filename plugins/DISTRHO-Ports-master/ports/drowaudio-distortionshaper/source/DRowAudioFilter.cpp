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
	currentSampleRate = 44100.0;

	// set up the parameters with the required limits and units

	params[INGAIN].init(parameterNames[INGAIN], UnitDecibels, "Changes the distortion ammount",
						0.0, 0.0, 24.0, 0.0);
//	params[INGAIN].setSkewFactorFromMidPoint(0.0);

	params[OUTGAIN].init(parameterNames[OUTGAIN], UnitDecibels, "Changes the output level",
						 0.0, -6.0, 6.0, 0.0);
	params[OUTGAIN].setSkewFactorFromMidPoint(0.0);

	params[PREFILTER].init(parameterNames[PREFILTER], UnitHertz, "Changes the input filtering",
					 500.0, 50.0, 5000.0, 500.0);
	params[PREFILTER].setSkewFactor(0.5);
	params[PREFILTER].setStep(1.0);

	params[POSTFILTER].init(parameterNames[POSTFILTER], UnitHertz, "Changes the output filtering",
					  500.0, 50.0, 5000.0, 500.0);
	params[POSTFILTER].setSkewFactor(0.5);
	params[POSTFILTER].setStep(1.0);


	params[X1].init(parameterNames[X1], UnitGeneric, String(),
					0.25, 0.0, 1.0, 0.25);

	params[Y1].init(parameterNames[Y1], UnitGeneric, String(),
					0.25, 0.0, 1.0, 0.25);

	params[X2].init(parameterNames[X2], UnitGeneric, String(),
					0.75, 0.0, 1.0, 0.75);

	params[Y2].init(parameterNames[Y2], UnitGeneric, String(),
					0.75, 0.0, 1.0, 0.75);

	// initialiase and fill the buffer
	distortionBuffer.calloc(distortionBufferSize);
	refillBuffer();

	inFilterL = new OnePoleFilter;
	inFilterR = new OnePoleFilter;
	outFilterL = new OnePoleFilter;
	outFilterR = new OnePoleFilter;
}

DRowAudioFilter::~DRowAudioFilter()
{
}

//==============================================================================
const String DRowAudioFilter::getName() const
{
    return "dRowAudio: Distortion Shaper";
}

int DRowAudioFilter::getNumParameters()
{
    return noParams;
}

float DRowAudioFilter::getParameter (int index)
{
	if (index >= 0 && index < noParams)
		return params[index].getNormalisedValue();
	return 0.0f;
}

double DRowAudioFilter::getScaledParameter (int index)
{
	if (index >= 0 && index < noParams)
		return params[index].getValue();
	return 0.0f;
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
	parameterChanged(index, newValue);
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
	parameterChanged(index, newValue);
}

void DRowAudioFilter::setScaledParameterNotifyingHost(int index, float newValue)
{
	for (int i = 0; i < noParams; i++)
        {
		if (index == i) {
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

	parameterChanged(PREFILTER, 0.0f);
	parameterChanged(POSTFILTER, 0.0f);
}

void DRowAudioFilter::releaseResources()
{
}

void DRowAudioFilter::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	smoothParameters();
	const int numInputChannels = getTotalNumInputChannels();
	const int numSamples = buffer.getNumSamples();

	// set up the parameters to be used
	float inGain = decibelsToAbsolute(params[INGAIN].getSmoothedValue());
	float outGain = decibelsToAbsolute(params[OUTGAIN].getSmoothedValue());

	buffer.applyGain(0, numSamples, inGain);

	if (numInputChannels == 2)
	{
		// get sample pointers
		float* channelL = buffer.getWritePointer(0);
		float* channelR = buffer.getWritePointer(1);

		// pre-filter
		inFilterL->processSamples(channelL, numSamples);
		inFilterR->processSamples(channelR, numSamples);

		for (int i=numSamples; --i >= 0;)
		{
			float sampleL = *channelL;
			float sampleR = *channelR;

			// clip samples
			sampleL = jlimit(-1.0f, 1.0f, sampleL);
			sampleR = jlimit(-1.0f, 1.0f, sampleR);

			if (sampleL < 0.0f) {
				sampleL *= -1.0f;
				sampleL = linearInterpolate<float>(distortionBuffer, distortionBufferSize, sampleL*distortionBufferMax);
				sampleL *= -1.0f;
			}
			else {
				sampleL = linearInterpolate<float>(distortionBuffer, distortionBufferSize, sampleL*distortionBufferMax);
			}

			if (sampleR < 0.0f) {
				sampleR *= -1.0f;
				sampleR = linearInterpolate<float>(distortionBuffer, distortionBufferSize, sampleR*distortionBufferMax);
				sampleR *= -1.0f;
			}
			else {
				sampleR = linearInterpolate<float>(distortionBuffer, distortionBufferSize, sampleR*distortionBufferMax);
			}

			*channelL++ = sampleL;
			*channelR++ = sampleR;
		}

		// post-filter
		outFilterL->processSamples(buffer.getWritePointer(0), numSamples);
		outFilterR->processSamples(buffer.getWritePointer(1), numSamples);

		buffer.applyGain(0, numSamples, outGain);
	}
	else if (numInputChannels == 1)
	{
		// get sample pointers
		float* channelL = buffer.getWritePointer(0);

		// pre-filter
		inFilterL->processSamples(channelL, numSamples);

		for (int i=numSamples; --i >= 0;)
		{
			float sampleL = *channelL;

			// clip samples
			sampleL = jlimit(-1.0f, 1.0f, sampleL);

			if (sampleL < 0.0f) {
				sampleL *= -1.0f;
				sampleL = linearInterpolate<float>(distortionBuffer, distortionBufferSize, sampleL*distortionBufferMax);
				sampleL *= -1.0f;
			}
			else {
				sampleL = linearInterpolate<float>(distortionBuffer, distortionBufferSize, sampleL*distortionBufferMax);
			}

			*channelL++ = sampleL;
		}

		// post-filter
		outFilterL->processSamples(buffer.getWritePointer(0), numSamples);

		buffer.applyGain(0, numSamples, outGain);
	}
	//========================================================================


    // in case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = numInputChannels; i < getTotalNumOutputChannels(); ++i)
    {
        buffer.clear (i, 0, numSamples);
    }
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

            // update all params
            refillBuffer();
            inFilterL->makeLowPass(currentSampleRate, params[PREFILTER].getValue());
            inFilterR->makeLowPass(currentSampleRate, params[PREFILTER].getValue());
            outFilterL->makeLowPass(currentSampleRate, params[POSTFILTER].getValue());
            outFilterR->makeLowPass(currentSampleRate, params[POSTFILTER].getValue());

            sendChangeMessage ();
        }

        delete xmlState;
    }
}

//==============================================================================
void DRowAudioFilter::parameterChanged (int index, float newValue)
{
	if (index == X1 || index == Y1 || index == X2 || index == Y2)
		refillBuffer();
	else if (index == PREFILTER)
	{
		inFilterL->makeLowPass(currentSampleRate, params[PREFILTER].getValue());
		inFilterR->makeLowPass(currentSampleRate, params[PREFILTER].getValue());
	}
	else if (index == POSTFILTER)
	{
		outFilterL->makeLowPass(currentSampleRate, params[POSTFILTER].getValue());
		outFilterR->makeLowPass(currentSampleRate, params[POSTFILTER].getValue());
	}
}

void DRowAudioFilter::refillBuffer()
{
	float bufferIncriment = 1.0f / (float)distortionBufferSize;
	float x = 0.0f;
	for (int i = 0; i < distortionBufferSize; i++)
	{
		x += bufferIncriment;
		x = jlimit(0.0f, 1.0f, x);
		distortionBuffer[i] = BezierCurve::cubicBezierNearlyThroughTwoPoints(x,
																			 params[X1].getValue(),
																			 params[Y1].getValue(),
																			 params[X2].getValue(),
																			 params[Y2].getValue());
	}
}

//==============================================================================
