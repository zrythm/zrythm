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
	int counter;
	float actualValue;
	float actualValueFiltered;
	float filterFactor;
	float filterFactorInversePlusOne;

	float invertedRandomMax;
	int randomPeriod;

	NoiseGenerator(int sampleRate) 
	{
		counter = 0;
		actualValue = actualValueFiltered = 0.0f;

		// No sample rate conversion here
		filterFactor = 5000.0f;
		filterFactorInversePlusOne = 1.0f / (filterFactor + 1.0f);

		invertedRandomMax = 1.0f / RAND_MAX;
		randomPeriod = getNextRandomPeriod();
	}

	// returns a random value [0..1] 
	inline float tickNoise() 
	{
		return rand() * invertedRandomMax;
	}

	// returns a lp filtered random value [0..1]
	inline float tickFilteredNoise() 
	{
		// Change random value from time to time
		if (counter++ % randomPeriod == 0) // modulo
		{
			actualValue = tickNoise();
			randomPeriod = getNextRandomPeriod();
		}
		actualValueFiltered =  (actualValueFiltered * filterFactor + actualValue) * filterFactorInversePlusOne;
		return actualValueFiltered;
	}

	inline int getNextRandomPeriod()
	{
		return (int)(tickNoise() * 22768.0f) + 22188;
	}

	inline float tickFilteredNoiseFast() 
	{
		return tickNoise();
	}
};
#endif
