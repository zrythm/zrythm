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

#include "Envelope.h"

#include <algorithm>
#include <cassert>
#include <cmath>


void ApplyEnvelope(float* data, size_t len, double attackLength, double attackShape, double decayShape)
{
  if (!data || len == 0)
  {
    return;
  }

  const size_t attackBegin = 0;
  const size_t attackEnd = static_cast<size_t>(attackLength * static_cast<double>(len));

  const size_t decayBegin = std::min(attackEnd + 1, len);
  const size_t decayEnd = len;

  const double attackSampleCount = static_cast<double>(attackEnd - attackBegin);
  for (size_t i=attackBegin; i<attackEnd; ++i)
  {
    const double attackPos = static_cast<double>(i-attackBegin) / attackSampleCount;
    const double gain = ::pow(attackPos, attackShape);
    data[i] *= static_cast<float>(gain);
  }

  const double decaySampleCount = static_cast<double>(decayEnd - decayBegin);
  for (size_t i=decayBegin; i<decayEnd; ++i)
  {    
    const double decayPos = static_cast<double>(i-decayBegin) / decaySampleCount;
    const double gain0 = 1.0 / (decayShape * decayShape * decayPos + 1.0);
    const double gain1 = ::pow(1.0 - decayPos, decayShape);
    const double fac0 = 1.0 - decayPos;
    const double fac1 = 1.0 - fac0;
    const double gain = fac0 * gain0 + fac1 * gain1;
    data[i] *= static_cast<float>(gain);
  }
}
