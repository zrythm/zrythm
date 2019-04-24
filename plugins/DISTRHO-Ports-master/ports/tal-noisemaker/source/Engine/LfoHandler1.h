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

#ifndef LfoHandler1_H
#define LfoHandler1_H

#include "LfoHandler.h"
#include "LfoHandler2.h"
#include "Lfo.h"

class LfoHandler1 : public LfoHandler
{
public:
	enum Destination
	{
		NOTHING = 1,
		FILTER,
		OSC1PITCH,
		OSC2PITCH,
		PW,
		FM,
        LFO2RATE,
        OSC12PITCH,
	};

private:
	Destination destination;

public:
	LfoHandler1(float sampleRate) : LfoHandler(sampleRate)
	{
		destination = FILTER;
	}

	void setDestination(const Destination destination)
	{
		this->destination = destination;
	}

	inline float getFilter()
	{
		if (destination == FILTER)
		{
			return (1.0f + value) * 0.5f * amount;
		}
		return 0.0f;
	}

	inline float getOsc1Pitch()
	{
		if (destination == OSC1PITCH || destination == OSC12PITCH)
		{
			return value * 48.0f * amount;
		}
		return 0.0f;
	}

	inline float getOsc2Pitch()
	{
		if (destination == OSC2PITCH || destination == OSC12PITCH)
		{
			return value * 48.0f * amount;
		}
		return 0.0f;
	}

	inline float getFm()
	{
		if (destination == FM)
		{
			return (0.5f * (value + 1.0f)) * amountPositive;
		}
		return 0.0f;
	}

	inline float getPw()
	{
		if (destination == PW)
		{
			return (0.5f * (value + 1.0f)) * amountPositive;
		}
		return 0.0f;
	}

	inline float getLfo2()
	{
		if (destination == LFO2RATE)
		{
            float tmp = (0.5f * (value * amount + amount));
			return  tmp * tmp * tmp;
		}
		return 0.0f;
	}
};
#endif