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

#ifndef _DECIBELSCALING_H
#define _DECIBELSCALING_H


#include "JuceHeader.h"


struct DecibelScaling
{
  static float MinusInfinity()
  {
    return -100.0f;
  }
  
  static float Db2Gain(float gain)
  {
    return Decibels::decibelsToGain(gain, MinusInfinity());
  }
  
  static float Gain2Db(float db)
  {
    return Decibels::gainToDecibels(db, MinusInfinity());
  }
  
  static float MinScaleDb()
  {
    return MinusInfinity();
  }
  
  static float MaxScaleDb()
  {
    return +20.0f;
  }
  
  static float Db2Scale(float db)
  {
    db = std::max(MinScaleDb(), std::min(MaxScaleDb(), db));
    const float n = (db - MinScaleDb()) / (MaxScaleDb() - MinScaleDb());
    return ::pow(n, 2.0f);
  }
  
  static float Scale2Db(float scale)
  {
    scale = std::max(0.0f, std::min(1.0f, scale));
    if (scale < 0.00001f)
    {
      return MinScaleDb();
    }
    const float factor = ::exp(::log(scale) / 2.0f);
    return MinScaleDb() + factor * (MaxScaleDb() - MinScaleDb());
  }
  
  static float Gain2Scale(float gain)
  {
    return Db2Scale(Gain2Db(gain));
  }
  
  static float Scale2Gain(float scale)
  {
    return Db2Gain(Scale2Db(scale));
  }
  
  static String DecibelString(float db)
  {
    if (::fabs(db - MinScaleDb()) < 0.001f)
    {
      return String("-inf");
    }
    else
    {
      String decibelString(db, 1);
      if (decibelString == "-0.0")
      {
        decibelString = "0.0";
      }
      return decibelString + String("dB");
    }
  }
  
  static String DecibelString(int db)
  {
    if (::fabs(static_cast<float>(db) - MinScaleDb()) < 0.001f)
    {
      return String("-inf");
    }
    else
    {
      return String(db) + String("dB");
    }
  }
  
};


#endif // Header guard
