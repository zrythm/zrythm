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

#if !defined(__TalEq_h)
#define __TalEq_h

#include "Filter6dB.h"
#include "HighPass.h"
#include "../../Engine/AudioUtils.h"

class TalEq 
{
private:
	Filter6dB *lowPass;
	HighPass *highPass;

	AudioUtils audioUtils;

public:
	TalEq(float sampleRate) 
	{
		initialize(sampleRate);
	}

	~TalEq()
	{
        delete lowPass;
        delete highPass;
	}

	void setHighPassFrequency(float highPassGain)
	{
        highPass->setCutoff(highPassGain);
	}

	void setLowPassFrequency(float lowPassGain)
	{
        lowPassGain = audioUtils.getLogScaledValue(lowPassGain);
        lowPass->setCutoff(lowPassGain);
	}

	void initialize(float sampleRate)
	{
		lowPass = new Filter6dB(sampleRate);
		highPass = new HighPass();
	}

	void process(float *sample) 
	{
		lowPass->tick(sample); // 0..0.5
		highPass->tick(sample); // 0..0.5
	}
};
#endif

