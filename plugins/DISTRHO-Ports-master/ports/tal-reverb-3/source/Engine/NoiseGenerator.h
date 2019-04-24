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

#if !defined(__NoiseGenerator_h)
#define __NoiseGenerator_h

#include <cstdlib>

class NoiseGenerator 
{
public:
    float sampleRate;

    int randomSeed;
	float actualValue;
	float deltaValue;

	float actualValueFiltered;
	float filterFactor;
	float filterFactorInversePlusOne;

    float periodRange;
    float periodOffset;

    float noiseFastModFilterValue;

	NoiseGenerator(float sampleRate, float randomSeed) 
	{
        this->sampleRate = sampleRate;
        this->randomSeed = (int)randomSeed;

		this->filterFactor = 44100.0f * 2800.0f / sampleRate;
		this->filterFactorInversePlusOne = 1.0f / (filterFactor + 1.0f);

        this->periodRange = 44100.0f * 25000.0f / sampleRate;
        this->periodOffset = 44100.0f * 1000.0f / sampleRate;

        this->noiseFastModFilterValue = 44100.0f * 10.0f / sampleRate;
        //this->noiseFastModFilterValue = 10.0f;

		this->deltaValue = 0.0f;
		this->actualValue = 0.0f;
		this->actualValueFiltered = 0.0f;

		this->getNextRandomPeriod(1.0f);

	}

	// returns a random value [0..1] 
	inline float tickNoise() 
	{
		// return ((float)(((randSeed = randSeed * 214013L + 2531011L) >> 16) & 0x7fff)/RAND_MAX);

        this->randomSeed *= 16807;
        return (float)(randomSeed &  0x7FFFFFFF) * 4.6566129e-010f;
        //return (float)randSeed * 4.6566129e-010f;
	}

	// returns a lp filtered random value [0..1]
	inline float tickFilteredNoise() 
	{
		if (actualValue >= 1.0f)
		{
			getNextRandomPeriod(-1.0f);
		}
		if (actualValue <= 0.0f)
		{
			getNextRandomPeriod(1.0f);
		}
		actualValue += deltaValue;

		// Exponential averager
		actualValueFiltered =  (actualValueFiltered * filterFactor + actualValue) * filterFactorInversePlusOne;
		return actualValueFiltered;
	}

	inline void getNextRandomPeriod(float sign)
	{
 		int randomPeriod = (int)(tickNoise() * this->periodRange + this->periodOffset);
		deltaValue = 1.0f / (float)randomPeriod;
		deltaValue *= sign;
	}

	inline float tickFilteredNoiseFast() 
	{
        actualValueFiltered = (actualValueFiltered * this->noiseFastModFilterValue + tickNoise()) / (this->noiseFastModFilterValue + 1.0f);
		return actualValueFiltered;
	}
};
#endif
