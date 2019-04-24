//
// This pitch shifting algorithm is based on mda's Detune: http://mda.smartelectronix.com/.
//

#ifndef __MDADETUNE_H_9944A7B6__
#define __MDADETUNE_H_9944A7B6__

#include "JuceHeader.h"
#include "PitchBase.h"

class DetunerBase
{
public:
	DetunerBase(int bufmax = 4096);

	void clear();
	void setWindowSize(int size, bool force = false);
	void setPitchSemitones(float newPitch);
	void setPitch(float newPitch);
	void setSampleRate(float newSampleRate);
	void processBlock(float* data, int numSamples);
	int getWindowSize();

private:

	int bufMax;
	int windowSize;
	float sampleRate;
	HeapBlock<float> buf;
	HeapBlock<float> win;

	int  pos0;             //buffer input
	float pos1, dpos1;      //buffer output, rate
	float pos2, dpos2;      //downwards shift

};


class Detune : public PitchBase
{
public:

	Detune(const String& name_, int windowSize);

	void prepareToPlay(double sampleRate, int blockSize);
	void processBlock(float* chL, float* chR, int numSamples);
	void processBlock(float* ch, int numSamples);
	void setPitch(float newPitch);

	int getLatency();
	void clear();
	String getName();

private:
	String name;
	DetunerBase tunerL;
	DetunerBase tunerR;
};






#endif  // __MDADETUNE_H_9944A7B6__
