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


#ifndef EnvelopeEditorHandler_H
#define EnvelopeEditorHandler_H

#include "EnvelopeEditor.h"

class EnvelopeEditorHandler
{
private:
    int destination1;

    float amount;
    float filteredEnvelopeValue;

    EnvelopeEditor *envleopEditor;

public:
	enum Destination
	{
		NOTHING = 1,
		FILTER,
		OSC1PITCH,
		OSC2PITCH,
		OSC12PITCH,
		FM,
        RINGMOD,
        VOLUME,
	};

    EnvelopeEditorHandler(EnvelopeEditor *envleopEditor)
    {
        this->envleopEditor = envleopEditor;

        this->destination1 = 1;
        this->amount = 0.0f;
        this->filteredEnvelopeValue = 0.0f;
    }

    bool hasNoDestination()
    {
        return this->destination1 == NOTHING;
    }

    float getAmount()
    {
        return this->amount;
    }

    float getEnvelopeValue(float phase)
    {
        return this->envleopEditor->getEnvelopeValue(phase);
    }

    float getPhaseInc()
    {
        return this->envleopEditor->getPhaseDelta();
    }

    void setDestination1(int value)
    {
        this->destination1 = value;
    }

    void setAmount(float value)
    {
        this->amount = value * value;
    }

    void setOneShot(bool value)
    {
        this->envleopEditor->setOneShot(value);
    }

    void setFixTempo(bool value)
    {
        this->envleopEditor->setFixTempo(value);
    }

    bool isOneShot()
    {
        return this->envleopEditor->isOneShot();
    }

    float getFilterValue(const float envelopeValue)
    {
        if (this->destination1 == FILTER)
        {
            return envelopeValue;
        }

        return 0.0f;
    }

    float getOsc1Value(const float envelopeValue)
    {
        if (this->destination1 == OSC1PITCH || this->destination1 == OSC12PITCH)
        {
            return envelopeValue * 24.0f;
        }

        return 0.0f;
    }

    float getOsc2Value(const float envelopeValue)
    {
        if (this->destination1 == OSC2PITCH || this->destination1 == OSC12PITCH)
        {
            return envelopeValue * 24.0f;
        }

        return 0.0f;
    }

    float getFmValue(const float envelopeValue)
    {
        if (this->destination1 == FM)
        {
            return envelopeValue;
        }

        return 0.0f;
    }

    float getRingmodValue(const float envelopeValue)
    {
        if (this->destination1 == RINGMOD)
        {
            return envelopeValue;
        }

        return 0.0f;
    }

    float getVolumeValue(const float envelopeValue)
    {
        if (this->destination1 == VOLUME)
        {
            this->filteredEnvelopeValue = (envelopeValue + 2.0f * this->filteredEnvelopeValue) * 0.25f;
            return this->filteredEnvelopeValue;
        }

        return 1.0f;
    }
};
#endif