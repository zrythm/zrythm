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

#include "LevelMeasurement.h"

#include <algorithm>


LevelMeasurement::LevelMeasurement(float decay) :
  _decay(decay),
  _level(0.0f)  
{
}


LevelMeasurement::LevelMeasurement(const LevelMeasurement& other) :
  _decay(other._decay),
  _level(other._level)
{
}


LevelMeasurement::~LevelMeasurement()
{
}


LevelMeasurement& LevelMeasurement::operator=(const LevelMeasurement& other)
{
  if (this != &other)
  {
    _decay = other._decay;
    _level = other._level;
  }
  return (*this);
}


void LevelMeasurement::process(size_t len, const float* data)
{
  if (len > 0)
  {
    float level = _level.get();
    if (data)
    {
      for (size_t i=0; i<len; ++i)
      {
        const float val = data[i];
        if (level < val)
        {
          level = val;
        }
        else if (level > 0.0001f)
        {
          level *= _decay;
        }
        else
        {
          level = 0.0f;
        }
      }
    }
    else
    {
      if (level > 0.0001f)
      {
        for (size_t i=0; i<len; ++i)
        {
          if (level > 0.0001f)
          {
            level *= _decay;
          }
          else
          {
            level = 0.0f;
            break;
          }
        }
      }
    }    
    _level.set(level);
  }
}


float LevelMeasurement::getLevel() const
{
  return _level.get();
}


void LevelMeasurement::reset()
{
  _level.set(0.0f);
}
