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


#if !defined(__Reverb_h)
#define __Reverb_h

#include "AllPassFilter.h"
#include "CombFilter.h"
#include "NoiseGenerator.h"
#include "Filter.h"
#include "math.h"
#include "AudioUtils.h"
#include "TalEq.h"

class Reverb
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

	AudioUtils audioUtils;

public:
	Reverb(int sampleRate)
	{
		createDelaysAndCoefficients(DELAY_LINES_COMB + DELAY_LINES_ALLPASS, 82.0f);

		combFiltersPreDelayL = new CombFilter((float)MAX_PRE_DELAY_MS, 0.0f, sampleRate);
		combFiltersPreDelayR = new CombFilter((float)MAX_PRE_DELAY_MS, 0.0f, sampleRate);

		combFiltersL = new CombFilter *[DELAY_LINES_COMB];
		combFiltersR = new CombFilter *[DELAY_LINES_COMB];
		noiseGeneratorAllPassL = new NoiseGenerator *[DELAY_LINES_COMB];
		noiseGeneratorAllPassR = new NoiseGenerator *[DELAY_LINES_COMB];
		noiseGeneratorDelayL = new NoiseGenerator *[DELAY_LINES_COMB];
		noiseGeneratorDelayR = new NoiseGenerator *[DELAY_LINES_COMB];
	    diffusionL = new NoiseGenerator *[DELAY_LINES_COMB];
	    diffusionR = new NoiseGenerator *[DELAY_LINES_COMB];

		float stereoSpreadValue = 0.008f;
		float stereoSpreadSign = 1.0f;
		for (int i = 0; i < DELAY_LINES_COMB; i++)
		{
			float stereoSpreadFactor = 1.0f + stereoSpreadValue;
			if (stereoSpreadSign > 0.0f)
			{
				combFiltersL[i] = new CombFilter(reflectionDelays[i] * stereoSpreadFactor, reflectionGains[i], sampleRate);
				combFiltersR[i] = new CombFilter(reflectionDelays[i], reflectionGains[i], sampleRate);
			}
			else
			{
				combFiltersL[i] = new CombFilter(reflectionDelays[i], reflectionGains[i], sampleRate);
				combFiltersR[i] = new CombFilter(reflectionDelays[i] * stereoSpreadFactor, reflectionGains[i], sampleRate);
			}
			stereoSpreadSign *= -1.0f;
			noiseGeneratorAllPassL[i] = new NoiseGenerator(sampleRate);
			noiseGeneratorAllPassR[i] = new NoiseGenerator(sampleRate);
			noiseGeneratorDelayL[i] = new NoiseGenerator(sampleRate);
			noiseGeneratorDelayR[i] = new NoiseGenerator(sampleRate);
			diffusionL[i] = new NoiseGenerator(sampleRate);
			diffusionR[i] = new NoiseGenerator(sampleRate);
		}

		preAllPassFilterL = new AllPassFilter(220.0f,  0.68f, sampleRate);
		preAllPassFilterR = new AllPassFilter(220.0f,  0.68f, sampleRate);

		postAllPassFilterL = new AllPassFilter(180.0f,  0.68f, sampleRate);
		postAllPassFilterR = new AllPassFilter(180.0f,  0.68f, sampleRate);
 
		allPassFiltersL = new AllPassFilter *[DELAY_LINES_ALLPASS];
		allPassFiltersR = new AllPassFilter *[DELAY_LINES_ALLPASS];
		for (int i = 0; i < DELAY_LINES_ALLPASS; i++)
		{
			allPassFiltersL[i] = new AllPassFilter(reflectionDelays[i + DELAY_LINES_COMB - 1] * 0.2f,  0.68f, sampleRate);
			allPassFiltersR[i] = new AllPassFilter(reflectionDelays[i + DELAY_LINES_COMB - 1] * 0.2f,  0.68f, sampleRate);
        }

		talEqL = new TalEq(sampleRate);
		talEqR = new TalEq(sampleRate);

		decayTime = 0.5f;
		preDelayTime = 0.0f;
		modulationIntensity = 0.12f;
		stereoMode = false;

		outL = 0.0f;
		outR = 0.0f;

        feedbackValueL = 0.0f;
        feedbackValueR = 0.0f;
	}

	~Reverb()
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
		this->decayTime = audioUtils.getLogScaledValueInverted(decayTime) * 0.99f;
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
		outL = feedbackValueL * 0.05f;
		outR = feedbackValueR * 0.05f;

		float scaledRoomSize = decayTime * 0.97f;
		for (int i = 0; i < DELAY_LINES_COMB; i++)
		{ 
            outL += combFiltersL[i]->processInterpolated(revL, diffusionL[i]->tickFilteredNoiseFast() * 0.01f, scaledRoomSize, scaledRoomSize + 0.035f * noiseGeneratorDelayL[i]->tickFilteredNoise());
			outR += combFiltersR[i]->processInterpolated(revR, diffusionR[i]->tickFilteredNoiseFast() * 0.01f, scaledRoomSize, scaledRoomSize + 0.035f * noiseGeneratorDelayR[i]->tickFilteredNoise());
		}

		// ----------------- Pre AllPass --------------------
		float modL = preAllPassFilterL->processInterpolated(outL, decayTime * decayTime * 0.999f + 0.001f * noiseGeneratorAllPassL[0]->tickFilteredNoise(), 0.68f, true);
		float modR = preAllPassFilterR->processInterpolated(outR, decayTime * decayTime * 0.999f + 0.001f * noiseGeneratorAllPassR[0]->tickFilteredNoise(), 0.68f, true);

		// ----------------- Post AllPass --------------------
	    outL += 0.3f * postAllPassFilterL->processInterpolated(modL, decayTime * decayTime * 1.0f, 0.68f, false);
		outR += 0.3f * postAllPassFilterR->processInterpolated(modR, decayTime * decayTime * 1.0f, 0.68f, false);

		// ----------------- AllPass Filter ------------------
		for (int i = 0; i < DELAY_LINES_ALLPASS; i++)
		{
			outL = allPassFiltersL[i]->process(outL);
			outR = allPassFiltersR[i]->process(outR);
		}

		// ----------------- Write to output / Stereo --------
		*sampleL = outL;
		*sampleR = outR;

        feedbackValueL = outL;
        feedbackValueR = outR;
	}

	void createDelaysAndCoefficients(int numlines, float delayLength)
	{
		reflectionDelays = new float[numlines];
		reflectionGains = new float[numlines];

		float volumeScale = (float)(-3.0 * delayLength / log10f(0.5f));
		for (int n = numlines - 1; n >= 0; n--)
		{
			reflectionDelays[numlines -1 - n] = delayLength / powf(2.0f, (float)n / numlines); 
			reflectionGains[numlines -1 - n] = powf(10.0f, - (3.0f * reflectionDelays[numlines -1 - n]) / volumeScale);
		}
	}
};
#endif
