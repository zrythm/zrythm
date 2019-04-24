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

#ifndef _CUSTOMLOOKANDFEEL_H
#define _CUSTOMLOOKANDFEEL_H

#include "../JuceHeader.h"

class CustomLookAndFeel : public juce::LookAndFeel_V3
{
public:
  CustomLookAndFeel();

  static const CustomLookAndFeel& GetCustomLookAndFeel(juce::Component* component);

  // Scales
  juce::Font getScaleFont() const;
  juce::Colour getScaleColour() const;
  juce::Colour getLevelColourMinusInfDb() const;
  juce::Colour getLevelColourMinus40Db() const;
  juce::Colour getLevelColourZeroDb() const;
  juce::Colour getLevelColourClipping() const;

  // Waveform
  juce::Colour getWaveformColour() const;
  juce::Colour getWaveformBackgroundColour() const;
  
  // Envelope
  juce::Colour getEnvelopeRestrictionColour() const;
  juce::Colour getEnvelopeNodeColour(bool highlighted) const;
  
private:
  CustomLookAndFeel(const CustomLookAndFeel&) = delete;
  CustomLookAndFeel& operator=(const CustomLookAndFeel&) = delete;
};


#endif // Header guard
