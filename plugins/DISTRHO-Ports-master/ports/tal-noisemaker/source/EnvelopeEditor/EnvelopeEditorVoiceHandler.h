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

#ifndef EnvelopeEditorVoiceHandler_H
#define EnvelopeEditorVoiceHandler_H

#include "EnvelopeEditorHandler.h"

class EnvelopeEditorVoiceHandler
{
private:
    EnvelopeEditorHandler *envelopeEditorHandler;
    
    float envelopeValue;
    float phase;

public:
    EnvelopeEditorVoiceHandler(EnvelopeEditorHandler *envelopeEditorHandler)
    {
        this->envelopeEditorHandler = envelopeEditorHandler;
        this->envelopeValue = 0.0f;
        this->phase = 0.0f;
    }

    void reset()
    {
        this->phase = 0.0f;
    }

    float getValue()
    {
        return this->envelopeValue;
    }

    float getValueCentered()
    {
        return this->envelopeValue * 2.0f - this->envelopeEditorHandler->getAmount();
    }

    void tick()
    {
        if (this->envelopeEditorHandler->getAmount() > 0.0f && !this->envelopeEditorHandler->hasNoDestination())
        {
            this->phase += envelopeEditorHandler->getPhaseInc();

            if (this->phase >= 1.0f && this->envelopeEditorHandler->isOneShot())
            {
                // Keep last value until phase reset
                this->phase = 1.0f;
                this->envelopeValue = this->envelopeEditorHandler->getEnvelopeValue(this->phase);
                this->envelopeValue *= this->envelopeEditorHandler->getAmount();
            }
            else
            {
                if (this->phase >= 1.0f)
                {
                    this->phase -= 1.0f;
                }

                this->envelopeValue = this->envelopeEditorHandler->getEnvelopeValue(this->phase);
                this->envelopeValue *= this->envelopeEditorHandler->getAmount();
            }
        }
        else
        {
            this->envelopeValue = 0.0f;
        }
    }
};
#endif