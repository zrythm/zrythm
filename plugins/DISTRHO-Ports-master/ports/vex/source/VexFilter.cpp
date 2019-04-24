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

#include "VexFilter.h"
#include "VexEditorComponent.h"

AudioProcessor* JUCE_CALLTYPE createPluginFilter ()
{
    return new VexFilter();
}

VexFilter::VexFilter()
        : AudioProcessor(),
          fArp1(&fArpSet1),
          fArp2(&fArpSet2),
          fArp3(&fArpSet3),
          fChorus(fParameters),
          fDelay(fParameters),
          fReverb(fParameters),
          fSynth(fParameters)
{
    std::memset(fParameters, 0, sizeof(float)*kParamCount);
    std::memset(fParamsChanged, 0, sizeof(bool)*92);

    fParameters[0] = 1.0f; // main volume

    for (int i = 0; i < 3; ++i)
    {
        const int offset = i * 24;

        fParameters[offset +  1] = 0.5f;
        fParameters[offset +  2] = 0.5f;
        fParameters[offset +  3] = 0.5f;
        fParameters[offset +  4] = 0.5f;
        fParameters[offset +  5] = 0.9f;
        fParameters[offset +  6] = 0.0f;
        fParameters[offset +  7] = 1.0f;
        fParameters[offset +  8] = 0.5f;
        fParameters[offset +  9] = 0.0f;
        fParameters[offset + 10] = 0.2f;
        fParameters[offset + 11] = 0.0f;
        fParameters[offset + 12] = 0.5f;
        fParameters[offset + 13] = 0.5f;
        fParameters[offset + 14] = 0.0f;
        fParameters[offset + 15] = 0.3f;
        fParameters[offset + 16] = 0.7f;
        fParameters[offset + 17] = 0.1f;
        fParameters[offset + 18] = 0.5f;
        fParameters[offset + 19] = 0.5f;
        fParameters[offset + 20] = 0.0f;
        fParameters[offset + 21] = 0.0f;
        fParameters[offset + 22] = 0.5f;
        fParameters[offset + 23] = 0.5f;
        fParameters[offset + 24] = 0.5f;
    }

    // ^1 - 72

    fParameters[73] = 0.5f; // Delay Time
    fParameters[74] = 0.4f; // Delay Feedback
    fParameters[75] = 0.0f; // Delay Volume

    fParameters[76] = 0.3f; // Chorus Rate
    fParameters[77] = 0.6f; // Chorus Depth
    fParameters[78] = 0.5f; // Chorus Volume

    fParameters[79] = 0.6f; // Reverb Size
    fParameters[80] = 0.7f; // Reverb Width
    fParameters[81] = 0.6f; // Reverb Damp
    fParameters[82] = 0.0f; // Reverb Volume

    fParameters[83] = 0.5f; // wave1 panning
    fParameters[84] = 0.5f; // wave2 panning
    fParameters[85] = 0.5f; // wave3 panning

    fParameters[86] = 0.5f; // wave1 volume
    fParameters[87] = 0.5f; // wave2 volume
    fParameters[88] = 0.5f; // wave3 volume

    fParameters[89] = 1.0f; // wave1 on/off
    fParameters[90] = 0.0f; // wave2 on/off
    fParameters[91] = 0.0f; // wave3 on/off

    for (unsigned int i = 0; i < kParamCount; ++i)
        fSynth.update(i);
}

VexFilter::~VexFilter()
{
}

const String VexFilter::getInputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

const String VexFilter::getOutputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

bool VexFilter::isInputChannelStereoPair (int index) const
{
    return false;
}

bool VexFilter::isOutputChannelStereoPair (int index) const
{
    return true;
}

float VexFilter::getParameter (int index)
{
    if (index >= (int)kParamCount)
        return 0.0f;

    return fParameters[index];
}

void VexFilter::setParameter (int index, float newValue)
{
    if (index >= (int)kParamCount)
        return;

    fParameters[index] = newValue;
    fParamsChanged[index] = true;
    fSynth.update(index);
}

const String VexFilter::getParameterName (int index)
{
    String retName;

    if (index >= 1 && index <= 72)
    {
        uint32_t ri = index % 24;

        switch (ri)
        {
        case 1:
            retName = "Part0 Oct";
            break;
        case 2:
            retName = "Part0 Cent";
            break;
        case 3:
            retName = "Part0 Phase";
            break;
        case 4:
            retName = "Part0 Tune";
            break;
        case 5:
            retName = "Part0 Filter Cut";
            break;
        case 6:
            retName = "Part0 Filter Res";
            break;
        case 7:
            retName = "Part0 Filter HP/LP";
            break;
        case 8:
            retName = "Part0 Filter Env";
            break;
        case 9:
            retName = "Part0 Filter Env Atk";
            break;
        case 10:
            retName = "Part0 Filter Env Dec";
            break;
        case 11:
            retName = "Part0 Filter Env Sus";
            break;
        case 12:
            retName = "Part0 Filter Env Rel";
            break;
        case 13:
            retName = "Part0 Filter Env Vel";
            break;
        case 14:
            retName = "Part0 Amp Env Atk";
            break;
        case 15:
            retName = "Part0 Amp Env Dec";
            break;
        case 16:
            retName = "Part0 Amp Env Sus";
            break;
        case 17:
            retName = "Part0 Amp Env Rel";
            break;
        case 18:
            retName = "Part0 Amp Env Vel";
            break;
        case 19:
            retName = "Part0 LFO Rate";
            break;
        case 20:
            retName = "Part0 LFO Amp";
            break;
        case 21:
            retName = "Part0 LFO Flt";
            break;
        case 22:
            retName = "Part0 Delay";
            break;
        case 23:
            retName = "Part0 Chorus";
            break;
        case 24:
        case 0:
            retName = "Part0 Reverb";
            break;
        default:
            retName = "Part0 Unknown";
            break;
        }

        const int partn = (index-1+24) / 24;

        switch (partn)
        {
        case 1:
            retName = retName.replace("Part0", "Part1");
            break;
        case 2:
            retName = retName.replace("Part0", "Part2");
            break;
        case 3:
            retName = retName.replace("Part0", "Part3");
            break;
        }

        return retName;
    }

    switch (index)
    {
    case 0:
        retName = "Master";
        break;
    case 73:
        retName = "Delay Time";
        break;
    case 74:
        retName = "Delay Feedback";
        break;
    case 75:
        retName = "Delay Level";
        break;
    case 76:
        retName = "Chorus Rate";
        break;
    case 77:
        retName = "Chorus Depth";
        break;
    case 78:
        retName = "Chorus Level";
        break;
    case 79:
        retName = "Reverb Size";
        break;
    case 80:
        retName = "Reverb Width";
        break;
    case 81:
        retName = "Reverb Damp";
        break;
    case 82:
        retName = "Reverb Level";
        break;
    case 83:
        retName = "Part1 Panning";
        break;
    case 84:
        retName = "Part2 Panning";
        break;
    case 85:
        retName = "Part3 Panning";
        break;
    case 86:
        retName = "Part1 Volume";
        break;
    case 87:
        retName = "Part2 Volume";
        break;
    case 88:
        retName = "Part3 Volume";
        break;
    case 89:
        retName = "Part1 on/off";
        break;
    case 90:
        retName = "Part2 on/off";
        break;
    case 91:
        retName = "Part3 on/off";
        break;
    default:
        retName = "unknown";
    }

    return retName;
}

const String VexFilter::getParameterText (int index)
{
    return String(fParameters[index], 2);
}

void VexFilter::prepareToPlay (double sampleRate, int bufferSize)
{
    obf.setSize(2, bufferSize);
    dbf1.setSize(2, bufferSize);
    dbf2.setSize(2, bufferSize);
    dbf3.setSize(2, bufferSize);

    fArp1.setSampleRate(sampleRate);
    fArp2.setSampleRate(sampleRate);
    fArp3.setSampleRate(sampleRate);
    fChorus.setSampleRate(sampleRate);
    fDelay.setSampleRate(sampleRate);

    fSynth.setBufferSize(bufferSize);
    fSynth.setSampleRate(sampleRate);

    // some params depend on sample rate
    for (unsigned int i = 0; i < kParamCount; ++i)
        fSynth.update(i);
}

void VexFilter::releaseResources()
{
}

void VexFilter::processBlock(AudioSampleBuffer& output, MidiBuffer& midiInBuffer)
{
    AudioPlayHead::CurrentPositionInfo pos;

    if (AudioPlayHead* const playhead = getPlayHead())
        playhead->getCurrentPosition(pos);
    else
        pos.resetToDefault();

    const int frames = output.getNumSamples();

    // process MIDI (arppeggiator)
    const MidiBuffer& part1Midi(fArpSet1.on ? fArp1.processMidi(midiInBuffer, pos.isPlaying, pos.ppqPosition, pos.ppqPositionOfLastBarStart, pos.bpm, frames) : midiInBuffer);
    const MidiBuffer& part2Midi(fArpSet2.on ? fArp2.processMidi(midiInBuffer, pos.isPlaying, pos.ppqPosition, pos.ppqPositionOfLastBarStart, pos.bpm, frames) : midiInBuffer);
    const MidiBuffer& part3Midi(fArpSet3.on ? fArp3.processMidi(midiInBuffer, pos.isPlaying, pos.ppqPosition, pos.ppqPositionOfLastBarStart, pos.bpm, frames) : midiInBuffer);

    int snum;
    MidiMessage midiMessage(0xf4);

    for (MidiBuffer::Iterator Iterator1(part1Midi); Iterator1.getNextEvent(midiMessage, snum);)
    {
        if (midiMessage.isNoteOn())
            fSynth.playNote(midiMessage.getNoteNumber(), midiMessage.getVelocity(), snum, 1);
        else if (midiMessage.isNoteOff())
            fSynth.releaseNote(midiMessage.getNoteNumber(), snum, 1);
        else if (midiMessage.isAllSoundOff())
            fSynth.kill();
        else if (midiMessage.isAllNotesOff())
            fSynth.releaseAll(snum);
    }

    for (MidiBuffer::Iterator Iterator2(part2Midi); Iterator2.getNextEvent(midiMessage, snum);)
    {
        if (midiMessage.isNoteOn())
            fSynth.playNote(midiMessage.getNoteNumber(), midiMessage.getVelocity(), snum, 2);
        else if (midiMessage.isNoteOff())
            fSynth.releaseNote(midiMessage.getNoteNumber(), snum, 2);
    }

    for (MidiBuffer::Iterator Iterator3(part3Midi); Iterator3.getNextEvent(midiMessage, snum);)
    {
        if (midiMessage.isNoteOn())
            fSynth.playNote(midiMessage.getNoteNumber(), midiMessage.getVelocity(), snum, 3);
        else if (midiMessage.isNoteOff())
            fSynth.releaseNote(midiMessage.getNoteNumber(), snum, 3);
    }

    midiInBuffer.clear();

    if (obf.getNumSamples() != frames)
    {
        obf .setSize(2, frames, false, false, true);
        dbf1.setSize(2, frames, false, false, true);
        dbf2.setSize(2, frames, false, false, true);
        dbf3.setSize(2, frames, false, false, true);
    }

    obf .clear();
    dbf1.clear();
    dbf2.clear();
    dbf3.clear();

    fSynth.doProcess(obf, dbf1, dbf2, dbf3);

    if (fParameters[75] > 0.001f)
    {
        fDelay.processBlock(dbf1, pos.bpm);
        obf.addFrom(0, 0, dbf1, 0, 0, frames, fParameters[75]);
        obf.addFrom(1, 0, dbf1, 1, 0, frames, fParameters[75]);
    }

    if (fParameters[78] > 0.001f)
    {
        fChorus.processBlock(dbf2);
        obf.addFrom(0, 0, dbf2, 0, 0, frames, fParameters[78]);
        obf.addFrom(1, 0, dbf2, 1, 0, frames, fParameters[78]);
    }

    if (fParameters[82] > 0.001f)
    {
        fReverb.processBlock(dbf3);
        obf.addFrom(0, 0, dbf3, 0, 0, frames, fParameters[82]);
        obf.addFrom(1, 0, dbf3, 1, 0, frames, fParameters[82]);
    }

    output.copyFrom(0, 0, obf.getReadPointer(0), frames, fParameters[0]);
    output.copyFrom(1, 0, obf.getReadPointer(1), frames, fParameters[0]);
}

void VexFilter::setStateInformation (const void* data_, int dataSize)
{
#if JucePlugin_Build_LV2
    static const int kParamDataSize = 0;
#else
    static const int kParamDataSize = sizeof(float)*kParamCount;
#endif
    static const int xmlOffset = kParamDataSize + sizeof(VexArpSettings) * 3;

    const char* data = (const char*)data_;

    if (XmlElement* const xmlState = getXmlFromBinary(data + xmlOffset, dataSize - xmlOffset))
    {
        if (xmlState->hasTagName("VEX"))
        {
            getCallbackLock().enter();

            fSynth.setWaveLater(1, xmlState->getStringAttribute("Wave1"));
            fSynth.setWaveLater(2, xmlState->getStringAttribute("Wave2"));
            fSynth.setWaveLater(3, xmlState->getStringAttribute("Wave3"));

#if ! JucePlugin_Build_LV2
            std::memcpy(fParameters, data, sizeof(float) * kParamCount);
#endif
            std::memcpy(&fArpSet1,   data + (kParamDataSize + sizeof(VexArpSettings)*0), sizeof(VexArpSettings));
            std::memcpy(&fArpSet2,   data + (kParamDataSize + sizeof(VexArpSettings)*1), sizeof(VexArpSettings));
            std::memcpy(&fArpSet3,   data + (kParamDataSize + sizeof(VexArpSettings)*2), sizeof(VexArpSettings));

#if ! JucePlugin_Build_LV2
            for (unsigned int i = 0; i < kParamCount; ++i)
               fSynth.update(i);
#endif

            getCallbackLock().exit();

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
            if (AudioProcessorEditor* const editor = getActiveEditor())
                ((VexEditorComponent*)editor)->setNeedsUpdate();
#endif
        }

        delete xmlState;
    }
}

void VexFilter::getStateInformation (MemoryBlock& destData)
{
#if ! JucePlugin_Build_LV2
    destData.append(fParameters, sizeof(float) * kParamCount);
#endif
    destData.append(&fArpSet1, sizeof(VexArpSettings));
    destData.append(&fArpSet2, sizeof(VexArpSettings));
    destData.append(&fArpSet3, sizeof(VexArpSettings));

    XmlElement xmlState("VEX");

    xmlState.setAttribute("Wave1", fSynth.getWaveName(1));
    xmlState.setAttribute("Wave2", fSynth.getWaveName(2));
    xmlState.setAttribute("Wave3", fSynth.getWaveName(3));

    MemoryBlock tmp;
    copyXmlToBinary(xmlState, tmp);

    destData.append(tmp.getData(), tmp.getSize());
}

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
AudioProcessorEditor* VexFilter::createEditor()
{
    return new VexEditorComponent(this, this, fArpSet1, fArpSet2, fArpSet3);
}
#endif

void VexFilter::getChangedParameters(bool params[92])
{
    std::memcpy(params, fParamsChanged, sizeof(bool)*92);
    std::memset(fParamsChanged, 0, sizeof(bool)*92);
}

float VexFilter::getFilterParameterValue(const uint32_t index) const
{
    if (index >= kParamCount)
        return 0.0f;
    return fParameters[index];
}

String VexFilter::getFilterWaveName(const int part) const
{
    if (part >= 1 && part <= 3)
        return fSynth.getWaveName(part);
    return String();
}

void VexFilter::editorParameterChanged(const uint32_t index, const float value)
{
    if (index >= kParamCount)
        return;
    if (fParameters[index] == value)
        return;

    fParameters[index] = value;
    fSynth.update(index);

    sendParamChangeMessageToListeners(index, value);
}

void VexFilter::editorWaveChanged(const int part, const String& wave)
{
    if (part >= 1 && part <= 3)
        fSynth.setWaveLater(part, wave);
}
