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

#if !defined(__ChorusEngine_h)
#define __ChorusEngine_h

#include "Chorus.h"
#include "Params.h"
#include "DCBlock.h"


class ChorusEngine 
{
public:
    Chorus *chorus1L;
    Chorus *chorus1R;
    Chorus *chorus2L;
    Chorus *chorus2R;

    DCBlock *dcBlock1L;
    DCBlock *dcBlock1R;
    DCBlock *dcBlock2L;
    DCBlock *dcBlock2R;

    bool isChorus1Enabled;
    bool isChorus2Enabled;

    ChorusEngine(float sampleRate) 
    {
        dcBlock1L= new DCBlock();
        dcBlock1R= new DCBlock();
        dcBlock2L= new DCBlock();
        dcBlock2R= new DCBlock();

        setUpChorus(sampleRate);
    }

    ~ChorusEngine()
    {
        delete chorus1L;
        delete chorus1R;
        delete chorus2L;
        delete chorus2R;
        delete dcBlock1L;
        delete dcBlock1R;
        delete dcBlock2L;
        delete dcBlock2R;

    }

    void setSampleRate(float sampleRate)
    {
        setUpChorus(sampleRate);
        setEnablesChorus(false, false);
    }

    void setEnablesChorus(bool isChorus1Enabled, bool isChorus2Enabled)
    {
        this->isChorus1Enabled = isChorus1Enabled;
        this->isChorus2Enabled = isChorus2Enabled;
    }

    void setUpChorus(float sampleRate)
    {
        chorus1L= new Chorus(sampleRate, 1.0f, 0.5f, 7.0f);
        chorus1R= new Chorus(sampleRate, 0.0f, 0.5f, 7.0f);
        chorus2L= new Chorus(sampleRate, 0.0f, 0.83f, 7.0f);
        chorus2R= new Chorus(sampleRate, 1.0f, 0.83f, 7.0f);
    }

    inline void process(float *sampleL, float *sampleR) 
    {
        float resultR= 0.0f;
        float resultL= 0.0f;
        if (isChorus1Enabled) 
        {
            resultL+= chorus1L->process(sampleL);
            resultR+= chorus1R->process(sampleR);
            dcBlock1L->tick(&resultL, 0.01f);
            dcBlock1R->tick(&resultR, 0.01f);
        }
        if (isChorus2Enabled) 
        {
            resultL+= chorus2L->process(sampleL);
            resultR+= chorus2R->process(sampleR);
            dcBlock2L->tick(&resultL, 0.01f);
            dcBlock2R->tick(&resultR, 0.01f);
        }
        *sampleL= *sampleL+resultL*1.4f;
        *sampleR= *sampleR+resultR*1.4f;
    }
};

#endif

