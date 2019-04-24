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

#include "IRAgent.h"

#include "Parameters.h"
#include "Processor.h"

#include <algorithm>





class ReferencingAudioSource : public AudioSource
{
public:
  explicit ReferencingAudioSource(float* buffer, size_t len) :
    juce::AudioSource(),
    _buffer(buffer),
    _len(len),
    _pos(0)
  {
  }
  
  virtual void prepareToPlay(int samplesPerBlockExpected, double sampleRate)
  {
    (void) samplesPerBlockExpected;
    (void) sampleRate;
    _pos = 0;
  }
  
  virtual void releaseResources()
  {
    _buffer = nullptr;
    _len = 0;
    _pos = 0;
  }
  
  virtual void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill)
  {
    AudioSampleBuffer* destBuffer = bufferToFill.buffer;
    const int len = std::min(bufferToFill.numSamples, static_cast<int>(_len-_pos));
    if (destBuffer)
    {
      for (int channel=0; channel<destBuffer->getNumChannels(); ++channel)
      {
        if (channel == 0 && _buffer)
        {
          destBuffer->copyFrom(channel, bufferToFill.startSample, _buffer+_pos, len);
          if (len < bufferToFill.numSamples)
          {
            const int startClear = bufferToFill.startSample + len;
            const int lenClear = bufferToFill.numSamples - len;
            destBuffer->clear(startClear, lenClear);
          }
        }
        else
        {
          destBuffer->clear(channel, bufferToFill.startSample, len);
        }
      }
    }
    _pos += len;
  }
  
private:
  float* _buffer;
  size_t _len;
  size_t _pos;
  
  ReferencingAudioSource(const ReferencingAudioSource&);
  ReferencingAudioSource& operator=(const ReferencingAudioSource&);
};


// =====================================================


IRAgent::IRAgent(Processor& processor, size_t inputChannel, size_t outputChannel) :
  ChangeNotifier(),
  _processor(processor),
  _inputChannel(inputChannel),
  _outputChannel(outputChannel),
  _mutex(),
  _file(File()),
  _fileSampleCount(0),
  _fileChannelCount(0),
  _fileSampleRate(0.0),
  _fileChannel(0),
  _irBuffer(nullptr),
  _convolverMutex(),
  _convolver(nullptr),
  _fadeFactor(0.0),
  _fadeIncrement(0.0),
  _eqLo(CookbookEq::HiPass2, Parameters::EqLowCutFreq.getMinValue(), 1.0f),
  _eqHi(CookbookEq::LoPass2, Parameters::EqHighCutFreq.getMaxValue(), 1.0f)
{
  initialize();
}


IRAgent::~IRAgent()
{
  clear();
}


Processor& IRAgent::getProcessor() const
{
  return _processor;
}


size_t IRAgent::getInputChannel() const
{
  return _inputChannel;
}


size_t IRAgent::getOutputChannel() const
{
  return _outputChannel;
}


void IRAgent::initialize()
{
  const float eqSampleRate = static_cast<float>(_processor.getSampleRate());
  const size_t eqBlockSize = _processor.getConvolverHeadBlockSize();
  
  const int eqLowType = _processor.getParameter(Parameters::EqLowType);
  if (eqLowType == Parameters::Cut)
  {
    _eqLo.setType(CookbookEq::HiPass2);
    _eqLo.setFreq(_processor.getParameter(Parameters::EqLowCutFreq));
  }
  else if (eqLowType == Parameters::Shelf)
  {
    _eqLo.setType(CookbookEq::LoShelf);
    _eqLo.setFreq(_processor.getParameter(Parameters::EqLowShelfFreq));
    _eqLo.setGain(_processor.getParameter(Parameters::EqLowShelfDecibels));
  }

  const int eqHighType = _processor.getParameter(Parameters::EqHighType);
  if (eqHighType == Parameters::Cut)
  {
    _eqHi.setType(CookbookEq::LoPass2);
    _eqHi.setFreq(_processor.getParameter(Parameters::EqHighCutFreq));
  }
  else if (eqHighType == Parameters::Shelf)
  {
    _eqHi.setType(CookbookEq::HiShelf);
    _eqHi.setFreq(_processor.getParameter(Parameters::EqHighShelfFreq));
    _eqHi.setGain(_processor.getParameter(Parameters::EqHighShelfDecibels));
  }
  
  _eqLo.prepareToPlay(eqSampleRate, eqBlockSize);
  _eqHi.prepareToPlay(eqSampleRate, eqBlockSize);
}


void IRAgent::clear()
{
  fadeOut();
  waitForFadeOut();
  setConvolver(nullptr);
  {
    ScopedLock lock(_mutex);
    _file = File();
    _fileSampleCount = 0;
    _fileChannelCount = 0;
    _fileSampleRate = 0.0;
    _fileChannel = 0;
    _irBuffer = nullptr;
  }
  propagateChange();
}


void IRAgent::setFile(const File& file, size_t fileChannel)
{
  AudioFormatManager formatManager;
  formatManager.registerBasicFormats();
  ScopedPointer<AudioFormatReader> audioFormatReader(formatManager.createReaderFor(file));
  {
    ScopedLock lock(_mutex);
    if (audioFormatReader)
    {
      if (_file == file && _fileChannel == fileChannel)
      {
        return;
      }
      _file = file;
      _fileSampleCount = static_cast<size_t>(audioFormatReader->lengthInSamples);
      _fileChannelCount = static_cast<size_t>(audioFormatReader->numChannels);
      _fileSampleRate = audioFormatReader->sampleRate;
      _fileChannel = fileChannel;
    }
    else
    {
      _file = File();
      _fileSampleCount = 0;
      _fileChannelCount = 0;
      _fileSampleRate = 0.0;
      _fileChannel = 0;
    }
  }
  propagateChange();
  updateConvolver();
}


File IRAgent::getFile() const
{
  ScopedLock lock(_mutex);
  return _file;
}


size_t IRAgent::getFileChannelCount() const
{
  ScopedLock lock(_mutex);
  return _fileChannelCount;
}


size_t IRAgent::getFileSampleCount() const
{
  ScopedLock lock(_mutex);
  return _fileSampleCount;
}


double IRAgent::getFileSampleRate() const
{
  ScopedLock lock(_mutex);
  return _fileSampleRate;
}


size_t IRAgent::getFileChannel() const
{
  ScopedLock lock(_mutex);
  return _fileChannel;
}


FloatBuffer::Ptr IRAgent::getImpulseResponse() const
{
  ScopedLock lock(_mutex);
  return _irBuffer;
}


void IRAgent::updateConvolver()
{
  _processor.updateConvolvers();
}


void IRAgent::resetIR(const FloatBuffer::Ptr& irBuffer, Convolver* convolver)
{
  {
    ScopedLock lock(_mutex);
    _irBuffer = irBuffer;
  }

  {
    ScopedPointer<Convolver> conv(convolver);
    {
      // Make sure that the convolver mutex is locked as short as
      // possible and that all destruction and deallocation happens
      // outside of the lock because this might block the audio thread
      ScopedLock convolverLock(_convolverMutex);
      if (_convolver != conv)
      {
        _convolver.swapWith(conv);
      }
    }
  }

  propagateChange();
}


CriticalSection& IRAgent::getConvolverMutex()
{
  return _convolverMutex;
}


Convolver* IRAgent::getConvolver()
{
  return _convolver.get();
}
                  

void IRAgent::setConvolver(Convolver* convolver)
{
  ScopedPointer<Convolver> conv(convolver);
  {
    // Make sure that the convolver mutex is locked as short as
    // possible and that all destruction and deallocation happens
    // outside of the lock because this might block the audio thread
    ScopedLock convolverLock(_convolverMutex);
    if (_convolver != conv)
    {
      _convolver.swapWith(conv);
    }
  }
  conv = nullptr;
}


void IRAgent::propagateChange()
{
  notifyAboutChange();
  _processor.notifyAboutChange();
}


void IRAgent::process(const float* input, float* output, size_t len)
{
  const float Epsilon = 0.0001f;
  
  // This is the hopefully one and only rare exception where we need to
  // lock a mutex in the realtime audio thread :-/ (however, the according
  // convolver mutex is locked somewhere else only once for a very short and
  // rare swapping operation, so this shouldn't be a problem, and most operating
  // systems internally try spinning before performing an expensive context switch,
  // so we will never give up the context here probably).
  juce::ScopedLock convolverLock(_convolverMutex);
  
  if (_convolver && (_fadeFactor > Epsilon || ::fabs(_fadeIncrement) > Epsilon))
  {
    _convolver->process(input, output, len);        
    if (::fabs(_fadeIncrement) > Epsilon || _fadeFactor < (1.0-Epsilon))
    {
      for (size_t i=0; i<len; ++i)
      {
        _fadeFactor = std::max(0.0f, std::min(1.0f, _fadeFactor+_fadeIncrement));
        output[i] *= _fadeFactor;
      }
      if (_fadeFactor < Epsilon || _fadeFactor > (1.0-Epsilon))
      {
        _fadeIncrement = 0.0;
      }
    }
  }
  else
  {
    ::memset(output, 0, len * sizeof(float));
    _fadeFactor = 0.0;
    _fadeIncrement = 0.0;
  }
  
  // EQ low
  const int eqLowType = _processor.getParameter(Parameters::EqLowType);
  if (eqLowType == Parameters::Cut)
  {    
    const float eqLowCutFreq = _processor.getParameter(Parameters::EqLowCutFreq);
    if (::fabs(eqLowCutFreq-Parameters::EqLowCutFreq.getMinValue()) > 0.0001f)
    {
      _eqLo.setType(CookbookEq::HiPass2);
      _eqLo.setFreq(eqLowCutFreq);
      _eqLo.filterOut(output, len);
    }
  }
  else if (eqLowType == Parameters::Shelf)
  {
    const float eqLowShelfDecibels = _processor.getParameter(Parameters::EqLowShelfDecibels);
    if (::fabs(eqLowShelfDecibels-0.0f) > 0.0001f)
    {
      _eqLo.setType(CookbookEq::LoShelf);
      _eqLo.setFreq(_processor.getParameter(Parameters::EqLowShelfFreq));
      _eqLo.setGain(eqLowShelfDecibels);
      _eqLo.filterOut(output, len);
    }
  }
  
  // EQ high
  const int eqHighType = _processor.getParameter(Parameters::EqHighType);
  if (eqHighType == Parameters::Cut)
  {
    const float eqHighCutFreq = _processor.getParameter(Parameters::EqHighCutFreq);
    if (::fabs(eqHighCutFreq-Parameters::EqHighCutFreq.getMaxValue()) > 0.0001f)
    {
      _eqHi.setType(CookbookEq::LoPass2);
      _eqHi.setFreq(eqHighCutFreq);
      _eqHi.filterOut(output, len);
    }
  }
  else if (eqHighType == Parameters::Shelf)
  {
    const float eqHighShelfDecibels = _processor.getParameter(Parameters::EqHighShelfDecibels);
    if (::fabs(eqHighShelfDecibels-0.0f) > 0.0001f)
    {
      _eqHi.setType(CookbookEq::HiShelf);
      _eqHi.setFreq(_processor.getParameter(Parameters::EqHighShelfFreq));
      _eqHi.setGain(eqHighShelfDecibels);
      _eqHi.filterOut(output, len);
    }
  }
}


void IRAgent::fadeIn()
{
  _fadeIncrement = +0.005f;
}


void IRAgent::fadeOut()
{
  _fadeIncrement = -0.005f;
}


bool IRAgent::waitForFadeOut(size_t waitTimeMs)
{
  for (size_t i=0; i < waitTimeMs && _fadeFactor >= 0.0001; ++i)
  {
    Thread::sleep(1);
  }
  return (_fadeFactor < 0.0001);
}
