/*
	==============================================================================
	This file is part of Tal-NoiseMaker by Patrick Kunz.

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

#if !defined(__ADSR_h)
#define __ADSR_h

class Adsr 
{
private:
	float attackReal;
	float attack;
	float decayReal;
	float decay;
	float sustainReal;
	float sustain;
	float release;

	float sampleRateFactor;

	float tmp;

	int state;
	float actualValue;
    float returnValue;

public:
	Adsr(float sampleRate)
	{
		state = 0;
		actualValue = 0.0f;
        returnValue = 0.0f;

		sampleRateFactor = 0.004f * 44100.0f / sampleRate;

		attack = 0.0f;
		decay = 0.0f;
		sustain = 1.0f;
		release = 0.0f;

		sustainReal = 1.0f;
		decayReal = 0.0f;
		attackReal = 0.0f;
	}

private:
	float scaleValue(float value)
	{
		value = 1.0f - value * 0.5f;
		return 0.0003f * sampleRateFactor + sampleRateFactor * powf(value, 22.0f) * 2.0f;
	}

	float scaleValueAttack(float value)
	{
		value = 1.0f - value * 0.5f;
		return 0.0003f * sampleRateFactor + sampleRateFactor * powf(value, 24.0f) * 7.0f;
	}

	float scaleValueDecay(float value)
	{
		value = 1.0f - value * 0.5f;
		return 0.0003f * sampleRateFactor + sampleRateFactor * powf(value, 23.0f) * 7.0f;
	}

public:
	void setAttack(float value)
	{
		this->attackReal = value;
		this->attack = this->scaleValueAttack(value);
	}

	void setDecay(float value)
	{
		this->decayReal = value;
		this->decay = this->scaleValueDecay(value);
	}

	void setSustain(float value)
	{
		this->sustainReal = value;
		this->sustain = this->scaleValue(value);
	}

	void setRelease(float value)
	{
		this->release = this->scaleValue(value) * 8.0f;
	}

	inline float tick(bool noteOn, bool isAttackExp) 
	{
		if (!noteOn && actualValue > 0.0f)
		{
			state = 3;
		}
		if (!noteOn && actualValue <= 0.0f)
		{
			state = 4;
		}
		switch (state)
		{
		case 0:
            if (isAttackExp)
            {
			    actualValue += attack * (actualValue + sampleRateFactor);
            }
            else
            {
                 actualValue += attack * sampleRateFactor * 200.0f * (1.04f + this->attackReal * 0.5f - actualValue);
            }
			if (actualValue > 1.0f)
			{
				actualValue = 1.0f; 
				state = 1;
			}
            if (isAttackExp)
            {
                returnValue = actualValue * actualValue * actualValue * actualValue * actualValue;
            }
            else
            {
                returnValue = actualValue;
            }
            break;
		case 1:
            actualValue -= decay * (actualValue + sampleRateFactor);
            tmp = actualValue;

			if (tmp <= sustainReal)
			{
				tmp = sustainReal;
				state = 2;
			}
            returnValue = tmp;
            break;
        case 2:
            actualValue = sustainReal;
            returnValue = sustainReal;
            break;
		case 3:
			actualValue -= release * (actualValue + sampleRateFactor);
			if (actualValue < 0.0f)
			{
				actualValue = 0.0f;
				state = 4;
			}
            returnValue = actualValue;
            break;
		case 4: 
			returnValue = actualValue = 0.0f;
			break;
		}
		return returnValue;
	}

	bool isNotePlaying(bool noteOn)
	{
		return noteOn || actualValue > 0;
	}

	void resetState()
	{
		state = 0;
	}

	float getValueFasterAttack()
	{
		if (state == 0 && this->attackReal == 0.0f)
		{
			return 1.0f;
		}
		return returnValue;
	}

	void resetAll()
	{
		actualValue = 0.0f;
		state = 0;
	}
};
#endif
