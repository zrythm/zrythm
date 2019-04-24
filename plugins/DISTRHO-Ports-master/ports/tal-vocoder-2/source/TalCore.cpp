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
#include "ProgramChunk.h"

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
    this->numberOfPrograms = 10;

	// init engine
	if (this->getSampleRate() > 0)
    {
		sampleRate = (float)this->getSampleRate();
    }
	else
    {
		sampleRate = 44100.0f;
    }

	this->engine = new VocoderEngine(sampleRate);
	this->talPresets = new TalPreset*[this->numberOfPrograms];

	for (int i = 0; i < this->numberOfPrograms; i++)
    {
        talPresets[i] = new TalPreset();
    }

	curProgram = 0;

	// load factory presets
	ProgramChunk chunk;
	setStateInformationString(chunk.getXmlChunk());
	setCurrentProgram(curProgram);

    nextMidiMessage = new MidiMessage(0xF0);
    midiMessage = new MidiMessage(0xF0);
}

TalCore::~TalCore()
{
	if (talPresets) delete[] talPresets;
	if (engine) delete engine;

    delete nextMidiMessage;
    delete midiMessage;
}

const String TalCore::getName() const
{
    return "Tal-Vocoder-II";
}

int TalCore::getNumParameters()
{
	return (int)NUMPARAM;
}

float TalCore::getParameter (int index)
{
	if (index < NUMPARAM)
		return talPresets[curProgram]->programData[index];
	else
		return 0;
}

void TalCore::setParameter (int index, float newValue)
{
	if (index < NUMPARAM)
	{
		talPresets[curProgram]->programData[index] = newValue;

		switch(index)
		{
			case TAL_VOLUME:
				engine->setVolume(newValue);
				break;
			case NOISEVOLUME:
				engine->setNoiseVolume(newValue);
				break;
			case PULSEVOLUME:
				engine->setPulseVolume(newValue);
				break;
			case SAWVOLUME:
				engine->setSawVolume(newValue);
				break;
			case SUBOSCVOLUME:
				engine->setSubOscVolume(newValue);
				break;
            case PORTAMENTO:
                engine->setPortamentoIntensity(newValue);
                break;
            case TAL_TUNE:
                engine->setTune(newValue);
                break;
            case PULSEFINETUNE:
                engine->setPulseFineTune(newValue);
                break;
            case PULSETUNE:
                engine->setPulseTune(newValue);
                break;
            case SAWFINETUNE:
                engine->setSawFineTune(newValue);
                break;
            case SAWTUNE:
                engine->setSawTune(newValue);
                break;
            case ESSERINTENSITY:
                engine->setEsserIntensity(newValue);
                break;
            case OSCTRANSPOSE:
                engine->setTranspose(newValue);
                break;
            case OSCSYNC:
                engine->setOscSync(newValue > 0.0f);
                break;
            case HARMONICS:
                engine->setHarmonicIntensity(newValue);
                break;

            case ENVELOPERELEASE:
				engine->setEnvelopeRelease(newValue);
				break;
            case POLYMODE:
				engine->setPolyMode(newValue > 0.0f);
				break;
            case INPUTMODE:
                engine->setInputMode(newValue > 0.0f);
				break;
            case CHORUS:
                engine->setChorus(newValue > 0.0f);
				break;
            case PANIC:
                engine->reset();
				break;

            case VOCODERBAND00:
				engine->setVocoderBandVolume(newValue, 0);
				break;
            case VOCODERBAND01:
				engine->setVocoderBandVolume(newValue, 1);
				break;
            case VOCODERBAND02:
				engine->setVocoderBandVolume(newValue, 2);
				break;
            case VOCODERBAND03:
				engine->setVocoderBandVolume(newValue, 3);
				break;
            case VOCODERBAND04:
				engine->setVocoderBandVolume(newValue, 4);
				break;
            case VOCODERBAND05:
				engine->setVocoderBandVolume(newValue, 5);
				break;
            case VOCODERBAND06:
				engine->setVocoderBandVolume(newValue, 6);
				break;
            case VOCODERBAND07:
				engine->setVocoderBandVolume(newValue, 7);
				break;
            case VOCODERBAND08:
				engine->setVocoderBandVolume(newValue, 8);
				break;
            case VOCODERBAND09:
				engine->setVocoderBandVolume(newValue, 9);
				break;
            case VOCODERBAND10:
				engine->setVocoderBandVolume(newValue, 10);
				break;
		}

		sendChangeMessage();
	}
}

const String TalCore::getParameterName (int index)
{
	switch(index)
	{
		case TAL_VOLUME: return T("TAL_VOLUME");
		case HARMONICS: return T("HARMONICS");
		case NOISEVOLUME: return T("NOISEVOLUME");
		case PULSEVOLUME: return T("PULSEVOLUME");
		case SAWVOLUME: return T("SAWVOLUME");
		case SUBOSCVOLUME: return T("SUBOSCVOLUME");
		case OSCTRANSPOSE: return T("OSCTRANSPOSE");
		case SUBOSCOCTAVE: return T("SUBOSCOCTAVE");
		case POLYMODE: return T("POLYMODE");
		case PORTAMENTO: return T("PORTAMENTO");
		case TAL_TUNE: return T("TAL_TUNE");
		case PANIC: return T("PANIC");
		case INPUTMODE: return T("INPUTMODE");
		case CHORUS: return T("CHORUS");
		case ENVELOPERELEASE: return T("ENVELOPERELEASE");
		case PULSETUNE: return T("PULSETUNE");
		case SAWTUNE: return T("SAWTUNE");
		case PULSEFINETUNE: return T("PULSEFINETUNE");
		case SAWFINETUNE: return T("SAWFINETUNE");
		case ESSERINTENSITY: return T("ESSERINTENSITY");
		case OSCSYNC: return T("OSCSYNC");

		case VOCODERBAND00: return T("VOCODERBAND00");
		case VOCODERBAND01: return T("VOCODERBAND01");
		case VOCODERBAND02: return T("VOCODERBAND02");
		case VOCODERBAND03: return T("VOCODERBAND03");
		case VOCODERBAND04: return T("VOCODERBAND04");
		case VOCODERBAND05: return T("VOCODERBAND05");
		case VOCODERBAND06: return T("VOCODERBAND06");
		case VOCODERBAND07: return T("VOCODERBAND07");
		case VOCODERBAND08: return T("VOCODERBAND08");
		case VOCODERBAND09: return T("VOCODERBAND09");
		case VOCODERBAND10: return T("VOCODERBAND10");

		case UNUSED: return "unused";
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

const String TalCore::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String TalCore::getOutputChannelName (int channelIndex) const
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
    return true;
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

void TalCore::setNewSampleRate()
{
    //const ScopedLock sl (myCriticalSectionBuffer);
	sampleRate = (float)this->getSampleRate();
    engine->setSampleRate(sampleRate);
}

void TalCore::processBlock (AudioSampleBuffer& buffer,
                                   MidiBuffer& midiMessages)
{
	// Change sample rate if required
	if (sampleRate != this->getSampleRate())
	{
        this->setNewSampleRate();
		setCurrentProgram(curProgram);
	}
    const ScopedLock sl (this->getCallbackLock());

    // for each of our input channels, we'll attenuate its level by the
    // amount that our volume parameter is set to.
	int numberOfChannels = getTotalNumInputChannels();
	//int bufferSize = buffer.getNumSamples();

	// midi buffer
    MidiBuffer::Iterator midiIterator(midiMessages);
    hasMidiMessage = midiIterator.getNextEvent(*nextMidiMessage, midiEventPos);

	if (numberOfChannels == 2)
	{
		float *samples0 = buffer.getWritePointer(0, 0);
		float *samples1 = buffer.getWritePointer(1, 0);

		int samplePos = 0;
		int numSamples = buffer.getNumSamples();
		while (numSamples > 0)
		{
			processMidiPerSample(&midiIterator, samplePos);
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
			processMidiPerSample(&midiIterator, samplePos);
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

void TalCore::processMidiPerSample(MidiBuffer::Iterator *midiIterator, int samplePos)
{
    // There can be more than one event at the same position
    while (this->getNextEvent(midiIterator, samplePos))
    {
        if (midiMessage->isController())
        {
        }
        else if (midiMessage->isNoteOn())
        {
            engine->setNoteOn(midiMessage->getNoteNumber(), midiMessage->getFloatVelocity());
        }
        else if (midiMessage->isNoteOff())
        {
            engine->setNoteOff(midiMessage->getNoteNumber());
        }
        else if (midiMessage->isPitchWheel())
        {
            // [0..16383] center = 8192;
            // engine->setPitchwheelAmount((midiMessage->getPitchWheelValue() - 8192.0f) / (16383.0f * 0.5f));
        }
        else if (midiMessage->isAllNotesOff())
        {
            engine->reset();
        }
    }
}

inline bool TalCore::getNextEvent(MidiBuffer::Iterator *midiIterator, const int samplePos)
{
    if (hasMidiMessage && midiEventPos <= samplePos)
    {
        *midiMessage = *nextMidiMessage;
        hasMidiMessage = midiIterator->getNextEvent(*nextMidiMessage, midiEventPos);
        return true;
    }
    return false;
}

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
AudioProcessorEditor* TalCore::createEditor()
{
    return new TalComponent(this);
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
    for (int i = 0; i < this->numberOfPrograms; i++)
    {
        getXmlPrograms(programList, i);
    }
    tal.addChildElement(programList);

    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (tal, destData);
}


void TalCore::getCurrentProgramStateInformation (MemoryBlock& destData)
{
    // header
    XmlElement tal("tal");
    tal.setAttribute (T("curprogram"), curProgram);
    tal.setAttribute (T("version"), 1.0);

    // programs
    XmlElement *programList = new XmlElement ("programs");

    getXmlPrograms(programList, this->curProgram);
    tal.addChildElement(programList);

    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (tal, destData);
}

XmlElement* TalCore::getCurrentProgramStateInformationAsXml()
{
    MemoryBlock destData;
    this->getCurrentProgramStateInformation(destData);
    return this->getXmlFromBinary(destData.getData(), destData.getSize());
}

void TalCore::setCurrentProgramStateInformation (const void* data, int sizeInBytes)
{
    XmlElement* const xmlState = getXmlFromBinary (data, sizeInBytes);
    setStateInformationFromXml(xmlState);
}

void TalCore::setStateInformation (const void* data, int sizeInBytes)
{
    // use this helper function to get the XML from this binary blob..
    XmlElement* const xmlState = getXmlFromBinary(data, sizeInBytes);
    setStateInformationFromXml(xmlState);
}

void TalCore::setStateInformationFromXml(XmlElement* xmlState)
{
    if (xmlState != 0 && xmlState->hasTagName(T("tal")))
    {
        float version = (float)xmlState->getDoubleAttribute (T("version"), 1);

        XmlElement* programs = xmlState->getFirstChildElement();
        if (programs->hasTagName(T("programs")))
        {
            int programNumber = 0;

            if (programs->getNumChildElements() != 1)
            {
                curProgram = (int)xmlState->getIntAttribute (T("curprogram"), 1);

                forEachXmlChildElement (*programs, e)
                {
                    this->setXmlPrograms(e, programNumber, version);
                    programNumber++;
                }
            }
            else
            {
                this->setXmlPrograms(programs->getFirstChildElement(), curProgram, version);
            }
        }

        delete xmlState;
        this->setCurrentProgram(curProgram);
        this->sendChangeMessage();
    }
}

void TalCore::getXmlPrograms(XmlElement *programList, int programNumber)
{
	XmlElement* program = new XmlElement ("program");
	program->setAttribute (T("programname"), talPresets[programNumber]->name);

    program->setAttribute(T("volume"), talPresets[programNumber]->programData[TAL_VOLUME]);
    program->setAttribute(T("harmonics"), talPresets[programNumber]->programData[HARMONICS]);
    program->setAttribute(T("noisevolume"), talPresets[programNumber]->programData[NOISEVOLUME]);
    program->setAttribute(T("pulsevolume"), talPresets[programNumber]->programData[PULSEVOLUME]);
    program->setAttribute(T("sawvolume"), talPresets[programNumber]->programData[SAWVOLUME]);
    program->setAttribute(T("suboscvolume"), talPresets[programNumber]->programData[SUBOSCVOLUME]);
    program->setAttribute(T("osctranspose"), talPresets[programNumber]->programData[OSCTRANSPOSE]);
    program->setAttribute(T("polymode"), talPresets[programNumber]->programData[POLYMODE]);
    program->setAttribute(T("portamento"), talPresets[programNumber]->programData[PORTAMENTO]);
    program->setAttribute(T("tune"), talPresets[programNumber]->programData[TAL_TUNE]);
    program->setAttribute(T("inputmode"), talPresets[programNumber]->programData[INPUTMODE]);
    program->setAttribute(T("chorus"), talPresets[programNumber]->programData[CHORUS]);
    program->setAttribute(T("enveloperelease"), talPresets[programNumber]->programData[ENVELOPERELEASE]);
    program->setAttribute(T("oscsync"), talPresets[programNumber]->programData[OSCSYNC]);

    program->setAttribute(T("pulsetune"), talPresets[programNumber]->programData[PULSETUNE]);
    program->setAttribute(T("sawtune"), talPresets[programNumber]->programData[SAWTUNE]);
    program->setAttribute(T("pulsefinetune"), talPresets[programNumber]->programData[PULSEFINETUNE]);
    program->setAttribute(T("sawfinetune"), talPresets[programNumber]->programData[SAWFINETUNE]);
    program->setAttribute(T("esserintensity"), talPresets[programNumber]->programData[ESSERINTENSITY]);

    program->setAttribute(T("vocoderband00"), talPresets[programNumber]->programData[VOCODERBAND00]);
    program->setAttribute(T("vocoderband01"), talPresets[programNumber]->programData[VOCODERBAND01]);
    program->setAttribute(T("vocoderband02"), talPresets[programNumber]->programData[VOCODERBAND02]);
    program->setAttribute(T("vocoderband03"), talPresets[programNumber]->programData[VOCODERBAND03]);
    program->setAttribute(T("vocoderband04"), talPresets[programNumber]->programData[VOCODERBAND04]);
    program->setAttribute(T("vocoderband05"), talPresets[programNumber]->programData[VOCODERBAND05]);
    program->setAttribute(T("vocoderband06"), talPresets[programNumber]->programData[VOCODERBAND06]);
    program->setAttribute(T("vocoderband07"), talPresets[programNumber]->programData[VOCODERBAND07]);
    program->setAttribute(T("vocoderband08"), talPresets[programNumber]->programData[VOCODERBAND08]);
    program->setAttribute(T("vocoderband09"), talPresets[programNumber]->programData[VOCODERBAND09]);
    program->setAttribute(T("vocoderband10"), talPresets[programNumber]->programData[VOCODERBAND10]);

	programList->addChildElement(program);
}

void TalCore::setXmlPrograms(XmlElement* e, int programNumber, float version)
{
    //File *file = new File("e:/wired.txt");
    if (e->hasTagName(T("program")) && programNumber < this->numberOfPrograms)
    {
        ///file->appendText("start");
	    talPresets[programNumber]->name = e->getStringAttribute(T("programname"), T("Not Saved"));
        talPresets[programNumber]->programData[TAL_VOLUME] = (float)e->getDoubleAttribute(T("volume"), 0.5f);
        talPresets[programNumber]->programData[HARMONICS] = (float)e->getDoubleAttribute(T("harmonics"), 0.5f);
        talPresets[programNumber]->programData[NOISEVOLUME] = (float)e->getDoubleAttribute(T("noisevolume"), 0.5f);
        talPresets[programNumber]->programData[PULSEVOLUME] = (float)e->getDoubleAttribute(T("pulsevolume"), 0.5f);
        talPresets[programNumber]->programData[SAWVOLUME] = (float)e->getDoubleAttribute(T("sawvolume"), 0.5f);
        talPresets[programNumber]->programData[SUBOSCVOLUME] = (float)e->getDoubleAttribute(T("suboscvolume"), 0.5f);
        talPresets[programNumber]->programData[OSCTRANSPOSE] = (float)e->getDoubleAttribute(T("osctranspose"), 0.5f);
        talPresets[programNumber]->programData[POLYMODE] = (float)e->getDoubleAttribute(T("polymode"), 0.5f);
        talPresets[programNumber]->programData[PORTAMENTO] = (float)e->getDoubleAttribute(T("portamento"), 0.5f);
        talPresets[programNumber]->programData[TAL_TUNE] = (float)e->getDoubleAttribute(T("tune"), 0.5f);
        talPresets[programNumber]->programData[INPUTMODE] = (float)e->getDoubleAttribute(T("inputmode"), 0.5f);
        talPresets[programNumber]->programData[CHORUS] = (float)e->getDoubleAttribute(T("chorus"), 0.0f);
        talPresets[programNumber]->programData[ENVELOPERELEASE] = (float)e->getDoubleAttribute(T("enveloperelease"), 0.5f);
        talPresets[programNumber]->programData[OSCSYNC] = (float)e->getDoubleAttribute(T("oscsync"), 0.0f);

        talPresets[programNumber]->programData[PULSETUNE] = (float)e->getDoubleAttribute(T("pulsetune"), 0.5f);
        talPresets[programNumber]->programData[SAWTUNE] = (float)e->getDoubleAttribute(T("sawtune"), 0.5f);
        talPresets[programNumber]->programData[PULSEFINETUNE] = (float)e->getDoubleAttribute(T("pulsefinetune"), 0.5f);
        talPresets[programNumber]->programData[SAWFINETUNE] = (float)e->getDoubleAttribute(T("sawfinetune"), 0.5f);
        talPresets[programNumber]->programData[ESSERINTENSITY] = (float)e->getDoubleAttribute(T("esserintensity"), 0.5f);

        talPresets[programNumber]->programData[VOCODERBAND00] = (float)e->getDoubleAttribute(T("vocoderband00"), 0.5f);
        talPresets[programNumber]->programData[VOCODERBAND01] = (float)e->getDoubleAttribute(T("vocoderband01"), 0.5f);
        talPresets[programNumber]->programData[VOCODERBAND02] = (float)e->getDoubleAttribute(T("vocoderband02"), 0.5f);
        talPresets[programNumber]->programData[VOCODERBAND03] = (float)e->getDoubleAttribute(T("vocoderband03"), 0.5f);
        talPresets[programNumber]->programData[VOCODERBAND04] = (float)e->getDoubleAttribute(T("vocoderband04"), 0.5f);
        talPresets[programNumber]->programData[VOCODERBAND05] = (float)e->getDoubleAttribute(T("vocoderband05"), 0.5f);
        talPresets[programNumber]->programData[VOCODERBAND06] = (float)e->getDoubleAttribute(T("vocoderband06"), 0.5f);
        talPresets[programNumber]->programData[VOCODERBAND07] = (float)e->getDoubleAttribute(T("vocoderband07"), 0.5f);
        talPresets[programNumber]->programData[VOCODERBAND08] = (float)e->getDoubleAttribute(T("vocoderband08"), 0.5f);
        talPresets[programNumber]->programData[VOCODERBAND09] = (float)e->getDoubleAttribute(T("vocoderband09"), 0.5f);
        talPresets[programNumber]->programData[VOCODERBAND10] = (float)e->getDoubleAttribute(T("vocoderband10"), 0.5f);

        //file->appendText("end");

    }
}

void TalCore::setStateInformationString (const String& data)
{
    XmlElement* const xmlState = XmlDocument::parse(data);
    setStateInformationFromXml(xmlState);
}

String TalCore::getStateInformationString ()
{
    // header
    XmlElement tal("tal");
    tal.setAttribute (T("curprogram"), curProgram);
    tal.setAttribute (T("version"), 1.0);

    // programs
    XmlElement *programList = new XmlElement ("programs");

    getXmlPrograms(programList, this->curProgram);
    tal.addChildElement(programList);

    return tal.createDocument (String());
}

int TalCore::getNumPrograms ()
{
	return this->numberOfPrograms;
}

int TalCore::getCurrentProgram ()
{
	return curProgram;
}

void TalCore::setCurrentProgram (int index)
{
    //const ScopedLock sl (myCriticalSectionBuffer);

	if (index < this->numberOfPrograms)
	{
		curProgram = index;
		for (int i = 0; i < NUMPARAM; i++)
		{
			setParameter(i, talPresets[index]->programData[i]);
		}

        this->sendChangeMessage();
	}
}

const String TalCore::getProgramName (int index)
{
	if (index < this->numberOfPrograms)
		return talPresets[index]->name;
	else
		return T("Invalid");
}

void TalCore::changeProgramName (int index, const String& newName)
{
	if (index < this->numberOfPrograms)
		talPresets[index]->name = newName;
}

bool TalCore::doesClip()
{
    return engine->doesClip();
}
