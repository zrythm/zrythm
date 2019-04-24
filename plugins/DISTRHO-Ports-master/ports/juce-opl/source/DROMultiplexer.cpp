#include "DROMultiplexer.h"
#include "JuceHeader.h"

/// Jeff-Russ added guard against windows.h include if not windows:
#ifdef _WIN32 // covers both 32 and 64-bit
    #include <Windows.h>
#else
    #include "windows.h"
#endif

/// Jeff-Russ added to replace mising itoa for xcode:
#if __APPLE__ || __linux__
    #include "itoa.h"
#endif

// Used by the first recording instance to claim master status
DROMultiplexer* DROMultiplexer::master = NULL;

// Mutex between plugin instances
CriticalSection DROMultiplexer::lock;

static Bit8u dro_header[] = {
	'D', 'B', 'R', 'A',		/* 0x00, Bit32u ID */
	'W', 'O', 'P', 'L',		/* 0x04, Bit32u ID */
	0x0, 0x00,				/* 0x08, Bit16u version low */
	0x1, 0x00,				/* 0x09, Bit16u version high */
	0x0, 0x0, 0x0, 0x0,		/* 0x0c, Bit32u total milliseconds */
	0x0, 0x0, 0x0, 0x0,		/* 0x10, Bit32u total data */
	0x0, 0x0, 0x0, 0x0			/* 0x14, Bit32u Type 0=opl2,1=opl3,2=dual-opl2 */
};

static Bit8u dro_opl3_enable[] = {
	0x03,	// switch to extended register bank
	0x05,	// register 0x105
	0x01,	// value 0x1
	0x02	// switch back to regular OPL2 registers
};

// offsets for the 15 two-operator melodic channels
// http://www.shikadi.net/moddingwiki/OPL_chip
static Bit32u OPERATOR_OFFSETS[15][2] = {
	{0x000, 0x003}, // 0, 3
	{0x001, 0x004}, // 1, 4
	{0x002, 0x005}, // 2, 5
	{0x008, 0x00b}, // 6, 9
	{0x009, 0x00c}, // 7, 10
	{0x00a, 0x00d}, // 8, 11
	{0x100, 0x103}, // 18, 21
	{0x101, 0x104}, // 19, 22
	{0x102, 0x105}, // 20, 23
	{0x108, 0x10b}, // 24, 27
	{0x109, 0x10c}, // 25, 28
	{0x10a, 0x10d}, // 26, 29
	{0x110, 0x113}, // 30, 33
	{0x111, 0x114}, // 31, 34
	{0x112, 0x115}, // 32, 35
};

static Bit32u CHANNEL_OFFSETS[15]= {
	0x0,
	0x1,
	0x2,
	0x3,
	0x4,
	0x5,
	0x100,
	0x101,
	0x102,
	0x103,
	0x104,
	0x105,
	0x106,
	0x107,
	0x108,
};

// bass drum uses two operators.
// others use either modulator or carrier.
static Bit32u PERCUSSION_OFFSETS[5][2] = {
	{ 0x13, 0x10 },  // bd
	{ 0x00, 0x14 },  // sd
	{ 0x12, 0x00 },  // tt
	{ 0x00, 0x15},   // cy
	{ 0x11, 0x00 },  // hh
};

static Bit32u PERCUSSION_CHANNELS[5] = {
	7, 7, 8, 8, 9
	// or 7, 8, 8, 9, 9?
};

INLINE void host_writed(Bit8u *off, Bit32u val) {
	off[0] = (Bit8u)(val);
	off[1] = (Bit8u)(val >> 8);
	off[2] = (Bit8u)(val >> 16);
	off[3] = (Bit8u)(val >> 24);
};

HANDLE conout;
DROMultiplexer::DROMultiplexer()
{
	InitCaptureVariables();
#ifdef _DEBUG
	AllocConsole();
	conout = GetStdHandle(STD_OUTPUT_HANDLE);
#endif	
}

DROMultiplexer::~DROMultiplexer()
{
#ifdef _DEBUG
	FreeConsole();
#endif
}

DROMultiplexer* DROMultiplexer::GetMaster() {
	return DROMultiplexer::master;
}

void DROMultiplexer::_CopyOplPercussionSettings(Hiopl* opl, int pIdx) {
	// input channel 1 is as good as any..
	int op1Off = opl->_GetOffset(1, 1);
	int op2Off = opl->_GetOffset(1, 2);
	Bit32u inAddr;
	Bit32u outAddr;

	Bit32u* outOff = PERCUSSION_OFFSETS[pIdx];
	// waveform select
	int base = 0xe0;
	if (outOff[0]) {
		inAddr = base + op1Off;
		outAddr = base + outOff[0];
		_CaptureRegWriteWithDelay(outAddr, opl->_ReadReg(inAddr));
	}
	if (outOff[1]) {
		inAddr = base + op2Off;
		outAddr = base + outOff[1];
		_CaptureRegWrite(outAddr, opl->_ReadReg(inAddr));
	}
	// other operator settings
	for (base = 0x20; base <= 0x80; base += 0x20) {
		if (outOff[0]) {
			inAddr = base + op1Off;
			outAddr = base + outOff[0];
			_CaptureRegWrite(outAddr, opl->_ReadReg(inAddr));
		}
		if (outOff[1]) {
			inAddr = base + op2Off;
			outAddr = base + outOff[1];
			_CaptureRegWrite(outAddr, opl->_ReadReg(inAddr));
		}
	}

	// channel wide settings
	int chInOff = opl->_GetOffset(1);
	inAddr = 0xc0 + chInOff;
	outAddr = 0xc0 + PERCUSSION_CHANNELS[pIdx];
	_CaptureRegWrite(outAddr, 0x30 | opl->_ReadReg(inAddr));	// make sure L+R channels always enabled
}

void DROMultiplexer::_CopyOplChannelSettings(Hiopl* opl, int inCh, int outCh) {
	// read all instrument settings and write them all to the file
	int op1Off = opl->_GetOffset(inCh, 1);
	int op2Off = opl->_GetOffset(inCh, 2);
	Bit32u inAddr;
	Bit32u outAddr;
	// waveform select
	int base = 0xe0;
	inAddr = base + op1Off;
	outAddr = base + OPERATOR_OFFSETS[outCh][0];
	_CaptureRegWriteWithDelay(outAddr, opl->_ReadReg(inAddr));
	inAddr = base + op2Off;
	outAddr = base + OPERATOR_OFFSETS[outCh][1];
	_CaptureRegWrite(outAddr, opl->_ReadReg(inAddr));
	// other operator settings
	for (base = 0x20; base <= 0x80; base += 0x20) {
		inAddr = base + op1Off;
		outAddr = base + OPERATOR_OFFSETS[outCh][0];
		_CaptureRegWrite(outAddr, opl->_ReadReg(inAddr));
		inAddr = base + op2Off;
		outAddr = base + OPERATOR_OFFSETS[outCh][1];
		_CaptureRegWrite(outAddr, opl->_ReadReg(inAddr));
	}

	// channel wide settings
	int chInOff = opl->_GetOffset(inCh);
	inAddr = 0xc0 + chInOff;
	outAddr = 0xc0 + CHANNEL_OFFSETS[outCh];
	_CaptureRegWrite(outAddr, 0x30 | opl->_ReadReg(inAddr));
}

void DROMultiplexer::TwoOpMelodicNoteOn(Hiopl* opl, int inCh) {
	const ScopedLock sl(lock);

	// find a free channel and mark it as used
	char addr[16];
	int outCh = _FindFreeChannel(opl, inCh);

	if (outCh >= 0) {
		//_DebugOut(" <- ");
		//_DebugOut(itoa((int)opl, addr, 16));
		//_DebugOut(" ");
		for (int i = 0; i < MELODIC_CHANNELS; i++) {
			Hiopl* tmpOpl = channels[i].opl;
			_DebugOut(NULL == tmpOpl ? "-" : tmpOpl->GetState(channels[i].ch));
		}
		//_DebugOut("\n");
		_CopyOplChannelSettings(opl, inCh, outCh);

		// note frequency
		int chInOff = opl->_GetOffset(inCh);
		int inAddr = 0xa0 + chInOff;
		int outAddr = 0xa0 + CHANNEL_OFFSETS[outCh];
		_CaptureRegWrite(outAddr, opl->_ReadReg(inAddr));
		_DebugOut(itoa(opl->_ReadReg(inAddr), addr, 16));
		// note-on
		inAddr = 0xb0 + chInOff;
		outAddr = 0xb0 + CHANNEL_OFFSETS[outCh];
		_CaptureRegWrite(outAddr, opl->_ReadReg(inAddr));
		_DebugOut(" ");
		_DebugOut(itoa(opl->_ReadReg(inAddr), addr, 16));
		_DebugOut("\n");
	}
}

void DROMultiplexer::TwoOpMelodicNoteOff(Hiopl* opl, int ch) {
	const ScopedLock sl(lock);

	int chOff = opl->_GetOffset(ch);
	OplCh_t key;
	key.opl = opl;
	key.ch = ch;

	int outCh = channelMap[key];
	char n[8];
	_DebugOut(itoa(outCh, n, 16));
	_DebugOut(" note off\n");
	// note-off
	Bit32u inAddr = 0xb0 + chOff;
	Bit32u outAddr = 0xb0 + CHANNEL_OFFSETS[outCh];
	_CaptureRegWriteWithDelay(outAddr, opl->_ReadReg(inAddr));
}

void DROMultiplexer::_DebugOut(const char* str) {
#ifdef _DEBUG
	DWORD count;
	count = strlen(str);
	WriteConsole(conout, str, count, &count, NULL);
#endif
}

int DROMultiplexer::_FindFreeChannel(Hiopl* opl, int inCh) {
	int i = 0;
	while (i < MELODIC_CHANNELS) {
		if (NULL == channels[i].opl || !channels[i].opl->IsActive(channels[i].ch)) {
			channels[i].opl = opl;
			channels[i].ch = inCh;
			channelMap[channels[i]] = i;
			char n[8];
			_DebugOut(itoa(i, n, 16));
			return i;
		}
		i += 1;
	}
	// fall back to a released channel for same opl instance
	i = 0;
	while (i < MELODIC_CHANNELS) {
		if (opl == channels[i].opl && 'R' == opl->GetState(channels[i].ch)[0]) {
			channels[i].opl = opl;
			channels[i].ch = inCh;
			channelMap[channels[i]] = i;
			char n[8];
			_DebugOut(itoa(i, n, 16));
			return i;
		}
		i += 1;
	}
	_DebugOut("Could not find free channel!");
	return -1;
}

void DROMultiplexer::PercussionChange(Hiopl* opl, int pIdx) {
	const ScopedLock sl(lock);
	_CopyOplPercussionSettings(opl, pIdx);
	Bit8u val = opl->_ReadReg(0xbd);
	Bit8u maskOut = 1 << abs(4 - pIdx);
	if (0 == (val & maskOut)) {	// note-off
		_CaptureRegWriteWithDelay(0xbd, 0xBD & (0xe0 | ~maskOut));
	} else {					// note-on
		char addr[16];
		// note frequency
		for (int i = 0; i < i; i++) {
			Bit32u outOff = PERCUSSION_OFFSETS[pIdx][i];
			if (0x0 != outOff) {
				int inAddr = 0xa0 + opl->_GetOffset(1);		// any channel is fine, they should have all been written
				int outAddr = 0xa0 + outOff;
				_CaptureRegWrite(outAddr, opl->_ReadReg(inAddr));
				_DebugOut(itoa(opl->_ReadReg(inAddr), addr, 16));
			}
		}
		_CaptureRegWriteWithDelay(0xbd, OxBD | maskOut);
	}
}

void DROMultiplexer::InitCaptureVariables() {
	captureHandle = NULL;
	captureLengthBytes = 0;
	lastWrite = -1;
	captureStart = -1;
	channelMap.clear();
	for (int i = 0; i < MELODIC_CHANNELS; i++) {
		channels[i].opl = NULL;
		channels[i].ch = -1;
	}
	OxBD = 0x20;	// percussion mode should always be enabled
}

bool DROMultiplexer::StartCapture(const char* filepath, Hiopl *opl) {
	captureHandle = fopen(filepath, "wb");
	if (captureHandle) {
		DROMultiplexer::master = this;
		lastWrite = -1;
		captureLengthBytes = 0;
		captureStart = Time::currentTimeMillis();
		fwrite(dro_header, 1, sizeof(dro_header), captureHandle);
		for (int i = 0; i <= 0xff; i++) {
			_CaptureRegWrite(i, 0);
		}
		_CaptureOpl3Enable();
		for (Bit8u i = 0x20; i <= 0x35; i++) {
			_CaptureRegWrite(i, opl->_ReadReg(i));
		}
		for (Bit8u i = 0x40; i <= 0x55; i++) {
			_CaptureRegWrite(i, opl->_ReadReg(i));
		}
		for (Bit8u i = 0x60; i <= 0x75; i++) {
			_CaptureRegWrite(i, opl->_ReadReg(i));
		}
		for (Bit8u i = 0x80; i <= 0x95; i++) {
			_CaptureRegWrite(i, opl->_ReadReg(i));
		}
		_CaptureRegWrite(0xbd, OxBD | (opl->_ReadReg(0xbd) & 0xc0));	// enable percmode, copy tremolo and vibrato depth only
		for (Bit8u i = 0xc0; i <= 0xc8; i++) {
			_CaptureRegWrite(i, opl->_ReadReg(i) | 0x30);	// enable L + R channels
		}
		for (Bit8u i = 0xe0; i <= 0xf5; i++) {
			_CaptureRegWrite(i, opl->_ReadReg(i));
		}
	}
	return (NULL != captureHandle);
}

void DROMultiplexer::StopCapture() {
	if (NULL != captureHandle) {
		Bit16u finalDelay = (Bit16u)(Time::currentTimeMillis() - lastWrite);
		_CaptureDelay(finalDelay);
		Bit32u lengthMilliseconds = (Bit32u)(finalDelay + Time::currentTimeMillis() - captureStart);
		host_writed(&dro_header[0x0c], lengthMilliseconds);
		host_writed(&dro_header[0x10], captureLengthBytes);
		//if (opl.raw.opl3 && opl.raw.dualopl2) host_writed(&dro_header[0x14],0x1);
		//else if (opl.raw.dualopl2) host_writed(&dro_header[0x14],0x2);
		//else host_writed(&dro_header[0x14],0x0);
		host_writed(&dro_header[0x14], 0x1);	// OPL3
		fseek(captureHandle, 0, 0);
		fwrite(dro_header, 1, sizeof(dro_header), captureHandle);
		fclose(captureHandle);
	}
	InitCaptureVariables();
	DROMultiplexer::master = NULL;
}

void DROMultiplexer::_CaptureDelay(Bit16u delayMs) {
	Bit8u delay[3];
	delay[0] = 0x01;
	delay[1] = delayMs & 0xff;
	delay[2] = (delayMs >> 8) & 0xff;
	fwrite(delay, 1, 3, captureHandle);
	captureLengthBytes += 3;
}

void DROMultiplexer::_CaptureRegWrite(Bit32u reg, Bit8u value) {
	if (reg <= 0x4) {
		Bit8u escape = 0x4;
		fwrite(&escape, 1, 1, captureHandle);
		captureLengthBytes += 1;
	}
	Bit8u regAndVal[2];
	regAndVal[0] = (Bit8u)reg;
	regAndVal[1] = value;
	fwrite(regAndVal, 1, 2, captureHandle);
	captureLengthBytes += 2;
	if (0xbd == reg) {
		OxBD = value;
	}
}

void DROMultiplexer::_CaptureRegWriteWithDelay(Bit32u reg, Bit8u value) {
	if (NULL != captureHandle) {
		Bit64s t = Time::currentTimeMillis();
		if (lastWrite >= 0) {
			// Delays of over 65 seconds will be truncated, but that kind of delay is a bit silly anyway..
			_CaptureDelay((Bit16u)(t - lastWrite));
		}
		_CaptureRegWrite(reg, value);
		lastWrite = t;
	}
}

void DROMultiplexer::_CaptureOpl3Enable() {
	fwrite(dro_opl3_enable, 1, 4, captureHandle);
	captureLengthBytes += 4;
}

bool DROMultiplexer::IsAnInstanceRecording() {
	return NULL != DROMultiplexer::master;
}

bool DROMultiplexer::IsAnotherInstanceRecording() {
	return this->IsAnInstanceRecording() && this != DROMultiplexer::master;
}
