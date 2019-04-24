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

#if !defined(__PORTAMENTO_h)
#define __PORTAMENTO_h

class Portamento 
{
private:
	float portamentoSampleRate;
	float slideValue;

public:

	Portamento(float sampleRate)
	{
		portamentoSampleRate= sampleRate / 44100.0f;
		slideValue = 42;
	}

    ~Portamento()
    {
    }

	inline float tick(float destValue, float portamentoValue, bool portamentoOn) 
	{
		if (portamentoOn)
		{
			float speed= (0.11f - (portamentoValue*0.1f)) * portamentoSampleRate;
			speed= speed*speed*speed*speed*speed*speed + 0.00000001f;
			speed*= 10000.0f;
			if (slideValue > destValue)
			{
				slideValue -= speed;
				if (slideValue < destValue)
				{
					slideValue = destValue;
				}
			}
			else if (slideValue < destValue)
			{
				slideValue += speed;
				if (slideValue > destValue)
				{
					slideValue = destValue;
				}
			}
			else
			{
				slideValue = destValue;
			}
		}
		else
		{
			slideValue = destValue; 
		}
		return slideValue;
	}

	// portamentoMode (0=off, 1=auto, 2=on)
	inline void setUpNote(float newNote)
	{
		slideValue = newNote;
	}
};
#endif
