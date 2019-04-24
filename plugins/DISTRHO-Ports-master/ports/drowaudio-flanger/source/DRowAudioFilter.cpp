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
    : pfCircularBufferL(nullptr),
      pfCircularBufferR(nullptr),
      pfLookupTable(nullptr)
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
    return "dRowAudio: Flanger";
}

void DRowAudioFilter::setupParams()
{
	params[RATE].init(parameterNames[RATE], UnitHertz, "Changes the rate",
					  0.5, 0.0, 20.0, 0.5);
	params[RATE].setSkewFactor(0.5f);
	params[RATE].setSmoothCoeff(0.1);
	params[RATE].setUnitSuffix(" Hz");
	params[DEPTH].init(parameterNames[DEPTH], UnitPercent, "Changes the depth",
					   20.0, 0.0, 100.0, 20.0);
	params[DEPTH].setSkewFactor(0.7f);
	params[DEPTH].setSmoothCoeff(0.1);
	params[DEPTH].setUnitSuffix(" %");
	params[FEEDBACK].init(parameterNames[FEEDBACK], UnitPercent, "Changes the feedback ammount",
						  0.0, 0.0, 99.0, 0.0);
	params[FEEDBACK].setStep(1);
	params[FEEDBACK].setUnitSuffix(" %");
	params[MIX].init(parameterNames[MIX], UnitPercent, "Changes the output mix",
					 100.0, 0.0, 100.0, 100.0);
	params[MIX].setStep(1);
	params[MIX].setUnitSuffix(" %");
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
	oneOverCurrentSampleRate = 1.0f/currentSampleRate;

	// set up wave buffer and fill with triangle data
	iLookupTableSize = 8192;
	iLookupTableSizeMask =  iLookupTableSize-1;

	pfLookupTable = new float[iLookupTableSize];
	float fPhaseStep = (2 * double_Pi) / iLookupTableSize;
	for(int i = 0; i < iLookupTableSize; i++){
		if(i < iLookupTableSize * 0.5)
			pfLookupTable[i] = -1.0f + (2.0/double_Pi)*i*fPhaseStep;
		else
			pfLookupTable[i] = 3.0f - (2.0/double_Pi)*i*fPhaseStep;
	}
	iLookupTablePos = 0;
	iSamplesProcessed = 0;


	// set up circular buffers
	iBufferSize = (int)sampleRate;
	pfCircularBufferL = new float[iBufferSize];
	for (int i = 0; i < iBufferSize; i++)
		pfCircularBufferL[i] = 0;

	if (getTotalNumInputChannels() == 2) {
		pfCircularBufferR = new float[iBufferSize];
		for (int i = 0; i < iBufferSize; i++)
			pfCircularBufferR[i] = 0;
	}
	iBufferWritePos = 0;

	updateFilters();
}

void DRowAudioFilter::releaseResources()
{
        if (pfLookupTable != nullptr)
            delete[] pfLookupTable;
        if (pfCircularBufferL != nullptr)
            delete[] pfCircularBufferL;
	if (getTotalNumInputChannels() == 2 && pfCircularBufferR != nullptr)
		delete[] pfCircularBufferR;
}

void DRowAudioFilter::processBlock (AudioSampleBuffer& buffer,
									MidiBuffer& midiMessages)
{
	smoothParameters();

	const int numInputChannels = getTotalNumInputChannels();

	// create parameters to use
	float fRate = params[RATE].getSmoothedValue();
	float fDepth = (params[DEPTH].getSmoothedNormalisedValue() * 0.006f) + 0.0001f;
	float fFeedback = params[FEEDBACK].getSmoothedNormalisedValue();
	float fWetDryMix = params[MIX].getSmoothedNormalisedValue();

	// calculate current phase step
	float phaseStep = iLookupTableSize * oneOverCurrentSampleRate * fRate;

	// set up array of pointers to samples
	int numSamples = buffer.getNumSamples();
	float* pfSample[numInputChannels];
	for (int channel = 0; channel < numInputChannels; channel++)
		pfSample[channel] = buffer.getWritePointer(channel);

	if (numInputChannels == 2)
	{
		//========================================================================
		while (--numSamples >= 0)
		{
			int index =  (int)(iSamplesProcessed * phaseStep) &	iLookupTableSizeMask;

			iSamplesProcessed++;
			float fOsc = (pfLookupTable[index] * fDepth) + fDepth;

			float fBufferReadPos1 = (iBufferWritePos - (fOsc * iBufferSize));
			if (fBufferReadPos1 < 0)
				fBufferReadPos1 += iBufferSize;

			// read values from buffers
			int iPos1, iPos2;
			float fDiff, fDelL, fDelR;

			iPos1 = (int)fBufferReadPos1;
			iPos2 = iPos1 + 1;
			if (iPos2 == iBufferSize)
				iPos2 = 0;
			fDiff = fBufferReadPos1 - iPos1;
			fDelL = pfCircularBufferL[iPos2]*fDiff + pfCircularBufferL[iPos1]*(1-fDiff);
			fDelR = pfCircularBufferR[iPos2]*fDiff + pfCircularBufferR[iPos1]*(1-fDiff);

			// store current samples in buffers
			iBufferWritePos++;
			if (iBufferWritePos >= iBufferSize)
				iBufferWritePos = 0;
			pfCircularBufferL[iBufferWritePos] = *pfSample[0] + (fFeedback * fDelL);
			pfCircularBufferR[iBufferWritePos] = *pfSample[1] + (fFeedback * fDelR);

			// calculate output samples
			float fOutL = 0.5f * (*pfSample[0] + fWetDryMix*fDelL);
			float fOutR = 0.5f * (*pfSample[1] + fWetDryMix*fDelR);

			*pfSample[0] = fOutL;
			*pfSample[1] = fOutR;

			// incriment sample pointers
			pfSample[0]++;
			pfSample[1]++;
		}
		//========================================================================
	}
	else if (numInputChannels == 1)
	{
		//========================================================================
		while (--numSamples >= 0)
		{
			int index =  (int)(iSamplesProcessed * phaseStep) &	iLookupTableSizeMask;

			iSamplesProcessed++;
			float fOsc = (pfLookupTable[index] * fDepth) + fDepth;

			float fBufferReadPos1 = (iBufferWritePos - (fOsc * iBufferSize));
			if (fBufferReadPos1 < 0)
				fBufferReadPos1 += iBufferSize;

			// read values from buffers
			int iPos1, iPos2;
			float fDiff, fDelL;

			iPos1 = (int)fBufferReadPos1;
			iPos2 = iPos1 + 1;
			if (iPos2 == iBufferSize)
				iPos2 = 0;
			fDiff = fBufferReadPos1 - iPos1;
			fDelL = pfCircularBufferL[iPos2]*fDiff + pfCircularBufferL[iPos1]*(1-fDiff);

			// store current samples in buffers
			iBufferWritePos++;
			if (iBufferWritePos >= iBufferSize)
				iBufferWritePos = 0;
			pfCircularBufferL[iBufferWritePos] = *pfSample[0] + (fFeedback * fDelL);

			// calculate output samples
			float fOutL = 0.5f * (*pfSample[0] + fWetDryMix*fDelL);

			*pfSample[0] = fOutL;

			// incriment sample pointers
			pfSample[0]++;
		}
		//========================================================================
	}

    // in case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = numInputChannels; i < getTotalNumOutputChannels(); ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
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

            sendChangeMessage ();
        }

        delete xmlState;
    }
}

//===============================================================================
void DRowAudioFilter::updateFilters()
{

}


void DRowAudioFilter::updateParameters()
{
}
