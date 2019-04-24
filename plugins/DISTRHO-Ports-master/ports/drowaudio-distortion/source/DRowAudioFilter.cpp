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
	:	RMSLeft(0),
		RMSRight(0),
		peakLeft(0),
		peakRight(0)

{
    setupParams();

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
    return "dRowAudio: Distortion";
}

void DRowAudioFilter::setupParams()
{
	params[PRE].init(parameterNames[PRE], UnitHertz, "Changes the input filtering",
					 500.0, 50.0, 5000.0, 500.0);
	params[PRE].setSkewFactor(0.5);
	params[PRE].setStep(1.0);
	params[PRE].setUnitSuffix(" Hz");
	params[POST].init(parameterNames[POST], UnitHertz, "Changes the output filtering",
					  500.0, 50.0, 5000.0, 500.0);
	params[POST].setSkewFactor(0.5);
	params[POST].setStep(1.0);
	params[POST].setUnitSuffix(" Hz");
	params[INGAIN].init(parameterNames[INGAIN], UnitGeneric, "Changes the distortin ammount",
						1.0, 0.0, 15.0, 1.0);
	params[OUTGAIN].init(parameterNames[OUTGAIN], UnitGeneric, "Changes the output level",
						 1.0, 0.0, 2.0, 1.0);
	params[COLOUR].init(parameterNames[COLOUR], UnitGeneric, "Changes the colour of the distortion",
						0.1, 0.0, 1.0, 0.1);
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

	updateFilters();
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

	updateFilters();
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

const String DRowAudioFilter::getParameterSuffix (int index)
{
	for (int i = 0; i < noParams; i++)
		if (index == i)
			return String(params[i].getUnitSuffix());

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
	oneOverCurrentSampleRate = 1.0/currentSampleRate;

	// update the filters
	fPreCf = params[PRE].getSmoothedValue();
	fPostCf = params[POST].getSmoothedValue();
	updateFilters();
}

void DRowAudioFilter::releaseResources()
{
}

void DRowAudioFilter::processBlock (AudioSampleBuffer& buffer,
                                   MidiBuffer& midiMessages)
{
	smoothParameters();

	const int numInputChannels = getTotalNumInputChannels();


	// create parameters to use
	fPreCf = params[PRE].getSmoothedValue();
	fPostCf = params[POST].getSmoothedValue();
	float fInGain = params[INGAIN].getSmoothedValue();
	float fOutGain = params[OUTGAIN].getSmoothedValue();
	float fColour = 100 * params[COLOUR].getSmoothedNormalisedValue();

	// set up array of pointers to samples
	const int numSamples = buffer.getNumSamples();
	int samplesLeft = numSamples;
	float* pfSample[numInputChannels];
	for (int channel = 0; channel < numInputChannels; channel++)
		pfSample[channel] = buffer.getWritePointer(channel);


	if (numInputChannels == 2)
	{
		// input filter the samples
		inFilterL.processSamples(pfSample[0], numSamples);
		inFilterR.processSamples(pfSample[1], numSamples);

		//========================================================================
		while (--samplesLeft >= 0)
		{
			// distort
			*pfSample[0] *= fInGain;
			*pfSample[1] *= fInGain;

			// shape (using the limit of tanh is 1 so no cliping required)
			*pfSample[0] = tanh(*pfSample[0] * fColour);
			*pfSample[1] = tanh(*pfSample[1] * fColour);

			// apply output gain
			*pfSample[0] *= fOutGain;
			*pfSample[1] *= fOutGain;

			// incriment sample pointers
			pfSample[0]++;
			pfSample[1]++;
		}
		//========================================================================

		// output filter the samples
		outFilterL.processSamples(buffer.getWritePointer(0), numSamples);
		outFilterR.processSamples(buffer.getWritePointer(1), numSamples);
	}
	else if (numInputChannels == 1)
	{
		// input filter the samples
		inFilterL.processSamples(pfSample[0], numSamples);

		//========================================================================
		while (--samplesLeft >= 0)
		{
			// distort
			*pfSample[0] *= fInGain;

			// shape (using the limit of tanh is 1 so no cliping required)
			*pfSample[0] = tanh(*pfSample[0] * fColour);

			// apply output gain
			*pfSample[0] *= fOutGain;

			// incriment sample pointers
			pfSample[0]++;
		}
		//========================================================================

		// output filter the samples
		outFilterL.processSamples(buffer.getWritePointer(0), numSamples);
	}


    // clear any output channels that didn't contain input data
    for (int i = numInputChannels; i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);
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

            updateFilters ();
            sendChangeMessage ();
        }

        delete xmlState;
    }
}

//===============================================================================
void DRowAudioFilter::updateFilters()
{
	inFilterL.makeLowPass(currentSampleRate, fPreCf);
	inFilterR.makeLowPass(currentSampleRate, fPreCf);
	outFilterL.makeLowPass(currentSampleRate, fPostCf);
	outFilterR.makeLowPass(currentSampleRate, fPostCf);
}


void DRowAudioFilter::updateParameters()
{
}
