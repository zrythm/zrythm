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

#include "Convolver.h"


class ConvolverBackgroundThread : public juce::Thread
{
public:
  explicit ConvolverBackgroundThread(Convolver& convolver) :
    juce::Thread("ConvolverBackgroundThread"),
    _convolver(convolver)
  {
    startThread(8); // Use a priority higher than the priority of normal threads
  }
  
  
  virtual ~ConvolverBackgroundThread()
  {
    signalThreadShouldExit();
    notify();
    stopThread(1000);
  }
  
  
  virtual void run()
  {
    while (!threadShouldExit())
    {
      wait(-1);
      if (threadShouldExit())
      {
        return;
      }
      _convolver.doBackgroundProcessing();
      _convolver._backgroundProcessingFinished.set(1);
      _convolver._backgroundProcessingFinishedEvent.signal();
    }
  }
  
private:
  Convolver& _convolver;
  
  ConvolverBackgroundThread(const ConvolverBackgroundThread&);
  ConvolverBackgroundThread& operator=(const ConvolverBackgroundThread&);
};


// =================================================

Convolver::Convolver() :
  fftconvolver::TwoStageFFTConvolver(),
  _thread(),
  _backgroundProcessingFinished(1),
  _backgroundProcessingFinishedEvent(true)
{
  _thread = new ConvolverBackgroundThread(*this);
  _backgroundProcessingFinishedEvent.signal();
}


Convolver::~Convolver()
{
  _thread = nullptr;
}


void Convolver::startBackgroundProcessing()
{
  _backgroundProcessingFinished.set(0);
  _backgroundProcessingFinishedEvent.reset();
  _thread->notify();
}


void Convolver::waitForBackgroundProcessing()
{
  _backgroundProcessingFinishedEvent.wait();
}
