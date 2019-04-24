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

#ifndef _WAVEFORMCOMPONENT_H
#define _WAVEFORMCOMPONENT_H


#include "../JuceHeader.h"

#include "../Envelope.h"
#include "../IRAgent.h"

#include <vector>


class WaveformComponent : public juce::Component
{
public:
  explicit WaveformComponent();
  virtual ~WaveformComponent();
  
  virtual void paint(juce::Graphics& g);
  virtual void resized();
  
  void init(IRAgent* irAgent, double sampleRate, size_t samplesPerPx);
  void clear();
  
  virtual void mouseUp(const juce::MouseEvent& mouseEvent);
  
  void envelopeChanged();
  
protected:  
  void updateArea();
  
private:
  IRAgent* _irAgent;
  std::vector<float> _maximaDecibels;
  size_t _irFingerprint;
  
  double _sampleRate;
  size_t _samplesPerPx;
  float _pxPerDecibel;
  double _predelayMs;
  double _attackLength;
  double _attackShape;
  double _decayShape;
  int _predelayOffsetX;
  Rectangle<int> _area;
    
  float _beatsPerMinute;
  
  WaveformComponent(const WaveformComponent&);
  WaveformComponent& operator=(const WaveformComponent&);
};


#endif // Header guard
