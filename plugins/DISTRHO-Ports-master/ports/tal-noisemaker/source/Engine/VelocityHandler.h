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

#ifndef VelocityHandler_H
#define VelocityHandler_H

#include "AudioUtils.h"

class VelocityHandler
{
private:
    float volume;
    float contour;
    float cutoff;

    AudioUtils audioUtils;

public:
    VelocityHandler(float sampleRate) 
    {
        this->volume = 0.0f;
        this->contour = 0.0f;
        this->cutoff = 0.0f;
    }

    ~VelocityHandler()
    {
    }

    void setVolume(float value)
    {
        this->volume = value;
    }

    void setContour(float value)
    {
        this->contour = audioUtils.getLogScaledValueFilter(value);
    }

    void setCutoff(float value)
    {
        this->cutoff = audioUtils.getLogScaledValueFilter(value);
    }

    inline float getVolume(float velocity)
    {
        return 1.0f - (this->volume * (1.0f - velocity));
    }

    inline float getContour()
    {
        return this->contour;
    }

    inline float getCutoff()
    {
        return this->cutoff;
    }
};
#endif