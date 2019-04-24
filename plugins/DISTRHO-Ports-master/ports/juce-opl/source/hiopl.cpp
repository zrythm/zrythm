#include "hiopl.h"

#include <assert.h>
#include "JuceHeader.h"

// A wrapper around the DOSBox and ZDoom OPL emulators.

Hiopl::Hiopl(Emulator emulator) {
	//InitCaptureVariables();

	adlib = new DBOPL::Handler();
	//zdoom = JavaOPLCreate(false);

	// channels reordered to match
	// 'in-memory' order in DOSBox emulator
	_op1offset[1] = 0x0;
	_op1offset[2] = 0x1;
	_op1offset[3] = 0x2;
	_op1offset[4] = 0x8;
	_op1offset[5] = 0x9;
	_op1offset[6] = 0xa;
	_op1offset[7] = 0x10;
	_op1offset[8] = 0x11;
	_op1offset[9] = 0x12;
	
	_op2offset[1] = 0x3;
	_op2offset[2] = 0x4;
	_op2offset[3] = 0x5;
	_op2offset[4] = 0xb;
	_op2offset[5] = 0xc;
	_op2offset[6] = 0xd;
	_op2offset[7] = 0x13;
	_op2offset[8] = 0x14;
	_op2offset[9] = 0x15;

	SetEmulator(emulator);
	_ClearRegisters();
}

void Hiopl::_ClearRegisters() {
	for (int i = 0; i < 256; i++) {
		_WriteReg(i, 0);
	}
}

void Hiopl::SetEmulator(Emulator emulator) {
	this->emulator = emulator;
}

void Hiopl::Generate(int length, float* buffer) {
	intermediateBufIdx = (intermediateBufIdx + 1) % INTERMEDIATE_BUF_N;
	Bit32s *iBuf = intermediateBuf[intermediateBufIdx];
	int fullCalls = length / 512; // the emulator is limited to 512 bytes per call
	int c = 0;
	for (; c < fullCalls; c++) {
		adlib->Generate(512, iBuf + (c * 512));
	}
	// final (or only) call
	if (length % 512 > 0) {
		adlib->Generate(length % 512, iBuf + (c * 512));
	}
	for (int i = 0; i < length; i++) {
		// Magic divisor taken from ZDoom wrapper for DOSBox emulator, line 892
		// https://github.com/rheit/zdoom/blob/master/src/oplsynth/dosbox/opl.cpp
		const float y = (float)(iBuf[i]) / 10240.0f;
		// http://stackoverflow.com/questions/427477/fastest-way-to-clamp-a-real-fixed-floating-point-value
		const float z = y < -1.0f ? -1.0f : y;
		buffer[i] = z > 1.0f ? 1.0f : z;
	}
	//} else if (ZDOOM == emulator) {
		// ZDoom hacked to write mono samples
	//	zdoom->Update(buffer, length);
	//}
}

void Hiopl::SetSampleRate(int hz) {
	adlib->Init(hz);
	EnableWaveformControl();
	for (int i = 0; i < OPL_N_REG; i++) {
		adlib->WriteReg(i, regCache[i]);
	}
}

void Hiopl::_WriteReg(Bit32u reg, Bit8u value, Bit8u mask) {
	if (mask > 0) {
		value = (regCache[reg] & (~mask)) | (value & mask);
	}
	// Write to the registers of both emulators.
	//if (DOSBOX == emulator) {
		adlib->WriteReg(reg, value);
	//} else if (ZDOOM == emulator) {
	//	zdoom->WriteReg(reg, value);
	//}
	regCache[reg] = value;
}

Bit8u Hiopl::_ReadReg(Bit32u reg) {
	return regCache[reg];
}

void Hiopl::_ClearRegBits(Bit32u reg, Bit8u mask) {
	_WriteReg(reg, regCache[reg] & ~mask);
}

void Hiopl::EnableWaveformControl() {
	_WriteReg(0x01, 0x20);
}

void Hiopl::SetWaveform(int ch, int osc, Waveform wave) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0xe0+offset, (Bit8u)wave, 0x7);
}

void Hiopl::SetAttenuation(int ch, int osc, int level) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0x40+offset, (Bit8u)level, 0x3f);
}

void Hiopl::SetKsl(int ch, int osc, int level) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0x40+offset, (Bit8u)(level<<6), 0xc0);
}

void Hiopl::SetFrequencyMultiple(int ch, int osc, FreqMultiple mult) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0x20+offset, (Bit8u)mult, 0xf);
}

void Hiopl::SetEnvelopeAttack(int ch, int osc, int t) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0x60+offset, (Bit8u)t<<4, 0xf0);
}

void Hiopl::SetEnvelopeDecay(int ch, int osc, int t) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0x60+offset, (Bit8u)t, 0x0f);
}

void Hiopl::SetEnvelopeSustain(int ch, int osc, int level) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0x80+offset, (Bit8u)level<<4, 0xf0);
}

void Hiopl::SetEnvelopeRelease(int ch, int osc, int t) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0x80+offset, (Bit8u)t, 0x0f);
}

void Hiopl::EnableTremolo(int ch, int osc, bool enable) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0x20+offset, enable ? 0x80 : 0x0, 0x80);
}

void Hiopl::EnableVibrato(int ch, int osc, bool enable) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0x20+offset, enable ? 0x40 : 0x0, 0x40);
}

void Hiopl::TremoloDepth(bool high) {
	_WriteReg(0xbd, high ? 0x80 : 0x0, 0x80);
}

void Hiopl::VibratoDepth(bool high) {
	_WriteReg(0xbd, high ? 0x40 : 0x0, 0x40);
}

void Hiopl::EnableSustain(int ch, int osc, bool enable) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0x20+offset, enable ? 0x20 : 0x0, 0x20);
}

void Hiopl::EnableKsr(int ch, int osc, bool enable) {
	int offset = this->_GetOffset(ch, osc);
	_WriteReg(0x20+offset, enable ? 0x10 : 0x0, 0x10);
}

void Hiopl::EnableAdditiveSynthesis(int ch, bool enable) {
	int offset = this->_GetOffset(ch);
	_WriteReg(0xc0+offset, enable ? 0x1 : 0x0, 0x1);
}

void Hiopl::SetModulatorFeedback(int ch, int level) {
	int offset = this->_GetOffset(ch);
	_WriteReg(0xc0+offset, (Bit8u)level << 1, 0x0e);
}

void Hiopl::SetPercussionMode(bool enable) {
	_WriteReg(0xbd, enable ? 0x20 : 0x0, 0x20);
}

void Hiopl::HitPercussion(Drum drum) {
	Bit8u mask = (Bit8u)drum;
	_WriteReg(0xbd, mask, mask);
}

void Hiopl::ReleasePercussion() {
	_WriteReg(0xbd, 0x0, 0x1f);
}

void Hiopl::KeyOn(int ch, float frqHz) {
	Hiopl::SetFrequency(ch, frqHz, true);
}

void Hiopl::KeyOff(int ch) {
	int offset = this->_GetOffset(ch);
	_ClearRegBits(0xb0+offset, 0x20);
}

static const char* STATE[] = {
	"-",
	"R",
	"S",
	"D",
	"A",
};
// in-memory ordering of DOSBox emulator
static int DOSBOX_CH_MAP[] = {
	-1, // channel numbering starts at 1
	0,
	2, // 1
	4, // 2
	1, // 3
	3, // 4
	5,
	6,
	7,
	8,
	9,
};
const char* Hiopl::GetState(int ch) const {
	int dosboxCh = DOSBOX_CH_MAP[ch];
	return STATE[adlib->chip.chan[dosboxCh].op[1].state];
}

bool Hiopl::IsActive(int ch) {
	// check carrier envelope state
	return DBOPL::Operator::State::OFF != adlib->chip.chan[ch - 1].op[1].state;
}

void Hiopl::SetFrequency(int ch, float frqHz, bool keyOn) {
	unsigned int fnum, block;
	int offset = this->_GetOffset(ch);
	// ZDoom emulator seems to be tuned down by two semitones for some reason. Sample rate difference?
	//if (ZDOOM == emulator) {
	//	frqHz *= 1.122461363636364f;
	//}
	_milliHertzToFnum((unsigned int)(frqHz * 1000.0), &fnum, &block);
	_WriteReg(0xa0+offset, fnum % 0x100);
	uint8 trig = (regCache[0xb0+offset] & 0x20) | (keyOn ? 0x20 : 0x00);
	_WriteReg(0xb0+offset, trig|((block&0x7)<<2)|(0x3&(fnum/0x100)));
}

// from libgamemusic, opl-util.cpp
void Hiopl::_milliHertzToFnum(unsigned int milliHertz,
	unsigned int *fnum, unsigned int *block, unsigned int conversionFactor)
{
	// Special case to avoid divide by zero
	if (milliHertz == 0) {
		*block = 0; // actually any block will work
		*fnum = 0;
		return;
	}
	// Special case for frequencies too high to produce
	if (milliHertz > 6208431) {
		*block = 7;
		*fnum = 1023;
		return;
	}

	// This is a bit more efficient and doesn't need log2() from math.h
	if (milliHertz > 3104215) *block = 7;
	else if (milliHertz > 1552107) *block = 6;
	else if (milliHertz > 776053) *block = 5;
	else if (milliHertz > 388026) *block = 4;
	else if (milliHertz > 194013) *block = 3;
	else if (milliHertz > 97006) *block = 2;
	else if (milliHertz > 48503) *block = 1;
	else *block = 0;

	// Slightly more efficient version
	*fnum = (unsigned int)(((unsigned long long)milliHertz << (20 - *block)) / (conversionFactor * 1000.0) + 0.5);
	if ((*block == 7) && (*fnum > 1023)) {
		// frequency out of range, clipping to maximum value.
		*fnum = 1023;
	}
	assert(*block <= 7);
	assert(*fnum < 1024);
	return;
}

Hiopl::~Hiopl() {

};

// Check that _GetOffset parameters are in range.
bool Hiopl::_CheckParams(int ch, int osc=OSCILLATORS) {
	return ch > 0 && ch <= CHANNELS && osc > 0 && osc <= OSCILLATORS;
}

// The register offset for parameters that affect a specific oscillator.
int Hiopl::_GetOffset(int ch, int osc) {
	assert(_CheckParams(ch, osc));
	return (1 == osc) ? _op1offset[ch] : _op2offset[ch];
}

// The register offset for parameters that affect the entire channel.
int Hiopl::_GetOffset(int ch) {
	assert(_CheckParams(ch));
	return ch - 1;
}

