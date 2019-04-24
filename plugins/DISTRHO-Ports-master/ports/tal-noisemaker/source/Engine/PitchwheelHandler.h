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

#ifndef PitchwheelHandler_H
#define PitchwheelHandler_H

#include "AudioUtils.h"

class PitchwheelHandler
{
public:
private:
    float pitch;
    float cutoff;
    float amount;

    AudioUtils audioUtils;

public:
    PitchwheelHandler(float sampleRate) 
    {
        this->pitch = 0.0f;
        this->cutoff = 0.0f;
        this->amount = 0.0f;
    }

    ~PitchwheelHandler()
    {
    }

    void setAmount(float value)
    {
        this->amount = value;
    }

    void setPitch(float value)
    {
        this->pitch = value;
    }

    void setCutoff(float value)
    {
        this->cutoff = value;
    }

    inline float getPitch()
    {
        return this->pitch * amount * 12.0f;
    }

    inline float getCutoff()
    {
        return this->cutoff * amount;
    }
};
#endif