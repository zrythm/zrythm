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
#include "TalComponent.h"
#include "TalCore.h"
#include "ProgramChunk.h"
#include "math.h"

/**
    This function must be implemented to create a new instance of your
    plugin object.
*/
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TalCore();
}

TalCore::TalCore()
{
	// init engine
	if (this->getSampleRate() > 0)
    {
		sampleRate = (float)this->getSampleRate();
    }
	else
    {
		sampleRate = 44100.0f;
    }

	engine = new Engine(sampleRate);
	talPresets = new TalPreset*[NUMPROGRAMS];

    // load factory presets
    talPresets = new TalPreset*[NUMPROGRAMS];
    for (int i = 0; i < NUMPROGRAMS; i++) talPresets[i] = new TalPreset();
    curProgram = 0;
    loadingProgram = false;

    ProgramChunk chunk;
    setStateInformationString(chunk.getXmlChunk());
    setCurrentProgram(curProgram);
}

TalCore::~TalCore()
{
	if (talPresets) delete[] talPresets;
	if (engine) delete engine;
}

const String TalCore::getName() const
{
    return "Tal-Filter-2";
}

int TalCore::getNumParameters()
{
	return (int)NUMPARAM;
}

float TalCore::getParameter (int index)
{
    if (index >= NUMPARAM)
        return 0.0f;

    float value = talPresets[curProgram]->programData[index];

    switch (index)
    {
    case SPEEDFACTOR:
        value = (value-1.0f)/6.0f;
        break;
    case FILTERTYPE:
        value = (value-1.0f)/9.0f;
        break;
    }

    return value;
}

void TalCore::setParameter (int index, float newValue)
{
    if (index >= NUMPARAM)
        return;

    switch (index)
    {
    case SPEEDFACTOR:
        if (! loadingProgram) newValue = newValue * 6.0f + 1.0f;
        engine->setSpeedFactor((int)newValue);
        break;
    case FILTERTYPE:
        if (! loadingProgram) newValue = newValue * 9.0f + 1.0f;
        engine->setFilterType((int)newValue);
        break;
    case RESONANCE:
        engine->setResonance(newValue);
        break;
    case VOLUMEIN:
        engine->setVolumeIn(newValue);
        break;
    case VOLUMEOUT:
        engine->setVolumeOut(newValue);
        break;
    case DEPTH:
        engine->setDepth(newValue);
        break;
    }

    talPresets[curProgram]->programData[index] = newValue;
    sendChangeMessage ();
}

const String TalCore::getParameterName (int index)
{
	switch(index)
	{
		case SPEEDFACTOR: return T("speedfactor");
		case FILTERTYPE: return T("filtertype");
		case RESONANCE: return T("resonance");
		case VOLUMEIN: return T("volumein");
		case VOLUMEOUT: return T("volumeout");
		case DEPTH: return T("depth");
	}
    return String();
}

const String TalCore::getParameterText (int index)
{
	if (index < NUMPARAM)
	{
		return String(talPresets[curProgram]->programData[index], 2);
	}
    return String();
}

const String TalCore::getInputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

const String TalCore::getOutputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

bool TalCore::isInputChannelStereoPair (int index) const
{
    return true;
}

bool TalCore::isOutputChannelStereoPair (int index) const
{
    return true;
}

bool TalCore::acceptsMidi() const
{
    return false;
}

bool TalCore::producesMidi() const
{
    return false;
}

void TalCore::prepareToPlay (double sampleRate, int samplesPerBlock)
{
}

void TalCore::releaseResources()
{
    // when playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void TalCore::processBlock (AudioSampleBuffer& buffer,
                                   MidiBuffer& midiMessages)
{
	// Change sample rate if required
	if (sampleRate != this->getSampleRate())
	{
		sampleRate = (float)this->getSampleRate();
		engine->setSampleRate(sampleRate);
		setCurrentProgram(curProgram);
	}

    if (getPlayHead() != 0 && getPlayHead()->getCurrentPosition(pos))
    {
        if (pos.bpm > 0)
        {
            this->bpm = (float)pos.bpm;
            this->engine->sync(this->bpm, pos);
        }
    }
    else
    {
        this->bpm = 120.0f;
    }

    const ScopedLock sl (this->getCallbackLock());

    // for each of our input channels, we'll attenuate its level by the
    // amount that our volume parameter is set to.
	int numberOfChannels = getTotalNumInputChannels();
	//int bufferSize = buffer.getNumSamples();

	if (numberOfChannels == 2)
	{
		float *samples0 = buffer.getWritePointer(0, 0);
		float *samples1 = buffer.getWritePointer(1, 0);

		int samplePos = 0;
		int numSamples = buffer.getNumSamples();
		while (numSamples > 0)
		{
			engine->process(samples0++, samples1++);
			numSamples--;
			samplePos++;
		}
	}
	if (numberOfChannels == 1)
	{
		float *samples0 = buffer.getWritePointer(0, 0);
		float *samples1 = buffer.getWritePointer(0, 0);

		int samplePos = 0;
		int numSamples = buffer.getNumSamples();
		while (numSamples > 0)
		{
			engine->process(samples0++, samples1++);
			numSamples--;
			samplePos++;
		}
	}
    // in case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
}

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
AudioProcessorEditor* TalCore::createEditor()
{
    return new TalComponent (this);
}
#endif

void TalCore::getStateInformation (MemoryBlock& destData)
{
    // header
    XmlElement tal("tal");
    tal.setAttribute (T("curprogram"), curProgram);
    tal.setAttribute (T("version"), 1.0);

    // programs
    XmlElement *programList = new XmlElement ("programs");
    for (int i = 0; i < NUMPROGRAMS; i++)
    {
        getXmlPrograms(programList, i);
    }
    tal.addChildElement(programList);

    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (tal, destData);

    // use this for new factory presets
    //#ifdef _DEBUG && WIN32
    //File *file = new File("e:/presets.txt");
    //String myXmlDoc = tal.createDocument ("presets.txt");
    //file->replaceWithText(myXmlDoc);
    //#endif
}

void TalCore::setStateInformation (const void* data, int sizeInBytes)
{
    // use this helper function to get the XML from this binary blob..
    XmlElement* const xmlState = getXmlFromBinary (data, sizeInBytes);
    setStateInformationFromXml(xmlState);
}

void TalCore::setStateInformationFromXml(XmlElement* xmlState)
{
    curProgram = 0;
    if (xmlState != 0 && xmlState->hasTagName(T("tal")))
    {
        curProgram = (int)xmlState->getIntAttribute (T("curprogram"), 1);
        float version = (float)xmlState->getDoubleAttribute (T("version"), 1.0);

        XmlElement* programs = xmlState->getFirstChildElement();
        if (programs->hasTagName(T("programs")))
        {
            int programNumber = 0;
            forEachXmlChildElement (*programs, e)
            {
                this->setXmlPrograms(e, programNumber, version);
                programNumber++;
            }
        }

        delete xmlState;
        this->setCurrentProgram(curProgram);
        this->sendChangeMessage ();
    }
}

void TalCore::getXmlPrograms(XmlElement *programList, int programNumber)
{
        XmlElement* program = new XmlElement ("program");
        program->setAttribute (T("programname"), talPresets[programNumber]->name);
        program->setAttribute (T("speedFactor"), talPresets[programNumber]->programData[SPEEDFACTOR]);
        program->setAttribute (T("resonance"), talPresets[programNumber]->programData[RESONANCE]);
        program->setAttribute (T("filtertype"), talPresets[programNumber]->programData[FILTERTYPE]);
        program->setAttribute (T("volumein"), talPresets[programNumber]->programData[VOLUMEIN]);
        program->setAttribute (T("volumeout"), talPresets[programNumber]->programData[VOLUMEOUT]);
        program->setAttribute (T("depth"), talPresets[programNumber]->programData[DEPTH]);

        EnvelopePresetUtility utility;
        utility.addEnvelopeDataToXml(talPresets[programNumber]->getPoints(), program);

        programList->addChildElement(program);
}

void TalCore::setXmlPrograms(XmlElement* program, int programNumber, float version)
{
    if (program->hasTagName(T("program")) && programNumber < NUMPROGRAMS)
    {
        talPresets[programNumber]->name = program->getStringAttribute (T("programname"), T("Not Saved") + programNumber);
        talPresets[programNumber]->programData[SPEEDFACTOR] = (float)program->getDoubleAttribute (T("speedFactor"), 1.0f);
        talPresets[programNumber]->programData[RESONANCE] = (float)program->getDoubleAttribute (T("resonance"), 0.0f);
        talPresets[programNumber]->programData[FILTERTYPE] = (float)program->getDoubleAttribute (T("filtertype"), 1.0f);
        talPresets[programNumber]->programData[VOLUMEIN] = (float)program->getDoubleAttribute (T("volumein"), 0.5f);
        talPresets[programNumber]->programData[VOLUMEOUT] = (float)program->getDoubleAttribute (T("volumeout"), 0.5f);
        talPresets[programNumber]->programData[DEPTH] = (float)program->getDoubleAttribute (T("depth"), 1.0f);

        EnvelopePresetUtility utility;
        Array<SplinePoint*> splinePoints = utility.getEnvelopeFromXml(program);
        talPresets[programNumber]->setPoints(splinePoints);
    }
}

String TalCore::getStateInformationString ()
{
    // header
    XmlElement tal("tal");
    tal.setAttribute (T("curprogram"), curProgram);
    tal.setAttribute (T("version"), 1.0);

    // programs
    XmlElement *programList = new XmlElement ("programs");
    for (int i = 0; i < NUMPROGRAMS; i++)
    {
        getXmlPrograms(programList, i);
    }
    tal.addChildElement(programList);

    return tal.createDocument (String());
}

void TalCore::setStateInformationString (const String& data)
{
    XmlElement* const xmlState = XmlDocument::parse(data);
    setStateInformationFromXml(xmlState);
}

int TalCore::getNumPrograms ()
{
	return NUMPROGRAMS;
}

int TalCore::getCurrentProgram ()
{
	return curProgram;
}

void TalCore::setCurrentProgram (int index)
{
	if (index < NUMPROGRAMS)
	{
		this->curProgram = index;

        this->engine->setPoints(talPresets[index]->getPoints());

                loadingProgram = true;
		for (int i = 0; i < NUMPARAM; i++)
			this->setParameter(i, talPresets[index]->programData[i]);
                loadingProgram = false;

		sendChangeMessage ();
	}
}

const String TalCore::getProgramName (int index)
{
	if (index < NUMPROGRAMS)
		return talPresets[index]->name;
	else
		return T("Invalid");
}

void TalCore::changeProgramName (int index, const String& newName)
{
	if (index < NUMPROGRAMS)
		talPresets[index]->name = newName;
}

EnvelopeEditor* TalCore::getEnvelopeEditor()
{
    return this->engine->getEnvelopeEditor();
}

void TalCore::envelopeChanged()
{
    this->talPresets[curProgram]->setPoints(this->engine->getEnvelopeEditor()->getPoints());
}
