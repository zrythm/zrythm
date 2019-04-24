#ifndef __PITCHBASE__
#define __PITCHBASE__

#include "JuceHeader.h"

class PitchBase 
{
public:
	PitchBase()
	{
	}

	virtual ~PitchBase() {}

	virtual void prepareToPlay(double sampleRate, int blockSize) = 0;
	virtual void processBlock(float* chL, float* chR, int numSamples) = 0;
	virtual void processBlock(float* ch, int numSamples) = 0;
	virtual void setPitch(float newPitch) = 0;

	virtual int getLatency() = 0;
	virtual void clear() = 0;
	virtual String getName() = 0;

protected:
};


class PitchProcessor
{
public:
	PitchProcessor() 
		: currentPitch(-1), pitch(1.f)
	{
	}

	virtual ~PitchProcessor() 
	{
		pitchProcs.clear();
	}

	void prepareToPlay(double sampleRate, int blockSize)
	{
		
		for (int i=0; i<pitchProcs.size(); ++i)
			pitchProcs.getUnchecked(i)->prepareToPlay(sampleRate, blockSize);
	}

	void processBlock(float* chL, float* chR, int numSamples)
	{
		PitchBase* p = pitchProcs[currentPitch];

		ScopedLock lock(processLock);

		if (p != 0)
			p->processBlock(chL, chR, numSamples);
	}

	void processBlock(float* ch, int numSamples)
	{
		PitchBase* p = pitchProcs[currentPitch];

		ScopedLock lock(processLock);

		if (p != 0)
			p->processBlock(ch, numSamples);

	}

	int getLatency()
	{
		PitchBase* p = pitchProcs[currentPitch];

		if (p != 0)
			return p->getLatency();

		return 0;
	}

	virtual void setPitch(float newPitch)
	{
		pitch = newPitch;

		for (int i=0; i<pitchProcs.size(); ++i)
			pitchProcs.getUnchecked(i)->setPitch(newPitch);
	}

	virtual void setPitchSemitones(float newPitch)
	{
		pitch = pow(2.f, newPitch / 12.f);

		for (int i=0; i<pitchProcs.size(); ++i)
			pitchProcs.getUnchecked(i)->setPitch(newPitch);
	}

	void addPitchProc(PitchBase* newPitch)
	{
		jassert(newPitch != nullptr);

		if (newPitch != nullptr)
		{
			newPitch->setPitch(pitch);
			pitchProcs.add(newPitch);
		}
	}

	void setPitchProc(int index)
	{
		jassert(index >= -1 && index < pitchProcs.size());

		PitchBase* p = pitchProcs[currentPitch];

		if (index != currentPitch)
		{
			ScopedLock lock(processLock);

			if (p != nullptr)
				p->clear();

			currentPitch = index;
		}
	}

	int getNumPitches() 
	{
		return pitchProcs.size();
	}

	int getPitchProc()
	{
		return currentPitch;
	}

	String getPitchName(int index)
	{
		PitchBase* p = pitchProcs[index];
		jassert(p != 0);

		if (p != 0)
			return p->getName();

		return "";
	}

	StringArray getPitchNames()
	{
		StringArray ret;

		for (int i=0; i<getNumPitches(); ++i)
			ret.add(getPitchName(i));

		return ret;
	}

private:
	OwnedArray<PitchBase> pitchProcs;
	int currentPitch;

	float pitch;

	CriticalSection processLock;

};



#endif // __PITCHBASE__