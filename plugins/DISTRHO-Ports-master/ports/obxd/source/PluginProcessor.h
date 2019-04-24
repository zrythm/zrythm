/*
	==============================================================================
	This file is part of Obxd synthesizer.

	Copyright © 2013-2014 Filatov Vadim
	
	Contact author via email :
	justdat_@_e1.ru

	This file may be licensed under the terms of of the
	GNU General Public License Version 2 (the ``GPL'').

	Software distributed under the License is distributed
	on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
	express or implied. See the GPL for the specific language
	governing rights and limitations.

	You should have received a copy of the GPL along with this
	program. If not, go to http://www.gnu.org/licenses/gpl.html
	or write to the Free Software Foundation, Inc.,  
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
	==============================================================================
 */

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "JuceHeader.h"
#include "Engine/SynthEngine.h"
//#include <stack>
#include "Engine/midiMap.h"
#include "Engine/ObxdBank.h"

//==============================================================================
const int fxbVersionNum = 1;

struct fxProgram
{
	int32 chunkMagic;    // 'CcnK'
	int32 byteSize;      // of this chunk, excl. magic + byteSize
	int32 fxMagic;       // 'FxCk'
	int32 version;
	int32 fxID;          // fx unique id
	int32 fxVersion;
	int32 numParams;
	char prgName[28];
	float params[1];        // variable no. of parameters
};

struct fxSet
{
	int32 chunkMagic;    // 'CcnK'
	int32 byteSize;      // of this chunk, excl. magic + byteSize
	int32 fxMagic;       // 'FxBk'
	int32 version;
	int32 fxID;          // fx unique id
	int32 fxVersion;
	int32 numPrograms;
	char future[128];
	fxProgram programs[1];  // variable no. of programs
};

struct fxChunkSet
{
	int32 chunkMagic;    // 'CcnK'
	int32 byteSize;      // of this chunk, excl. magic + byteSize
	int32 fxMagic;       // 'FxCh', 'FPCh', or 'FBCh'
	int32 version;
	int32 fxID;          // fx unique id
	int32 fxVersion;
	int32 numPrograms;
	char future[128];
	int32 chunkSize;
	char chunk[8];          // variable
};

struct fxProgramSet
{
	int32 chunkMagic;    // 'CcnK'
	int32 byteSize;      // of this chunk, excl. magic + byteSize
	int32 fxMagic;       // 'FxCh', 'FPCh', or 'FBCh'
	int32 version;
	int32 fxID;          // fx unique id
	int32 fxVersion;
	int32 numPrograms;
	char name[28];
	int32 chunkSize;
	char chunk[8];          // variable
};

// Compares a magic value in either endianness.
static inline bool compareMagic (int32 magic, const char* name) noexcept
{
	return magic == (int32) ByteOrder::littleEndianInt (name)
	|| magic == (int32) ByteOrder::bigEndianInt (name);
}

static inline int32 fxbName (const char* name) noexcept   { return (int32) ByteOrder::littleEndianInt (name); }
static inline int32 fxbSwap (const int32 x) noexcept   { return (int32) ByteOrder::swapIfLittleEndian ((uint32) x); }

static inline float fxbSwapFloat (const float x) noexcept
{
#ifdef JUCE_LITTLE_ENDIAN
	union { uint32 asInt; float asFloat; } n;
	n.asFloat = x;
	n.asInt = ByteOrder::swap (n.asInt);
	return n.asFloat;
#else
	return x;
#endif
}

//==============================================================================
/**
*/
class ObxdAudioProcessor :
	public AudioProcessor,
	// public AudioProcessorListener,
	public ChangeBroadcaster
{
public:
    //==============================================================================
    ObxdAudioProcessor();
    ~ObxdAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

    //==============================================================================
    AudioProcessorEditor* createEditor();
    bool hasEditor() const;
	
	//==============================================================================
	void processMidiPerSample(MidiBuffer::Iterator* iter,const int samplePos);
	bool getNextEvent(MidiBuffer::Iterator* iter,const int samplePos);

	//==============================================================================
	void initAllParams();

	int getNumParameters();

    float getParameter (int index);
    void setParameter (int index, float newValue);

    const String getParameterName (int index);
    const String getParameterText (int index);

    const String getInputChannelName (int channelIndex) const;
    const String getOutputChannelName (int channelIndex) const;
    bool isInputChannelStereoPair (int index) const;
    bool isOutputChannelStereoPair (int index) const;

    bool acceptsMidi() const;
    bool producesMidi() const;
    bool silenceInProducesSilenceOut() const;
    double getTailLengthSeconds() const;
	const String getName() const;

    //==============================================================================
    int getNumPrograms();
    int getCurrentProgram();
    void setCurrentProgram (int index);
    const String getProgramName (int index);
    void changeProgramName (int index, const String& newName);

    //==============================================================================
    void getStateInformation (MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);
	void setCurrentProgramStateInformation(const void* data,int sizeInBytes);
	void getCurrentProgramStateInformation(MemoryBlock& destData);

	//==============================================================================
	void scanAndUpdateBanks();
	const Array<File>& getBankFiles() const;
	bool loadFromFXBFile(const File& fxbFile);
	bool restoreProgramSettings(const fxProgram* const prog);
	File getCurrentBankFile() const;

	//==============================================================================
	const ObxdBank& getPrograms() const { return programs; }

	//==============================================================================
	File getDocumentFolder() const;
	File getSkinFolder() const;
	File getBanksFolder() const;

	File getCurrentSkinFolder() const;
	void setCurrentSkinFolder(const String& folderName);

private:
	//==============================================================================
	bool isHostAutomatedChange;

        MidiMessage nextMidi, midiMsg;

	bool hasMidiMessage;
	int midiEventPos;

	SynthEngine synth;
	ObxdBank programs;

	String currentSkin;
	String currentBank;
	Array<File> bankFiles;

	ScopedPointer<PropertiesFile> config;
	InterProcessLock configLock;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObxdAudioProcessor)
};

#endif  // PLUGINPROCESSOR_H_INCLUDED
