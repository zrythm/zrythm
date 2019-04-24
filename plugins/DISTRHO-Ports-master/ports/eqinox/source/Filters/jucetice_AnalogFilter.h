/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2007 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

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

   @author  Paul Nasca
   @tweaker Lucio Asnaghi

 ==============================================================================
*/

#ifndef __JUCETICE_ANALOGFILTER_HEADER__
#define __JUCETICE_ANALOGFILTER_HEADER__

#include "../StandardHeader.h"

#define MAX_ANALOG_FILTER_STAGES  2

class AnalogFilter
{
public:

    AnalogFilter (uint8 Ftype, float Ffreq, float Fq, uint8 Fstages);
    ~AnalogFilter();

    void prepareToPlay (float sampleRate, int samplesPerBlock);
    void releaseResources();
    
    void filterOut (float *smp, int numSamples);

    void setFreq (float frequency);
    void setFreqAndQ (float frequency, float q);
    void setQ (float q);
    void setType (int type_);
    void setGain (float dBgain);
    void setStages (int stages);

    void cleanup ();

    float H (float freq); // Obtains the response for a given frequency


private:

    struct AnalogFilterStage {
      float c1, c2;
    } x[MAX_ANALOG_FILTER_STAGES + 1],
      y[MAX_ANALOG_FILTER_STAGES + 1],
      oldx[MAX_ANALOG_FILTER_STAGES + 1],
      oldy[MAX_ANALOG_FILTER_STAGES + 1];

    void singleFilterOut (float *smp,
                          AnalogFilterStage &x,
                          AnalogFilterStage &y,
                          float *c,
                          float *d,
                          int numSamples);

    void computeFilterCoefs ();

    int type;                         // The type of the filter (LPF1,HPF1,LPF2,HPF2...)
    int stages;                       // how many times the filter is applied (0->1,1->2,etc.)
    int order;                        // the order of the filter (number of poles)
    float freq;                       // Frequency given in Hz
    float q;                          // Q factor (resonance or Q factor)
    float gain;                       // the gain of the filter (if are shelf/peak) filters
    float c[3],d[3];                  // coefficients
    float oldc[3],oldd[3];            // old coefficients(used only if some filter paremeters changes very fast, and it needs interpolation)
    float xd[3],yd[3];                // used if the filter is applied more times
    float *ismp;                      // used if it needs interpolation
    //float outgain;
    int samplerate, blocksize;
    int needsinterpolation, firsttime;
    int abovenq;                      // this is 1 if the frequency is above the nyquist
    int oldabovenq;                   // if the last time was above nyquist (used to see if it needs interpolation)
};


#endif

