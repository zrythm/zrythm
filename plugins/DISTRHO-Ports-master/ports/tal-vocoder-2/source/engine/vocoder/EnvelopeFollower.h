/*
	==============================================================================
	This file is part of Tal-Vocoder by Patrick Kunz.

	Copyright(c) 2005-2010 Patrick Kunz, TAL
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

#ifndef __EnvelopeFollower_h_
#define __EnvelopeFollower_h_

class EnvelopeFollower
{
private:
	float sampleRate;
	float actualValue;
	float deltaValue;
	float releaseTime;

    int samplesHolded;

public:

	EnvelopeFollower(float sampleRate, float releaseTime)
	{
		this->sampleRate = sampleRate;
		this->actualValue = 0.0f;
		this->setReleaseTime(releaseTime);

        this->samplesHolded = 0;
	}

	inline float getActualValue()
	{
		return actualValue;
	}

	void reset()
	{
		actualValue = 0.0f; 
	}

	void setReleaseTime(float releaseTime)
	{
		this->releaseTime = (releaseTime * sampleRate) / 44100.0f;
	}

	inline void process(float value)
	{
		if (value > actualValue)
		{
			float tmp = releaseTime * 20.0f;
			actualValue = (1.0f/(tmp + 1.0f)) * (value + actualValue * tmp);
		} 
		else
		{
			float tmp = releaseTime * 45.0f;
			tmp = (1.0f/(tmp + 1.0f)) * (value + actualValue * tmp);
			actualValue = tmp;
		}
	}
};
#endif


