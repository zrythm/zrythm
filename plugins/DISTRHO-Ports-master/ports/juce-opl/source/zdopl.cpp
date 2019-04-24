/*
 * File: OPL3.java
 * Software implementation of the Yamaha YMF262 sound generator.
 * Copyright (C) 2008 Robson Cozendey <robson@cozendey.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * One of the objectives of this emulator is to stimulate further research in the
 * OPL3 chip emulation. There was an explicit effort in making no optimizations, 
 * and making the code as legible as possible, so that a new programmer 
 * interested in modify and improve upon it could do so more easily. 
 * This emulator's main body of information was taken from reverse engineering of 
 * the OPL3 chip, from the YMF262 Datasheet and from the OPL3 section in the 
 * YMF278b Application's Manual,
 * together with the vibrato table information, eighth waveform parameter 
 * information and feedback averaging information provided in MAME's YMF262 and	
 * YM3812 emulators, by Jarek Burczynski and Tatsuyuki Satoh.
 * This emulator has a high degree of accuracy, and most of music files sound 
 * almost identical, exception made in some games which uses specific parts of 
 * the rhythm section. In this respect, some parts of the rhythm mode are still 
 * only an approximation of the real chip.
 * The other thing to note is that this emulator was done through recordings of 
 * the SB16 DAC, so it has not bitwise precision. Additional equipment should be 
 * used to verify the samples directly from the chip, and allow this exact 
 * per-sample correspondence. As a good side-effect, since this emulator uses 
 * floating point and has a more fine-grained envelope generator, it can produce 
 * sometimes a crystal-clear, denser kind of OPL3 sound that, because of that, 
 * may be useful for creating new music. 
 * 
 * Version 1.0.6
 * 
 */

#include <math.h>
#include <string.h>
#include <limits>
#include <cstdlib>    /// Jeff-Russ added to get rand() and RAND_MAX 


#ifndef M_PI    /// Jeff-Russ added 
    #define M_PI	3.141592654
#endif

#include "config.h"
#include "zdopl.h"

// Disable warnings for unreferenced parameters
#pragma warning( disable : 4100 )

inline static Bit32s xs_RoundToInt(Real64 val)
{
    return (Bit32s)floor(val+.5);
}
inline static Bit32s xs_FloorToInt(Real64 val)
{
    return (Bit32s)floor(val);
}
double Rand_Real1()
{
    return (double)rand() / (double)RAND_MAX ;
}


//static FRandom pr_opl3;

#define VOLUME_MUL		0.3333

class Operator;

static inline double StripIntPart(double num)
{
#if 0
	double dontcare;
	return modf(num, &dontcare);
#else
	return num - xs_RoundToInt(num);
#endif
}

//
// Channels
//


class Channel 
{
protected:
	double feedback[2];
	
	int fnuml, fnumh, kon, block, fb, cha, chb, cnt;     

	// Factor to convert between normalized amplitude to normalized
	// radians. The amplitude maximum is equivalent to 8*Pi radians.
#define toPhase (4.f)

public:
	int channelBaseAddress;

	double leftPan, rightPan;
	
	Channel (int baseAddress, double startvol);
	virtual ~Channel() {}
	void update_2_KON1_BLOCK3_FNUMH2(class OPL3 *OPL3);
	void update_FNUML8(class OPL3 *OPL3);
	void update_CHD1_CHC1_CHB1_CHA1_FB3_CNT1(class OPL3 *OPL3);
	void updateChannel(class OPL3 *OPL3);
	void updatePan(class OPL3 *OPL3);
	virtual double getChannelOutput(class OPL3 *OPL3) = 0;

	virtual void keyOn() = 0;
	virtual void keyOff() = 0;
	virtual void updateOperators(class OPL3 *OPL3) = 0;
};


class Channel2op : public Channel
{
public:
	Operator *op1, *op2;
	
	Channel2op (int baseAddress, double startvol, Operator *o1, Operator *o2);
	double getChannelOutput(class OPL3 *OPL3);
	
	void keyOn();
	void keyOff();
	void updateOperators(class OPL3 *OPL3);
};


class Channel4op : public Channel
{
public:
	Operator *op1, *op2, *op3, *op4;

	Channel4op (int baseAddress, double startvol, Operator *o1, Operator *o2, Operator *o3, Operator *o4);
	double getChannelOutput(class OPL3 *OPL3);
	
	void keyOn();
	void keyOff();
	void updateOperators(class OPL3 *OPL3);
};

// There's just one instance of this class, that fills the eventual gaps in the Channel array;
class DisabledChannel : public Channel
{
public:
	DisabledChannel() : Channel(0, 0) { }
	double getChannelOutput(class OPL3 *OPL3) { return 0; }    
	void keyOn() { }
	void keyOff() { }
	void updateOperators(class OPL3 *OPL3) { }
};



//
// Envelope Generator
//


class EnvelopeGenerator
{
public:
	enum Stage {ATTACK,DECAY,SUSTAIN,RELEASE,OFF};
	Stage stage;    
	int actualAttackRate, actualDecayRate, actualReleaseRate;        
	double xAttackIncrement, xMinimumInAttack;             
	double dBdecayIncrement;
	double dBreleaseIncrement;
	double attenuation, totalLevel, sustainLevel;    
	double x, envelope;

public:
	EnvelopeGenerator();
	void setActualSustainLevel(int sl);
	void setTotalLevel(int tl);
	void setAtennuation(int f_number, int block, int ksl);
	void setActualAttackRate(int attackRate, int ksr, int keyScaleNumber);
	void setActualDecayRate(int decayRate, int ksr, int keyScaleNumber);
	void setActualReleaseRate(int releaseRate, int ksr, int keyScaleNumber);
private:
	int calculateActualRate(int rate, int ksr, int keyScaleNumber);
public:
	double getEnvelope(OPL3 *OPL3, int egt, int am);
	void keyOn();
	void keyOff();

private:
	static double dBtoX(double dB);
	static double percentageToDB(double percentage);
	static double percentageToX(double percentage);
};


//
// Phase Generator
//


class PhaseGenerator {
	double phase, phaseIncrement;

public:
	PhaseGenerator();
	void setFrequency(int f_number, int block, int mult);
	double getPhase(class OPL3 *OPL3, int vib);
	void keyOn();
};


//
// Operators
//


class Operator
{
public:
	PhaseGenerator phaseGenerator;
	EnvelopeGenerator envelopeGenerator;
	
	double envelope, phase;
	
	int operatorBaseAddress;
	int am, vib, ksr, egt, mult, ksl, tl, ar, dr, sl, rr, ws; 
	int keyScaleNumber, f_number, block;
	
	static const double noModulator;

public:
	Operator(int baseAddress);
	void update_AM1_VIB1_EGT1_KSR1_MULT4(class OPL3 *OPL3);
	void update_KSL2_TL6(class OPL3 *OPL3);
	void update_AR4_DR4(class OPL3 *OPL3);
	void update_SL4_RR4(class OPL3 *OPL3);
	void update_5_WS3(class OPL3 *OPL3);
	double getOperatorOutput(class OPL3 *OPL3, double modulator);

	void keyOn();
	void keyOff();
 	void updateOperator(class OPL3 *OPL3, int ksn, int f_num, int blk);
protected:
	double getOutput(double modulator, double outputPhase, double *waveform);
};


//
// Rhythm
//

// The getOperatorOutput() method in TopCymbalOperator, HighHatOperator and SnareDrumOperator 
// were made through purely empyrical reverse engineering of the OPL3 output.

class RhythmChannel : public Channel2op
{
public:
	RhythmChannel(int baseAddress, double startvol, Operator *o1, Operator *o2)
	: Channel2op(baseAddress, startvol, o1, o2)
	{ }
	double getChannelOutput(class OPL3 *OPL3);

	// Rhythm channels are always running, 
	// only the envelope is activated by the user.
	void keyOn() { }
	void keyOff() { }   
};

class HighHatSnareDrumChannel : public RhythmChannel {
	static const int highHatSnareDrumChannelBaseAddress = 7;
public:
	HighHatSnareDrumChannel(double startvol, Operator *o1, Operator *o2)
	: RhythmChannel(highHatSnareDrumChannelBaseAddress, startvol, o1, o2)
	{ }
};

class TomTomTopCymbalChannel : public RhythmChannel {
	static const int tomTomTopCymbalChannelBaseAddress = 8;    
public:
	TomTomTopCymbalChannel(double startvol, Operator *o1, Operator *o2)
	: RhythmChannel(tomTomTopCymbalChannelBaseAddress, startvol, o1, o2)
	{ }
};
 
class TopCymbalOperator : public Operator {
	static const int topCymbalOperatorBaseAddress = 0x15;
public:
	TopCymbalOperator(int baseAddress);
	TopCymbalOperator();
	double getOperatorOutput(class OPL3 *OPL3, double modulator);
	double getOperatorOutput(class OPL3 *OPL3, double modulator, double externalPhase);
};

class HighHatOperator : public TopCymbalOperator {
	static const int highHatOperatorBaseAddress = 0x11;     
public:
	HighHatOperator();
	double getOperatorOutput(class OPL3 *OPL3, double modulator);
};

class SnareDrumOperator : public Operator {
	static const int snareDrumOperatorBaseAddress = 0x14;
public:
	SnareDrumOperator();
	double getOperatorOutput(class OPL3 *OPL3, double modulator);
};

class TomTomOperator : public Operator {
	static const int tomTomOperatorBaseAddress = 0x12;
public:
	TomTomOperator() : Operator(tomTomOperatorBaseAddress) { }
};

class BassDrumChannel : public Channel2op {
	static const int bassDrumChannelBaseAddress = 6;
	static const int op1BaseAddress = 0x10; 
	static const int op2BaseAddress = 0x13;

	Operator my_op1, my_op2;

public:
	BassDrumChannel(double startvol);
	double getChannelOutput(class OPL3 *OPL3);
	
	// Key ON and OFF are unused in rhythm channels.
	void keyOn() { }
	void keyOff() { }
};


//
// OPl3 Data
//


struct OPL3DataStruct
{
public:
	// OPL3-wide registers offsets:
	static const int 
		 _1_NTS1_6_Offset = 0x08, 
		 DAM1_DVB1_RYT1_BD1_SD1_TOM1_TC1_HH1_Offset = 0xBD, 
		 _7_NEW1_Offset = 0x105, 
		 _2_CONNECTIONSEL6_Offset = 0x104;        

	// The OPL3 tremolo repetition rate is 3.7 Hz.  
	#define tremoloFrequency (3.7)

	static const int tremoloTableLength = (int)(OPL_SAMPLE_RATE/tremoloFrequency);
	static const int vibratoTableLength = 8192;

	OPL3DataStruct()
	{
		loadVibratoTable();
		loadTremoloTable();
	}

	// The first array is used when DVB=0 and the second array is used when DVB=1.
	double vibratoTable[2][vibratoTableLength];

	// First array used when AM = 0 and second array used when AM = 1.
	double tremoloTable[2][tremoloTableLength];

	static double calculateIncrement(double begin, double end, double period) {
		return (end-begin)/OPL_SAMPLE_RATE * (1/period);
	}

private:
	void loadVibratoTable();
	void loadTremoloTable();
};


//
// Channel Data
// 


struct ChannelData
{
	static const int
		_2_KON1_BLOCK3_FNUMH2_Offset = 0xB0,
		FNUML8_Offset = 0xA0,
		CHD1_CHC1_CHB1_CHA1_FB3_CNT1_Offset = 0xC0;
	
	// Feedback rate in fractions of 2*Pi, normalized to (0,1): 
	// 0, Pi/16, Pi/8, Pi/4, Pi/2, Pi, 2*Pi, 4*Pi turns to be:
	static const float feedback[8];
};
const float ChannelData::feedback[8] = {0,1/32.f,1/16.f,1/8.f,1/4.f,1/2.f,1,2};


//
// Operator Data
//


struct OperatorDataStruct
{
	static const int
		AM1_VIB1_EGT1_KSR1_MULT4_Offset = 0x20,
		KSL2_TL6_Offset = 0x40,
		AR4_DR4_Offset = 0x60,
		SL4_RR4_Offset = 0x80,
		_5_WS3_Offset = 0xE0;
	
	enum type {NO_MODULATION, CARRIER, FEEDBACK};
	
	static const int waveLength = 1024;
	
	static const float multTable[16];
	static const float ksl3dBtable[16][8];
	
	//OPL3 has eight waveforms:
	double waveforms[8][waveLength];

#define MIN_DB				(-120.0)
#define DB_TABLE_RES		(4.0)
#define DB_TABLE_SIZE		(int)(-MIN_DB * DB_TABLE_RES)

	double dbpow[DB_TABLE_SIZE];

#define ATTACK_MIN			(-5.0)
#define ATTACK_MAX			(8.0)
#define ATTACK_RES			(0.03125)
#define ATTACK_TABLE_SIZE	(int)((ATTACK_MAX - ATTACK_MIN) / ATTACK_RES)

	double attackTable[ATTACK_TABLE_SIZE];

	OperatorDataStruct()
	{
		loadWaveforms();
		loaddBPowTable();
		loadAttackTable();
	}
	
	static double log2(double x) {
		return log(x)/log(2.0);
	}
private:
	void loadWaveforms();
	void loaddBPowTable();
	void loadAttackTable();
};
const float OperatorDataStruct::multTable[16] = {0.5,1,2,3,4,5,6,7,8,9,10,10,12,12,15,15};

const float OperatorDataStruct::ksl3dBtable[16][8] = {
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,-3,-6,-9},
	{0,0,0,0,-3,-6,-9,-12},
	{0,0,0, -1.875, -4.875, -7.875, -10.875, -13.875},
	
	{0,0,0,-3,-6,-9,-12,-15},
	{0,0, -1.125, -4.125, -7.125, -10.125, -13.125, -16.125}, 
	{0,0, -1.875, -4.875, -7.875, -10.875, -13.875, -16.875},
	{0,0, -2.625, -5.625, -8.625, -11.625, -14.625, -17.625},
	
	{0,0,-3,-6,-9,-12,-15,-18},
	{0, -0.750, -3.750, -6.750, -9.750, -12.750, -15.750, -18.750},
	{0, -1.125, -4.125, -7.125, -10.125, -13.125, -16.125, -19.125},
	{0, -1.500, -4.500, -7.500, -10.500, -13.500, -16.500, -19.500},
	
	{0, -1.875, -4.875, -7.875, -10.875, -13.875, -16.875, -19.875},
	{0, -2.250, -5.250, -8.250, -11.250, -14.250, -17.250, -20.250},
	{0, -2.625, -5.625, -8.625, -11.625, -14.625, -17.625, -20.625},
	{0,-3,-6,-9,-12,-15,-18,-21}
};

//
// Envelope Generator Data
//


namespace EnvelopeGeneratorData
{
	static const double MUGEN = std::numeric_limits<double>::infinity();
	// This table is indexed by the value of Operator.ksr 
	// and the value of ChannelRegister.keyScaleNumber.
	static const int rateOffset[2][16] = {
		{0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
		{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}
	};    
	// These attack periods in miliseconds were taken from the YMF278B manual. 
	// The attack actual rates range from 0 to 63, with different data for 
	// 0%-100% and for 10%-90%: 
	static const double attackTimeValuesTable[64][2] = {
		{MUGEN,MUGEN},    {MUGEN,MUGEN},    {MUGEN,MUGEN},    {MUGEN,MUGEN},
		{2826.24,1482.75}, {2252.80,1155.07}, {1884.16,991.23}, {1597.44,868.35},
		{1413.12,741.38}, {1126.40,577.54}, {942.08,495.62}, {798.72,434.18},
		{706.56,370.69}, {563.20,288.77}, {471.04,247.81}, {399.36,217.09},
		
		{353.28,185.34}, {281.60,144.38}, {235.52,123.90}, {199.68,108.54},
		{176.76,92.67}, {140.80,72.19}, {117.76,61.95}, {99.84,54.27},
		{88.32,46.34}, {70.40,36.10}, {58.88,30.98}, {49.92,27.14},
		{44.16,23.17}, {35.20,18.05}, {29.44,15.49}, {24.96,13.57},
		
		{22.08,11.58}, {17.60,9.02}, {14.72,7.74}, {12.48,6.78},
		{11.04,5.79}, {8.80,4.51}, {7.36,3.87}, {6.24,3.39},
		{5.52,2.90}, {4.40,2.26}, {3.68,1.94}, {3.12,1.70},
		{2.76,1.45}, {2.20,1.13}, {1.84,0.97}, {1.56,0.85},
		
		{1.40,0.73}, {1.12,0.61}, {0.92,0.49}, {0.80,0.43},
		{0.70,0.37}, {0.56,0.31}, {0.46,0.26}, {0.42,0.22},
		{0.38,0.19}, {0.30,0.14}, {0.24,0.11}, {0.20,0.11},
		{0.00,0.00}, {0.00,0.00}, {0.00,0.00}, {0.00,0.00}
	};

	// These decay and release periods in milliseconds were taken from the YMF278B manual. 
	// The rate index range from 0 to 63, with different data for 
	// 0%-100% and for 10%-90%: 
	static const double decayAndReleaseTimeValuesTable[64][2] = {
		{MUGEN,MUGEN},    {MUGEN,MUGEN},    {MUGEN,MUGEN},    {MUGEN,MUGEN},
		{39280.64,8212.48}, {31416.32,6574.08}, {26173.44,5509.12}, {22446.08,4730.88},
		{19640.32,4106.24}, {15708.16,3287.04}, {13086.72,2754.56}, {11223.04,2365.44},
		{9820.16,2053.12}, {7854.08,1643.52}, {6543.36,1377.28}, {5611.52,1182.72},
		
		{4910.08,1026.56}, {3927.04,821.76}, {3271.68,688.64}, {2805.76,591.36},
		{2455.04,513.28}, {1936.52,410.88}, {1635.84,344.34}, {1402.88,295.68},
		{1227.52,256.64}, {981.76,205.44}, {817.92,172.16}, {701.44,147.84},
		{613.76,128.32}, {490.88,102.72}, {488.96,86.08}, {350.72,73.92},
		
		{306.88,64.16}, {245.44,51.36}, {204.48,43.04}, {175.36,36.96},
		{153.44,32.08}, {122.72,25.68}, {102.24,21.52}, {87.68,18.48},
		{76.72,16.04}, {61.36,12.84}, {51.12,10.76}, {43.84,9.24},
		{38.36,8.02}, {30.68,6.42}, {25.56,5.38}, {21.92,4.62},
		
		{19.20,4.02}, {15.36,3.22}, {12.80,2.68}, {10.96,2.32},
		{9.60,2.02}, {7.68,1.62}, {6.40,1.35}, {5.48,1.15},
		{4.80,1.01}, {3.84,0.81}, {3.20,0.69}, {2.74,0.58},
		{2.40,0.51}, {2.40,0.51}, {2.40,0.51}, {2.40,0.51}
	};
};

class OPL3 : public OPLEmul
{
public:
	BYTE registers[0x200];

	Operator *operators[2][0x20];
	Channel2op *channels2op[2][9];
	Channel4op *channels4op[2][3];
	Channel *channels[2][9];

    // Unique instance to fill future gaps in the Channel array,
    // when there will be switches between 2op and 4op mode.
	DisabledChannel disabledChannel;
	
    // Specific operators to switch when in rhythm mode:
	HighHatOperator highHatOperator;
	SnareDrumOperator snareDrumOperator;
	TomTomOperator tomTomOperator;
	TomTomTopCymbalChannel tomTomTopCymbalChannel;

	// Rhythm channels
	BassDrumChannel bassDrumChannel;
	HighHatSnareDrumChannel highHatSnareDrumChannel;
	TopCymbalOperator topCymbalOperator;

	Operator *highHatOperatorInNonRhythmMode;
	Operator *snareDrumOperatorInNonRhythmMode;
	Operator *tomTomOperatorInNonRhythmMode;
	Operator *topCymbalOperatorInNonRhythmMode;
			
	int nts, dam, dvb, ryt, bd, sd, tom, tc, hh, _new, connectionsel;
	int vibratoIndex, tremoloIndex;

	bool FullPan;
	
	static OperatorDataStruct *OperatorData;
	static OPL3DataStruct *OPL3Data;

	// The methods read() and write() are the only 
	// ones needed by the user to interface with the emulator.
	// read() returns one frame at a time, to be played at 49700 Hz, 
	// with each frame being four 16-bit samples,
	// corresponding to the OPL3 four output channels CHA...CHD.
public:
	//void read(float output[2]);
	void write(int array, int address, int data);

	OPL3(bool fullpan);
	~OPL3();
	
private:
	void initOperators();
	void initChannels2op();
	void initChannels4op();
	void initRhythmChannels();
	void initChannels();
	void update_1_NTS1_6();
	void update_DAM1_DVB1_RYT1_BD1_SD1_TOM1_TC1_HH1();
	void update_7_NEW1();
	void setEnabledChannels();
	void updateChannelPans();
	void update_2_CONNECTIONSEL6();
	void set4opConnections();
	void setRhythmMode();

	static int InstanceCount;

	// OPLEmul interface
public:
	void Reset();
	void WriteReg(int reg, int v);
	void Update(float *buffer, int length);
	void SetPanning(int c, float left, float right);
};

OperatorDataStruct *OPL3::OperatorData;
OPL3DataStruct *OPL3::OPL3Data;
int OPL3::InstanceCount;

void OPL3::Update(float *output, int numsamples) {
	while (numsamples--) {
		// If _new = 0, use OPL2 mode with 9 channels. If _new = 1, use OPL3 18 channels;
		for(int array=0; array < (_new + 1); array++)
			for(int channelNumber=0; channelNumber < 9; channelNumber++) {
				// Reads output from each OPL3 channel, and accumulates it in the output buffer:
				Channel *channel = channels[array][channelNumber];
				if (channel != &disabledChannel)
				{
					double channelOutput = channel->getChannelOutput(this);
					// We don't need no stinking stereo
					//output[0] += float(channelOutput * channel->leftPan);
					//output[1] += float(channelOutput * channel->rightPan);
					output[0] += float(channelOutput);
				}
			}
			output[0] /= 2.0f;	// scale output down to avoid clipping

		// Advances the OPL3-wide vibrato index, which is used by 
		// PhaseGenerator.getPhase() in each Operator.
		vibratoIndex = (vibratoIndex + 1) & (OPL3DataStruct::vibratoTableLength - 1);
		// Advances the OPL3-wide tremolo index, which is used by 
		// EnvelopeGenerator.getEnvelope() in each Operator.
		tremoloIndex++;
		if(tremoloIndex >= OPL3DataStruct::tremoloTableLength) tremoloIndex = 0;
		// We don't need no stinking stereo
		//output += 2;
		output += 1;
	}
}

void OPL3::write(int array, int address, int data) {
    // The OPL3 has two registers arrays, each with adresses ranging
    // from 0x00 to 0xF5.
    // This emulator uses one array, with the two original register arrays
    // starting at 0x00 and at 0x100.
    int registerAddress = (array<<8) | address;        
    // If the address is out of the OPL3 memory map, returns.
    if(registerAddress<0 || registerAddress>=0x200) return;

    registers[registerAddress] = (BYTE)data;
    switch(address&0xE0) {
        // The first 3 bits masking gives the type of the register by using its base address:
        // 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xE0 
        // When it is needed, we further separate the register type inside each base address,
        // which is the case of 0x00 and 0xA0.
        
        // Through out this emulator we will use the same name convention to
        // reference a byte with several bit registers.
        // The name of each bit register will be followed by the number of bits
        // it occupies inside the byte. 
        // Numbers without accompanying names are unused bits.
        case 0x00:
            // Unique registers for the entire OPL3:                
           if(array==1) {
                if(address==0x04) 
                    update_2_CONNECTIONSEL6();
                else if(address==0x05) 
                    update_7_NEW1();
            }
            else if(address==0x08) update_1_NTS1_6();
            break;
            
        case 0xA0:
            // 0xBD is a control register for the entire OPL3:
            if(address==0xBD) {
                if(array==0) 
                    update_DAM1_DVB1_RYT1_BD1_SD1_TOM1_TC1_HH1();
                break;
            }
            // Registers for each channel are in A0-A8, B0-B8, C0-C8, in both register arrays.
            // 0xB0...0xB8 keeps kon,block,fnum(h) for each channel.
            if( (address&0xF0) == 0xB0 && address <= 0xB8) {
                // If the address is in the second register array, adds 9 to the channel number.
                // The channel number is given by the last four bits, like in A0,...,A8.
                channels[array][address&0x0F]->update_2_KON1_BLOCK3_FNUMH2(this);
                break;                    
            }
            // 0xA0...0xA8 keeps fnum(l) for each channel.
            if( (address&0xF0) == 0xA0 && address <= 0xA8)
                channels[array][address&0x0F]->update_FNUML8(this);
            break;                    
        // 0xC0...0xC8 keeps cha,chb,chc,chd,fb,cnt for each channel:
        case 0xC0:
            if(address <= 0xC8)
                channels[array][address&0x0F]->update_CHD1_CHC1_CHB1_CHA1_FB3_CNT1(this);
            break;
            
        // Registers for each of the 36 Operators:
        default:
            int operatorOffset = address&0x1F;
            if(operators[array][operatorOffset] == NULL) break;
            switch(address&0xE0) {
                // 0x20...0x35 keeps am,vib,egt,ksr,mult for each operator:                
                case 0x20:
                    operators[array][operatorOffset]->update_AM1_VIB1_EGT1_KSR1_MULT4(this);
                    break;
                // 0x40...0x55 keeps ksl,tl for each operator: 
                case 0x40:
                    operators[array][operatorOffset]->update_KSL2_TL6(this);
                    break;
                // 0x60...0x75 keeps ar,dr for each operator: 
                case 0x60:
                    operators[array][operatorOffset]->update_AR4_DR4(this);
                    break;
                // 0x80...0x95 keeps sl,rr for each operator:
                case 0x80:
                    operators[array][operatorOffset]->update_SL4_RR4(this);
                    break;
                // 0xE0...0xF5 keeps ws for each operator:
                case 0xE0:     
                    operators[array][operatorOffset]->update_5_WS3(this);
            }
    }
}

OPL3::OPL3(bool fullpan)
: tomTomTopCymbalChannel(fullpan ? CENTER_PANNING_POWER : 1, &tomTomOperator, &topCymbalOperator),
  bassDrumChannel(fullpan ? CENTER_PANNING_POWER : 1),
  highHatSnareDrumChannel(fullpan ? CENTER_PANNING_POWER : 1, &highHatOperator, &snareDrumOperator)
{
	FullPan = fullpan;
    nts = dam = dvb = ryt = bd = sd = tom = tc = hh = _new = connectionsel = 0;
    vibratoIndex = tremoloIndex = 0; 

	if (InstanceCount++ == 0)
	{
		OPL3Data = new struct OPL3DataStruct;
		OperatorData = new struct OperatorDataStruct;
	}

    initOperators();
    initChannels2op();
    initChannels4op();
    initRhythmChannels();
    initChannels();
}

OPL3::~OPL3()
{
	ryt = 0;
	setRhythmMode();	// Make sure all operators point to the dynamically allocated ones.
	for (int array = 0; array < 2; array++)
	{
		for (int operatorNumber = 0; operatorNumber < 0x20; operatorNumber++)
		{
			if (operators[array][operatorNumber] != NULL)
			{
				delete operators[array][operatorNumber];
			}
		}
		for (int channelNumber = 0; channelNumber < 9; channelNumber++)
		{
			delete channels2op[array][channelNumber];
		}
		for (int channelNumber = 0; channelNumber < 3; channelNumber++)
		{
			delete channels4op[array][channelNumber];
		}
	}
	if (--InstanceCount == 0)
	{
		delete OPL3Data;
		OPL3Data = NULL;
		delete OperatorData;
		OperatorData = NULL;
	}
}


void OPL3::initOperators() {
    int baseAddress;
    // The YMF262 has 36 operators:
	memset(operators, 0, sizeof(operators));
    for(int array=0; array<2; array++)
        for(int group = 0; group<=0x10; group+=8)
            for(int offset=0; offset<6; offset++) {
                baseAddress = (array<<8) | (group+offset);
                operators[array][group+offset] = new Operator(baseAddress);
            }
    
    // Save operators when they are in non-rhythm mode:
    // Channel 7:
    highHatOperatorInNonRhythmMode = operators[0][0x11];
    snareDrumOperatorInNonRhythmMode = operators[0][0x14];
    // Channel 8:
    tomTomOperatorInNonRhythmMode = operators[0][0x12];
    topCymbalOperatorInNonRhythmMode = operators[0][0x15];
    
}

void OPL3::initChannels2op() {
    // The YMF262 has 18 2-op channels.
    // Each 2-op channel can be at a serial or parallel operator configuration:
    memset(channels2op, 0, sizeof(channels2op));
	double startvol = FullPan ? CENTER_PANNING_POWER : 1;
    for(int array=0; array<2; array++)
        for(int channelNumber=0; channelNumber<3; channelNumber++) {
            int baseAddress = (array<<8) | channelNumber;
            // Channels 1, 2, 3 -> Operator offsets 0x0,0x3; 0x1,0x4; 0x2,0x5
            channels2op[array][channelNumber]   = new Channel2op(baseAddress, startvol, operators[array][channelNumber], operators[array][channelNumber+0x3]);
            // Channels 4, 5, 6 -> Operator offsets 0x8,0xB; 0x9,0xC; 0xA,0xD
            channels2op[array][channelNumber+3] = new Channel2op(baseAddress+3, startvol, operators[array][channelNumber+0x8], operators[array][channelNumber+0xB]);
            // Channels 7, 8, 9 -> Operators 0x10,0x13; 0x11,0x14; 0x12,0x15
            channels2op[array][channelNumber+6] = new Channel2op(baseAddress+6, startvol, operators[array][channelNumber+0x10], operators[array][channelNumber+0x13]);
        }   
}

void OPL3::initChannels4op() {
    // The YMF262 has 3 4-op channels in each array:
	memset(channels4op, 0, sizeof(channels4op));
	double startvol = FullPan ? CENTER_PANNING_POWER : 1;
    for(int array=0; array<2; array++)
        for(int channelNumber=0; channelNumber<3; channelNumber++) {
            int baseAddress = (array<<8) | channelNumber;
            // Channels 1, 2, 3 -> Operators 0x0,0x3,0x8,0xB; 0x1,0x4,0x9,0xC; 0x2,0x5,0xA,0xD;
            channels4op[array][channelNumber]   = new Channel4op(baseAddress, startvol, operators[array][channelNumber], operators[array][channelNumber+0x3], operators[array][channelNumber+0x8], operators[array][channelNumber+0xB]);
        }   
}

void OPL3::initRhythmChannels() {
}

void OPL3::initChannels() {
    // Channel is an abstract class that can be a 2-op, 4-op, rhythm or disabled channel, 
    // depending on the OPL3 configuration at the time.
    // channels[] inits as a 2-op serial channel array:
    for(int array=0; array<2; array++)
        for(int i=0; i<9; i++) channels[array][i] = channels2op[array][i];
}

void OPL3::update_1_NTS1_6() {
	int _1_nts1_6 = registers[OPL3DataStruct::_1_NTS1_6_Offset];
    // Note Selection. This register is used in Channel.updateOperators() implementations,
    // to calculate the channel4s Key Scale Number.
    // The value of the actual envelope rate follows the value of
    // OPL3.nts,Operator.keyScaleNumber and Operator.ksr
    nts = (_1_nts1_6 & 0x40) >> 6;
}

void OPL3::update_DAM1_DVB1_RYT1_BD1_SD1_TOM1_TC1_HH1() {
	int dam1_dvb1_ryt1_bd1_sd1_tom1_tc1_hh1 = registers[OPL3DataStruct::DAM1_DVB1_RYT1_BD1_SD1_TOM1_TC1_HH1_Offset];
    // Depth of amplitude. This register is used in EnvelopeGenerator.getEnvelope();
    dam = (dam1_dvb1_ryt1_bd1_sd1_tom1_tc1_hh1 & 0x80) >> 7;
    
    // Depth of vibrato. This register is used in PhaseGenerator.getPhase();
    dvb = (dam1_dvb1_ryt1_bd1_sd1_tom1_tc1_hh1 & 0x40) >> 6;
    
    int new_ryt = (dam1_dvb1_ryt1_bd1_sd1_tom1_tc1_hh1 & 0x20) >> 5;
    if(new_ryt != ryt) {
        ryt = new_ryt;
        setRhythmMode();               
    }
    
    int new_bd  = (dam1_dvb1_ryt1_bd1_sd1_tom1_tc1_hh1 & 0x10) >> 4;
    if(new_bd != bd) {
        bd = new_bd;
        if(bd==1) {
            bassDrumChannel.op1->keyOn();
            bassDrumChannel.op2->keyOn();
        }
    }
    
    int new_sd  = (dam1_dvb1_ryt1_bd1_sd1_tom1_tc1_hh1 & 0x08) >> 3;
    if(new_sd != sd) {
        sd = new_sd;
        if(sd==1) snareDrumOperator.keyOn();
    }
    
    int new_tom = (dam1_dvb1_ryt1_bd1_sd1_tom1_tc1_hh1 & 0x04) >> 2;
    if(new_tom != tom) {
        tom = new_tom;
        if(tom==1) tomTomOperator.keyOn();
    }
    
    int new_tc  = (dam1_dvb1_ryt1_bd1_sd1_tom1_tc1_hh1 & 0x02) >> 1;
    if(new_tc != tc) {
        tc = new_tc;
        if(tc==1) topCymbalOperator.keyOn();
    }
    
    int new_hh  = dam1_dvb1_ryt1_bd1_sd1_tom1_tc1_hh1 & 0x01;
    if(new_hh != hh) {
        hh = new_hh;
        if(hh==1) highHatOperator.keyOn();
    }
    
}

void OPL3::update_7_NEW1() {
	int _7_new1 = registers[OPL3DataStruct::_7_NEW1_Offset];
    // OPL2/OPL3 mode selection. This register is used in 
    // OPL3.read(), OPL3.write() and Operator.getOperatorOutput();
    _new = (_7_new1 & 0x01);
    if(_new==1) setEnabledChannels();
    set4opConnections();
	updateChannelPans();
}

void OPL3::setEnabledChannels() {
    for(int array=0; array<2; array++)
        for(int i=0; i<9; i++) {
            int baseAddress = channels[array][i]->channelBaseAddress;
			registers[baseAddress+ChannelData::CHD1_CHC1_CHB1_CHA1_FB3_CNT1_Offset] |= 0xF0;
            channels[array][i]->update_CHD1_CHC1_CHB1_CHA1_FB3_CNT1(this);
        }
}

void OPL3::updateChannelPans() {
    for(int array=0; array<2; array++)
        for(int i=0; i<9; i++) {
            int baseAddress = channels[array][i]->channelBaseAddress;
			registers[baseAddress+ChannelData::CHD1_CHC1_CHB1_CHA1_FB3_CNT1_Offset] |= 0xF0;
            channels[array][i]->updatePan(this);
        }

}

void OPL3::update_2_CONNECTIONSEL6() {
    // This method is called only if _new is set.
	int _2_connectionsel6 = registers[OPL3DataStruct::_2_CONNECTIONSEL6_Offset];
    // 2-op/4-op channel selection. This register is used here to configure the OPL3.channels[] array.
    connectionsel = (_2_connectionsel6 & 0x3F);
    set4opConnections();
}

void OPL3::set4opConnections() {
    // bits 0, 1, 2 sets respectively 2-op channels (1,4), (2,5), (3,6) to 4-op operation.
    // bits 3, 4, 5 sets respectively 2-op channels (10,13), (11,14), (12,15) to 4-op operation.
    for(int array=0; array<2; array++)
        for(int i=0; i<3; i++) {
            if(_new == 1) {
                int shift = array*3 + i;
                int connectionBit = (connectionsel >> shift) & 0x01;
                if(connectionBit == 1) {
                    channels[array][i] = channels4op[array][i];
                    channels[array][i+3] = &disabledChannel;
                    channels[array][i]->updateChannel(this);
                    continue;    
                }
            }
            channels[array][i] = channels2op[array][i];
            channels[array][i+3] = channels2op[array][i+3];
            channels[array][i]->updateChannel(this);
            channels[array][i+3]->updateChannel(this);
        }
} 

void OPL3::setRhythmMode() {
    if(ryt==1) {
        channels[0][6] = &bassDrumChannel;
        channels[0][7] = &highHatSnareDrumChannel;
        channels[0][8] = &tomTomTopCymbalChannel;
        operators[0][0x11] = &highHatOperator;
        operators[0][0x14] = &snareDrumOperator;
        operators[0][0x12] = &tomTomOperator;
        operators[0][0x15] = &topCymbalOperator;
    }
    else {
        for(int i=6; i<=8; i++) channels[0][i] = channels2op[0][i];
        operators[0][0x11] = highHatOperatorInNonRhythmMode;
        operators[0][0x14] = snareDrumOperatorInNonRhythmMode;
        operators[0][0x12] = tomTomOperatorInNonRhythmMode;
        operators[0][0x15] = topCymbalOperatorInNonRhythmMode;                
    }
    for(int i=6; i<=8; i++) channels[0][i]->updateChannel(this);
}

static double EnvelopeFromDB(double db)
{
#if 0
	return pow(10.0, db/10);
#else
	if (db < MIN_DB)
		return 0;
	return OPL3::OperatorData->dbpow[xs_FloorToInt(-db * DB_TABLE_RES)];
#endif
}

Channel::Channel (int baseAddress, double startvol) {
	channelBaseAddress = baseAddress;
	fnuml = fnumh = kon = block = fb = cnt = 0;
	feedback[0] = feedback[1] = 0;
	leftPan = rightPan = startvol;
}

void Channel::update_2_KON1_BLOCK3_FNUMH2(OPL3 *OPL3) {
	
	int _2_kon1_block3_fnumh2 = OPL3->registers[channelBaseAddress+ChannelData::_2_KON1_BLOCK3_FNUMH2_Offset];
	
	// Frequency Number (hi-register) and Block. These two registers, together with fnuml, 
	// sets the Channel4s base frequency;
	block = (_2_kon1_block3_fnumh2 & 0x1C) >> 2;
	fnumh = _2_kon1_block3_fnumh2 & 0x03;        
	updateOperators(OPL3);
	
	// Key On. If changed, calls Channel.keyOn() / keyOff().
	int newKon   = (_2_kon1_block3_fnumh2 & 0x20) >> 5;
	if(newKon != kon) {
		if(newKon == 1) keyOn();
		else keyOff();
		kon = newKon;
	}
}

void Channel::update_FNUML8(OPL3 *OPL3) {
	int fnuml8 = OPL3->registers[channelBaseAddress+ChannelData::FNUML8_Offset];
	// Frequency Number, low register.
	fnuml = fnuml8&0xFF;
	updateOperators(OPL3);
}

void Channel::update_CHD1_CHC1_CHB1_CHA1_FB3_CNT1(OPL3 *OPL3) {
	int chd1_chc1_chb1_cha1_fb3_cnt1 = OPL3->registers[channelBaseAddress+ChannelData::CHD1_CHC1_CHB1_CHA1_FB3_CNT1_Offset];
//	chd   = (chd1_chc1_chb1_cha1_fb3_cnt1 & 0x80) >> 7;
//	chc   = (chd1_chc1_chb1_cha1_fb3_cnt1 & 0x40) >> 6;
	chb   = (chd1_chc1_chb1_cha1_fb3_cnt1 & 0x20) >> 5;
	cha   = (chd1_chc1_chb1_cha1_fb3_cnt1 & 0x10) >> 4;
	fb    = (chd1_chc1_chb1_cha1_fb3_cnt1 & 0x0E) >> 1;
	cnt   = chd1_chc1_chb1_cha1_fb3_cnt1 & 0x01;
	updatePan(OPL3);
	updateOperators(OPL3);
}

void Channel::updatePan(OPL3 *OPL3) {
	if (!OPL3->FullPan)
	{
		if (OPL3->_new == 0)
		{
			leftPan = VOLUME_MUL;
			rightPan = VOLUME_MUL;
		}
		else
		{
			leftPan = cha * VOLUME_MUL;
			rightPan = chb * VOLUME_MUL;
		}
	}
}

void Channel::updateChannel(OPL3 *OPL3) {
	update_2_KON1_BLOCK3_FNUMH2(OPL3);
	update_FNUML8(OPL3);
	update_CHD1_CHC1_CHB1_CHA1_FB3_CNT1(OPL3);
}

Channel2op::Channel2op (int baseAddress, double startvol, Operator *o1, Operator *o2)
: Channel(baseAddress, startvol)
{
	op1 = o1;
	op2 = o2;
}

double Channel2op::getChannelOutput(OPL3 *OPL3) {
	double channelOutput = 0, op1Output = 0, op2Output = 0;
	// The feedback uses the last two outputs from
	// the first operator, instead of just the last one. 
	double feedbackOutput = (feedback[0] + feedback[1]) / 2;
	
	switch(cnt) {
		// CNT = 0, the operators are in series, with the first in feedback.
		case 0:
			if(op2->envelopeGenerator.stage==EnvelopeGenerator::OFF) 
				return 0;
			op1Output = op1->getOperatorOutput(OPL3, feedbackOutput);
			channelOutput = op2->getOperatorOutput(OPL3, op1Output*toPhase);
			break;
		// CNT = 1, the operators are in parallel, with the first in feedback.
		case 1:
			if(op1->envelopeGenerator.stage==EnvelopeGenerator::OFF && 
				op2->envelopeGenerator.stage==EnvelopeGenerator::OFF) 
					return 0;
			op1Output = op1->getOperatorOutput(OPL3, feedbackOutput);
			op2Output = op2->getOperatorOutput(OPL3, Operator::noModulator);
			channelOutput = (op1Output + op2Output) / 2;
	}
	
	feedback[0] = feedback[1];
	feedback[1] = StripIntPart(op1Output * ChannelData::feedback[fb]);
	return channelOutput;
}

void Channel2op::keyOn() {
	op1->keyOn();
	op2->keyOn();
	feedback[0] = feedback[1] = 0;
}

void Channel2op::keyOff() {
	op1->keyOff();
	op2->keyOff();
}

void Channel2op::updateOperators(OPL3 *OPL3) {
	// Key Scale Number, used in EnvelopeGenerator.setActualRates().
	int keyScaleNumber = block*2 + ((fnumh>>OPL3->nts)&0x01);
	int f_number = (fnumh<<8) | fnuml;
	op1->updateOperator(OPL3, keyScaleNumber, f_number, block);
	op2->updateOperator(OPL3, keyScaleNumber, f_number, block);
}

Channel4op::Channel4op (int baseAddress, double startvol, Operator *o1, Operator *o2, Operator *o3, Operator *o4)
: Channel(baseAddress, startvol)
{
	op1 = o1;
	op2 = o2;
	op3 = o3;
	op4 = o4;
}

double Channel4op::getChannelOutput(OPL3 *OPL3) {
	double channelOutput = 0, 
		   op1Output = 0, op2Output = 0, op3Output = 0, op4Output = 0;
	
	int secondChannelBaseAddress = channelBaseAddress+3;
	int secondCnt = OPL3->registers[secondChannelBaseAddress+ChannelData::CHD1_CHC1_CHB1_CHA1_FB3_CNT1_Offset] & 0x1;
	int cnt4op = (cnt << 1) | secondCnt;
	
	double feedbackOutput = (feedback[0] + feedback[1]) / 2;
	
	switch(cnt4op) {
		case 0:
			if(op4->envelopeGenerator.stage==EnvelopeGenerator::OFF) 
				return 0;
			
			op1Output = op1->getOperatorOutput(OPL3, feedbackOutput);
			op2Output = op2->getOperatorOutput(OPL3, op1Output*toPhase);
			op3Output = op3->getOperatorOutput(OPL3, op2Output*toPhase);
			channelOutput = op4->getOperatorOutput(OPL3, op3Output*toPhase);

			break;
		case 1:
			if(op2->envelopeGenerator.stage==EnvelopeGenerator::OFF && 
				op4->envelopeGenerator.stage==EnvelopeGenerator::OFF) 
				   return 0;
			
			op1Output = op1->getOperatorOutput(OPL3, feedbackOutput);
			op2Output = op2->getOperatorOutput(OPL3, op1Output*toPhase);
			
			op3Output = op3->getOperatorOutput(OPL3, Operator::noModulator);
			op4Output = op4->getOperatorOutput(OPL3, op3Output*toPhase);

			channelOutput = (op2Output + op4Output) / 2;
			break;
		case 2:
			if(op1->envelopeGenerator.stage==EnvelopeGenerator::OFF && 
				op4->envelopeGenerator.stage==EnvelopeGenerator::OFF) 
				   return 0;

			op1Output = op1->getOperatorOutput(OPL3, feedbackOutput);
			
			op2Output = op2->getOperatorOutput(OPL3, Operator::noModulator);
			op3Output = op3->getOperatorOutput(OPL3, op2Output*toPhase);
			op4Output = op4->getOperatorOutput(OPL3, op3Output*toPhase);

			channelOutput = (op1Output + op4Output) / 2;
			break;
		case 3:
			if(op1->envelopeGenerator.stage==EnvelopeGenerator::OFF && 
				op3->envelopeGenerator.stage==EnvelopeGenerator::OFF && 
				op4->envelopeGenerator.stage==EnvelopeGenerator::OFF) 
				   return 0;
			
			op1Output = op1->getOperatorOutput(OPL3, feedbackOutput);
			
			op2Output = op2->getOperatorOutput(OPL3, Operator::noModulator);
			op3Output = op3->getOperatorOutput(OPL3, op2Output*toPhase);
			
			op4Output = op4->getOperatorOutput(OPL3, Operator::noModulator);

			channelOutput = (op1Output + op3Output + op4Output) / 3;
	}
	
	feedback[0] = feedback[1];
	feedback[1] = StripIntPart(op1Output * ChannelData::feedback[fb]);

	return channelOutput;
}

void Channel4op::keyOn() {
	op1->keyOn();
	op2->keyOn();
	op3->keyOn();
	op4->keyOn();
	feedback[0] = feedback[1] = 0;
}

void Channel4op::keyOff() {
	op1->keyOff();
	op2->keyOff();
	op3->keyOff();
	op4->keyOff();
}

void Channel4op::updateOperators(OPL3 *OPL3) {
	// Key Scale Number, used in EnvelopeGenerator.setActualRates().
	int keyScaleNumber = block*2 + ((fnumh>>OPL3->nts)&0x01);
	int f_number = (fnumh<<8) | fnuml;
	op1->updateOperator(OPL3, keyScaleNumber, f_number, block);
	op2->updateOperator(OPL3, keyScaleNumber, f_number, block);
	op3->updateOperator(OPL3, keyScaleNumber, f_number, block);
	op4->updateOperator(OPL3, keyScaleNumber, f_number, block);
}

const double Operator::noModulator = 0;

Operator::Operator(int baseAddress) {
	operatorBaseAddress = baseAddress;

	envelope = 0;
	am = vib = ksr = egt = mult = ksl = tl = ar = dr = sl = rr = ws = 0;
	keyScaleNumber = f_number = block = 0;
}

void Operator::update_AM1_VIB1_EGT1_KSR1_MULT4(OPL3 *OPL3) {
	
	int am1_vib1_egt1_ksr1_mult4 = OPL3->registers[operatorBaseAddress+OperatorDataStruct::AM1_VIB1_EGT1_KSR1_MULT4_Offset];
	
	// Amplitude Modulation. This register is used int EnvelopeGenerator.getEnvelope();
	am  = (am1_vib1_egt1_ksr1_mult4 & 0x80) >> 7;
	// Vibrato. This register is used in PhaseGenerator.getPhase();
	vib = (am1_vib1_egt1_ksr1_mult4 & 0x40) >> 6;
	// Envelope Generator Type. This register is used in EnvelopeGenerator.getEnvelope();
	egt = (am1_vib1_egt1_ksr1_mult4 & 0x20) >> 5;
	// Key Scale Rate. Sets the actual envelope rate together with rate and keyScaleNumber.
	// This register os used in EnvelopeGenerator.setActualAttackRate().
	ksr = (am1_vib1_egt1_ksr1_mult4 & 0x10) >> 4;
	// Multiple. Multiplies the Channel.baseFrequency to get the Operator.operatorFrequency.
	// This register is used in PhaseGenerator.setFrequency().
	mult = am1_vib1_egt1_ksr1_mult4 & 0x0F;
	
	phaseGenerator.setFrequency(f_number, block, mult);
	envelopeGenerator.setActualAttackRate(ar, ksr, keyScaleNumber);
	envelopeGenerator.setActualDecayRate(dr, ksr, keyScaleNumber); 
	envelopeGenerator.setActualReleaseRate(rr, ksr, keyScaleNumber);
}

void Operator::update_KSL2_TL6(OPL3 *OPL3) {
	
	int ksl2_tl6 = OPL3->registers[operatorBaseAddress+OperatorDataStruct::KSL2_TL6_Offset];
	
	// Key Scale Level. Sets the attenuation in accordance with the octave.
	ksl = (ksl2_tl6 & 0xC0) >> 6;
	// Total Level. Sets the overall damping for the envelope.
	tl  =  ksl2_tl6 & 0x3F;
	
	envelopeGenerator.setAtennuation(f_number, block, ksl);
	envelopeGenerator.setTotalLevel(tl);
}

void Operator::update_AR4_DR4(OPL3 *OPL3) {
	
	int ar4_dr4 = OPL3->registers[operatorBaseAddress+OperatorDataStruct::AR4_DR4_Offset];
	
	// Attack Rate.
	ar = (ar4_dr4 & 0xF0) >> 4;
	// Decay Rate.
	dr =  ar4_dr4 & 0x0F;

	envelopeGenerator.setActualAttackRate(ar, ksr, keyScaleNumber);        
	envelopeGenerator.setActualDecayRate(dr, ksr, keyScaleNumber); 
}

void Operator::update_SL4_RR4(OPL3 *OPL3) {     
	
	int sl4_rr4 = OPL3->registers[operatorBaseAddress+OperatorDataStruct::SL4_RR4_Offset];
	
	// Sustain Level.
	sl = (sl4_rr4 & 0xF0) >> 4;
	// Release Rate.
	rr =  sl4_rr4 & 0x0F;
	
	envelopeGenerator.setActualSustainLevel(sl);        
	envelopeGenerator.setActualReleaseRate(rr, ksr, keyScaleNumber);        
}

void Operator::update_5_WS3(OPL3 *OPL3) {     
	int _5_ws3 = OPL3->registers[operatorBaseAddress+OperatorDataStruct::_5_WS3_Offset];
	ws =  _5_ws3 & 0x07;
}

double Operator::getOperatorOutput(OPL3 *OPL3, double modulator) {
	if(envelopeGenerator.stage == EnvelopeGenerator::OFF) return 0;
	
	double envelopeInDB = envelopeGenerator.getEnvelope(OPL3, egt, am);
	envelope = EnvelopeFromDB(envelopeInDB);
	
	// If it is in OPL2 mode, use first four waveforms only:
	ws &= ((OPL3->_new<<2) + 3); 
	double *waveform = OPL3::OperatorData->waveforms[ws];
	
	phase = phaseGenerator.getPhase(OPL3, vib);
	
	double operatorOutput = getOutput(modulator, phase, waveform);
	return operatorOutput;
}

double Operator::getOutput(double modulator, double outputPhase, double *waveform) {
	int sampleIndex = xs_FloorToInt((outputPhase + modulator) * OperatorDataStruct::waveLength) & (OperatorDataStruct::waveLength - 1);
	return waveform[sampleIndex] * envelope;
}    

void Operator::keyOn() {
	if(ar > 0) {
		envelopeGenerator.keyOn();
		phaseGenerator.keyOn();
	}
	else envelopeGenerator.stage = EnvelopeGenerator::OFF;
}

void Operator::keyOff() {
	envelopeGenerator.keyOff();
}

void Operator::updateOperator(OPL3 *OPL3, int ksn, int f_num, int blk) {
	keyScaleNumber = ksn;
	f_number = f_num;
	block = blk;
	update_AM1_VIB1_EGT1_KSR1_MULT4(OPL3);
	update_KSL2_TL6(OPL3);
	update_AR4_DR4(OPL3);
	update_SL4_RR4(OPL3);
	update_5_WS3(OPL3);
}

EnvelopeGenerator::EnvelopeGenerator() {
	stage = OFF;
	actualAttackRate = actualDecayRate = actualReleaseRate = 0;
	xAttackIncrement = xMinimumInAttack = 0;
	dBdecayIncrement = 0;
	dBreleaseIncrement = 0;
	attenuation = totalLevel = sustainLevel = 0;
	x = dBtoX(-96);
	envelope = -96;
}

void EnvelopeGenerator::setActualSustainLevel(int sl) {
	// If all SL bits are 1, sustain level is set to -93 dB:
   if(sl == 0x0F) {
	   sustainLevel = -93;
	   return;
   } 
   // The datasheet states that the SL formula is
   // sustainLevel = -24*d7 -12*d6 -6*d5 -3*d4,
   // translated as:
   sustainLevel = -3*sl;
}

void EnvelopeGenerator::setTotalLevel(int tl) {
   // The datasheet states that the TL formula is
   // TL = -(24*d5 + 12*d4 + 6*d3 + 3*d2 + 1.5*d1 + 0.75*d0),
   // translated as:
   totalLevel = tl*-0.75;
}

void EnvelopeGenerator::setAtennuation(int f_number, int block, int ksl) {
	int hi4bits = (f_number>>6)&0x0F;
	switch(ksl) {
		case 0:
			attenuation = 0;
			break;
		case 1:
			// ~3 dB/Octave
			attenuation = OperatorDataStruct::ksl3dBtable[hi4bits][block];
			break;
		case 2:
			// ~1.5 dB/Octave
			attenuation = OperatorDataStruct::ksl3dBtable[hi4bits][block]/2;
			break;
		case 3:
			// ~6 dB/Octave
			attenuation = OperatorDataStruct::ksl3dBtable[hi4bits][block]*2;
	}
}

void EnvelopeGenerator::setActualAttackRate(int attackRate, int ksr, int keyScaleNumber) {
	// According to the YMF278B manual's OPL3 section, the attack curve is exponential,
	// with a dynamic range from -96 dB to 0 dB and a resolution of 0.1875 dB 
	// per level.
	//
	// This method sets an attack increment and attack minimum value 
	// that creates a exponential dB curve with 'period0to100' seconds in length
	// and 'period10to90' seconds between 10% and 90% of the curve total level.
	actualAttackRate = calculateActualRate(attackRate, ksr, keyScaleNumber);
	double period0to100inSeconds = EnvelopeGeneratorData::attackTimeValuesTable[actualAttackRate][0]/1000.0;
	int period0to100inSamples = (int)(period0to100inSeconds*OPL_SAMPLE_RATE);       
	double period10to90inSeconds = EnvelopeGeneratorData::attackTimeValuesTable[actualAttackRate][1]/1000.0;
	int period10to90inSamples = (int)(period10to90inSeconds*OPL_SAMPLE_RATE);
	// The x increment is dictated by the period between 10% and 90%:
	xAttackIncrement = OPL3DataStruct::calculateIncrement(percentageToX(0.1), percentageToX(0.9), period10to90inSeconds);
	// Discover how many samples are still from the top.
	// It cannot reach 0 dB, since x is a logarithmic parameter and would be
	// negative infinity. So we will use -0.1875 dB as the resolution
	// maximum.
	//
	// percentageToX(0.9) + samplesToTheTop*xAttackIncrement = dBToX(-0.1875); ->
	// samplesToTheTop = (dBtoX(-0.1875) - percentageToX(0.9)) / xAttackIncrement); ->
	// period10to100InSamples = period10to90InSamples + samplesToTheTop; ->
	int period10to100inSamples = (int) (period10to90inSamples + (dBtoX(-0.1875) - percentageToX(0.9)) / xAttackIncrement);
	// Discover the minimum x that, through the attackIncrement value, keeps 
	// the 10%-90% period, and reaches 0 dB at the total period:
	xMinimumInAttack = percentageToX(0.1) - (period0to100inSamples-period10to100inSamples)*xAttackIncrement;
} 


void EnvelopeGenerator::setActualDecayRate(int decayRate, int ksr, int keyScaleNumber) {
	actualDecayRate = calculateActualRate(decayRate, ksr, keyScaleNumber);
	double period10to90inSeconds = EnvelopeGeneratorData::decayAndReleaseTimeValuesTable[actualDecayRate][1]/1000.0;
	// Differently from the attack curve, the decay/release curve is linear.        
	// The dB increment is dictated by the period between 10% and 90%:
	dBdecayIncrement = OPL3DataStruct::calculateIncrement(percentageToDB(0.1), percentageToDB(0.9), period10to90inSeconds);
}

void EnvelopeGenerator::setActualReleaseRate(int releaseRate, int ksr, int keyScaleNumber) {
	actualReleaseRate =  calculateActualRate(releaseRate, ksr, keyScaleNumber);
	double period10to90inSeconds = EnvelopeGeneratorData::decayAndReleaseTimeValuesTable[actualReleaseRate][1]/1000.0;
	dBreleaseIncrement = OPL3DataStruct::calculateIncrement(percentageToDB(0.1), percentageToDB(0.9), period10to90inSeconds);
} 

int EnvelopeGenerator::calculateActualRate(int rate, int ksr, int keyScaleNumber) {
	int rof = EnvelopeGeneratorData::rateOffset[ksr][keyScaleNumber];
	int actualRate = rate*4 + rof;
	// If, as an example at the maximum, rate is 15 and the rate offset is 15, 
	// the value would
	// be 75, but the maximum allowed is 63:
	if(actualRate > 63) actualRate = 63;
	return actualRate;
}

double EnvelopeGenerator::getEnvelope(OPL3 *OPL3, int egt, int am) {
	// The datasheets attenuation values
	// must be halved to match the real OPL3 output.
	double envelopeSustainLevel = sustainLevel / 2;
	double envelopeTremolo = 
		OPL3::OPL3Data->tremoloTable[OPL3->dam][OPL3->tremoloIndex] / 2;
	double envelopeAttenuation = attenuation / 2;
	double envelopeTotalLevel = totalLevel / 2;
	
	double envelopeMinimum = -96;
	double envelopeResolution = 0.1875;

	double outputEnvelope;
	//
	// Envelope Generation
	//
	switch(stage) {
		case ATTACK:
			// Since the attack is exponential, it will never reach 0 dB, so
			// we4ll work with the next to maximum in the envelope resolution.
			if(envelope<-envelopeResolution && xAttackIncrement != -EnvelopeGeneratorData::MUGEN) {
				// The attack is exponential.
#if 0
				envelope = -pow(2.0,x);
#else
				int index = xs_FloorToInt((x - ATTACK_MIN) / ATTACK_RES);
				if (index < 0)
					envelope = OPL3::OperatorData->attackTable[0];
				else if (index >= ATTACK_TABLE_SIZE)
					envelope = OPL3::OperatorData->attackTable[ATTACK_TABLE_SIZE-1];
				else
					envelope = OPL3::OperatorData->attackTable[index];
#endif
				x += xAttackIncrement;
				break;
			}
			else {
				// It is needed here to explicitly set envelope = 0, since
				// only the attack can have a period of
				// 0 seconds and produce an infinity envelope increment.
				envelope = 0;
				stage = DECAY;
			}
		case DECAY:   
			// The decay and release are linear.                
			if(envelope>envelopeSustainLevel) {
				envelope -= dBdecayIncrement;
				break;
			}
			else 
				stage = SUSTAIN;
		case SUSTAIN:
			// The Sustain stage is mantained all the time of the Key ON,
			// even if we are in non-sustaining mode.
			// This is necessary because, if the key is still pressed, we can
			// change back and forth the state of EGT, and it will release and
			// hold again accordingly.
			if(egt==1) break;                
			else {
				if(envelope > envelopeMinimum)
					envelope -= dBreleaseIncrement;
				else stage = OFF;
			}
			break;
		case RELEASE:
			// If we have Key OFF, only here we are in the Release stage.
			// Now, we can turn EGT back and forth and it will have no effect,i.e.,
			// it will release inexorably to the Off stage.
			if(envelope > envelopeMinimum) 
				envelope -= dBreleaseIncrement;
			else stage = OFF;
		case OFF:
			break;
	}
	
	// Ongoing original envelope
	outputEnvelope = envelope;    
	
	//Tremolo
	if(am == 1) outputEnvelope += envelopeTremolo;

	//Attenuation
	outputEnvelope += envelopeAttenuation;

	//Total Level
	outputEnvelope += envelopeTotalLevel;

	return outputEnvelope;
}

void EnvelopeGenerator::keyOn() {
	// If we are taking it in the middle of a previous envelope, 
	// start to rise from the current level:
	// envelope = - (2 ^ x); ->
	// 2 ^ x = -envelope ->
	// x = log2(-envelope); ->
	double xCurrent = OperatorDataStruct::log2(-envelope);
	x = xCurrent < xMinimumInAttack ? xCurrent : xMinimumInAttack;
	stage = ATTACK;
}

void EnvelopeGenerator::keyOff() {
	if(stage != OFF) stage = RELEASE;
}

double EnvelopeGenerator::dBtoX(double dB) {
	return OperatorDataStruct::log2(-dB);
}

double EnvelopeGenerator::percentageToDB(double percentage) {
	return log10(percentage) * 10.0;
}    

double EnvelopeGenerator::percentageToX(double percentage) {
	return dBtoX(percentageToDB(percentage));
}  

PhaseGenerator::PhaseGenerator() {
	phase = phaseIncrement = 0;
}

void PhaseGenerator::setFrequency(int f_number, int block, int mult) {
	// This frequency formula is derived from the following equation:
	// f_number = baseFrequency * pow(2,19) / OPL_SAMPLE_RATE / pow(2,block-1);        
	double baseFrequency = 
		f_number * pow(2.0, block-1) * OPL_SAMPLE_RATE / pow(2.0,19);
	double operatorFrequency = baseFrequency*OperatorDataStruct::multTable[mult];
	
	// phase goes from 0 to 1 at 
	// period = (1/frequency) seconds ->
	// Samples in each period is (1/frequency)*OPL_SAMPLE_RATE =
	// = OPL_SAMPLE_RATE/frequency ->
	// So the increment in each sample, to go from 0 to 1, is:
	// increment = (1-0) / samples in the period -> 
	// increment = 1 / (OPL_SAMPLE_RATE/operatorFrequency) ->
	phaseIncrement = operatorFrequency/OPL_SAMPLE_RATE;
}

double PhaseGenerator::getPhase(OPL3 *OPL3, int vib) {
	if(vib==1) 
		// phaseIncrement = (operatorFrequency * vibrato) / OPL_SAMPLE_RATE
		phase += phaseIncrement*OPL3::OPL3Data->vibratoTable[OPL3->dvb][OPL3->vibratoIndex];
	else 
		// phaseIncrement = operatorFrequency / OPL_SAMPLE_RATE
		phase += phaseIncrement;
	// Originally clamped phase to [0,1), but that's not needed
	return phase;
}

void PhaseGenerator::keyOn() {
	phase = 0;
}

double RhythmChannel::getChannelOutput(OPL3 *OPL3) { 
	double channelOutput = 0, op1Output = 0, op2Output = 0;
	
	// Note that, different from the common channel,
	// we do not check to see if the Operator's envelopes are Off.
	// Instead, we always do the calculations, 
	// to update the publicly available phase.
	op1Output = op1->getOperatorOutput(OPL3, Operator::noModulator);
	op2Output = op2->getOperatorOutput(OPL3, Operator::noModulator);        
	channelOutput = (op1Output + op2Output) / 2;
	
	return channelOutput;
};

TopCymbalOperator::TopCymbalOperator(int baseAddress)
: Operator(baseAddress)
{ }

TopCymbalOperator::TopCymbalOperator()
: Operator(topCymbalOperatorBaseAddress)
{ }

double TopCymbalOperator::getOperatorOutput(OPL3 *OPL3, double modulator) {
	double highHatOperatorPhase = 
		OPL3->highHatOperator.phase * OperatorDataStruct::multTable[OPL3->highHatOperator.mult];
	// The Top Cymbal operator uses its own phase together with the High Hat phase.
	return getOperatorOutput(OPL3, modulator, highHatOperatorPhase);
}

// This method is used here with the HighHatOperator phase
// as the externalPhase. 
// Conversely, this method is also used through inheritance by the HighHatOperator, 
// now with the TopCymbalOperator phase as the externalPhase.
double TopCymbalOperator::getOperatorOutput(OPL3 *OPL3, double modulator, double externalPhase) {
	double envelopeInDB = envelopeGenerator.getEnvelope(OPL3, egt, am);
	envelope = EnvelopeFromDB(envelopeInDB);
	
	phase = phaseGenerator.getPhase(OPL3, vib);
	
	int waveIndex = ws & ((OPL3->_new<<2) + 3); 
	double *waveform = OPL3::OperatorData->waveforms[waveIndex];
	
	// Empirically tested multiplied phase for the Top Cymbal:
	double carrierPhase = 8 * phase;
	double modulatorPhase = externalPhase;
	double modulatorOutput = getOutput(Operator::noModulator, modulatorPhase, waveform);
	double carrierOutput = getOutput(modulatorOutput, carrierPhase, waveform);
	
	int cycles = 4;
	double chopped = (carrierPhase * cycles) /* %cycles */;
	chopped = chopped - floor(chopped / cycles) * cycles;
	if( chopped > 0.1) carrierOutput = 0;
	
	return carrierOutput*2;  
}

HighHatOperator::HighHatOperator()
: TopCymbalOperator(highHatOperatorBaseAddress)
{ }

double HighHatOperator::getOperatorOutput(OPL3 *OPL3, double modulator) {
	double topCymbalOperatorPhase = 
		OPL3->topCymbalOperator.phase * OperatorDataStruct::multTable[OPL3->topCymbalOperator.mult];
	// The sound output from the High Hat resembles the one from
	// Top Cymbal, so we use the parent method and modify its output
	// accordingly afterwards.
	double operatorOutput = TopCymbalOperator::getOperatorOutput(OPL3, modulator, topCymbalOperatorPhase);
	if(operatorOutput == 0) operatorOutput = Rand_Real1()*envelope;
	return operatorOutput;
}

SnareDrumOperator::SnareDrumOperator()
: Operator(snareDrumOperatorBaseAddress)
{ }

double SnareDrumOperator::getOperatorOutput(OPL3 *OPL3, double modulator) {
	if(envelopeGenerator.stage == EnvelopeGenerator::OFF) return 0;
	
	double envelopeInDB = envelopeGenerator.getEnvelope(OPL3, egt, am);
	envelope = EnvelopeFromDB(envelopeInDB);
	
	// If it is in OPL2 mode, use first four waveforms only:
	int waveIndex = ws & ((OPL3->_new<<2) + 3); 
	double *waveform = OPL3::OperatorData->waveforms[waveIndex];
	
	phase = OPL3->highHatOperator.phase * 2;
	
	double operatorOutput = getOutput(modulator, phase, waveform);

	double noise = Rand_Real1() * envelope;        
	
	if(operatorOutput/envelope != 1 && operatorOutput/envelope != -1) {
		if(operatorOutput > 0)  operatorOutput = noise;
		else if(operatorOutput < 0) operatorOutput = -noise;
		else operatorOutput = 0;            
	}
	
	return operatorOutput*2;
}

BassDrumChannel::BassDrumChannel(double startvol)
: Channel2op(bassDrumChannelBaseAddress, startvol, &my_op1, &my_op2),
  my_op1(op1BaseAddress), my_op2(op2BaseAddress)
{ }

double BassDrumChannel::getChannelOutput(OPL3 *OPL3) {
	// Bass Drum ignores first operator, when it is in series.
	if(cnt == 1) op1->ar=0;
	return Channel2op::getChannelOutput(OPL3);
}

void OPL3DataStruct::loadVibratoTable() {

	// According to the YMF262 datasheet, the OPL3 vibrato repetition rate is 6.1 Hz.
	// According to the YMF278B manual, it is 6.0 Hz. 
	// The information that the vibrato table has 8 levels standing 1024 samples each
	// was taken from the emulator by Jarek Burczynski and Tatsuyuki Satoh,
	// with a frequency of 6,06689453125 Hz, what  makes sense with the difference 
	// in the information on the datasheets.
	
	const double semitone = pow(2.0,1/12.0);
	// A cent is 1/100 of a semitone:
	const double cent = pow(semitone, 1/100.0);
	
	// When dvb=0, the depth is 7 cents, when it is 1, the depth is 14 cents.
	const double DVB0 = pow(cent,7.0);
	const double DVB1 = pow(cent,14.0);        
	int i;
	for(i = 0; i<1024; i++) 
		vibratoTable[0][i] = vibratoTable[1][i] = 1;        
	for(;i<2048; i++) {
		vibratoTable[0][i] = sqrt(DVB0);
		vibratoTable[1][i] = sqrt(DVB1);
	}
	for(;i<3072; i++) {
		vibratoTable[0][i] = DVB0;
		vibratoTable[1][i] = DVB1;
	}
	for(;i<4096; i++) {
		vibratoTable[0][i] = sqrt(DVB0);
		vibratoTable[1][i] = sqrt(DVB1);
	}
	for(; i<5120; i++) 
		vibratoTable[0][i] = vibratoTable[1][i] = 1;        
	for(;i<6144; i++) {
		vibratoTable[0][i] = 1/sqrt(DVB0);
		vibratoTable[1][i] = 1/sqrt(DVB1);
	}
	for(;i<7168; i++) {
		vibratoTable[0][i] = 1/DVB0;
		vibratoTable[1][i] = 1/DVB1;
	}
	for(;i<8192; i++) {
		vibratoTable[0][i] = 1/sqrt(DVB0);
		vibratoTable[1][i] = 1/sqrt(DVB1);
	}
	
}

void OPL3DataStruct::loadTremoloTable()
{
	// The tremolo depth is -1 dB when DAM = 0, and -4.8 dB when DAM = 1.
	static const double tremoloDepth[] = {-1, -4.8};
	
	//  According to the YMF278B manual's OPL3 section graph, 
	//              the tremolo waveform is not 
	//   \      /   a sine wave, but a single triangle waveform.
	//    \    /    Thus, the period to achieve the tremolo depth is T/2, and      
	//     \  /     the increment in each T/2 section uses a frequency of 2*f.
	//      \/      Tremolo varies from 0 dB to depth, to 0 dB again, at frequency*2:
	const double tremoloIncrement[] = {
		calculateIncrement(tremoloDepth[0],0,1/(2*tremoloFrequency)),
		calculateIncrement(tremoloDepth[1],0,1/(2*tremoloFrequency))
	};
	
	int tremoloTableLength = (int)(OPL_SAMPLE_RATE/tremoloFrequency);
	
	// This is undocumented. The tremolo starts at the maximum attenuation,
	// instead of at 0 dB:
	tremoloTable[0][0] = tremoloDepth[0];
	tremoloTable[1][0] = tremoloDepth[1];
	int counter = 0;
	// The first half of the triangle waveform:
	while(tremoloTable[0][counter]<0) {
		counter++;
		tremoloTable[0][counter] = tremoloTable[0][counter-1] + tremoloIncrement[0];
		tremoloTable[1][counter] = tremoloTable[1][counter-1] + tremoloIncrement[1];
	}
	// The second half of the triangle waveform:
	while(tremoloTable[0][counter]>tremoloDepth[0] && counter<tremoloTableLength-1) {
		counter++;
		tremoloTable[0][counter] = tremoloTable[0][counter-1] - tremoloIncrement[0];
		tremoloTable[1][counter] = tremoloTable[1][counter-1] - tremoloIncrement[1];
	}
}

void OperatorDataStruct::loadWaveforms() {
	int i;
	// 1st waveform: sinusoid.
	double theta = 0, thetaIncrement = 2*M_PI / 1024;
	
	for(i=0, theta=0; i<1024; i++, theta += thetaIncrement)
		waveforms[0][i] = sin(theta);
	
	double *sineTable = waveforms[0];
	// 2nd: first half of a sinusoid.
	for(i=0; i<512; i++) {
		waveforms[1][i] = sineTable[i];
		waveforms[1][512+i] = 0;
	} 
	// 3rd: double positive sinusoid.
	for(i=0; i<512; i++) 
		waveforms[2][i] = waveforms[2][512+i] = sineTable[i];         
	// 4th: first and third quarter of double positive sinusoid.
	for(i=0; i<256; i++) {
		waveforms[3][i] = waveforms[3][512+i] = sineTable[i];
		waveforms[3][256+i] = waveforms[3][768+i] = 0;
	}
	// 5th: first half with double frequency sinusoid.
	for(i=0; i<512; i++) {
		waveforms[4][i] = sineTable[i*2];
		waveforms[4][512+i] = 0;
	} 
	// 6th: first half with double frequency positive sinusoid.
	for(i=0; i<256; i++) {
		waveforms[5][i] = waveforms[5][256+i] = sineTable[i*2];
		waveforms[5][512+i] = waveforms[5][768+i] = 0;
	}
	// 7th: square wave
	for(i=0; i<512; i++) {
		waveforms[6][i] = 1;
		waveforms[6][512+i] = -1;
	}                
	// 8th: exponential
	double x;
	double xIncrement = 1 * 16.0 / 256.0;
	for(i=0, x=0; i<512; i++, x+=xIncrement) {
		waveforms[7][i] = pow(2.0,-x);
		waveforms[7][1023-i] = -pow(2.0,-(x + 1/16.0));
	}
}

void OperatorDataStruct::loaddBPowTable()
{
	for (int i = 0; i < DB_TABLE_SIZE; ++i)
	{
		dbpow[i] = pow(10.0, -(i / DB_TABLE_RES) / 10.0);
	}
}

void OperatorDataStruct::loadAttackTable()
{
	for (int i = 0; i < ATTACK_TABLE_SIZE; ++i)
	{
		attackTable[i] = -pow(2.0, ATTACK_MIN + i * ATTACK_RES);
	}
}

void OPL3::Reset()
{
}

void OPL3::WriteReg(int reg, int v)
{
	write(reg >> 8, reg & 0xFF, v);
}

void OPL3::SetPanning(int c, float left, float right)
{
	if (FullPan)
	{
		Channel *channel;

		if (c < 9)
		{
			channel = channels[0][c];
		}
		else
		{
			channel = channels[1][c - 9];
		}
		channel->leftPan = left;
		channel->rightPan = right;
	}
}

OPLEmul *JavaOPLCreate(bool stereo)
{
	return new OPL3(stereo);
}
