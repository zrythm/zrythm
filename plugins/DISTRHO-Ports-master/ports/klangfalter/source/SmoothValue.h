// ==================================================================================
// Copyright (c) 2013 HiFi-LoFi
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

#ifndef _SMOOTHVALUE_H
#define _SMOOTHVALUE_H

#include <cassert>
#include <cmath>


/**
* @class SmoothValue
* @brief Encapsulates some kind of floating point value and smoothens changes to this value (e.g. for preventing noise when changing audio plugin parameters)
*/
template<typename T>
class SmoothValue
{
public:
  explicit SmoothValue(T val, T interpolationStep = T(0.01)) :
    _valueDesired(val),
    _valueCurrent(val),
    _interpolation0(T(1.0) - interpolationStep),
    _interpolation1(interpolationStep)
  {
    assert(interpolationStep > T(0.000001));
  }
  
  SmoothValue(const SmoothValue& other) :
    _valueDesired(other._valueDesired),
    _valueCurrent(other._valueCurrent),    
    _interpolation0(other._interpolation0),
    _interpolation1(other._interpolation1)
  {
  }

  SmoothValue& operator=(const SmoothValue<T>& other)
  {
    _valueDesired = other._valueDesired;
    _valueCurrent = other._valueCurrent; 
    _interpolation0 = other._interpolation0;
    _interpolation1 = other._interpolation1;
    return (*this);
  }
  
  void initializeValue(T val)
  {
    _valueDesired = val;
    _valueCurrent = _valueDesired;
  }
  
  void updateValue(T val)
  {
    _valueDesired = val;
  }
  
  T getValue() const
  {
    return _valueDesired;
  }
  
  T getSmoothValue()
  {
    _valueCurrent = _interpolation0 * _valueCurrent + _interpolation1 * _valueDesired;
    return _valueCurrent;
  }
  
  void getSmoothValues(size_t len, T& val0, T& val1)
  {
    const T diff = ::fabs(_valueCurrent - _valueDesired);
    if (diff < _interpolation1)
    {
      val0 = _valueDesired;
      val1 = _valueDesired;
      _valueCurrent = _valueDesired;
    }
    else
    {
      val0 = _valueCurrent;
      if (static_cast<size_t>(diff / _interpolation1) < len)
      {
        val1 = _valueDesired; 
      }
      else
      {
        const T increment = (static_cast<T>(len) * _interpolation1);
        if (_valueCurrent < _valueDesired)
        {
          val1 = _valueCurrent + increment;
        }
        else
        {
          val1 = _valueCurrent - increment;
        }
      }
      _valueCurrent = val1;
    }
  }
  
private:
  T _valueDesired;
  T _valueCurrent;
  const T _interpolation0;
  const T _interpolation1;
};

#endif // Header guard
