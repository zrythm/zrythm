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
    int randSeed;
	float actualValue;
	float deltaValue;

	float actualValueFiltered;
	float filterFactor;
	float filterFactorInversePlusOne;

	float invertedRandomMax;

	NoiseGenerator(int sampleRate) 
	{
		// No sample rate conversion here
		filterFactor = 5000.0f;
		filterFactorInversePlusOne = 1.0f / (filterFactor + 1.0f);

		actualValue = 0.0f;
		actualValueFiltered = 0.0f;
		deltaValue = 0.0f;

		getNextRandomPeriod(1.0f);

        randSeed = rand();
	}

	// returns a random value [0..1] 
	inline float tickNoise() 
	{
		// return ((float)(((randSeed = randSeed * 214013L + 2531011L) >> 16) & 0x7fff)/RAND_MAX);

        randSeed *= 16807;
        return (float)(randSeed &  0x7FFFFFFF) * 4.6566129e-010f;
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
 		int randomPeriod = (int)(tickNoise() * 22768.0f) + 22188;
		deltaValue = 1.0f / (float)randomPeriod;
		deltaValue *= sign;
	}

	inline float tickFilteredNoiseFast() 
	{
        actualValueFiltered = (actualValueFiltered * 1000.0f + tickNoise()) / 1001.0f;
		return actualValueFiltered;
	}
};
#endif
