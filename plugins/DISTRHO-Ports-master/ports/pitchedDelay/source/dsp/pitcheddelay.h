#ifndef __PITCHEDDELAY__
#define __PITCHEDDELAY__

#include "simpledelay.h"
#include "basicfilters.h"
#include "../parameters.h"
//#include "pitchsoundtouch.h"
#include "PitchBase.h"

#ifndef MAXDELAYSECONDS
#define MAXDELAYSECONDS 4
#endif

class PitchedDelay
{
public:
	PitchedDelay(float samplerate = 44100.f);
	~PitchedDelay();

	void prepareToPlay(double samplerate, int blockSize);
	int getLatency();
	void setPitch(double newPitch);
	void setPitchSemitones(double semitones);
	double getPitchSemitones();
	void setDelay(double time, bool prePitch);
	double getDelay();
	bool isPrePitch();
	Range<double> getDelayRange();
	
	Range<double> getDelayRangePrePitch();
	Range<double> getCurrentDelayRange();
	int getLatencyWhenPrePitched();
	void processBlock(float* data, int numSamples);
	void processBlock(float* dataL, float* dataR, int numSamples);
	void setFeedback(float newFeedback);
	float getFeedback();
	void setGain(double gain);
	double getGain();
	void setQ(double Q);
	double getQ();
	void setFreq(double freq);
	double getFreq();
	void setType(BasicFilters::Type type);
	BasicFilters::Type getType();
	void setPingPong(bool enable);
	bool getPingPong();

	void setCurrentPitch(int index);
	StringArray getPitchNames() { return pitcher.getPitchNames(); }
	int getNumPitches() { return pitcher.getNumPitches(); }
	int getCurrentPitch() { return pitcher.getPitchProc(); }

private:

	void updateLatency(int latency);
	void clearLastData();


	PitchProcessor pitcher;
	FirstOrderFilter dcBlock;

	double pitch;
	double sampleRate;
	float feedback;
	bool pingpong;
	bool preDelayPitch;
	bool enablePitch;

	int latency;

	double currentTime;

	SimpleDelay delayL;
	SimpleDelay delayR;
	Range<double> delayRange;

	HeapBlock<float> lastDataL;
	HeapBlock<float> lastDataR;
	int sizeLastData;

	BasicFilters filterL;
	BasicFilters filterR;

	SampleDelay latencyCompensation;
	SampleDelay unpitchedDelay;

};




#endif // __PITCHEDDELAY__