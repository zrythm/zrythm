#ifndef __DELAYTABDSP__
#define __DELAYTABDSP__

#define PITCHRANGE 12
#define MAXDELAYSECONDS 4
#define FILTERGAIN 24

#include "JuceHeader.h"
#include "../parameters.h"
#include "pitcheddelay.h"



class DelayTabDsp : public Parameters
{
public:

	enum ParamIndicies
	{
		kPitch,
		kSync,
		kPitchType,
		kPrePitch,
		kPreDelay,
		kPreDelayVol,
		kDelay,
		kFeedback,
		
		kFilterType,
		kFilterFreq,
		kFilterQ, 
		kFilterGain,

		kMode,

		kVolume,
		kPan,

		kEnabled,

		kNumParameters
	};

	enum Mode
	{
		kMono,
		kStereo,
		kPingpong,

		kNumModes
	};

	DelayTabDsp(const String& id);
	virtual ~DelayTabDsp() {}
	
	void setParam(int index, double val);
	double getParam(int index);

	bool isEnabled() { return getParam(kEnabled) > 0.5; }

	void prepareToPlay(double sampleRate, int numSamples);

	void processBlock(const float* inL, const float* inR, int numSamples);

	Range<double> getCurrentDelayRange()
	{
		return delay.getCurrentDelayRange();
	}

	const float* getLeftData() { jassert(dataL != 0); return dataL; }
	const float* getRightData() { jassert(dataR != 0); return dataR; }


	int getLatencySamples() { return delay.getLatency(); }

	StringArray getPitchNames() { return delay.getPitchNames(); }
	int getNumPitches() { return delay.getNumPitches(); }
	int getCurrentPitch() { return delay.getCurrentPitch(); }


private:

	void clearData();
	void checkDataSize(int numSamples);
	void processMono(const float* inL, const float* inR, int numSamples);
	void processStereo(const float* inL, const float* inR, int numSamples);

	SimpleDelay preDelayL;
	SimpleDelay preDelayR;
	PitchedDelay delay;

	double volume;
	float volumeLin;
	float panning;

	double preVolume;

	bool enabled;

	Mode mode;

	double sync;

	HeapBlock<float> dataL;
	HeapBlock<float> dataR;

	HeapBlock<float> dataPreL;
	HeapBlock<float> dataPreR;

	int dataSize;
};



#endif //__DELAYTABDSP__