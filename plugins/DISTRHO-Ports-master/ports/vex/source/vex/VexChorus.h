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

#ifndef DISTRHO_VEX_CHORUS_HEADER_INCLUDED
#define DISTRHO_VEX_CHORUS_HEADER_INCLUDED

#ifdef CARLA_EXPORT
 #include "juce_audio_basics.h"
#else
 #include "../StandardHeader.h"
#endif

class VexChorus
{
public:
    VexChorus(const float* const p)
        : lfo1(0.0f),
          lfo2(0.0f),
          lastlfo1(0.0f),
          lastlfo2(0.0f),
          parameters(p),
          sampleRate(0.0f),
          cycle(0),
          iRead(0),
          iWrite(0),
          buffer(2, 0)
    {
        lfoS[0] = 0.5f;
        lfoS[1] = 0.0f;

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

        cycle  = int(sampleRate / 32);
        iRead  = int(cycle * 0.5f);
        iWrite = 0;

        buffer.setSize(2, cycle, false, false, true);
        buffer.clear();
    }

    void processBlock(AudioSampleBuffer& outBuffer)
    {
        processBlock(outBuffer.getWritePointer(0), outBuffer.getWritePointer(1), outBuffer.getNumSamples());
    }

    void processBlock(float* const outBufferL, float* const outBufferR, const int numSamples)
    {
        const float speed = parameters[76] * parameters[76];
        const float depth = parameters[77] * 0.2f + 0.01f;

        const int   delay = int(cycle * 0.5f);
        const float lfoC  = 2.0f * sinf(float_Pi * (speed * 5.0f) / sampleRate);

        float* const bufferL = buffer.getWritePointer(0);
        float* const bufferR = buffer.getWritePointer(1);

        float a, b, alpha, readpoint;
        int rp;

        for (int i = 0; i < numSamples ; ++i)
        {
            // LFO
            lfoS[0] = lfoS[0] - lfoC*lfoS[1];
            lfoS[1] = lfoS[1] + lfoC*lfoS[0];
            lastlfo1 = lfo1;
            lastlfo2 = lfo2;
            lfo1 = (lfoS[0] + 1) * depth;
            lfo2 = (lfoS[1] + 1) * depth;

            // Write to buffer
            bufferL[iWrite] = outBufferL[i];
            bufferR[iWrite] = outBufferR[i];
            iWrite++; //inc and cycle the write index
            iWrite = iWrite % cycle;

            iRead = iWrite + delay; //cycle the read index
            iRead = iRead % cycle;

            // Read left
            readpoint = cycle * lfo1 * 0.5f;
            rp = roundFloatToInt(readpoint - 0.5f);
            alpha = readpoint - rp;
            a = bufferL[(iRead + rp -1) % cycle];
            b = bufferL[(iRead + rp) % cycle];
            outBufferL[i] = a + alpha * (b - a);

            // Read right
            readpoint = cycle * lfo2 * 0.5f;
            rp = roundFloatToInt(readpoint - 0.5f);
            alpha = readpoint - rp;
            a = bufferR[(iRead + rp -1) % cycle];
            b = bufferR[(iRead + rp) % cycle];
            outBufferR[i] = a + alpha * (b - a);
        }
    }

private:
    float lfo1, lfo2, lastlfo1, lastlfo2;
    float lfoS[2];

    const float* parameters;
    float sampleRate;

    int cycle;
    int iRead, iWrite;

    AudioSampleBuffer buffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexChorus)
};

#endif // DISTRHO_VEX_CHORUS_HEADER_INCLUDED
