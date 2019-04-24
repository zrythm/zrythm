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

#ifndef _STEREOWIDTH_H
#define _STEREOWIDTH_H

#include <cstddef>


class StereoWidth
{
public:
  StereoWidth();
  virtual ~StereoWidth();

  void initializeWidth(float width);
  void updateWidth(float width);
  void process(float* left, float* right, size_t len);

private:
  float _widthCurrent;
  float _widthDesired;
  float _interpolationStep;

  // Prevent uncontrolled usage
  StereoWidth(const StereoWidth&);
  StereoWidth& operator=(const StereoWidth&);
};

#endif // Header guard
