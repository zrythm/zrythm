/*
	==============================================================================
	This file is part of Tal-Reverb by Patrick Kunz.

	Copyright(c) 2005-2009 Patrick Kunz, TAL
	Togu Audio Line, Inc.
	http://kunz.corrupt.ch

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

#if !defined(__TalReverb_h)
#define __TalReverb_h

#include "AllPassFilter.h"
#include "CombFilter.h"
#include "NoiseGenerator.h"
#include "Filter.h"
#include "math.h"
#include "AudioUtils.h"
#include "TalEq.h"

class TalReverb
{
private:
	static const int DELAY_LINES_COMB = 4;
	static const int DELAY_LINES_ALLPASS = 5;

	static const int MAX_PRE_DELAY_MS = 1000;
	float* reflectionGains;
	float* reflectionDelays;

	CombFilter *combFiltersPreDelayL;
	CombFilter *combFiltersPreDelayR;

	CombFilter **combFiltersL;
	CombFilter **combFiltersR;
	NoiseGenerator **noiseGeneratorAllPassL;
	NoiseGenerator **noiseGeneratorAllPassR;
	NoiseGenerator **noiseGeneratorDelayL;
	NoiseGenerator **noiseGeneratorDelayR;
	NoiseGenerator **diffusionL;
	NoiseGenerator **diffusionR;

	AllPassFilter **allPassFiltersL;
	AllPassFilter **allPassFiltersR;

	AllPassFilter *preAllPassFilterL;
	AllPassFilter *preAllPassFilterR;

	AllPassFilter *postAllPassFilterL;
	AllPassFilter *postAllPassFilterR;

	TalEq* talEqL;
	TalEq* talEqR;

    float feedbackValueL;
    float feedbackValueR;

	float decayTime;
	float preDelayTime;
	bool stereoMode;
	float modulationIntensity;

	float outL;
	float outR;

    float feedbackSampleRateFactor;

	AudioUtils audioUtils;

public:
	TalReverb(long sampleRate)
	{
        // Ignore sample rates smaller then 44100 Hz
        this->feedbackSampleRateFactor = sampleRate / 44100.0f;
        if (this->feedbackSampleRateFactor < 1.0f) this->feedbackSampleRateFactor = 1.0f;

		this->createDelaysAndCoefficients(DELAY_LINES_ALLPASS, 100.0f);

		this->combFiltersPreDelayL = new CombFilter((float)MAX_PRE_DELAY_MS, 0.0f, sampleRate);
		this->combFiltersPreDelayR = new CombFilter((float)MAX_PRE_DELAY_MS, 0.0f, sampleRate);

		this->combFiltersL = new CombFilter *[DELAY_LINES_COMB];
		this->combFiltersR = new CombFilter *[DELAY_LINES_COMB];
		this->noiseGeneratorAllPassL = new NoiseGenerator *[DELAY_LINES_COMB];
		this->noiseGeneratorAllPassR = new NoiseGenerator *[DELAY_LINES_COMB];
		this->noiseGeneratorDelayL = new NoiseGenerator *[DELAY_LINES_COMB];
		this->noiseGeneratorDelayR = new NoiseGenerator *[DELAY_LINES_COMB];
	    this->diffusionL = new NoiseGenerator *[DELAY_LINES_COMB];
	    this->diffusionR = new NoiseGenerator *[DELAY_LINES_COMB];

		float stereoSpreadValue = 0.008f;
		float stereoSpreadSign = 1.0f;
		for (int i = 0; i < DELAY_LINES_COMB; i++)
		{
			float stereoSpreadFactor = 1.0f + stereoSpreadValue;
			if (stereoSpreadSign > 0.0f)
			{
				combFiltersL[i] = new CombFilter(reflectionDelays[i] * stereoSpreadFactor, 1.0f, sampleRate);
				combFiltersR[i] = new CombFilter(reflectionDelays[i], 1.0f, sampleRate);
			}
			else
			{
				combFiltersL[i] = new CombFilter(reflectionDelays[i], 1.0f, sampleRate);
				combFiltersR[i] = new CombFilter(reflectionDelays[i] * stereoSpreadFactor, 1.0f, sampleRate);
			}
			stereoSpreadSign *= -1.0f;

			this->noiseGeneratorAllPassL[i] = new NoiseGenerator((float)sampleRate, (float)i);
			this->noiseGeneratorAllPassR[i] = new NoiseGenerator((float)sampleRate, i + 163.0f);
			this->noiseGeneratorDelayL[i] = new NoiseGenerator((float)sampleRate, i + 257.0f);
			this->noiseGeneratorDelayR[i] = new NoiseGenerator((float)sampleRate, i + 353.0f);
			this->diffusionL[i] = new NoiseGenerator((float)sampleRate, i + 455.0f);
			this->diffusionR[i] = new NoiseGenerator((float)sampleRate, i + 633.0f);
		}

		this->preAllPassFilterL = new AllPassFilter(90.0f,  0.68f, sampleRate);
		this->preAllPassFilterR = new AllPassFilter(90.0f,  0.68f, sampleRate);

		this->postAllPassFilterL = new AllPassFilter(91.0f,  0.68f, sampleRate);
		this->postAllPassFilterR = new AllPassFilter(91.0f,  0.68f, sampleRate);
 
		this->allPassFiltersL = new AllPassFilter *[DELAY_LINES_ALLPASS];
		this->allPassFiltersR = new AllPassFilter *[DELAY_LINES_ALLPASS];
		for (int i = 0; i < DELAY_LINES_ALLPASS; i++)
		{
			this->allPassFiltersL[i] = new AllPassFilter(reflectionDelays[i] * 0.2f,  0.68f, sampleRate);
			this->allPassFiltersR[i] = new AllPassFilter(reflectionDelays[i] * 0.2f,  0.68f, sampleRate);
        }

		this->talEqL = new TalEq((float)sampleRate);
		this->talEqR = new TalEq((float)sampleRate);

		this->decayTime = 0.5f;
		this->preDelayTime = 0.0f;
		this->modulationIntensity = 0.12f;
		this->stereoMode = false;

		this->outL = 0.0f;
		this->outR = 0.0f;

        this->feedbackValueL = 0.0f;
        this->feedbackValueR = 0.0f;
	}

	~TalReverb()
	{
		delete[] reflectionGains;
		delete[] reflectionDelays;

		delete combFiltersPreDelayL;
		delete combFiltersPreDelayR;

        for (int i = 0; i < DELAY_LINES_COMB; i++) delete combFiltersL[i];
		delete[] combFiltersL;
        for (int i = 0; i < DELAY_LINES_COMB; i++) delete combFiltersR[i];
		delete[] combFiltersR;

        for (int i = 0; i < DELAY_LINES_ALLPASS; i++) delete allPassFiltersL[i];
		delete[] allPassFiltersL;
        for (int i = 0; i < DELAY_LINES_ALLPASS; i++) delete allPassFiltersR[i];
		delete[] allPassFiltersR;

        for (int i = 0; i < DELAY_LINES_COMB; i++) delete noiseGeneratorAllPassL[i];
		delete[] noiseGeneratorAllPassL;
        for (int i = 0; i < DELAY_LINES_COMB; i++) delete noiseGeneratorAllPassR[i];
		delete[] noiseGeneratorAllPassR;
        for (int i = 0; i < DELAY_LINES_COMB; i++) delete noiseGeneratorDelayL[i];
		delete[] noiseGeneratorDelayL;
        for (int i = 0; i < DELAY_LINES_COMB; i++) delete noiseGeneratorDelayR[i];
		delete[] noiseGeneratorDelayR;

        for (int i = 0; i < DELAY_LINES_COMB; i++) delete diffusionL[i];
	    delete[] diffusionL;
        for (int i = 0; i < DELAY_LINES_COMB; i++) delete diffusionR[i];
	    delete[] diffusionR;

		delete preAllPassFilterL;
		delete preAllPassFilterR;

		delete postAllPassFilterL;
		delete postAllPassFilterR;

		delete talEqL;
		delete talEqR;
	}

	void setDecayTime(float decayTime)
	{
		this->decayTime = audioUtils.getLogScaledValueInverted(decayTime) * 0.9f + 0.1f;
	}

	void setPreDelay(float preDelayTime)
	{
		this->preDelayTime = audioUtils.getLogScaledValue(preDelayTime);
	}

	void setStereoMode(bool stereoMode)
	{
		this->stereoMode = stereoMode;
	}

	void setLowShelfGain(float lowShelfGain)
	{
		talEqL->setLowShelfGain(lowShelfGain);
		talEqR->setLowShelfGain(lowShelfGain);
	}

	void setHighShelfGain(float highShelfGain)
	{
		talEqL->setHighShelfGain(highShelfGain);
		talEqR->setHighShelfGain(highShelfGain);
	}

	void setLowShelfFrequency(float lowShelfFrequency)
	{
		talEqL->setLowShelfFrequency(lowShelfFrequency);
		talEqR->setLowShelfFrequency(lowShelfFrequency);
	}

	void setHighShelfFrequency(float highShelfFrequency)
	{
		talEqL->setHighShelfFrequency(highShelfFrequency);
		talEqR->setHighShelfFrequency(highShelfFrequency);
	}

	// All input values [0..1]
	inline void process(float* sampleL, float* sampleR)
	{
		float revL = 0.0f;
		float revR = 0.0f;

		if (!stereoMode)
		{
			revL += (*sampleL + *sampleR) * 0.125f; 
			revL += combFiltersPreDelayL->process(revL, 0.0f, 0.0f, preDelayTime);
			talEqL->process(&revL);
			revR = revL;
		}
		else
		{
			revL += combFiltersPreDelayL->process(*sampleL * 0.5f, 0.0f, 0.0f, preDelayTime);
			revR += combFiltersPreDelayR->process(*sampleR * 0.5f, 0.0f, 0.0f, preDelayTime);
			talEqL->process(&revL);
			talEqR->process(&revR);
		}

		// ----------------- Comb Filter --------------------
        float feedbackFactor = 0.15f;
		outL = feedbackValueL * feedbackFactor;
		outR = feedbackValueR * feedbackFactor;

		float scaledRoomSizeDelay = this->decayTime * 0.972f;
		float scaledRoomSizeDamp = this->decayTime * 0.99f;
        float invDecayTime = 0.9f * (1.0f - this->decayTime);
		for (int i = 0; i < DELAY_LINES_COMB; i++)
		{ 
            outL += combFiltersL[i]->processInterpolated(revL, diffusionL[i]->tickFilteredNoiseFast() * invDecayTime, scaledRoomSizeDamp, scaledRoomSizeDelay + 0.028f * noiseGeneratorDelayL[i]->tickFilteredNoise());
			outR += combFiltersR[i]->processInterpolated(revR, diffusionR[i]->tickFilteredNoiseFast() * invDecayTime, scaledRoomSizeDamp, scaledRoomSizeDelay + 0.028f * noiseGeneratorDelayR[i]->tickFilteredNoise());
		}

		// ----------------- Pre AllPass --------------------
		float modL = preAllPassFilterL->processInterpolated(revL, 0.995f + 0.005f * noiseGeneratorAllPassL[0]->tickFilteredNoise(), 0.68f, true);
		float modR = preAllPassFilterR->processInterpolated(revR, 0.995f + 0.005f * noiseGeneratorAllPassR[0]->tickFilteredNoise(), 0.68f, true);

		// ----------------- Post AllPass --------------------
	    outL += 0.4f * postAllPassFilterL->processInterpolated(modL, decayTime, 0.68f, false);
		outR += 0.4f * postAllPassFilterR->processInterpolated(modR, decayTime, 0.68f, false);

		// ----------------- AllPass Filter ------------------
		for (int i = 0; i < DELAY_LINES_ALLPASS; i++)
		{
			outL = allPassFiltersL[i]->process(outL);
			outR = allPassFiltersR[i]->process(outR);
		}

		// ----------------- Write to output / Stereo --------
        //if (outL > 2.0f)  outL = 2.0f;
        //if (outR > 2.0f)  outR = 2.0f;
        //if (outL < -2.0f)  outL = -2.0f;
        //if (outR < -2.0f)  outR = -2.0f;

		*sampleL = outL;
		*sampleR = outR;

        feedbackValueL = outL;
        feedbackValueR = outR;
	}

	void createDelaysAndCoefficients(int numlines, float delayLength)
	{
		this->reflectionDelays = new float[numlines];
		this->reflectionGains = new float[numlines];

		float volumeScale = (float)(-3.0 * delayLength / log10f(0.55f));
		for (int n = numlines - 1; n >= 0; n--)
		{
			reflectionDelays[numlines -1 - n] = delayLength / powf(2.0f, (float)n / numlines); 
			reflectionGains[numlines -1 - n] = powf(10.0f, - (3.0f * reflectionDelays[numlines -1 - n]) / volumeScale);
		}
	}
};
#endif
