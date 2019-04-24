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

#ifndef DISTRHO_VEX_REVERB_HEADER_INCLUDED
#define DISTRHO_VEX_REVERB_HEADER_INCLUDED

#ifdef CARLA_EXPORT
 #include "juce_audio_basics.h"
#else
 #include "../StandardHeader.h"
#endif

#include "freeverb/revmodel.hpp"

class VexReverb
{
public:
    VexReverb(const float* const p)
        : parameters(p)
    {
        model.setwet(1.0f);
        model.setdry(0.0f);
    }

    void updateParameterPtr(const float* const p)
    {
        parameters = p;
    }

    void processBlock(AudioSampleBuffer& outBuffer)
    {
        processBlock(outBuffer.getWritePointer(0), outBuffer.getWritePointer(1), outBuffer.getNumSamples());
    }

    void processBlock(float* const outBufferL, float* const outBufferR, const int numSamples)
    {
        model.setroomsize(parameters[79]);
        model.setwidth(parameters[80]);
        model.setdamp(parameters[81]);

        model.processreplace(outBufferL, outBufferR, outBufferL, outBufferR, numSamples, 1);
    }

private:
    revmodel model;
    const float* parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexReverb)
};

#endif // DISTRHO_VEX_REVERB_HEADER_INCLUDED
