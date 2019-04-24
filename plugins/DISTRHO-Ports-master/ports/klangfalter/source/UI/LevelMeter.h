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

#ifndef _LEVELMETER_H
#define _LEVELMETER_H

#include "../JuceHeader.h"

#include "../LevelMeasurement.h"


class LevelMeter : public juce::Component
{
public:
  LevelMeter();
  virtual ~LevelMeter();
  
  virtual void paint(juce::Graphics& g);
  virtual void resized();
  
  void setChannelCount(size_t channelCount);
  void setLevel(size_t channel, float level);
  
private:
  std::vector<float> _levels;
  juce::ColourGradient _colourGradient;
  
  LevelMeter(const LevelMeter&);
  LevelMeter& operator=(const LevelMeter&);
};



#endif // Header guard
