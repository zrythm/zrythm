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
#include "../../Engine/AudioUtils.h"

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
	AllPassFilter **allPassFiltersL;
	AllPassFilter **allPassFiltersR;

	AllPassFilter *preAllPassFilterL;
	AllPassFilter *preAllPassFilterR;

	AllPassFilter *postAllPassFilterL;
	AllPassFilter *postAllPassFilterR;

    Filter *filterLowCut;
    Filter *filterHighCut;

	float decayTime;
	float preDelayTime;
	bool stereoMode;
	float modulationIntensity;

    float highCut;
    float lowCut;

	AudioUtils audioUtils;

public:
	TalReverb(const float sampleRate)
	{
		createDelaysAndCoefficients(DELAY_LINES_COMB + DELAY_LINES_ALLPASS, 82.0f);

		combFiltersPreDelayL = new CombFilter((float)MAX_PRE_DELAY_MS, 0.0f, (long)sampleRate);
		combFiltersPreDelayR = new CombFilter((float)MAX_PRE_DELAY_MS, 0.0f, (long)sampleRate);

		combFiltersL = new CombFilter *[DELAY_LINES_COMB];
		combFiltersR = new CombFilter *[DELAY_LINES_COMB];
		noiseGeneratorAllPassL = new NoiseGenerator *[DELAY_LINES_COMB];
		noiseGeneratorAllPassR = new NoiseGenerator *[DELAY_LINES_COMB];
		noiseGeneratorDelayL = new NoiseGenerator *[DELAY_LINES_COMB];
		noiseGeneratorDelayR = new NoiseGenerator *[DELAY_LINES_COMB];
		float stereoSpreadValue = 0.008f;
		float stereoSpreadSign = 1.0f;
		for (int i = 0; i < DELAY_LINES_COMB; i++)
		{
			float stereoSpreadFactor = 1.0f + stereoSpreadValue;
			if (stereoSpreadSign > 0.0f)
			{
				combFiltersL[i] = new CombFilter(reflectionDelays[i] * stereoSpreadFactor, reflectionGains[i], (long)sampleRate);
				combFiltersR[i] = new CombFilter(reflectionDelays[i], reflectionGains[i], (long)sampleRate);
			}
			else
			{
				combFiltersL[i] = new CombFilter(reflectionDelays[i], reflectionGains[i], (long)sampleRate);
				combFiltersR[i] = new CombFilter(reflectionDelays[i] * stereoSpreadFactor, reflectionGains[i], (long)sampleRate);
			}
			stereoSpreadSign *= -1.0f;
			noiseGeneratorAllPassL[i] = new NoiseGenerator(sampleRate);
			noiseGeneratorAllPassR[i] = new NoiseGenerator(sampleRate);
			noiseGeneratorDelayL[i] = new NoiseGenerator(sampleRate);
			noiseGeneratorDelayR[i] = new NoiseGenerator(sampleRate);
		}
		preAllPassFilterL = new AllPassFilter(14.0f,  0.68f, (long)sampleRate);
		preAllPassFilterR = new AllPassFilter(15.0f,  0.68f, (long)sampleRate);

		postAllPassFilterL = new AllPassFilter(40.0f,  0.68f, (long)sampleRate);
		postAllPassFilterR = new AllPassFilter(38.0f,  0.68f, (long)sampleRate);
 
		allPassFiltersL = new AllPassFilter *[DELAY_LINES_ALLPASS];
		allPassFiltersR = new AllPassFilter *[DELAY_LINES_ALLPASS];
		for (int i = 0; i < DELAY_LINES_ALLPASS; i++)
		{
			allPassFiltersL[i] = new AllPassFilter(reflectionDelays[i + DELAY_LINES_COMB - 1] * 0.105f,  0.68f, (long)sampleRate);
			allPassFiltersR[i] = new AllPassFilter(reflectionDelays[i + DELAY_LINES_COMB - 1] * 0.1f,  0.68f, (long)sampleRate);
		}

        this->filterLowCut = new Filter(sampleRate);
        this->filterHighCut = new Filter(sampleRate);

		decayTime = 0.5f;
		preDelayTime = 0.0f;
		modulationIntensity = 0.12f;
		stereoMode = true;

        highCut = 1.0f;
        lowCut = 0.0f;
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

		delete preAllPassFilterL;
		delete preAllPassFilterR;

		delete postAllPassFilterL;
		delete postAllPassFilterR;

        delete filterLowCut;
        delete filterHighCut;
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

	void setLowCut(float value)
	{
        this->lowCut = audioUtils.getLogScaledValue(value);
	}

	void setHighCut(float value)
	{
        this->highCut = audioUtils.getLogScaledValue(value);
	}

	// All input values [0..1]
	inline void process(float* sampleL, float* sampleR)
	{
		float revR;
		float revL;

		revL = (*sampleL + *sampleR) * 0.25f; 
		revL = combFiltersPreDelayL->process(revL, 0.0f, 0.0f, preDelayTime);
		filterLowCut->process(&revL, lowCut, false);
		filterHighCut->process(&revL, highCut, true);
		revR = revL;

		// ----------------- Comb Filter --------------------
		float outL = 0.0f;
		float outR = 0.0f;

		float scaledRoomSize = decayTime * 0.998f;
		float sign = 1.0f;
		for (int i = 0; i < DELAY_LINES_COMB; i++)
		{ 
			outL += sign * combFiltersL[i]->processInterpolated(revL, 0.0f, scaledRoomSize, scaledRoomSize + 0.012f * noiseGeneratorDelayL[i]->tickFilteredNoise());
			outR += sign * combFiltersR[i]->processInterpolated(revR, 0.0f, scaledRoomSize, scaledRoomSize + 0.012f * noiseGeneratorDelayR[i]->tickFilteredNoise());
			sign *= -1.0f;
		}

		// ----------------- Pre AllPass --------------------
		//outL += 0.5f * preAllPassFilterL->processInterpolated(revL, 0.9f + 0.1f * noiseGeneratorAllPassL[0]->tickFilteredNoise(), 0.68f, true);
		//outR += 0.5f * preAllPassFilterR->processInterpolated(revR, 0.9f + 0.1f * noiseGeneratorAllPassR[0]->tickFilteredNoise(), 0.68f, true);

		//// ----------------- Post AllPass --------------------
		//outL += 0.45f * postAllPassFilterL->processInterpolated(revL, 0.85f + 0.15f * noiseGeneratorAllPassL[1]->tickFilteredNoise(), 0.68f, true);
		//outR += 0.45f * postAllPassFilterR->processInterpolated(revR, 0.85f + 0.15f * noiseGeneratorAllPassR[1]->tickFilteredNoise(), 0.68f, true);

		// ----------------- AllPass Filter ------------------
		for (int i = 0; i < DELAY_LINES_ALLPASS; i++)
		{
			outL = allPassFiltersL[i]->process(outL);
			outR = allPassFiltersR[i]->process(outR);
		}

		// ----------------- Write to output / Stereo --------
		*sampleL = outL;
		*sampleR = outR;
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
