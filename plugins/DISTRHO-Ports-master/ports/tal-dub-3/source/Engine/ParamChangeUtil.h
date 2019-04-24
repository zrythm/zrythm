/*
	==============================================================================
	This file is part of Tal-Dub-III by Patrick Kunz.

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


#if !defined(__ParamChangeUtil_h)
#define __ParamChangeUtil_h

#include "math.h"

// Low pass filter for gentle parameter changes
class ParamChangeUtil
{
private:
	float currentValue;
	float paramWeight;
	float paramWeigthInverse;

public:
	// param weight should be bigger than 1
	ParamChangeUtil(float sampleRate, float paramWeight)
	{
		currentValue = 0;
		this->paramWeight = paramWeight * sampleRate / 44100.0f;
		paramWeigthInverse = 1.0f / (this->paramWeight + 1.0f);
	}

	inline float tick(float input)
	{
		currentValue = (paramWeight * currentValue + input) * paramWeigthInverse;
		return currentValue;
	}
};
#endif
