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


#if !defined(__CombFilter_h)
#define __CombFilter_h

#include "math.h"
#include "AudioUtils.h"

class CombFilter
{
private:
	float gain, minDamp;
	float* buffer;
	int readPtr1, readPtr2, writePtr;
	float z1;
	int bufferLengthDelay;
	float filterStore;

	AudioUtils audioUtils;

public:
	// delay times in milliseconds
	CombFilter(float delayTime, float minDamp, long samplingRate)
	{
		//OutputDebugString("start init combfilter");
		bufferLengthDelay = (int)(delayTime * samplingRate / 1000);
		bufferLengthDelay = audioUtils.getNextNearPrime(bufferLengthDelay);
		buffer = new float[bufferLengthDelay];

		// Print out samples
		//File *file = new File("d:/delaytimes.txt");
		//String myXmlDoc = String((const int)bufferLengthDelay) << "\n";
		//file->appendText(myXmlDoc);

		//zero out the buffer (silence)
		for (int i = 0; i < bufferLengthDelay; i++)
			buffer[i] = 0.0f;

		writePtr = 0;

		//set read pointer to start of buffer
		readPtr1 = 0;
		z1 = filterStore = 0.0f;
		this->minDamp = minDamp;
		//OutputDebugString("end init combfilter");
	}

	~CombFilter()
	{
		delete buffer;
	}

	// delayIntensity [0..1]
	inline float processInterpolated(float input, float damp, float feedback, float delay)
	{
		float offset = (bufferLengthDelay - 2) * delay + 1.0f;
		readPtr1 = writePtr - (int)floorf(offset);

		if (readPtr1 < 0) 
			readPtr1 += bufferLengthDelay;

		readPtr2 = readPtr1 - 1;
		if (readPtr2 < 0) 
			readPtr2 += bufferLengthDelay;

		// interpolate, see paper: http://www.stanford.edu/~dattorro/EffectDesignPart2.pdf
		float frac = offset - (int)floorf(offset);
		float output = buffer[readPtr2] + buffer[readPtr1] * (1-frac) - (1-frac) * z1;
		z1 = output;

		damp = minDamp * damp;
		filterStore =  output * (1.0f - damp) + filterStore * damp;
		buffer[writePtr] = input + (filterStore * feedback);

		if (++writePtr >= bufferLengthDelay) 
			writePtr = 0;
		return output;
	}

	inline float process(float input, float damp, float feedback, float delay)
	{
		float offset = (bufferLengthDelay - 2) * delay + 1.0f;
		readPtr1 = writePtr - (int)floorf(offset);

		if (readPtr1 < 0) 
			readPtr1 += bufferLengthDelay;

		float output = buffer[readPtr1];
		filterStore =  output * (1.0f - damp) + filterStore * damp;
		buffer[writePtr] = input + (filterStore * feedback);

		if (++writePtr >= bufferLengthDelay)
			writePtr = 0;
		return output;
	}
};
#endif
