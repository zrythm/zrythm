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

#ifndef DISTRHO_VEX_DELAY_HEADER_INCLUDED
#define DISTRHO_VEX_DELAY_HEADER_INCLUDED

#ifdef CARLA_EXPORT
 #include "juce_audio_basics.h"
#else
 #include "../StandardHeader.h"
#endif

class VexDelay
{
public:
    VexDelay(const float* const p)
        : parameters(p),
          sampleRate(0.0f),
          bufferSize(0),
          iRead(0),
          iWrite(0),
          buffer(2, 0)
    {
        buffer.clear();

        setSampleRate(44100.0f);
    }

    void updateParameterPtr(const float* const p)
    {
        parameters = p;
    }

    void setSampleRate(const float s)
    {
        if (sampleRate == s)
            return;

        sampleRate = s;
        bufferSize = sampleRate * 2;

        iRead  = 0;
        iWrite = 0;

        buffer.setSize(2, bufferSize, false, false, true);
        buffer.clear();
     }

    void processBlock(AudioSampleBuffer& outBuffer, double bpm)
    {
        processBlock(outBuffer.getWritePointer(0), outBuffer.getWritePointer(1), outBuffer.getNumSamples(), bpm);
    }

    void processBlock(float* const outBufferL, float* const outBufferR, const int numSamples, double bpm)
    {
        bpm = jlimit(10.0, 500.0, bpm);

        const int   delay    = jmin(int(parameters[73] * 8.0f) * int(((60.0 / bpm) * sampleRate) / 4.0), 44100);
        const float feedback = parameters[74];

        float* const bufferL = buffer.getWritePointer(0);
        float* const bufferR = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            iRead = iWrite - delay;

            if (iRead < 0)
                iRead += (int)sampleRate;

            bufferL[iWrite]  = outBufferL[i];
            bufferR[iWrite]  = outBufferR[i];
            bufferR[iWrite] += bufferL[iRead] * feedback;
            bufferL[iWrite] += bufferR[iRead] * feedback;

            jassert(i < numSamples);
            jassert(iRead < bufferSize);
            jassert(iWrite < bufferSize);

            outBufferL[i] = bufferL[iRead];
            outBufferR[i] = bufferR[iRead];

            if (++iWrite == sampleRate)
                iWrite = 0;
        }
     }

private:
     const float* parameters;
     float sampleRate;
     int bufferSize, iRead, iWrite;
     AudioSampleBuffer buffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexDelay)
};

#endif // DISTRHO_VEX_DELAY_HEADER_INCLUDED
