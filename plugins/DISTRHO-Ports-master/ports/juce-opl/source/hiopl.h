#pragma once
#include <map>

#include "adlib.h"
#include "dbopl.h"
#include "zdopl.h"
// Integer buffer used by DOSBox OPL emulator, later converted to floating point.

// Number of 32-bit samples to use. ~1 MB per buffer. Probably excessive, but should be safe.
// In future, consider using samplesPerBlock value in AdlibBlasterAudioProcessor::prepareToPlay
#define INTERMEDIATE_BUF_SAMPLES		100000
// Number of static buffers to use. Again probably excessive, but let's be safe.
#define INTERMEDIATE_BUF_N				4

#define OPL_N_REG 256

enum Waveform
{
	SIN = 0, HALF_SIN = 1, ABS_SIN = 2, QUART_SIN = 3
};

enum FreqMultiple
{
	xHALF=0, x1=1, x2=2, x3=3, x4=4, x5=5, x6=6, x7=7, x8=8, x9=9, x10=10, x12=12, x15=15
};

enum Emulator
{
	DOSBOX=0, ZDOOM=1
};

enum Drum
{
	BDRUM=0x10, SNARE=0x8, TOM=0x4, CYMBAL=0x2, HIHAT=0x1
};

class Hiopl {
	public:
		static const int CHANNELS = 9;
		static const int OSCILLATORS = 2;
		Hiopl(Emulator emulator = DOSBOX);
		void SetEmulator(Emulator emulator);
		void SetPercussionMode(bool enable);
		void HitPercussion(Drum drum);
		void ReleasePercussion();

		void Generate(int length, short* buffer);
		void Generate(int length, float* buffer);
		void SetSampleRate(int hz);
		void EnableWaveformControl();
		void EnableOpl3Mode();

		void EnableTremolo(int ch, int osc, bool enable);
		void EnableVibrato(int ch, int osc, bool enable);
		void VibratoDepth(bool high);
		void TremoloDepth(bool high);
		void EnableSustain(int ch, int osc, bool enable);
		void EnableKsr(int ch, int osc, bool enable);
		// true = additive; false = frequency modulation
		void EnableAdditiveSynthesis(int ch, bool enable);
		
		void SetWaveform(int ch, int osc, Waveform wave);
		void SetAttenuation(int ch, int osc, int level);
		void SetKsl(int ch, int osc, int level);
		void SetFrequencyMultiple(int ch, int osc, FreqMultiple mult);
		void SetEnvelopeAttack(int ch, int osc, int t);
		void SetEnvelopeDecay(int ch, int osc, int t);
		void SetEnvelopeSustain(int ch, int osc, int level);
		void SetEnvelopeRelease(int ch, int osc, int t);
		// Get register address offset for operator settings for the specified channel and operator.
		int _GetOffset(int ch, int osc);
		// Get register address offset for channel-wide register settings for the specified channel.
		int _GetOffset(int ch);

		void SetModulatorFeedback(int ch, int level);
		void KeyOn(int ch, float frqHz);
		void KeyOff(int ch);
		// Return false if no note is active on the channel (ie release is complete)
		bool IsActive(int ch);
		// Return a single character string representing the stage of the envelope for the carrier operator for the channel
		const char* GetState(int ch) const;
		void SetFrequency(int ch, float frqHz, bool keyOn=false);
		void _WriteReg(Bit32u reg, Bit8u value, Bit8u mask=0x0);
		void _ClearRegBits(Bit32u reg, Bit8u mask);
		// Read the last value written to the specified register address (cached)
		Bit8u _ReadReg(Bit32u reg);

		~Hiopl();
	private:
		Emulator emulator;
		DBOPL::Handler *adlib;
		OPLEmul *zdoom;
		Bit8u regCache[OPL_N_REG];
		int intermediateBufIdx;
		Bit32s intermediateBuf[INTERMEDIATE_BUF_N][INTERMEDIATE_BUF_SAMPLES];
		bool _CheckParams(int ch, int osc);
		void _milliHertzToFnum(unsigned int milliHertz, unsigned int *fnum, unsigned int *block, unsigned int conversionFactor=49716);
		void _ClearRegisters();

		std::map<int, int> _op1offset;
		std::map<int, int> _op2offset;

};
