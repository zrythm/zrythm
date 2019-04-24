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


#if !defined(__TalEnvelope_h)
#define __TalEnvelope_h

#include "math.h"

// Low pass filter for gentle parameter changes
class Envelope
{
private:
	float currentValue;
	float paramWeight;
	float paramWeigthInverse;

public:
	Envelope(float sampleRate)
	{
		currentValue = 0;
		paramWeight = 100.0f;
		this->paramWeight = paramWeight * sampleRate / 44100.0f;
	}

	// speed [0,1] --> 1 = fast, 0 = slow
	// input [-1,1]
	inline float tick(float input, float amount, float speed)
	{
		input = fabs(input);
		if (input > 1.0f) input = 1.0f;
		speed = paramWeight + speed * 100.0f * paramWeight;
		currentValue = ((speed - 1.0f) * currentValue + input) / speed;
		return currentValue * amount * 4.0f;
	}
};
#endif
