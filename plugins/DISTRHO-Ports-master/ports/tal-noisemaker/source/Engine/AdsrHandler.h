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

#ifndef AdsrHandler_H
#define AdsrHandler_H

#include "AdsrHandler.h"
#include "Adsr.h"
#include "math.h"

class AdsrHandler
{
public:
	enum Destination
	{
		OFF = 1,
		FILTER,
		OSC1,
		OSC2,
		PW,
		FM,
	};

private:
	Adsr *adsr;
	Destination destination;

public:
	float value;
	float amount;
	float amountPositive;

	AdsrHandler(float sampleRate) 
	{
		this->adsr = new Adsr(sampleRate);
		this->destination = FILTER;
		this->value = 0.0f;
		this->amount = 1.0f;
		this->amountPositive = 1.0f;
	}

	~AdsrHandler()
	{
		delete adsr;
	}

	void setAttack(float value)
	{
		this->adsr->setAttack(value);
	}

	void setDecay(float value)
	{
		this->adsr->setDecay(value);
	}

	void setSustain(float value)
	{
		this->adsr->setSustain(value);
	}

	void setRelease(float value)
	{
		this->adsr->setRelease(value);
	}

	void resetState()
	{
		this->adsr->resetState();
	}

	void resetAll()
	{
		this->adsr->resetAll();
	}

	void setDestination(const Destination destination)
	{
		this->destination = destination;
	}

	void setAmount(const float amount)
	{
		this->amount = amount;
		this->amountPositive = fabs(amount);
	}

	void process(bool isNoteOn)
	{
		this->adsr->tick(isNoteOn);
		value = this->adsr->getValueFasterAttack();
	}

	inline float getFilter()
	{
		if (this->destination == FILTER)
		{
			return value * amount;
		}
		return 0.0f;
	}

	inline float getOsc1()
	{
		if (this->destination == OSC1)
		{
			return 48.0f * value * amount;
		}
		return 0.0f;
	}

	inline float getOsc2()
	{
		if (this->destination == OSC2)
		{
			return 48.0f * value * amount;
		}
		return 0.0f;
	}

	inline float getPw()
	{
		if (this->destination == PW)
		{
			return value * amountPositive;
		}
		return 0.0f;
	}

	inline float getFm()
	{
		if (this->destination == FM)
		{
			return value * amountPositive;
		}
		return 0.0f;
	}
};
#endif