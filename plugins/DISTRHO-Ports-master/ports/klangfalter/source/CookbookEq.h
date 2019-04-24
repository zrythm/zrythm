// ==================================================================================
// Copyright (c) 2012 HiFi-LoFi
//
// This is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ==================================================================================


// ==================================================================================
//
// The CookbookEq is heavily based on the AnalogFilter class (by Paul Nasca and
// Lucio Asnaghi) contained in the JUCETICE project (which - again - is based on
// the great EQ cookbook by Robert Bristow-Johnson).
//
// ==================================================================================

#ifndef _COOKBOOKEQ_H
#define _COOKBOOKEQ_H

#include <cmath>
#include <vector>


class CookbookEq
{
public:
  enum Type
  {
    LoPass1,  // 1 pole
    HiPass1,  // 1 pole
    LoPass2,  // 2 poles
    HiPass2,  // 2 poles
    BandPass,
    Notch,
    Peak,
    LoShelf,
    HiShelf
  };
  
  CookbookEq(Type type, float freq, float q);
  virtual ~CookbookEq();
  
  void prepareToPlay(float sampleRate, int samplesPerBlock);
  void releaseResources();
  
  void filterOut(float *smp, int numSamples);
  
  void setFreq(float freq);
  void setFreqAndQ(float freq, float q);
  void setQ(float q);
  void setType(Type type);
  void setGain(float gainDb);
  void setStages(int stages);
  
  void cleanup();
    
private:
  static inline double Pi()                  { return 3.1415926535897932384626433832795; }
  static inline double Log10()               { return 2.302585093;                       }
  static inline bool Equal(float a, float b) { return ::fabs(a-b) < 0.00001f;            }

  struct Stage
  {
    float c1, c2;
  };
  
  void singleFilterOut (float *smp,
                        Stage &x,
                        Stage &y,
                        float *c,
                        float *d,
                        int numSamples);
  
  void computeFilterCoefs ();
  
  Type _type;                              // The type of the filter
  int _order;                              // the order of the filter (number of poles)
  float _freq;                             // Frequency given in Hz
  float _q;                                // Q factor (resonance or Q factor)
  float _gainDb;                           // the gain of the filter (if are shelf/peak) filters
  float _c[3], _d[3];                      // coefficients
  float _oldc[3], _oldd[3];                // old coefficients(used only if some filter paremeters changes very fast, and it needs interpolation)
  Stage _x, _y, _oldx, _oldy;
  std::vector<float> _interpolationBuffer; // used if it needs interpolation
  int _sampleRate;
  bool _needsInterpolation;
  bool _firstTime;
  bool _aboveNyquist;                      // this is 1 if the frequency is above the nyquist
  bool _aboveNyquistOld;                   // if the last time was above nyquist (used to see if it needs interpolation)
};


#endif

