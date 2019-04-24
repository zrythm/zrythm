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


#include "CookbookEq.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>


CookbookEq::CookbookEq (CookbookEq::Type type, float freq, float q) :
  _type(type),
  _order(0),
  _freq(freq),
  _q(q),
  _gainDb(0.0f),  
  _interpolationBuffer(),
  _sampleRate (44100),
  _needsInterpolation(false),
  _firstTime(true),
  _aboveNyquist(false),
  _aboveNyquistOld(false)
{
}

CookbookEq::~CookbookEq()
{
}

void CookbookEq::cleanup()
{
  _x.c1 = 0.0f;
  _x.c2 = 0.0f;
  _y.c1 = 0.0f;
  _y.c2 = 0.0f;
  _oldx = _x;
  _oldy = _y;
  _needsInterpolation = false;
}

void CookbookEq::prepareToPlay(float sampleRate, int samplesPerBlock)
{
  _sampleRate = (int) sampleRate;
  
  _interpolationBuffer.resize(samplesPerBlock * 2);
  
  for (size_t i=0; i<3; ++i)
  {
    _oldc[i] = 0.0f;
    _oldd[i] = 0.0f;
    _c[i] = 0.0f;
    _d[i] = 0.0f;
  }
  
  cleanup();
  
  _firstTime = false;
  _aboveNyquist = false;
  _aboveNyquistOld = false;
  
  setFreqAndQ(_freq, _q);
  
  _firstTime = true;
  _d[0] = 0.0f; // this is not used
  
  computeFilterCoefs();
}

void CookbookEq::releaseResources()
{
}

void CookbookEq::computeFilterCoefs()
{ 
  float tmp = 0.0f;
  float omega, sn, cs, alpha, beta;
  bool zeroCoefs = false; // this is used if the freq is too high
  
  // do not allow frequencies bigger than samplerate/2
  float freq = _freq;
  if (freq > (static_cast<float>(_sampleRate) / 2.0f - 500.0f))
  {
    freq = static_cast<float>(_sampleRate) / 2.0f - 500.0f;
    zeroCoefs = true;
  }
  freq = std::max(freq, 0.1f);
  
  const float gain = static_cast<float>(::exp(_gainDb * Log10() / 20.0));
  
  // do not allow bogus Q
  float q = std::max(_q, 0.0f);
    
  // most of theese are implementations of
  // the "Cookbook formulae for audio EQ" by Robert Bristow-Johnson
  switch (_type)
  {
    case LoPass1:
      if (!zeroCoefs)
      {
        tmp = static_cast<float>(::exp(-2.0 * Pi() * static_cast<double>(freq) / static_cast<double>(_sampleRate)));
      }
      _c[0] = 1.0f - tmp;
      _c[1] = 0.0f;
      _c[2] = 0.0f;
      _d[1] = tmp;
      _d[2] = 0.0f;
      _order = 1;
      break;
    case HiPass1:
      if (!zeroCoefs)
      {
        tmp = static_cast<float>(::exp(-2.0 * Pi() * static_cast<double>(freq) / static_cast<double>(_sampleRate)));
      }
      _c[0] = (1.0f + tmp) / 2.0f;
      _c[1] = -(1.0f + tmp) / 2.0f;
      _c[2] = 0.0f;
      _d[1] = tmp;
      _d[2] = 0.0f;
      _order = 1;
      break;
    case LoPass2:
      if (!zeroCoefs)
      {
        omega = static_cast<float>(2.0 * Pi() * static_cast<double>(freq) / static_cast<double>(_sampleRate));
        sn = sin (omega);
        cs = cos (omega);
        alpha = sn / (2.0f * q);
        tmp = 1.0f + alpha;
        _c[0] = (1.0f - cs) / 2.0f / tmp;
        _c[1] = (1.0f - cs) / tmp;
        _c[2] = (1.0f - cs) / 2.0f / tmp;
        _d[1] = -2.0f * cs / tmp * (-1.0f);
        _d[2] = (1.0f - alpha) / tmp * (-1.0f);
      }
      else
      {
        _c[0] = 1.0f;
        _c[1] = 0.0f;
        _c[2] = 0.0f;
        _d[1] = 0.0f;
        _d[2] = 0.0f;
      }
      _order = 2;
      break;
    case HiPass2:
      if (!zeroCoefs)
      {
        omega = static_cast<float>(2.0 * Pi() * static_cast<double>(freq) / static_cast<double>(_sampleRate));
        sn = sin (omega);
        cs = cos (omega);
        alpha = sn / (2.0f * q);
        tmp = 1.0f + alpha;
        _c[0] = (1.0f + cs) / 2.0f / tmp;
        _c[1] = -(1.0f + cs) / tmp;
        _c[2] = (1.0f + cs) / 2.0f / tmp;
        _d[1] = -2.0f * cs / tmp * (-1.0f);
        _d[2] = (1.0f - alpha) / tmp * (-1.0f);
      }
      else
      {
        _c[0] = 0.0f;
        _c[1] = 0.0f;
        _c[2] = 0.0f;
        _d[1] = 0.0f;
        _d[2] = 0.0f;
      }
      _order = 2;
      break;
    case BandPass:
      if (!zeroCoefs)
      {
        omega = static_cast<float>(2.0 * Pi() * static_cast<double>(freq) / static_cast<double>(_sampleRate));
        sn = sin(omega);
        cs = cos(omega);
        alpha = sn / (2.0f * q);
        tmp = 1.0f + alpha;
        _c[0] = alpha / tmp * sqrt (q + 1.0f);
        _c[1] = 0.0f;
        _c[2] = -alpha / tmp * sqrt (q + 1.0f);
        _d[1] = -2.0f * cs / tmp * (-1.0f);
        _d[2] = (1.0f - alpha) / tmp * (-1.0f);
      }
      else
      {
        _c[0] = 0.0f;
        _c[1] = 0.0f;
        _c[2] = 0.0f;
        _d[1] = 0.0f;
        _d[2] = 0.0f;
      }
      _order = 2;
      break;
    case Notch:
      if (!zeroCoefs)
      {
        omega = static_cast<float>(2.0 * Pi() * static_cast<double>(freq) / static_cast<double>(_sampleRate));
        sn = sin(omega);
        cs = cos(omega);
        alpha = sn / (2 * sqrt (q));
        tmp = 1.0f + alpha;
        _c[0] = 1.0f / tmp;
        _c[1] = -2.0f * cs / tmp;
        _c[2] = 1.0f / tmp;
        _d[1] = -2.0f * cs / tmp * (-1.0f);
        _d[2] = (1.0f - alpha) / tmp * (-1.0f);
      }
      else
      {
        _c[0] = 1.0f;
        _c[1] = 0.0f;
        _c[2] = 0.0f;
        _d[1] = 0.0f;
        _d[2] = 0.0f;
      }
      _order = 2;
      break;
    case Peak:
      if (!zeroCoefs)
      {
        omega = static_cast<float>(2.0 * Pi() * static_cast<double>(freq) / static_cast<double>(_sampleRate));
        sn = sin(omega);
        cs = cos(omega);
        q *= 3.0;
        alpha = sn / (2 * q);
        tmp = 1 + alpha / gain;
        _c[0] = (1.0f + alpha * gain) / tmp;
        _c[1] = (-2.0f * cs) / tmp;
        _c[2] = (1.0f - alpha * gain) / tmp;
        _d[1] = -2.0f * cs / tmp * (-1.0f);
        _d[2] = (1.0f - alpha / gain) / tmp * (-1.0f);
      }
      else
      {
        _c[0] = 1.0f;
        _c[1] = 0.0f;
        _c[2] = 0.0f;
        _d[1] = 0.0f;
        _d[2] = 0.0f;
      }
      _order = 2;
      break;
    case LoShelf:
      if (!zeroCoefs)
      {
        omega = static_cast<float>(2.0 * Pi() * static_cast<double>(freq) / static_cast<double>(_sampleRate));
        sn = sin (omega);
        cs = cos (omega);
        q = sqrt (q);
        alpha = sn / (2 * q);
        beta = sqrt (gain) / q;
        tmp = (gain + 1.0f) + (gain - 1.0f) * cs + beta * sn;
        _c[0] = gain * ((gain + 1.0f) - (gain - 1.0f) * cs + beta * sn) / tmp;
        _c[1] = 2.0f * gain * ((gain - 1.0f) - (gain + 1.0f) * cs) / tmp;
        _c[2] = gain * ((gain + 1.0f) - (gain - 1.0f) * cs - beta * sn) / tmp;
        _d[1] = -2.0f * ((gain - 1.0f) + (gain + 1.0f) * cs) / tmp * (-1.0f);
        _d[2] = ((gain + 1.0f) + (gain - 1.0f) * cs - beta * sn) / tmp * (-1.0f);
      }
      else
      {
        _c[0] = gain;
        _c[1] = 0.0f;
        _c[2] = 0.0f;
        _d[1] = 0.0f;
        _d[2] = 0.0f;
      }
      _order = 2;
      break;
    case HiShelf:
      if (!zeroCoefs)
      {
        omega = static_cast<float>(2.0 * Pi() * static_cast<double>(freq) / static_cast<double>(_sampleRate));
        sn = sin(omega);
        cs = cos(omega);
        q = sqrt(q);
        alpha = sn / (2.0f * q);
        beta = sqrt (gain) / q;
        tmp = (gain + 1.0f) - (gain - 1.0f) * cs + beta * sn;
        _c[0] = gain * ((gain + 1.0f) + (gain - 1.0f) * cs + beta * sn) / tmp;
        _c[1] = -2.0f * gain * ((gain - 1.0f) + (gain + 1.0f) * cs) / tmp;
        _c[2] = gain * ((gain + 1.0f) + (gain - 1.0f) * cs - beta * sn) / tmp;
        _d[1] = 2.0f * ((gain - 1.0f) - (gain + 1.0f) * cs) / tmp * (-1.0f);
        _d[2] = ((gain + 1.0f) - (gain - 1.0f) * cs - beta * sn) / tmp * (-1.0f);
      }
      else
      {
        _c[0] = 1.0f;
        _c[1] = 0.0f;
        _c[2] = 0.0f;
        _d[1] = 0.0f;
        _d[2] = 0.0f;
      }
      _order = 2;
      break;
    default: // wrong type
      assert(false);
      break;
  }
}

void CookbookEq::setFreq (float freq)
{
  freq = std::max(freq, 0.1f);
 
  if (!Equal(_freq, freq))
  {
    float rap = _freq / freq;
    if (rap < 1.0f)
    {
      rap = 1.0f / rap;
    }
  
    _aboveNyquistOld = _aboveNyquist;
    _aboveNyquist = freq > (static_cast<float>(_sampleRate) / 2.0f - 500.0f);
  
    bool nyquistThreshold = (_aboveNyquist ^ _aboveNyquistOld);
  
    if (rap > 3.0f || nyquistThreshold)
    {
      // if the frequency is changed fast, it needs interpolation
      // (now, filter and coefficients backup)
      for (size_t i = 0; i < 3; ++i)
      {
        _oldc[i] = _c[i];
        _oldd[i] = _d[i];
      }
    
      _oldx = _x;
      _oldy = _y;
    
      if (!_firstTime)
      {
        _needsInterpolation = true;
      }
    }
  
    _freq = freq;
    computeFilterCoefs();
    _firstTime = false;
  }
}

void CookbookEq::setFreqAndQ(float freq, float q)
{
  if (!Equal(_freq, freq) || !Equal(_q, q))
  {
    _q = q;
    setFreq(freq);
  }
}

void CookbookEq::setQ(float q)
{
  if (!Equal(_q, q))
  {
    _q = q;
    computeFilterCoefs();
  }
}

void CookbookEq::setType(CookbookEq::Type type)
{
  if (_type != type)
  {
    _type = type;
    computeFilterCoefs();
  }
}

void CookbookEq::setGain(float gainDb)
{
  if (!Equal(_gainDb, gainDb))
  {
    _gainDb = gainDb;
    computeFilterCoefs();
  }
}


void CookbookEq::singleFilterOut (float *smp,
                                  Stage &x,
                                  Stage &y,
                                  float *c,
                                  float *d,
                                  int numSamples)
{
  if (_order == 1)
  {
    // First order filter
    for (int i = 0; i < numSamples; i++)
    {
      const float y0 = smp[i] * c[0] + x.c1 * c[1] + y.c1 * d[1];
      y.c1 = y0;
      x.c1 = smp[i];
      smp[i] = y0;
    }
  }
  else if (_order == 2)
  {
    // Second order filter
    for (int i = 0; i < numSamples; i++)
    {
      const float y0 = smp[i] * c[0] + x.c1 * c[1] + x.c2 * c[2] + y.c1 * d[1] + y.c2 * d[2];
      y.c2 = y.c1;
      y.c1 = y0;
      x.c2 = x.c1;
      x.c1 = smp[i];
      smp[i] = y0;
    }
  }
}

void CookbookEq::filterOut(float* smp, int numSamples)
{
  if (_needsInterpolation)
  {
    if (static_cast<int>(_interpolationBuffer.size()) < numSamples)
    {
      _interpolationBuffer.resize(numSamples); // Better re-allocation than crashing...
    }
    ::memcpy(&_interpolationBuffer[0], smp, numSamples * sizeof(float));
    singleFilterOut(&_interpolationBuffer[0], _oldx, _oldy, _oldc, _oldd, numSamples);
  }
  
  singleFilterOut (smp, _x, _y, _c, _d, numSamples);
  
  if (_needsInterpolation)
  {
    const float samplesInv = 1.0f / static_cast<float>(numSamples);
    
    for (int i = 0; i < numSamples; i++)
    {
      float x = i * samplesInv;
      smp[i] = _interpolationBuffer[i] * (1.0f - x) + smp[i] * x;
    }
    
    _needsInterpolation = false;
  }
}

