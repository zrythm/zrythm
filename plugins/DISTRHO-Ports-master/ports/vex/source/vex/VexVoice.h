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

#ifndef DISTRHO_VEX_VOICE_HEADER_INCLUDED
#define DISTRHO_VEX_VOICE_HEADER_INCLUDED

#ifdef CARLA_EXPORT
 #include "juce_audio_basics.h"
#else
 #include "../StandardHeader.h"
#endif

#include "VexADSR.h"
#include "VexWaveRenderer.h"

class VexVoice
{
public:
    VexVoice(const float* const p, const int poff, WaveRenderer& w, float sr);

    void updateParameterPtr(const float* const p);

    void doProcess(float* const outBufferL, float* const outBufferR, const int bufferSize);
    void start(float f, float v, int n, int preroll, double s, long o);
    void release(int p);
    void quickRelease();

    void kill();
    void update(const int index);

    long getOrdinal() const;
    int  getNote() const;
    bool getIsOn() const;
    bool getIsReleased() const;

private:
    OscSet oL;
    OscSet oR;
    WaveRenderer& wr;

    VexADSR aadsr;
    VexADSR fadsr;

    const float* parameters;
    const int poff;

    bool isOn, isReleased;
    int note;

    long Ordinal;

    float Avelocity;
    float Fvelocity;
    double SampleRate;
    float BaseFrequency;
    float lfoC, LFOA, LFOF;
    float lfoS[2];
    float lowL, bandL, highL, lowR, bandR, highR, q, cut;
};

#endif // DISTRHO_VEX_VOICE_HEADER_INCLUDED
