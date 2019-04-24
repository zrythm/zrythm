/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2008 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2008 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU Lesser General Public License, as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

   @author  rockhardbuns
   @tweaker Lucio Asnaghi
   @tweaker falkTX

 ==============================================================================
*/

#ifndef DISTRHO_VEX_ADSR_HEADER_INCLUDED
#define DISTRHO_VEX_ADSR_HEADER_INCLUDED

#ifdef CARLA_EXPORT
 #include "juce_audio_basics.h"
#else
 #include "../StandardHeader.h"
#endif

//Process every sample in a buffer
class VexADSR
{
public:
    enum State {
        kStateDone = 0,
        kStateAttack,
        kStateDecay,
        kStateSustain,
        kStateRelease,
        kStateQRelease,
        kStateCount
    };
    typedef void (VexADSR::*StateFunc)();

    VexADSR()
        : state(kStateAttack),
          attackRate(0.1f),
          decayRate(0.3f),
          sustainLevel(0.5f),
          releaseRate(0.1f),
          preCount(0),
          postCount(0),
          value(0.0f)
    {
        stateFn[kStateDone]     = &VexADSR::fn_done;
        stateFn[kStateAttack]   = &VexADSR::fn_attack;
        stateFn[kStateDecay]    = &VexADSR::fn_decay;
        stateFn[kStateSustain]  = &VexADSR::fn_sustain;
        stateFn[kStateRelease]  = &VexADSR::fn_release;
        stateFn[kStateQRelease] = &VexADSR::fn_qrelease;
    }

    // Process a buffer
    void doProcess(float* BufferL, float* BufferR, int bSize)
    {
        for(int i = 0; i < bSize; i++)
        {
            (this->*stateFn[state])();

            BufferL[i] *= value * value;
            BufferR[i] *= value * value;
        }
    }

    // Process a Sample
    float getSample()
    {
        (this->*stateFn[state])();
        return value * value;
    }

    void reset(int p)
    {
        preCount = p;
        state = kStateAttack;
        value = 0.0f;
    }

    void release(int p)
    {
        postCount = p;
        state = kStateRelease;
    }

    void quickRelease()
    {
        state = kStateQRelease;
    }

    bool getState()
    {
        return (state != kStateDone);
    }

    void kill()
    {
        state = kStateDone;
    }

    void setADSR(double a, double d, double s, double r, double sr)
    {
        double rate = sr * 5.0;

        attackRate   = float(1.0f / (rate * jmax(a * a, 0.001)));
        decayRate    = float(1.0f / (rate * jmax(d * d, 0.005)));
        releaseRate  = float(1.0f / (rate * jmax(r * r, 0.0002)));
        sustainLevel = (float)s;
    }

private:
    void fn_done()
    {
    }

    void fn_attack()
    {
        --preCount;
        value += attackRate * float(preCount < 1);

        if (value >= 1.0f)
        {
            state = kStateDecay;
            value = 1.0f;
        }
    }

    void fn_decay()
    {
        value -= decayRate;

        if (value <= sustainLevel)
            state = kStateSustain;
    }

    void fn_sustain()
    {
    }

    void fn_release()
    {
        --postCount;
        value -= releaseRate * bool(postCount < 1);
        postCount -= int(postCount > 0);

        if (value <= 0.0f)
        {
            state = kStateDone;
            value = 0.0f;
        }
    }

    void fn_qrelease()
    {
        value -= 0.006f;

        if (value <= 0.0f)
        {
            state = kStateDone;
            value = 0.0f;
        }
    }

    State state;
    StateFunc stateFn[kStateCount]; //Function pointer array for the state functions

    float attackRate, decayRate, sustainLevel, releaseRate;
    int preCount, postCount; //counter to delay release to sync with the sampleframe
    float value; //this is the current value of the envelope

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexADSR)
};

#endif // DISTRHO_VEX_ADSR_HEADER_INCLUDED
