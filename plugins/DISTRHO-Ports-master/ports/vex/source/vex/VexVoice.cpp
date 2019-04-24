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

#include "VexVoice.h"

static float convertPitch(const float pitch)
{
    long convert;
    float *p = (float*)&convert;
    float fl, fr, warp, out;

    fl = std::floor(pitch);
    fr = pitch - fl;
    warp = fr*0.696f + fr*fr*0.225f + fr*fr*fr*0.079f; // chebychev approx

    out = fl+warp;
    out *= 8388608.0; //2^23;
    out += 127.0 * 8388608.0; //2^23;

    convert = (long)out; //magic

    return *p;
}

static float bipolar(const float in)
{
    return in * 2.0f - 1.0f;
}

VexVoice::VexVoice(const float* const p, int po, WaveRenderer& w, float sr)
    : wr(w),
      parameters(p),
      poff(po)
{
    Ordinal = 1;
    Avelocity = 0.0f;
    Fvelocity = 0.0f;
    BaseFrequency = 0.0f;
    SampleRate = sr;
    isOn = false;
    isReleased = true;
    note = 0;

    lfoC = 2.f * (float)std::sin(float_Pi * 5.0f / SampleRate);
    LFOA = 0.0f;
    LFOF = 0.0f;

    lfoS[0] = 0.3f;
    lfoS[1] = 0.0f;

    lowL = 0.0f;
    bandL = 0.0f;
    highL = 0.0f;
    lowR = 0.0f;
    bandR = 0.0f;
    highR = 0.0f;
    q = 0.0f;
    cut = 0.0f;

    zerostruct(oL);
    zerostruct(oR);
}

void VexVoice::updateParameterPtr(const float* const p)
{
    parameters = p;
}

void VexVoice::doProcess(float* const outBufferL, float* const outBufferR, const int bufferSize)
{
    if (outBufferL == nullptr || outBufferR == nullptr || bufferSize == 0)
        return;

    //float resAmt = 0.0;
    float A, B;
    float amod;

    wr.fillBuffer(outBufferL, bufferSize, oL);
    wr.fillBuffer(outBufferR, bufferSize, oR);

    for (int i = 0; i < bufferSize; i++)
    {
        //LFO
        lfoS[0] = lfoS[0] - lfoC * lfoS[1];
        lfoS[1] = lfoS[1] + lfoC * lfoS[0];
        LFOA = lfoS[0] * parameters[20 + poff];
        LFOF = lfoS[0] * parameters[21 + poff];

        //Filter Mod
        q = 1.1f - parameters[6 + poff];
        cut =  jlimit(0.001f, 0.999f, parameters[5 + poff]
                                    + (fadsr.getSample() * bipolar(parameters[8 + poff]))
                                    + Fvelocity
                                    + LFOF);

        amod = LFOA + Avelocity;

        //Filter
        //Left
        lowL = lowL + cut * bandL;
        highL =  outBufferL[i] - lowL - (q * bandL);
        bandL = cut * highL + bandL;
        B = (lowL  * ((q * 0.5f) + 0.5f));
        A = (highL * ((q * 0.5f) + 0.5f));
        outBufferL[i]  = A + parameters[7 + poff] * ( B - A );
        outBufferL[i] += outBufferL[i] * amod;

        //Right
        lowR = lowR + cut * bandR;
        highR = outBufferR[i] - lowR - (q * bandR);
        bandR = cut * highR + bandR;
        B = (lowR  * ((q * 0.5f) + 0.5f));
        A = (highR * ((q * 0.5f) + 0.5f));
        outBufferR[i]  = A + parameters[7 + poff] * ( B - A );
        outBufferR[i] += outBufferR[i] * amod;
    }

    aadsr.doProcess(outBufferL, outBufferR, bufferSize);
    isOn = aadsr.getState();
}

void VexVoice::start(float f, float v, int n, int preroll, double s, long o)
{
    Ordinal = o;
    SampleRate = s;
    float oct = (parameters[1 + poff] - 0.5f) * 4.0f;
    float cent = (parameters[2 + poff] - 0.5f) * 0.1f;
    BaseFrequency = f * convertPitch(cent + oct);

    wr.reset(BaseFrequency, SampleRate, oL);
    wr.reset(BaseFrequency, SampleRate, oR);
    note = n;

    lfoS[0] = 0.5f;
    lfoS[1] = 0.0f;

    isOn = true;
    isReleased = false;
    v = (v * v) - 1.0f;

    Avelocity = (v * bipolar(parameters[18 + poff]));
    Fvelocity = (1.0f + v) * bipolar(parameters[13 + poff]);

    aadsr.reset(preroll);
    fadsr.reset(preroll);
}

void VexVoice::release(const int p)
{
    isReleased = true;
    aadsr.release(p);
    fadsr.release(p);
}

void VexVoice::quickRelease()
{
    isReleased = true;
    aadsr.quickRelease();
}

void VexVoice::kill()
{
    isOn = false;
}

/*
 * params:

  1 = oct
  2 = cent
  3 = phaseOffset
  4 = phaseIncOffset
  5,6,7,8    = filter
  9,10,11,12 = F ADSR
 13          = F velocity
 14,15,16,17 = A ADSR
 18          = A velocity
 19 = lfoC
 20 = lfoA
 21 = lfoF

 22,23,24 = fx volumes

 *3

 83,84,85 = panning synths
 86,87,88 = volume synths
 89,90,91 = on/off synths

 */
void VexVoice::update(const int index)
{
    float p;

    switch (index-poff)
    {
    case 14:
    case 15:
    case 16:
    case 17:
        aadsr.setADSR(parameters[14+poff], parameters[15+poff], parameters[16+poff], parameters[17+poff], SampleRate);
        break;

    case  9:
    case 10:
    case 11:
    case 12:
        fadsr.setADSR(parameters[9+poff], parameters[10+poff], parameters[11+poff], parameters[12+poff], SampleRate);
        break;

    case 19:
        lfoC = 2.f * (float)std::sin(float_Pi*(parameters[19 + poff] * 10.0f) / SampleRate);
        break;

    case 3:
        p = bipolar(parameters[3 + poff]);
        oL.phaseOffset = p > 0.0f ? p : 0.0f;
        oR.phaseOffset = p < 0.0f ? std::abs(p) : 0.0f;
        break;

    case 4:
        p = bipolar(parameters[4 + poff]);
        oL.phaseIncOffset = p > 0.0f ? p : 0.0f;
        oR.phaseIncOffset = p < 0.0f ? std::abs(p) : 0.0f;
        break;
    }
}

long VexVoice::getOrdinal() const
{
    return Ordinal;
}

int VexVoice::getNote() const
{
    return note;
}

bool VexVoice::getIsOn() const
{
    return isOn;
}

bool VexVoice::getIsReleased() const
{
    return isReleased;
}
