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

class TalReverb
{
private:
	static const int DELAY_LINES_COMB = 5;
	static const int DELAY_LINES_ALLPASS = 6;

	static const int MAX_PRE_DELAY_MS = 1000;
	float* reflectionGains;
	float* reflectionDelays;

	CombFilter *combFiltersPreDelay;

	CombFilter **combFiltersL;
	CombFilter **combFiltersR;
	NoiseGenerator **noiseGeneratorDampL;
	NoiseGenerator **noiseGeneratorDampR;
	NoiseGenerator **noiseGeneratorDelayL;
	NoiseGenerator **noiseGeneratorDelayR;
	AllPassFilter **allPassFiltersL;
	AllPassFilter **allPassFiltersR;

	AllPassFilter *preAllPassFilter;

	Filter *lpFilter;
	Filter *hpFilter;


public:
	TalReverb(int sampleRate)
	{
		createDelaysAndCoefficients(DELAY_LINES_COMB + DELAY_LINES_ALLPASS, 80.0f);

		combFiltersPreDelay = new CombFilter((float)MAX_PRE_DELAY_MS, 0.0f, sampleRate);

		combFiltersL = new CombFilter *[DELAY_LINES_COMB];
		combFiltersR = new CombFilter *[DELAY_LINES_COMB];
		noiseGeneratorDampL = new NoiseGenerator *[DELAY_LINES_COMB];
		noiseGeneratorDampR = new NoiseGenerator *[DELAY_LINES_COMB];
		noiseGeneratorDelayL = new NoiseGenerator *[DELAY_LINES_COMB];
		noiseGeneratorDelayR = new NoiseGenerator *[DELAY_LINES_COMB];
		float stereoSpreadValue = 0.005f;
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
			noiseGeneratorDampL[i] = new NoiseGenerator(sampleRate);
			noiseGeneratorDampR[i] = new NoiseGenerator(sampleRate);
			noiseGeneratorDelayL[i] = new NoiseGenerator(sampleRate);
			noiseGeneratorDelayR[i] = new NoiseGenerator(sampleRate);
		}
		preAllPassFilter = new AllPassFilter(15.0f,  0.68f, sampleRate);

		allPassFiltersL = new AllPassFilter *[DELAY_LINES_ALLPASS];
		allPassFiltersR = new AllPassFilter *[DELAY_LINES_ALLPASS];
		for (int i = 0; i < DELAY_LINES_ALLPASS; i++)
		{
			allPassFiltersL[i] = new AllPassFilter(reflectionDelays[i + DELAY_LINES_COMB - 1] * 0.21f,  0.68f, sampleRate);
			allPassFiltersR[i] = new AllPassFilter(reflectionDelays[i + DELAY_LINES_COMB - 1] * 0.22f,  0.68f, sampleRate);
		}
		lpFilter = new Filter(sampleRate);
		hpFilter = new Filter(sampleRate);
	}

	~TalReverb()
	{
		delete reflectionGains;
		delete reflectionDelays;

		delete combFiltersL;
		delete combFiltersR;
		delete noiseGeneratorDampL;
		delete noiseGeneratorDampR;
		delete noiseGeneratorDelayL;
		delete noiseGeneratorDelayR;
		delete allPassFiltersL;
		delete allPassFiltersR;

		delete lpFilter;
		delete hpFilter;
		delete preAllPassFilter;
	}

	// All input values [0..1]
	inline void process(float* sampleL, float* sampleR, float gain, float roomSize, const float preDelay, const float lowCut, const float damp, const float highCut, const float stereoWidth)
	{
		// Very diffuse verb
		// --------------------------------------------------
		roomSize = (1.0f - roomSize);
		roomSize *= roomSize * roomSize;
		roomSize = (1.0f - roomSize);

		float revL = (*sampleL + *sampleR);

		// Add small noise -> avoid cpu spikes
		float noise = noiseGeneratorDampL[0]->tickNoise();
		revL += noise * 0.000000001f;

		// ----------------- Pre Delay ----------------------
		revL = combFiltersPreDelay->process(revL, 0.0f, 0.0f, preDelay * preDelay);

		// ----------------- Pre AllPass --------------------
		revL += preAllPassFilter->processInterpolated(revL, 0.01f + roomSize * 0.99f, 0.97f + noise * 0.03f) * 0.2f;

		// ----------------- Filters ------------------------
		revL *= 0.2f;
		revL = lpFilter->process(revL, highCut, 0.0f, false);
		revL = hpFilter->process(revL, 0.95f * lowCut + 0.05f, 0.0f, true);
		float revR = revL;

		// ----------------- Comb Filter --------------------
		float outL = 0.0f;
		float outR = 0.0f;
		float scaledDamp = 0.95f * damp * damp;
		float scaledRoomSize = roomSize * 0.97f;
		for (int i = 0; i < DELAY_LINES_COMB; i++)
		{ 
			outL += combFiltersL[i]->processInterpolated(revL, scaledDamp + 0.05f * noiseGeneratorDampL[i]->tickFilteredNoiseFast(), scaledRoomSize, 0.6f * scaledRoomSize + 0.395f + 0.012f * noiseGeneratorDelayL[i]->tickFilteredNoise() * roomSize);
			outR += combFiltersR[i]->processInterpolated(revR, scaledDamp + 0.05f * noiseGeneratorDampR[i]->tickFilteredNoiseFast(), scaledRoomSize, 0.6f * scaledRoomSize + 0.398f + 0.012f * noiseGeneratorDelayR[i]->tickFilteredNoise() * roomSize);
		}
		// ----------------- AllPass Filter ------------------
		for (int i = 0; i < DELAY_LINES_ALLPASS; i++)
		{
			outL = allPassFiltersL[i]->process(outL);
			outR = allPassFiltersR[i]->process(outR);
		}
		// ----------------- Write to output / Stereo --------
		gain *= gain;
		float wet1 = gain * (stereoWidth * 0.5f + 0.5f);
		float wet2 = gain * ((1.0f - stereoWidth) * 0.5f);
		*sampleL = outL * wet1 + outR * wet2;
		*sampleR = outR * wet1 + outL * wet2;
	}

	void createDelaysAndCoefficients(int numlines, float delayLength)
	{
		reflectionDelays = new float[numlines];
		reflectionGains = new float[numlines];

		float volumeScale = (float)(-3.0 * delayLength / log10f(1.0f));
		for (int n = numlines - 1; n >= 0; n--)
		{
			reflectionDelays[numlines -1 - n] = delayLength / powf(2.0f, (float)n / numlines); 
			reflectionGains[numlines -1 - n] = powf(10.0f, - (3.0f * reflectionDelays[numlines -1 - n]) / volumeScale);
		}
	}
};
#endif
