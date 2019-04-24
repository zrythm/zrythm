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

#ifndef _IRMANAGER_H
#define _IRMANAGER_H

#include "JuceHeader.h"

#include "Processor.h"


class IRCalculation : public juce::Thread
{
public:
  explicit IRCalculation(Processor& processor);
  virtual ~IRCalculation();
  
  virtual void run();
  
private:
  FloatBuffer::Ptr importAudioFile(const File& file, size_t fileChannel, double& fileSampleRate) const;
  void reverseBuffer(FloatBuffer::Ptr& buffer) const;
  FloatBuffer::Ptr changeSampleRate(const FloatBuffer::Ptr& inputBuffer, double inputSampleRate, double outputSampleRate) const;
  void unifyBufferSize(std::vector<FloatBuffer::Ptr>& buffers) const;  
  float calculateAutoGain(const std::vector<FloatBuffer::Ptr>& buffers) const;
  std::vector<FloatBuffer::Ptr> cropBuffers(const std::vector<FloatBuffer::Ptr>& buffers, double irBegin, double irEnd) const;
  
  Processor& _processor;
  
  // Prevent uncontrolled usage
  IRCalculation(const IRCalculation&);
  IRCalculation& operator=(const IRCalculation&);
};


#endif // Header guard
