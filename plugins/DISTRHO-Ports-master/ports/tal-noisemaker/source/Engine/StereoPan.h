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

#if !defined(__StereoPan_h)
#define __StereoPan_h

#include "AudioUtils.h"
#include "LfoHandler2.h"

class StereoPan 
{
public:
    AudioUtils audioUtils;
    LfoHandler2 *lfoHandler;

    StereoPan(LfoHandler2 *lfoHandler) 
    {
        this->lfoHandler = lfoHandler;
    }

    inline float getModulationAmount()
    {
        return this->lfoHandler->getAmount();
    }

    inline void process(float *sampleL, float *sampleR)
    {
        if (lfoHandler->getDestination() == LfoHandler2::PAN)
        {
            float pan = 0.5f * (this->lfoHandler->getPan() + 1.0f);
            float amount = fabs(this->getModulationAmount());
            *sampleL *= 1.0f - pan * amount;
            *sampleR *= 1.0f - (1.0f - pan) * amount;
        }
    }
};

#endif
